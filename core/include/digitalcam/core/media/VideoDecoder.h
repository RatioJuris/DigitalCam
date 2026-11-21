#pragma once

#include <filesystem>
#include <memory>

#include <digitalcam/core/MediaMetadata.h>
#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class VideoDecoderImpl;

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    VideoDecoder(VideoDecoder&& other) noexcept;
    VideoDecoder& operator=(VideoDecoder&& other) noexcept;

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    Result open(const std::filesystem::path& path, MediaMetadata& metadata);
    DecodeStatus decodeNext(VideoFrame& frame, Result& result);
    Result seek(Timestamp position);
    Result reset();
    bool isOpen() const noexcept;

private:
    std::unique_ptr<VideoDecoderImpl> impl_;
};

}
