#pragma once

#include <cstdint>

namespace digitalcam::core {

enum class PlaybackState : std::uint8_t {
    Idle,
    Ready,
    Playing,
    Paused,
    Stopped,
    Error
};

bool canTransition(PlaybackState from, PlaybackState to) noexcept;
const char* toString(PlaybackState state) noexcept;

}
