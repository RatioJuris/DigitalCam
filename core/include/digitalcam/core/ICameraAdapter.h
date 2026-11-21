#pragma once

#include <memory>
#include <string>

#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class ICameraAdapter {
public:
    virtual ~ICameraAdapter() = default;

    virtual std::string name() const = 0;
    virtual Result install() = 0;
    virtual Result start(const VideoOutputFormat& format) = 0;
    virtual Result publishFrame(const VideoFrame& frame) = 0;
    virtual void stop() = 0;
    virtual bool isAvailable() const = 0;
};

using CameraAdapterPtr = std::shared_ptr<ICameraAdapter>;

}
