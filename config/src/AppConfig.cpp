#include <digitalcam/config/AppConfig.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace digitalcam::config {

ConfigResult ConfigResult::success() {
    return ConfigResult{true, {}};
}

ConfigResult ConfigResult::failure(std::string message) {
    return ConfigResult{false, std::move(message)};
}

ConfigResult::operator bool() const noexcept {
    return ok;
}

const char* toString(ConfigPixelFormat format) noexcept {
    switch (format) {
        case ConfigPixelFormat::Bgra: return "bgra";
        case ConfigPixelFormat::Rgba: return "rgba";
        case ConfigPixelFormat::Rgb24: return "rgb24";
        case ConfigPixelFormat::Bgr24: return "bgr24";
    }
    return "bgra";
}

const char* toString(ConfigAspectMode mode) noexcept {
    switch (mode) {
        case ConfigAspectMode::Fit: return "fit";
        case ConfigAspectMode::Fill: return "fill";
        case ConfigAspectMode::Original: return "original";
        case ConfigAspectMode::Crop: return "crop";
    }
    return "fit";
}

bool fromString(const std::string& text, ConfigPixelFormat& format) noexcept {
    if (text == "bgra") format = ConfigPixelFormat::Bgra;
    else if (text == "rgba") format = ConfigPixelFormat::Rgba;
    else if (text == "rgb24") format = ConfigPixelFormat::Rgb24;
    else if (text == "bgr24") format = ConfigPixelFormat::Bgr24;
    else return false;
    return true;
}

bool fromString(const std::string& text, ConfigAspectMode& mode) noexcept {
    if (text == "fit") mode = ConfigAspectMode::Fit;
    else if (text == "fill") mode = ConfigAspectMode::Fill;
    else if (text == "original") mode = ConfigAspectMode::Original;
    else if (text == "crop") mode = ConfigAspectMode::Crop;
    else return false;
    return true;
}

namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool parseBool(const std::string& value, bool& output) {
    if (value == "true" || value == "1" || value == "yes") {
        output = true;
        return true;
    }
    if (value == "false" || value == "0" || value == "no") {
        output = false;
        return true;
    }
    return false;
}

template <typename T>
bool parseNumber(const std::string& value, T& output) {
    std::istringstream stream(value);
    stream >> output;
    return !stream.fail();
}

void writeKeyValue(std::ostream& stream, const std::string& key, const std::string& value) {
    stream << key << '=' << value << '\n';
}

}

ConfigResult AppConfigStore::load(const std::string& path, AppConfig& config) const {
    std::ifstream stream(path);
    if (!stream) {
        return ConfigResult::failure("Unable to open configuration: " + path);
    }

    std::string section;
    std::string line;
    while (std::getline(stream, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == ';' || trimmed.front() == '#') {
            continue;
        }
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            section = trim(trimmed.substr(1, trimmed.size() - 2));
            continue;
        }

        const auto separator = trimmed.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim(trimmed.substr(0, separator));
        const std::string value = trim(trimmed.substr(separator + 1));

        if (section == "video") {
            if (key == "width") parseNumber(value, config.video.width);
            else if (key == "height") parseNumber(value, config.video.height);
            else if (key == "framerate") parseNumber(value, config.video.frameRate);
            else if (key == "pixelformat") fromString(value, config.video.pixelFormat);
            else if (key == "aspectmode") fromString(value, config.video.aspectMode);
            else if (key == "backgroundred") parseNumber(value, config.video.backgroundRed);
            else if (key == "backgroundgreen") parseNumber(value, config.video.backgroundGreen);
            else if (key == "backgroundblue") parseNumber(value, config.video.backgroundBlue);
        } else if (section == "playback") {
            if (key == "automaticmode") parseBool(value, config.playback.automaticMode);
            else if (key == "autostart") parseBool(value, config.playback.autoStart);
            else if (key == "loopvideos") parseBool(value, config.playback.loopVideos);
            else if (key == "volume") parseNumber(value, config.playback.volume);
            else if (key == "muted") parseBool(value, config.playback.muted);
        }
    }

    return ConfigResult::success();
}

ConfigResult AppConfigStore::save(const std::string& path, const AppConfig& config) const {
    const std::filesystem::path filePath(path);
    if (filePath.has_parent_path()) {
        std::error_code error;
        std::filesystem::create_directories(filePath.parent_path(), error);
    }

    std::ofstream stream(path, std::ios::trunc);
    if (!stream) {
        return ConfigResult::failure("Unable to write configuration: " + path);
    }

    stream << "[video]\n";
    writeKeyValue(stream, "width", std::to_string(config.video.width));
    writeKeyValue(stream, "height", std::to_string(config.video.height));
    writeKeyValue(stream, "framerate", std::to_string(config.video.frameRate));
    writeKeyValue(stream, "pixelformat", toString(config.video.pixelFormat));
    writeKeyValue(stream, "aspectmode", toString(config.video.aspectMode));
    writeKeyValue(stream, "backgroundred", std::to_string(config.video.backgroundRed));
    writeKeyValue(stream, "backgroundgreen", std::to_string(config.video.backgroundGreen));
    writeKeyValue(stream, "backgroundblue", std::to_string(config.video.backgroundBlue));

    stream << "\n[playback]\n";
    writeKeyValue(stream, "automaticmode", config.playback.automaticMode ? "true" : "false");
    writeKeyValue(stream, "autostart", config.playback.autoStart ? "true" : "false");
    writeKeyValue(stream, "loopvideos", config.playback.loopVideos ? "true" : "false");
    writeKeyValue(stream, "volume", std::to_string(config.playback.volume));
    writeKeyValue(stream, "muted", config.playback.muted ? "true" : "false");

    return ConfigResult::success();
}

}
