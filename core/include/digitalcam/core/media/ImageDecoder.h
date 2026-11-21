#pragma once

#include <filesystem>
#include <memory>

#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class ImageDecoderImpl;

class ImageDecoder {
public:
    ImageDecoder();
    ~ImageDecoder();

    ImageDecoder(ImageDecoder&& other) noexcept;
    ImageDecoder& operator=(ImageDecoder&& other) noexcept;

    ImageDecoder(const ImageDecoder&) = delete;
    ImageDecoder& operator=(const ImageDecoder&) = delete;

    Result load(
        const std::filesystem::path& path,
        Size requestedSize,
        VideoFrame& frame
    );

private:
    std::unique_ptr<ImageDecoderImpl> impl_;
};

}
