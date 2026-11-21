#pragma once

#include <digitalcam/core/Result.h>
#include <digitalcam/core/playback/PlaybackState.h>

namespace digitalcam::core {

class PlaybackController {
public:
    PlaybackState state() const noexcept;

    Result transitionTo(PlaybackState next);
    Result markReady();
    Result markPlaying();
    Result markPaused();
    Result markStopped();
    Result markError(std::string message);

private:
    PlaybackState state_ = PlaybackState::Idle;
    std::string lastError_;
};

}
