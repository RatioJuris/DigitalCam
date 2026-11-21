#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <digitalcam/core/Result.h>

namespace digitalcam::core {
class CoreEngine;
}

namespace digitalcam::app {

struct CliOptions {
    std::filesystem::path media;
    int width = 1280;
    int height = 720;
    double fps = 30.0;
    bool startCamera = false;
    bool headless = false;
    bool verbose = false;
    bool help = false;
    bool version = false;
    std::optional<double> durationSeconds;
    std::string logDirectory = "logs";
    std::string error;
};

bool parseCliOptions(int argc, char* argv[], CliOptions& options);
std::string cliUsage();
digitalcam::core::Result applyCliOptions(
    const CliOptions& options,
    digitalcam::core::CoreEngine& engine
);

}
