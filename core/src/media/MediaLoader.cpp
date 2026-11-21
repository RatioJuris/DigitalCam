#include <digitalcam/core/media/MediaLoader.h>

#include <algorithm>
#include <cmath>
#include <filesystem>

#include <digitalcam/core/audio/AudioDecoder.h>
#include <digitalcam/core/media/ImageDecoder.h>
#include <digitalcam/core/media/VideoDecoder.h>
#include <digitalcam/core/video/FrameProcessor.h>

namespace digitalcam::core {

Result MediaLoader::probe(
    const std::filesystem::path& path,
    MediaMetadata& metadata
) const {
    std::error_code error;
    if (!std::filesystem::exists(path, error) || !std::filesystem::is_regular_file(path, error)) {
        return Result::failure("Media file does not exist: " + path.string());
    }
    if (!isSupportedExtension(path)) {
        return Result::failure("Unsupported media format: " + path.extension().string());
    }

    if (isSupportedImageExtension(path)) {
        ImageDecoder decoder;
        VideoFrame frame;
        const Result result = decoder.load(path, {}, frame);
        if (!result) {
            return result;
        }
        metadata = MediaMetadata{};
        metadata.path = path;
        metadata.type = MediaType::Image;
        metadata.size = frame.size;
        metadata.hasVideo = true;
        metadata.duration = Timestamp{0};
        return Result::success();
    }

    VideoDecoder decoder;
    return decoder.open(path, metadata);
}

Result MediaLoader::thumbnail(
    const std::filesystem::path& path,
    Size maxSize,
    VideoFrame& frame
) const {
    if (!maxSize.valid()) {
        return Result::failure("Invalid thumbnail size");
    }

    VideoFrame source;
    if (isSupportedImageExtension(path)) {
        ImageDecoder decoder;
        const Result result = decoder.load(path, {}, source);
        if (!result) {
            return result;
        }
    } else if (isSupportedVideoExtension(path)) {
        VideoDecoder decoder;
        MediaMetadata metadata;
        Result result = decoder.open(path, metadata);
        if (!result) {
            return result;
        }
        Result decodeResult;
        const DecodeStatus status = decoder.decodeNext(source, decodeResult);
        if (status != DecodeStatus::Ok) {
            return decodeResult.ok ? Result::failure("Unable to decode video thumbnail") : decodeResult;
        }
    } else {
        return Result::failure("Unsupported media format: " + path.extension().string());
    }

    FrameProcessor processor;
    FrameProcessorOptions options;
    options.outputSize = maxSize;
    options.outputFormat = PixelFormat::Bgra;
    options.aspectMode = AspectMode::Fit;
    processor.configure(options);
    return processor.process(source, frame);
}

}
