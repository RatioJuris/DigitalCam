#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace digitalcam::core {

using Timestamp = std::chrono::nanoseconds;

enum class PixelFormat : std::uint8_t {
    Unknown,
    Bgra,
    Rgba,
    Rgb24,
    Bgr24,
    I420,
    Nv12
};

struct Size {
    int width = 0;
    int height = 0;

    bool valid() const noexcept;
};

struct RgbColor {
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
};

struct VideoFrame {
    Size size;
    PixelFormat format = PixelFormat::Unknown;
    std::size_t stride = 0;
    std::vector<std::uint8_t> data;
    Timestamp timestamp{0};
};

struct AudioBuffer {
    int sampleRate = 48000;
    int channels = 2;
    std::vector<float> samples;
    Timestamp timestamp{0};
};

struct VideoOutputFormat {
    Size size;
    double frameRate = 30.0;
    PixelFormat format = PixelFormat::Bgra;
};

const char* toString(PixelFormat format) noexcept;
std::size_t bytesPerPixel(PixelFormat format) noexcept;

}
