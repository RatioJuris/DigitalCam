#include <gtest/gtest.h>

#include <digitalcam/core/playback/LoopController.h>

using digitalcam::core::LoopController;

TEST(LoopController, CountsEndsWhenEnabled) {
    LoopController controller(true);
    EXPECT_TRUE(controller.shouldLoop());
    controller.onEndReached();
    controller.onEndReached();
    EXPECT_EQ(controller.loopCount(), 2U);
}

TEST(LoopController, DoesNotCountWhenDisabled) {
    LoopController controller(false);
    controller.onEndReached();
    EXPECT_FALSE(controller.shouldLoop());
    EXPECT_EQ(controller.loopCount(), 0U);
}

TEST(LoopController, ResetClearsCount) {
    LoopController controller(true);
    controller.onEndReached();
    controller.reset();
    EXPECT_EQ(controller.loopCount(), 0U);
}
