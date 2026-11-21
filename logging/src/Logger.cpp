#include <digitalcam/logging/Logger.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>

#ifdef DIGITALCAM_HAS_SPDLOG
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#endif

namespace digitalcam::logging {

namespace {
std::mutex logMutex;

std::string levelText(Level level) {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error: return "ERROR";
        case Level::Critical: return "FATAL";
    }
    return "INFO";
}

#ifdef DIGITALCAM_HAS_SPDLOG
spdlog::level::level_enum toSpdlogLevel(Level level) {
    switch (level) {
        case Level::Trace: return spdlog::level::trace;
        case Level::Debug: return spdlog::level::debug;
        case Level::Info: return spdlog::level::info;
        case Level::Warning: return spdlog::level::warn;
        case Level::Error: return spdlog::level::err;
        case Level::Critical: return spdlog::level::critical;
    }
    return spdlog::level::info;
}
#endif

std::string timestampText() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;

    std::ostringstream stream;
    stream << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    stream << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
    return stream.str();
}

}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::initialize(const std::string& directory) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (initialized_) {
        return;
    }

    std::error_code error;
    std::filesystem::create_directories(directory, error);
    filePath_ = (std::filesystem::path(directory) / "digitalcam.log").string();

#ifdef DIGITALCAM_HAS_SPDLOG
    if (!spdlog::get("digitalcam")) {
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath_, true);
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        std::vector<spdlog::sink_ptr> sinks{fileSink, consoleSink};
        auto logger = std::make_shared<spdlog::logger>("digitalcam", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);
        spdlog::register_logger(logger);
    }
#else
    std::ofstream stream(filePath_, std::ios::app);
    if (!stream) {
        std::cerr << "Unable to open log file: " << filePath_ << '\n';
    }
#endif

    initialized_ = true;
}

void Logger::setLevel(Level level) {
    std::lock_guard<std::mutex> lock(logMutex);
    level_ = level;
#ifdef DIGITALCAM_HAS_SPDLOG
    if (const auto logger = spdlog::get("digitalcam")) {
        logger->set_level(toSpdlogLevel(level));
    }
#endif
}

void Logger::log(Level level, std::string_view message) {
    bool needsInitialization = false;
    {
        std::lock_guard<std::mutex> lock(logMutex);
        if (static_cast<std::uint8_t>(level) < static_cast<std::uint8_t>(level_)) {
            return;
        }
        needsInitialization = !initialized_;
    }

    if (needsInitialization) {
        const_cast<Logger*>(this)->initialize();
    }

    std::lock_guard<std::mutex> lock(logMutex);

#ifdef DIGITALCAM_HAS_SPDLOG
    if (const auto logger = spdlog::get("digitalcam")) {
        logger->log(toSpdlogLevel(level), message);
        return;
    }
#endif

    std::ofstream stream(filePath_, std::ios::app);
    if (stream) {
        stream << timestampText() << " [" << levelText(level) << "] " << message << '\n';
    }
    std::cout << '[' << levelText(level) << "] " << message << '\n';
}

void Logger::trace(std::string_view message) { log(Level::Trace, message); }
void Logger::debug(std::string_view message) { log(Level::Debug, message); }
void Logger::info(std::string_view message) { log(Level::Info, message); }
void Logger::warning(std::string_view message) { log(Level::Warning, message); }
void Logger::error(std::string_view message) { log(Level::Error, message); }
void Logger::critical(std::string_view message) { log(Level::Critical, message); }

}
