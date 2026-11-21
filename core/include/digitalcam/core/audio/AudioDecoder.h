#pragma once

#include <filesystem>
#include <memory>

#include <digitalcam/core/MediaMetadata.h>
#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class AudioDecoderImpl;

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    AudioDecoder(AudioDecoder&& other) noexcept;
    AudioDecoder& operator=(AudioDecoder&& other) noexcept;

    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    Result open(const std::filesystem::path& path, MediaMetadata& metadata);
    DecodeStatus decodeNext(AudioBuffer& buffer, Result& result);
    Result reset();
    bool isOpen() const noexcept;

private:
    std::unique_ptr<AudioDecoderImpl> impl_;
};

}
