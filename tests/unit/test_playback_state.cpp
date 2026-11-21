#include <gtest/gtest.h>

#include <digitalcam/core/playback/PlaybackState.h>

using digitalcam::core::PlaybackState;
using digitalcam::core::canTransition;

TEST(PlaybackState, AllowsInitialReadyTransition) {
    EXPECT_TRUE(canTransition(PlaybackState::Idle, PlaybackState::Ready));
}

TEST(PlaybackState, RejectsInvalidReadyToIdleTransition) {
    EXPECT_FALSE(canTransition(PlaybackState::Ready, PlaybackState::Idle));
}

TEST(PlaybackState, AllowsPlaybackControlTransitions) {
    EXPECT_TRUE(canTransition(PlaybackState::Ready, PlaybackState::Playing));
    EXPECT_TRUE(canTransition(PlaybackState::Playing, PlaybackState::Paused));
    EXPECT_TRUE(canTransition(PlaybackState::Paused, PlaybackState::Playing));
    EXPECT_TRUE(canTransition(PlaybackState::Playing, PlaybackState::Stopped));
}

TEST(PlaybackState, RejectsSameStateTransition) {
    EXPECT_FALSE(canTransition(PlaybackState::Playing, PlaybackState::Playing));
}
