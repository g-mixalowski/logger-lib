#include <condition_variable>
#include <iostream>
#include <loggerlib/logger.hpp>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

struct LoggerEntry {
    std::string msg;
    loggerlib::LogLevel lvl;
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <logfile> [default_level]\n";
        return 1;
    }

    std::string filename = argv[1];

    loggerlib::LogLevel default_level = loggerlib::LogLevel::INFO;

    if (argc >= 3) {
        int lvl = std::stoi(argv[2]);
        default_level = static_cast<loggerlib::LogLevel>(lvl);
    }

    loggerlib::Logger logger(filename, default_level);

    std::queue<LoggerEntry> queue;  // queue of entries for logging
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;

    // creating a worker thread for logging
    std::thread worker([&]() {
        std::unique_lock lock(mtx);

        while (!done || !queue.empty()) {
            cv.wait(lock, [&] { return done || !queue.empty(); });

            while (!queue.empty()) {
                auto entry = queue.front();
                queue.pop();
                lock.unlock();
                logger.log(entry.msg, entry.lvl);
                lock.lock();
            }
        }
    });

    // a cycle for user to create messages
    while (true) {
        std::cout << "Enter message ('exit'): ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        std::cout << "Level (0=DEBUG,1=INFO,2=ERROR) [default "
                  << static_cast<int>(default_level) << "]: ";

        std::string str_lvl;
        std::getline(std::cin, str_lvl);

        loggerlib::LogLevel lvl = default_level;

        if (!str_lvl.empty()) {
            lvl = static_cast<loggerlib::LogLevel>(std::stoi(str_lvl));
        }

        {
            std::unique_lock lock(mtx);
            queue.push(LoggerEntry{input, lvl});
        }

        cv.notify_one();
    }

    {
        std::unique_lock lock(mtx);
        done = true;
    }

    cv.notify_one();
    worker.join();

    return 0;
}