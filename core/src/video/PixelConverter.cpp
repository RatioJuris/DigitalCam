#include <digitalcam/core/video/PixelConverter.h>

#include <array>
#include <cstdint>

namespace digitalcam::core {

namespace {

using Pixel = std::array<std::uint8_t, 4>;

bool isPacked(PixelFormat format) noexcept {
    switch (format) {
        case PixelFormat::Bgra:
        case PixelFormat::Rgba:
        case PixelFormat::Rgb24:
        case PixelFormat::Bgr24:
            return true;
        default:
            return false;
    }
}

Pixel readPixel(const std::uint8_t* data, PixelFormat format) noexcept {
    switch (format) {
        case PixelFormat::Bgra:
            return {data[2], data[1], data[0], data[3]};
        case PixelFormat::Rgba:
            return {data[0], data[1], data[2], data[3]};
        case PixelFormat::Rgb24:
            return {data[0], data[1], data[2], 255};
        case PixelFormat::Bgr24:
            return {data[2], data[1], data[0], 255};
        default:
            return {0, 0, 0, 255};
    }
}

void writePixel(std::uint8_t* data, PixelFormat format, const Pixel& pixel) noexcept {
    switch (format) {
        case PixelFormat::Bgra:
            data[0] = pixel[2];
            data[1] = pixel[1];
            data[2] = pixel[0];
            data[3] = pixel[3];
            break;
        case PixelFormat::Rgba:
            data[0] = pixel[0];
            data[1] = pixel[1];
            data[2] = pixel[2];
            data[3] = pixel[3];
            break;
        case PixelFormat::Rgb24:
            data[0] = pixel[0];
            data[1] = pixel[1];
            data[2] = pixel[2];
            break;
        case PixelFormat::Bgr24:
            data[0] = pixel[2];
            data[1] = pixel[1];
            data[2] = pixel[0];
            break;
        default:
            break;
    }
}

}

bool PixelConverter::canConvert(PixelFormat source, PixelFormat target) noexcept {
    return isPacked(source) && isPacked(target);
}

Result PixelConverter::convert(
    const VideoFrame& source,
    PixelFormat target,
    VideoFrame& output
) {
    if (!source.size.valid()) {
        return Result::failure("Invalid source frame size");
    }
    if (!canConvert(source.format, target)) {
        return Result::failure("Unsupported pixel-format conversion");
    }

    const std::size_t sourceBytes = bytesPerPixel(source.format);
    const std::size_t targetBytes = bytesPerPixel(target);
    const std::size_t sourceStride =
        source.stride == 0 ? static_cast<std::size_t>(source.size.width) * sourceBytes : source.stride;
    const std::size_t targetStride = static_cast<std::size_t>(source.size.width) * targetBytes;

    output.size = source.size;
    output.format = target;
    output.timestamp = source.timestamp;
    output.stride = targetStride;
    output.data.assign(targetStride * static_cast<std::size_t>(source.size.height), 0);

    for (int y = 0; y < source.size.height; ++y) {
        const std::uint8_t* sourceRow = source.data.data() + static_cast<std::size_t>(y) * sourceStride;
        std::uint8_t* targetRow = output.data.data() + static_cast<std::size_t>(y) * targetStride;
        for (int x = 0; x < source.size.width; ++x) {
            const Pixel pixel = readPixel(sourceRow + static_cast<std::size_t>(x) * sourceBytes, source.format);
            writePixel(targetRow + static_cast<std::size_t>(x) * targetBytes, target, pixel);
        }
    }

    return Result::success();
}

}
