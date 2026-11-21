#include <digitalcam/core/playback/LoopController.h>

namespace digitalcam::core {

LoopController::LoopController(bool enabled) : enabled_(enabled) {}

void LoopController::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
}

bool LoopController::isEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_;
}

bool LoopController::shouldLoop() const {
    return isEnabled();
}

void LoopController::onEndReached() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (enabled_) {
        ++loopCount_;
    }
}

std::uint64_t LoopController::loopCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return loopCount_;
}

void LoopController::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    loopCount_ = 0;
}

}
