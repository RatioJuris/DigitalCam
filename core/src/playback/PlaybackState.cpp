#include <digitalcam/core/playback/PlaybackState.h>

namespace digitalcam::core {

bool canTransition(PlaybackState from, PlaybackState to) noexcept {
    if (from == to) {
        return false;
    }

    switch (from) {
        case PlaybackState::Idle:
            return to == PlaybackState::Ready || to == PlaybackState::Error;
        case PlaybackState::Ready:
            return to == PlaybackState::Playing ||
                   to == PlaybackState::Stopped ||
                   to == PlaybackState::Error;
        case PlaybackState::Playing:
            return to == PlaybackState::Paused ||
                   to == PlaybackState::Stopped ||
                   to == PlaybackState::Ready ||
                   to == PlaybackState::Error;
        case PlaybackState::Paused:
            return to == PlaybackState::Playing ||
                   to == PlaybackState::Stopped ||
                   to == PlaybackState::Ready ||
                   to == PlaybackState::Error;
        case PlaybackState::Stopped:
            return to == PlaybackState::Ready ||
                   to == PlaybackState::Playing ||
                   to == PlaybackState::Error;
        case PlaybackState::Error:
            return to == PlaybackState::Idle || to == PlaybackState::Ready;
    }
    return false;
}

const char* toString(PlaybackState state) noexcept {
    switch (state) {
        case PlaybackState::Idle: return "idle";
        case PlaybackState::Ready: return "ready";
        case PlaybackState::Playing: return "playing";
        case PlaybackState::Paused: return "paused";
        case PlaybackState::Stopped: return "stopped";
        case PlaybackState::Error: return "error";
    }
    return "idle";
}

}
