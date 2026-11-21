#pragma once

#include <memory>
#include <string>

#include <digitalcam/core/ICameraAdapter.h>
#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::virtual_camera {

class CameraManager {
public:
    explicit CameraManager(digitalcam::core::CameraAdapterPtr adapter);

    digitalcam::core::Result install();
    digitalcam::core::Result start(const digitalcam::core::VideoOutputFormat& format);
    digitalcam::core::Result publish(const digitalcam::core::VideoFrame& frame);
    void stop();

    bool isAvailable() const;
    std::string name() const;

    digitalcam::core::ICameraAdapter& adapter() const;

private:
    digitalcam::core::CameraAdapterPtr adapter_;
};

}
