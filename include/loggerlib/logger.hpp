#ifndef LOGGERLIB_LOGGER_HPP_
#define LOGGERLIB_LOGGER_HPP_

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <fstream>
#include <loggerlib/export.hpp>
#include <mutex>
#include <stdexcept>
#include <string>
#include <variant>

namespace loggerlib {

enum class LOGGERLIB_EXPORT LogLevel { DEBUG = 0, INFO, ERROR };

class LOGGERLIB_EXPORT Logger {
public:
    // File writing ctor
    LOGGERLIB_EXPORT explicit Logger(
        const std::string &filename,
        LogLevel level = LogLevel::INFO
    );
    // TCP-socket writing ctor
    LOGGERLIB_EXPORT
    Logger(const std::string &host, int port, LogLevel level = LogLevel::INFO);
    LOGGERLIB_EXPORT ~Logger();

    // Log message
    LOGGERLIB_EXPORT void log(const std::string &message, LogLevel level);

    // Set/get default message level
    LOGGERLIB_EXPORT void set_level(LogLevel level);
    LOGGERLIB_EXPORT LogLevel get_level() const;

    // Get timestamp in desired format
    std::string get_current_timestamp();

private:
    // Common fields
    LogLevel level_;
    std::mutex mutex_;

    // Destination point: file or socket
    std::variant<int, std::ofstream> dest_;
};

}  // namespace loggerlib

#endif  // LOGGERLIB_LOGGER_HPP_