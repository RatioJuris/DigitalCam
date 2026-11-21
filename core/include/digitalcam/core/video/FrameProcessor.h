#pragma once

#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::core {

enum class AspectMode : std::uint8_t {
    Fit,
    Fill,
    Original,
    Crop
};

struct FrameProcessorOptions {
    Size outputSize{1920, 1080};
    PixelFormat outputFormat = PixelFormat::Bgra;
    AspectMode aspectMode = AspectMode::Fit;
    RgbColor background;
};

class FrameProcessor {
public:
    void configure(const FrameProcessorOptions& options);
    const FrameProcessorOptions& options() const noexcept;

    Result process(const VideoFrame& input, VideoFrame& output) const;

private:
    FrameProcessorOptions options_;
};

}
