#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace digitalcam::logging {

enum class Level : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    static Logger& instance();

    void initialize(const std::string& directory = "logs");
    void setLevel(Level level);
    void log(Level level, std::string_view message);

    void trace(std::string_view message);
    void debug(std::string_view message);
    void info(std::string_view message);
    void warning(std::string_view message);
    void error(std::string_view message);
    void critical(std::string_view message);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;
    ~Logger() = default;

    bool initialized_ = false;
    Level level_ = Level::Info;
    std::string filePath_;
};

}

#define DCAM_LOG_TRACE(message) ::digitalcam::logging::Logger::instance().trace(message)
#define DCAM_LOG_DEBUG(message) ::digitalcam::logging::Logger::instance().debug(message)
#define DCAM_LOG_INFO(message) ::digitalcam::logging::Logger::instance().info(message)
#define DCAM_LOG_WARNING(message) ::digitalcam::logging::Logger::instance().warning(message)
#define DCAM_LOG_ERROR(message) ::digitalcam::logging::Logger::instance().error(message)
#define DCAM_LOG_CRITICAL(message) ::digitalcam::logging::Logger::instance().critical(message)
