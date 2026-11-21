#include <digitalcam/core/video/FrameProcessor.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>

#include <digitalcam/core/video/PixelConverter.h>

namespace digitalcam::core {

namespace {

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

using Pixel = std::array<std::uint8_t, 4>;

Pixel backgroundPixel(const RgbColor& color) {
    return {color.red, color.green, color.blue, 255};
}

void writeBgra(std::uint8_t* data, const Pixel& pixel) noexcept {
    data[0] = pixel[2];
    data[1] = pixel[1];
    data[2] = pixel[0];
    data[3] = pixel[3];
}

Rect sourceRectFor(const Size& source, const Size& target, bool nativeLimit) {
    const double sourceAspect = static_cast<double>(source.width) / source.height;
    const double targetAspect = static_cast<double>(target.width) / target.height;

    int width = source.width;
    int height = source.height;
    if (sourceAspect > targetAspect) {
        height = source.height;
        width = std::max(1, static_cast<int>(std::lround(height * targetAspect)));
    } else {
        width = source.width;
        height = std::max(1, static_cast<int>(std::lround(width / targetAspect)));
    }

    if (nativeLimit) {
        width = std::min(width, target.width);
        height = std::min(height, target.height);
    }

    width = std::min(width, source.width);
    height = std::min(height, source.height);

    return {
        (source.width - width) / 2,
        (source.height - height) / 2,
        width,
        height
    };
}

Rect containedRect(const Size& content, const Size& bounds, double scale) {
    const int width = std::max(1, static_cast<int>(std::lround(content.width * scale)));
    const int height = std::max(1, static_cast<int>(std::lround(content.height * scale)));
    return {
        (bounds.width - width) / 2,
        (bounds.height - height) / 2,
        std::min(width, bounds.width),
        std::min(height, bounds.height)
    };
}

}

void FrameProcessor::configure(const FrameProcessorOptions& options) {
    if (options.outputSize.valid()) {
        options_ = options;
    }
}

const FrameProcessorOptions& FrameProcessor::options() const noexcept {
    return options_;
}

Result FrameProcessor::process(const VideoFrame& input, VideoFrame& output) const {
    if (!input.size.valid() || input.data.empty()) {
        return Result::failure("Invalid input frame");
    }
    if (!options_.outputSize.valid()) {
        return Result::failure("Invalid output size");
    }

    VideoFrame bgra;
    const Result toBgra = PixelConverter::convert(input, PixelFormat::Bgra, bgra);
    if (!toBgra) {
        return toBgra;
    }

    const Size& target = options_.outputSize;
    Rect sourceRect;
    Rect destinationRect;

    const double widthScale = static_cast<double>(target.width) / bgra.size.width;
    const double heightScale = static_cast<double>(target.height) / bgra.size.height;

    switch (options_.aspectMode) {
        case AspectMode::Fit:
            sourceRect = {0, 0, bgra.size.width, bgra.size.height};
            destinationRect = containedRect(bgra.size, target, std::min(widthScale, heightScale));
            break;
        case AspectMode::Original:
            sourceRect = {0, 0, bgra.size.width, bgra.size.height};
            destinationRect = containedRect(bgra.size, target, std::min(1.0, std::min(widthScale, heightScale)));
            break;
        case AspectMode::Fill:
            sourceRect = sourceRectFor(bgra.size, target, false);
            destinationRect = {0, 0, target.width, target.height};
            break;
        case AspectMode::Crop:
            sourceRect = sourceRectFor(bgra.size, target, true);
            destinationRect = {0, 0, target.width, target.height};
            break;
    }

    VideoFrame scaled;
    scaled.size = target;
    scaled.format = PixelFormat::Bgra;
    scaled.stride = static_cast<std::size_t>(target.width) * 4U;
    scaled.timestamp = input.timestamp;
    scaled.data.assign(scaled.stride * static_cast<std::size_t>(target.height), 0);

    const Pixel background = backgroundPixel(options_.background);
    for (int y = 0; y < target.height; ++y) {
        std::uint8_t* row = scaled.data.data() + static_cast<std::size_t>(y) * scaled.stride;
        for (int x = 0; x < target.width; ++x) {
            writeBgra(row + static_cast<std::size_t>(x) * 4U, background);
        }
    }

    const std::size_t sourceStride = bgra.stride == 0 ? static_cast<std::size_t>(bgra.size.width) * 4U : bgra.stride;
    for (int y = 0; y < destinationRect.height; ++y) {
        const int sourceY = sourceRect.y + static_cast<int>(
            static_cast<long long>(y) * sourceRect.height / destinationRect.height
        );
        const std::uint8_t* sourceRow = bgra.data.data() + static_cast<std::size_t>(sourceY) * sourceStride;
        std::uint8_t* targetRow = scaled.data.data() + static_cast<std::size_t>(destinationRect.y + y) * scaled.stride;
        for (int x = 0; x < destinationRect.width; ++x) {
            const int sourceX = sourceRect.x + static_cast<int>(
                static_cast<long long>(x) * sourceRect.width / destinationRect.width
            );
            const std::uint8_t* sourcePixel = sourceRow + static_cast<std::size_t>(sourceX) * 4U;
            std::uint8_t* targetPixel = targetRow + static_cast<std::size_t>(destinationRect.x + x) * 4U;
            targetPixel[0] = sourcePixel[0];
            targetPixel[1] = sourcePixel[1];
            targetPixel[2] = sourcePixel[2];
            targetPixel[3] = sourcePixel[3];
        }
    }

    if (options_.outputFormat == PixelFormat::Bgra) {
        output = std::move(scaled);
        return Result::success();
    }

    return PixelConverter::convert(scaled, options_.outputFormat, output);
}

}
