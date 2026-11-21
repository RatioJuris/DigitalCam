#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

#include <digitalcam/core/Types.h>

namespace digitalcam::core {

enum class MediaType : std::uint8_t {
    Unknown,
    Image,
    Video
};

struct MediaMetadata {
    std::filesystem::path path;
    MediaType type = MediaType::Unknown;
    Size size;
    double frameRate = 0.0;
    Timestamp duration{0};
    bool hasVideo = false;
    bool hasAudio = false;
    std::string videoCodec;
    int audioChannels = 0;
    int audioSampleRate = 0;
    std::string audioCodec;
};

const char* toString(MediaType type) noexcept;

bool isSupportedImageExtension(const std::filesystem::path& path) noexcept;
bool isSupportedVideoExtension(const std::filesystem::path& path) noexcept;
bool isSupportedExtension(const std::filesystem::path& path) noexcept;

}
