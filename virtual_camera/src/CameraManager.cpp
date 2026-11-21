#include <digitalcam/virtual_camera/CameraManager.h>

#include <utility>

namespace digitalcam::virtual_camera {

CameraManager::CameraManager(digitalcam::core::CameraAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

digitalcam::core::Result CameraManager::install() {
    if (!adapter_) {
        return digitalcam::core::Result::failure("No camera adapter configured");
    }
    return adapter_->install();
}

digitalcam::core::Result CameraManager::start(
    const digitalcam::core::VideoOutputFormat& format
) {
    if (!adapter_) {
        return digitalcam::core::Result::failure("No camera adapter configured");
    }
    return adapter_->start(format);
}

digitalcam::core::Result CameraManager::publish(
    const digitalcam::core::VideoFrame& frame
) {
    if (!adapter_) {
        return digitalcam::core::Result::failure("No camera adapter configured");
    }
    return adapter_->publishFrame(frame);
}

void CameraManager::stop() {
    if (adapter_) {
        adapter_->stop();
    }
}

bool CameraManager::isAvailable() const {
    return adapter_ != nullptr && adapter_->isAvailable();
}

std::string CameraManager::name() const {
    return adapter_ ? adapter_->name() : std::string();
}

digitalcam::core::ICameraAdapter& CameraManager::adapter() const {
    return *adapter_;
}

}
