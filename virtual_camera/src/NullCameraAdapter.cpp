#include <digitalcam/virtual_camera/NullCameraAdapter.h>

#include <mutex>
#include <utility>

namespace digitalcam::virtual_camera {

std::string NullCameraAdapter::name() const {
    return "Null Camera";
}

digitalcam::core::Result NullCameraAdapter::install() {
    installed_.store(true);
    return digitalcam::core::Result::success();
}

digitalcam::core::Result NullCameraAdapter::start(
    const digitalcam::core::VideoOutputFormat& format
) {
    if (!installed_.load()) {
        return digitalcam::core::Result::failure("Null camera adapter is not installed");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    format_ = format;
    running_.store(true);
    return digitalcam::core::Result::success();
}

digitalcam::core::Result NullCameraAdapter::publishFrame(
    const digitalcam::core::VideoFrame& frame
) {
    if (!running_.load()) {
        return digitalcam::core::Result::failure("Null camera adapter is not running");
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lastFrame_ = frame;
    }
    publishedFrames_.fetch_add(1);
    return digitalcam::core::Result::success();
}

void NullCameraAdapter::stop() {
    running_.store(false);
}

bool NullCameraAdapter::isAvailable() const {
    return installed_.load() && running_.load();
}

digitalcam::core::VideoFrame NullCameraAdapter::lastFrame() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastFrame_;
}

std::uint64_t NullCameraAdapter::publishedFrames() const noexcept {
    return publishedFrames_.load();
}

digitalcam::core::VideoOutputFormat NullCameraAdapter::format() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return format_;
}

}
