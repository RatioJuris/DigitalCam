#pragma once

#include <chrono>
#include <mutex>

#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class PlaybackScheduler {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit PlaybackScheduler(double frameRate = 30.0);

    void setFrameRate(double frameRate);
    double frameRate() const;
    Timestamp interval() const;

    void reset();
    bool due() const;
    void markPublished();
    void waitUntilNext() const;

    static TimePoint now() noexcept;

private:
    Timestamp intervalLocked() const noexcept;

    mutable std::mutex mutex_;
    double frameRate_ = 30.0;
    TimePoint lastPublish_{};
};

}
