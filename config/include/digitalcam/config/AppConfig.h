#pragma once

#include <cstdint>
#include <string>

namespace digitalcam::config {

enum class ConfigPixelFormat : std::uint8_t {
    Bgra,
    Rgba,
    Rgb24,
    Bgr24
};

enum class ConfigAspectMode : std::uint8_t {
    Fit,
    Fill,
    Original,
    Crop
};

struct ConfigResult {
    bool ok = false;
    std::string message;

    static ConfigResult success();
    static ConfigResult failure(std::string message);

    explicit operator bool() const noexcept;
};

struct VideoConfig {
    int width = 1920;
    int height = 1080;
    double frameRate = 30.0;
    ConfigPixelFormat pixelFormat = ConfigPixelFormat::Bgra;
    ConfigAspectMode aspectMode = ConfigAspectMode::Fit;
    std::uint8_t backgroundRed = 0;
    std::uint8_t backgroundGreen = 0;
    std::uint8_t backgroundBlue = 0;
};

struct PlaybackConfig {
    bool automaticMode = true;
    bool autoStart = true;
    bool loopVideos = true;
    float volume = 1.0F;
    bool muted = false;
};

struct AppConfig {
    VideoConfig video;
    PlaybackConfig playback;
    std::string path = "config/digitalcam.ini";
};

class AppConfigStore {
public:
    ConfigResult load(const std::string& path, AppConfig& config) const;
    ConfigResult save(const std::string& path, const AppConfig& config) const;
};

const char* toString(ConfigPixelFormat format) noexcept;
const char* toString(ConfigAspectMode mode) noexcept;

bool fromString(const std::string& text, ConfigPixelFormat& format) noexcept;
bool fromString(const std::string& text, ConfigAspectMode& mode) noexcept;

}
