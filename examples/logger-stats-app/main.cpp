#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

constexpr int BACKLOG = 10;

struct Stats {
    std::atomic<std::size_t> total = 0;
    std::array<std::atomic<std::size_t>, 3> messages_per_level;
    mutable std::mutex length_mutex;
    std::vector<std::size_t> lengths;
    mutable std::mutex time_mutex;
    std::vector<std::chrono::system_clock::time_point> timestamps;
};

void printStats(const Stats &stats) {
    // get current stats
    std::size_t total = stats.total.load();
    std::size_t cnt_debug = stats.messages_per_level[0].load();
    std::size_t cnt_info = stats.messages_per_level[1].load();
    std::size_t cnt_error = stats.messages_per_level[2].load();

    std::vector<std::size_t> lens;

    {
        std::unique_lock lock(stats.length_mutex);
        lens = stats.lengths;
    }

    // count min, max, avg
    std::size_t min_len =
        lens.empty() ? 0 : *std::min_element(lens.begin(), lens.end());
    std::size_t max_len =
        lens.empty() ? 0 : *std::max_element(lens.begin(), lens.end());
    double avg_len =
        lens.empty()
            ? 0.0
            : std::accumulate(lens.begin(), lens.end(), 0.0) / lens.size();

    // count last hour msgs
    auto now = std::chrono::system_clock::now();
    size_t cnt_last_hr = 0;
    {
        std::unique_lock lock(stats.time_mutex);
        for (auto &tp : stats.timestamps) {
            if (std::chrono::duration_cast<std::chrono::hours>(now - tp).count(
                ) < 1) {
                ++cnt_last_hr;
            }
        }
    }

    // print stats
    std::cout << "\n============ Statistics ============\n"
              << "Total messages: " << total << "\n"
              << "DEBUG: " << cnt_debug << ", INFO: " << cnt_info
              << ", ERROR: " << cnt_error << "\n"
              << "Last hour: " << cnt_last_hr << "\n"
              << "Length - min: " << min_len << ", max: " << max_len
              << ", avg: " << avg_len << "\n"
              << "====================================\n";
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <N> <T>\n";
        return 1;
    }

    const char *host = argv[1];
    const char *port = argv[2];
    size_t N = std::stoul(argv[3]);
    int T = std::stoi(argv[4]);

    Stats stats{};
    std::atomic<bool> running{true};

    // same as for socket in Logger::Logger(std::string, int, LogLevel)
    addrinfo hints{}, *servinfo, *p;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (int rv = getaddrinfo(host, port, &hints, &servinfo); rv != 0) {
        throw std::runtime_error(
            std::string("getaddrinfo: ") + gai_strerror(rv)
        );
    }

    int server_fd = -1;
    int yes = 1;
    for (p = servinfo; p; p = p->ai_next) {
        server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (server_fd < 0) {
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) <
            0) {
            close(server_fd);
            throw std::system_error(
                errno, std::generic_category(), "setsockopt"
            );
        }

        if (bind(server_fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }
        close(server_fd);
    }
    freeaddrinfo(servinfo);
    if (!p) {
        throw std::runtime_error(
            "failed to bind on " + std::string(host) + ":" + std::string(port)
        );
    }

    // define real port if 0 given
    if (std::string(port) == "0") {
        sockaddr_storage ss{};
        socklen_t len = sizeof(ss);
        if (getsockname(server_fd, (sockaddr *)&ss, &len) == 0) {
            if (ss.ss_family == AF_INET) {
                port = std::to_string(ntohs(((sockaddr_in *)&ss)->sin_port))
                           .c_str();
            } else {
                port = std::to_string(ntohs(((sockaddr_in6 *)&ss)->sin6_port))
                           .c_str();
            }
        }
    }

    // try to listen
    if (listen(server_fd, 10) < 0) {
        throw std::system_error(errno, std::generic_category(), "listen");
    }

    // thread for timing
    std::thread timer_thread([&]() {
        size_t last_total = 0;
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(T));
            size_t curr = stats.total.load();
            if (curr != last_total) {
                printStats(stats);
                last_total = curr;
            }
        }
    });

    // cycle for clients to connect
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            break;
        }

        std::thread([&, client_fd]() {
            char buffer[1024];
            while (true) {
                ssize_t len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (len <= 0) {
                    break;
                }
                buffer[len] = '\0';
                std::string line(buffer);

                int lvl = 1;
                if (line.find("DEBUG:") != std::string::npos) {
                    lvl = 0;
                } else if (line.find("ERROR:") != std::string::npos) {
                    lvl = 2;
                }

                stats.total.fetch_add(1);
                stats.messages_per_level[lvl].fetch_add(1);
                {
                    std::lock_guard lock(stats.length_mutex);
                    stats.lengths.push_back(line.size());
                }
                {
                    std::lock_guard lock(stats.time_mutex);
                    stats.timestamps.push_back(std::chrono::system_clock::now()
                    );
                }

                std::cout << line;
                if (stats.total.load() % N == 0) {
                    printStats(stats);
                }
            }
            close(client_fd);
        }).detach();
    }

    running.store(false);
    timer_thread.join();
    close(server_fd);
    return 0;
}