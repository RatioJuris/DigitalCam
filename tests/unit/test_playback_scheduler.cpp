#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include <digitalcam/core/playback/PlaybackScheduler.h>

using digitalcam::core::PlaybackScheduler;
using digitalcam::core::Timestamp;

TEST(PlaybackScheduler, UsesRequestedFrameRate) {
    PlaybackScheduler scheduler(30.0);
    EXPECT_DOUBLE_EQ(scheduler.frameRate(), 30.0);
    EXPECT_NEAR(scheduler.interval().count(), 33333333, 1000);
}

TEST(PlaybackScheduler, ReportsDueAfterInterval) {
    PlaybackScheduler scheduler(1000.0);
    scheduler.reset();
    EXPECT_FALSE(scheduler.due());
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_TRUE(scheduler.due());
}

TEST(PlaybackScheduler, RejectsInvalidFrameRate) {
    PlaybackScheduler scheduler(0.0);
    EXPECT_DOUBLE_EQ(scheduler.frameRate(), 30.0);
}
