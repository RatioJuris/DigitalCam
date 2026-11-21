#include <digitalcam/core/playback/PlaybackScheduler.h>

#include <algorithm>
#include <thread>

namespace digitalcam::core {

PlaybackScheduler::PlaybackScheduler(double frameRate)
    : frameRate_(frameRate > 0.0 ? frameRate : 30.0),
      lastPublish_(Clock::now()) {}

void PlaybackScheduler::setFrameRate(double frameRate) {
    std::lock_guard<std::mutex> lock(mutex_);
    frameRate_ = frameRate > 0.0 ? frameRate : 30.0;
    lastPublish_ = Clock::now();
}

double PlaybackScheduler::frameRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frameRate_;
}

Timestamp PlaybackScheduler::intervalLocked() const noexcept {
    const std::chrono::duration<double> seconds(1.0 / frameRate_);
    return std::chrono::duration_cast<Timestamp>(seconds);
}

Timestamp PlaybackScheduler::interval() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return intervalLocked();
}

void PlaybackScheduler::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    lastPublish_ = Clock::now();
}

bool PlaybackScheduler::due() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return Clock::now() - lastPublish_ >= intervalLocked();
}

void PlaybackScheduler::markPublished() {
    std::lock_guard<std::mutex> lock(mutex_);
    lastPublish_ = Clock::now();
}

void PlaybackScheduler::waitUntilNext() const {
    std::unique_lock<std::mutex> lock(mutex_);
    const auto deadline = lastPublish_ + intervalLocked();
    lock.unlock();
    std::this_thread::sleep_until(deadline);
}

PlaybackScheduler::TimePoint PlaybackScheduler::now() noexcept {
    return Clock::now();
}

}
