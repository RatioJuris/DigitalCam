#pragma once

#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class PixelConverter {
public:
    static bool canConvert(PixelFormat source, PixelFormat target) noexcept;
    static Result convert(const VideoFrame& source, PixelFormat target, VideoFrame& output);
};

}
