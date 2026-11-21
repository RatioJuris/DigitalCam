#pragma once

#include <memory>
#include <mutex>
#include <string>

#include <mfvirtualcamera.h>
#include <wil/com.h>

#include <digitalcam/core/ICameraAdapter.h>

#include <digitalcam/virtual_camera/windows/SharedFrame.h>

namespace digitalcam::virtual_camera::windows {

class WindowsVirtualCameraAdapter final : public digitalcam::core::ICameraAdapter {
public:
    WindowsVirtualCameraAdapter();
    ~WindowsVirtualCameraAdapter() override;

    std::string name() const override;
    digitalcam::core::Result install() override;
    digitalcam::core::Result start(const digitalcam::core::VideoOutputFormat& format) override;
    digitalcam::core::Result publishFrame(const digitalcam::core::VideoFrame& frame) override;
    void stop() override;
    bool isAvailable() const override;

private:
    mutable std::mutex mutex_;
    SharedFrameWriter writer_;
    wil::com_ptr_nothrow<IMFVirtualCamera> camera_;
    bool mfStarted_ = false;
    bool available_ = false;

    void stopLocked();
};

}
