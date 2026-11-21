#include <digitalcam/core/audio/AudioSynchronizer.h>

#include <algorithm>
#include <cstdlib>

namespace digitalcam::core {

void AudioSynchronizer::setVideoTimestamp(Timestamp timestamp) noexcept {
    videoTimestamp_ = timestamp;
}

Timestamp AudioSynchronizer::videoTimestamp() const noexcept {
    return videoTimestamp_;
}

Timestamp AudioSynchronizer::align(Timestamp audioTimestamp) const noexcept {
    const Timestamp limit{20000000};
    const Timestamp delta = audioTimestamp - videoTimestamp_;
    if (delta > limit) {
        return audioTimestamp - (delta - limit);
    }
    if (delta < -limit) {
        return audioTimestamp + (-limit - delta);
    }
    return audioTimestamp;
}

void AudioSynchronizer::reset() noexcept {
    videoTimestamp_ = Timestamp{0};
}

}
