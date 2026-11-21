#include "CliOptions.h"

#include <charconv>
#include <cmath>
#include <system_error>
#include <utility>

#include <digitalcam/core/CoreEngine.h>
#include <digitalcam/core/Types.h>
#include <digitalcam/logging/Logger.h>

namespace digitalcam::app {

namespace {

constexpr int kMaxWidth = 1920;
constexpr int kMaxHeight = 1080;

void fail(CliOptions& options, const std::string& message) {
    options.error = message;
}

bool parseInteger(std::string_view text, int& result) {
    if (text.empty()) {
        return false;
    }
    const char* begin = text.data();
    const char* end = begin + text.size();
    int value = 0;
    const auto parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return false;
    }
    result = value;
    return true;
}

bool parseReal(std::string_view text, double& result) {
    if (text.empty()) {
        return false;
    }
    const char* begin = text.data();
    const char* end = begin + text.size();
    double value = 0.0;
    const auto parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end || !std::isfinite(value)) {
        return false;
    }
    result = value;
    return true;
}

}

bool parseCliOptions(int argc, char* argv[], CliOptions& options) {
    options = CliOptions{};
    bool positionalMediaUsed = false;

    for (int index = 1; index < argc; ++index) {
        std::string_view argument(argv[index]);
        std::string_view key = argument;
        std::string_view inlineValue;
        bool hasInlineValue = false;

        if (argument.size() > 1 && argument.front() == '-' && argument[1] != '-') {
            switch (argument[1]) {
                case 'm': key = "--media"; break;
                case 'w': key = "--width"; break;
                case 'H': key = "--height"; break;
                case 'f': key = "--fps"; break;
                case 'd': key = "--duration"; break;
                case 'l': key = "--log-dir"; break;
                case 's': key = "--start-camera"; break;
                default:
                    fail(options, "Unknown option: " + std::string(argument));
                    return false;
            }
            if (argument.size() > 2) {
                inlineValue = argument.substr(2);
                hasInlineValue = true;
            }
        } else if (argument.starts_with("--")) {
            const std::size_t separator = argument.find('=');
            if (separator != std::string_view::npos) {
                key = argument.substr(0, separator);
                inlineValue = argument.substr(separator + 1);
                hasInlineValue = true;
            }
        } else {
            if (positionalMediaUsed) {
                fail(options, "Only one media path can be supplied");
                return false;
            }
            options.media = std::string(argument);
            positionalMediaUsed = true;
            continue;
        }

        auto readValue = [&](std::string_view& value) -> bool {
            if (hasInlineValue) {
                if (inlineValue.empty()) {
                    fail(options, "Missing value for " + std::string(key));
                    return false;
                }
                value = inlineValue;
                return true;
            }
            if (index + 1 >= argc) {
                fail(options, "Missing value for " + std::string(key));
                return false;
            }
            value = argv[++index];
            return true;
        };

        if (key == "--help") {
            options.help = true;
        } else if (key == "--version") {
            options.version = true;
        } else if (key == "--start-camera") {
            options.startCamera = true;
        } else if (key == "--headless") {
            options.headless = true;
        } else if (key == "--verbose") {
            options.verbose = true;
        } else if (key == "--media") {
            std::string_view value;
            if (!readValue(value)) {
                return false;
            }
            options.media = std::string(value);
            positionalMediaUsed = true;
        } else if (key == "--width") {
            std::string_view value;
            if (!readValue(value) || !parseInteger(value, options.width)) {
                fail(options, "Invalid --width value: " + std::string(value));
                return false;
            }
        } else if (key == "--height") {
            std::string_view value;
            if (!readValue(value) || !parseInteger(value, options.height)) {
                fail(options, "Invalid --height value: " + std::string(value));
                return false;
            }
        } else if (key == "--fps") {
            std::string_view value;
            if (!readValue(value) || !parseReal(value, options.fps)) {
                fail(options, "Invalid --fps value: " + std::string(value));
                return false;
            }
        } else if (key == "--duration") {
            std::string_view value;
            double seconds = 0.0;
            if (!readValue(value) || !parseReal(value, seconds)) {
                fail(options, "Invalid --duration value: " + std::string(value));
                return false;
            }
            options.durationSeconds = seconds;
        } else if (key == "--log-dir") {
            std::string_view value;
            if (!readValue(value)) {
                return false;
            }
            options.logDirectory = std::string(value);
        } else {
            fail(options, "Unknown option: " + std::string(argument));
            return false;
        }
    }

    if (options.width <= 0 || options.width > kMaxWidth) {
        fail(options, "--width must be between 1 and " + std::to_string(kMaxWidth));
        return false;
    }
    if (options.height <= 0 || options.height > kMaxHeight) {
        fail(options, "--height must be between 1 and " + std::to_string(kMaxHeight));
        return false;
    }
    if (options.fps <= 0.0) {
        fail(options, "--fps must be greater than zero");
        return false;
    }
    if (options.durationSeconds && *options.durationSeconds <= 0.0) {
        fail(options, "--duration must be greater than zero");
        return false;
    }

    return true;
}

std::string cliUsage() {
    return
        "DigitalCam Studio 0.1.0\n"
        "Usage: DigitalCamStudio [options] [media-path]\n"
        "\n"
        "Options:\n"
        "  --media <path>        Image or video to publish (or pass path directly)\n"
        "  --width <pixels>      Output width, 1-1920 (default 1280)\n"
        "  --height <pixels>     Output height, 1-1080 (default 720)\n"
        "  --fps <rate>          Output frame rate (default 30)\n"
        "  --start-camera        Create the OS virtual camera at startup\n"
        "  --headless            Run without a GUI window\n"
        "  --duration <seconds>  Stop after the given number of seconds\n"
        "  --log-dir <path>      Log directory (default logs)\n"
        "  --verbose             Log debug messages\n"
        "  --help                Show this help\n"
        "  --version             Show the version\n"
        "\n"
        "Example:\n"
        "  DigitalCamStudio --headless --media sample.png --width 1280 --height 720\n"
        "    --fps 30 --start-camera --duration 60\n";
}

core::Result applyCliOptions(const CliOptions& options, core::CoreEngine& engine) {
    if (!options.media.empty()) {
        std::error_code error;
        if (!std::filesystem::exists(options.media, error)) {
            return core::Result::failure(
                "Media file not found: " + options.media.string()
            );
        }
        const core::Result result = engine.loadMedia(options.media, true);
        if (!result) {
            return result;
        }
    }

    if (options.startCamera) {
        core::VideoOutputFormat format;
        format.size.width = options.width;
        format.size.height = options.height;
        format.frameRate = options.fps;
        format.format = core::PixelFormat::Bgra;
        DCAM_LOG_INFO(
            "CLI camera start requested: " + std::to_string(options.width) + "x" +
            std::to_string(options.height) + " @" + std::to_string(options.fps) + " fps"
        );
        const core::Result result = engine.startCamera(format);
        if (!result) {
            return result;
        }
    }

    return core::Result::success();
}

}
