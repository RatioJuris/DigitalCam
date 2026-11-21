#include <digitalcam/core/playback/PlaybackController.h>

namespace digitalcam::core {

PlaybackState PlaybackController::state() const noexcept {
    return state_;
}

Result PlaybackController::transitionTo(PlaybackState next) {
    if (!canTransition(state_, next)) {
        return Result::failure(
            std::string("Invalid playback transition: ") +
            toString(state_) + " -> " + toString(next)
        );
    }
    state_ = next;
    if (next != PlaybackState::Error) {
        lastError_.clear();
    }
    return Result::success();
}

Result PlaybackController::markReady() {
    return transitionTo(PlaybackState::Ready);
}

Result PlaybackController::markPlaying() {
    return transitionTo(PlaybackState::Playing);
}

Result PlaybackController::markPaused() {
    return transitionTo(PlaybackState::Paused);
}

Result PlaybackController::markStopped() {
    return transitionTo(PlaybackState::Stopped);
}

Result PlaybackController::markError(std::string message) {
    lastError_ = std::move(message);
    if (state_ == PlaybackState::Error) {
        return Result::success();
    }
    return transitionTo(PlaybackState::Error);
}

}
