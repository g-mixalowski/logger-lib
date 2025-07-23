#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <loggerlib/logger.hpp>
#include <mytest.hpp>
#include <regex>
#include <stdexcept>
#include <thread>
#include <typeinfo>
#include <vector>
namespace fs = std::filesystem;
using namespace loggerlib;

int start_test_server(int &out_port) {
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        throw std::runtime_error("Не удалось создать сокет сервера");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;  // выбрать свободный порт

    if (bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        close(listen_fd);
        throw std::runtime_error("Не удалось привязать серверный сокет");
    }

    // Узнаём динамически назначенный порт
    socklen_t len = sizeof(addr);
    if (getsockname(listen_fd, (sockaddr *)&addr, &len) < 0) {
        close(listen_fd);
        throw std::runtime_error("Не удалось получить порт сервера");
    }
    out_port = ntohs(addr.sin_port);

    if (listen(listen_fd, 1) < 0) {
        close(listen_fd);
        throw std::runtime_error("Не удалось слушать на серверном сокете");
    }

    return listen_fd;
}

// File ctor

TEST_CASE("File ctor opens file if possible") {
    std::string filename = "test_file.log";

    loggerlib::Logger lg(filename, loggerlib::LogLevel::INFO);

    CHECK_MESSAGE(
        fs::exists(filename) && fs::is_regular_file(filename),
        "File is not created"
    );

    fs::remove(filename);
}

TEST_CASE("File ctor throws correctly when opening file is impossible") {
    std::string filename = "\\/?<>*|.forbidden";
    try {
        loggerlib::Logger lg(filename, loggerlib::LogLevel::INFO);
        CHECK_MESSAGE(false, "Ctor didn't throw when couldn't create a file");
    } catch (const std::runtime_error &e) {
        std::string expected_message =
            std::string("Cannot open log file: ") + filename;
        std::string real_message = e.what();
        CHECK_MESSAGE(
            real_message == expected_message,
            "Wrong message: \"" + expected_message + "\" expected, \"" +
                real_message + "\" given."
        );
    } catch (const std::exception &e) {
        CHECK_MESSAGE(
            false,
            std::string("Wrong type: \"St13runtime_error\" expected, \"") +
                typeid(e).name() + "\" given."
        );
    }
    if (fs::exists(filename)) {
        fs::remove(filename);
    }
}

// Socket ctor

TEST_CASE("Logger constructor error handling") {
    SUBCASE("Getaddrinfo error for invalid host") {
        try {
            loggerlib::Logger(
                "__invalid_host__", 12345, loggerlib::LogLevel::INFO
            );
            CHECK_MESSAGE(false, "Ctor didn't throw when host is invalid");
        } catch (const std::runtime_error &e) {
            std::string expected_part = "getaddrinfo";
            std::string message = e.what();
            CHECK_MESSAGE(
                message.find(expected_part) != std::string::npos,
                "Wrong message: \" getaddrinfo: <error_str> \" expected, \"" +
                    message + "\" given."
            );
        } catch (const std::exception &e) {
            CHECK_MESSAGE(
                false,
                std::string("Wrong type: \"St13runtime_error\" expected, \"") +
                    typeid(e).name() + "\" given."
            );
        }
    }

    SUBCASE("Socket connection fails on closed port") {
        try {
            loggerlib::Logger("127.0.0.1", 1, loggerlib::LogLevel::INFO);
            CHECK_MESSAGE(false, "Ctor didn't throw when port is invalid");
        } catch (const std::runtime_error &e) {
            std::string expected_part = "Socket connection failed";
            std::string message = e.what();
            CHECK_MESSAGE(
                message.find(expected_part) != std::string::npos,
                "Wrong message: \" Socket connection failed:* \" expected, \"" +
                    message + "\" given."
            );
        } catch (const std::exception &e) {
            CHECK_MESSAGE(
                false,
                std::string("Wrong type: \"St13runtime_error\" expected, \"") +
                    typeid(e).name() + "\" given."
            );
        }
    }
}

TEST_CASE("Logger constructor successful connection") {
    int port;
    int server_fd = start_test_server(port);

    std::thread server_thread([server_fd]() {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        int conn_fd = accept(server_fd, (sockaddr *)&client_addr, &addr_len);
        if (conn_fd >= 0) {
            close(conn_fd);
        }
        close(server_fd);
    });
    try {
        loggerlib::Logger("127.0.0.1", port, loggerlib::LogLevel::INFO);
    } catch (...) {
        CHECK_MESSAGE(false, "Thrown when host is correct");
    }

    server_thread.join();
}

// Dtor

TEST_CASE("Logger destructor closes file") {
    const std::string filepath = "temp_logger_test.txt";
    Logger logger(filepath, LogLevel::DEBUG);
    std::ofstream f(filepath, std::ios::app);
    CHECK(f.is_open());
    f.close();
    CHECK(std::remove(filepath.c_str()) == 0);
}

TEST_CASE("Logger destructor closes socket") {
    int port;
    int server_fd = start_test_server(port);
    std::vector<int> conn_fd_holder;

    std::thread server_thread([&]() {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        int conn_fd = accept(server_fd, (sockaddr *)&client_addr, &addr_len);
        if (conn_fd >= 0) {
            conn_fd_holder.push_back(conn_fd);
        }
        char buf;
        int n = read(conn_fd_holder[0], &buf, 1);
        CHECK(n == 0);
        close(conn_fd_holder[0]);
        close(server_fd);
    });

    { Logger logger("127.0.0.1", port, LogLevel::INFO); }
    server_thread.join();
}

// log()
TEST_CASE("Logger.log writes to file with correct formatting and level filter"
) {
    const std::string filepath = "temp_logger_log.txt";
    // Уровень INFO, DEBUG-сообщения игнорируются
    {
        Logger logger(filepath, LogLevel::INFO);
        logger.log("debug message", LogLevel::DEBUG);
        logger.log("info message", LogLevel::INFO);
        logger.log("error message", LogLevel::ERROR);
    }
    // Читаем файл и проверяем содержимое
    std::ifstream f(filepath);
    CHECK(f.is_open());
    std::string content((std::istreambuf_iterator<char>(f)), {});
    f.close();

    // Ожидаем две строки: INFO и ERROR
    std::vector<std::string> lines;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    CHECK(lines.size() == 2);
    // Шаблон: [TIMESTAMP] LEVEL:  message
    std::regex info_re(R"(\[.*\] INFO:  info message)");
    std::regex error_re(R"(\[.*\] ERROR: error message)");
    CHECK(std::regex_match(lines[0], info_re));
    CHECK(std::regex_match(lines[1], error_re));
    // Удаляем файл
    CHECK(std::remove(filepath.c_str()) == 0);
}

TEST_CASE("Logger.log sends to socket with correct formatting") {
    int port;
    int server_fd = start_test_server(port);
    std::vector<std::string> received;
    std::thread server_thread([&]() {
        int conn_fd;
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        conn_fd = accept(server_fd, (sockaddr *)&client_addr, &addr_len);
        char buf[1024];
        int n = recv(conn_fd, buf, sizeof(buf) - 1, 0);
        buf[n] = '\0';
        received.emplace_back(buf);
        close(conn_fd);
        close(server_fd);
    });

    {
        Logger logger("127.0.0.1", port, LogLevel::DEBUG);
        logger.log("socket test", LogLevel::DEBUG);
    }
    server_thread.join();

    CHECK(received.size() == 1);
    // Проверяем формат сообщения
    std::regex re(R"(\[.*\] DEBUG: socket test
)");
    CHECK(std::regex_match(received[0], re));
}

TEST_CASE("Logger.get_level returns the level set in constructor") {
    const std::string filepath = "temp_level_get.txt";
    Logger logger(filepath, LogLevel::ERROR);
    CHECK(logger.get_level() == LogLevel::ERROR);
    std::remove(filepath.c_str());
}

TEST_CASE("Logger.set_level updates the level and get_level returns new value"
) {
    const std::string filepath = "temp_level_set.txt";
    Logger logger(filepath, LogLevel::INFO);
    CHECK(logger.get_level() == LogLevel::INFO);
    logger.set_level(LogLevel::DEBUG);
    CHECK(logger.get_level() == LogLevel::DEBUG);
    std::remove(filepath.c_str());
}

TEST_CASE(
    "Logger::get_current_timestamp returns string in YYYY-MM-DD HH:MM:SS format"
) {
    Logger logger("temp_timestamp.txt", LogLevel::DEBUG);
    std::string ts = logger.get_current_timestamp();
    std::regex re(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})");
    CHECK(std::regex_match(ts, re));
    std::remove("temp_timestamp.txt");
}