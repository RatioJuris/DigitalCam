#pragma once

#include <filesystem>

#include <digitalcam/core/MediaMetadata.h>
#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class MediaLoader {
public:
    Result probe(const std::filesystem::path& path, MediaMetadata& metadata) const;
    Result thumbnail(
        const std::filesystem::path& path,
        Size maxSize,
        VideoFrame& frame
    ) const;
};

}
