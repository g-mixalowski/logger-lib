#include <cerrno>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <loggerlib/logger.hpp>
#include <sstream>
#include <variant>

namespace loggerlib {

// File writing ctor
Logger::Logger(const std::string &filename, LogLevel level)
    : level_(level), dest_(std::ofstream(filename, std::ios::app)) {
    auto &ofs = std::get<std::ofstream>(dest_);

    if (!ofs.is_open()) {
        throw std::runtime_error("Cannot open log file: " + filename);
    }
}

// Socket writing ctor
Logger::Logger(const std::string &host, int port, LogLevel level)
    : level_(level) {
    int sockfd;                 // socket file descriptor
    struct addrinfo hints;      // for getaddrinfo search
    struct addrinfo *servinfo;  // search results
    struct addrinfo *p;         // iterating through servinfo
    int rv;                     // error code

    memset(&hints, 0, sizeof hints);  // to prevent trash
    hints.ai_family = AF_UNSPEC;      // ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;  // TCP-socket

    if ((rv = getaddrinfo(
             host.c_str(), std::to_string(port).c_str(), &hints, &servinfo
         )) != 0) {
        throw std::runtime_error(
            std::string("getaddrinfo: ") + gai_strerror(rv)
        );
    }

    // iterating in servinfo trying to create socket & connect
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
            -1) {
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        break;
    }

    // couldn't connect by no address in servinfo
    if (p == NULL) {
        throw std::runtime_error("Socket connection failed");
    }

    freeaddrinfo(servinfo);

    dest_ = sockfd;
}

// Dtor closes file/socket
Logger::~Logger() {
    std::visit(
        [&](auto &dest) {
            using T = std::decay_t<decltype(dest)>;

            if constexpr (std::is_same_v<T, std::ofstream>) {
                if (dest.is_open()) {
                    dest.close();
                }
            } else if constexpr (std::is_same_v<T, int>) {
                close(dest);
            }
        },
        dest_
    );
}

void Logger::log(const std::string &message, LogLevel level) {
    // Ignore if level is too low
    if (level < level_) {
        return;
    }

    std::unique_lock lock(mutex_);

    // Forming the message:
    std::ostringstream oss;
    oss << "[" << get_current_timestamp() << "] ";

    switch (level) {
        case LogLevel::DEBUG:
            oss << "DEBUG: ";
            break;
        case LogLevel::INFO:
            oss << "INFO:  ";
            break;
        case LogLevel::ERROR:
            oss << "ERROR: ";
            break;
    }

    oss << message << "\n";
    auto out = oss.str();

    // writing to file/sending to socket
    std::visit(
        [&](auto &dest) {
            using T = std::decay_t<decltype(dest)>;

            if constexpr (std::is_same_v<T, std::ofstream>) {
                dest << out;
                dest.flush();
            } else if constexpr (std::is_same_v<T, int>) {
                send(dest, out.c_str(), static_cast<int>(out.size()), 0);
            }
        },
        dest_
    );
}

void Logger::set_level(LogLevel level) {
    std::unique_lock lock(mutex_);
    level_ = level;
}

LogLevel Logger::get_level() const {
    return level_;
}

std::string Logger::get_current_timestamp() {
    // get current time
    auto in_time =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    // localise the time
    std::tm buf;
    localtime_r(&in_time, &buf);

    // get formatted timestamp
    std::ostringstream oss;
    oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

}  // namespace loggerlib