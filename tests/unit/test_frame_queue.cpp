#include <gtest/gtest.h>

#include <digitalcam/core/video/FrameQueue.h>

using digitalcam::core::FrameQueue;

TEST(FrameQueue, PushesAndPopsInOrder) {
    FrameQueue<int> queue(4);
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    int value = 0;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, 2);
    EXPECT_FALSE(queue.pop(value));
}

TEST(FrameQueue, DropsOldestWhenFull) {
    FrameQueue<int> queue(2);
    queue.push(1);
    queue.push(2);
    queue.push(3);
    EXPECT_EQ(queue.size(), 2U);
    int value = 0;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, 2);
}

TEST(FrameQueue, ClearRemovesAllFrames) {
    FrameQueue<int> queue(4);
    queue.push(1);
    queue.push(2);
    queue.clear();
    EXPECT_TRUE(queue.empty());
}
