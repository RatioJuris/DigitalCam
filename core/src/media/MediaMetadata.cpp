#include <digitalcam/core/MediaMetadata.h>

#include <algorithm>
#include <cctype>
#include <initializer_list>

namespace digitalcam::core {

namespace {

std::string lowerExtension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char value) { return static_cast<char>(std::tolower(value)); }
    );
    return extension;
}

bool matches(const std::filesystem::path& path, std::initializer_list<const char*> extensions) {
    const std::string extension = lowerExtension(path);
    return std::any_of(
        extensions.begin(),
        extensions.end(),
        [&extension](const char* candidate) { return extension == candidate; }
    );
}

}

const char* toString(MediaType type) noexcept {
    switch (type) {
        case MediaType::Image: return "image";
        case MediaType::Video: return "video";
        case MediaType::Unknown: return "unknown";
    }
    return "unknown";
}

const char* toString(PixelFormat format) noexcept {
    switch (format) {
        case PixelFormat::Bgra: return "bgra";
        case PixelFormat::Rgba: return "rgba";
        case PixelFormat::Rgb24: return "rgb24";
        case PixelFormat::Bgr24: return "bgr24";
        case PixelFormat::I420: return "i420";
        case PixelFormat::Nv12: return "nv12";
        case PixelFormat::Unknown: return "unknown";
    }
    return "unknown";
}

std::size_t bytesPerPixel(PixelFormat format) noexcept {
    switch (format) {
        case PixelFormat::Bgra:
        case PixelFormat::Rgba:
            return 4;
        case PixelFormat::Rgb24:
        case PixelFormat::Bgr24:
            return 3;
        case PixelFormat::I420:
        case PixelFormat::Nv12:
            return 2;
        case PixelFormat::Unknown:
            return 0;
    }
    return 0;
}

bool Size::valid() const noexcept {
    return width > 0 && height > 0;
}

bool isSupportedImageExtension(const std::filesystem::path& path) noexcept {
    return matches(path, {".jpg", ".jpeg", ".png", ".bmp", ".webp", ".tif", ".tiff"});
}

bool isSupportedVideoExtension(const std::filesystem::path& path) noexcept {
    return matches(path, {".mp4", ".mov", ".mkv", ".avi", ".webm", ".mpeg", ".mpg"});
}

bool isSupportedExtension(const std::filesystem::path& path) noexcept {
    return isSupportedImageExtension(path) || isSupportedVideoExtension(path);
}

}
