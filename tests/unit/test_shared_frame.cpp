#include <gtest/gtest.h>

#include <digitalcam/virtual_camera/windows/SharedFrame.h>

using digitalcam::virtual_camera::windows::FrameHeader;
using digitalcam::virtual_camera::windows::kFrameHeaderSize;
using digitalcam::virtual_camera::windows::kFrameSlotSize;
using digitalcam::virtual_camera::windows::kSlotCount;
using digitalcam::virtual_camera::windows::frameSlotOffset;
using digitalcam::virtual_camera::windows::sharedFrameFileSize;
using digitalcam::virtual_camera::windows::virtualCameraClsid;

TEST(SharedFrame, HeaderHasFixedLayout) {
    EXPECT_EQ(sizeof(FrameHeader), kFrameHeaderSize);
    EXPECT_EQ(kSlotCount, 2U);
}

TEST(SharedFrame, SlotsDoNotOverlap) {
    EXPECT_EQ(frameSlotOffset(0), kFrameHeaderSize);
    EXPECT_GT(frameSlotOffset(1), frameSlotOffset(0));
    EXPECT_EQ(sharedFrameFileSize(), frameSlotOffset(1) + kFrameSlotSize);
}

TEST(SharedFrame, VirtualCameraClsidIsStable) {
    const GUID id = virtualCameraClsid();
    EXPECT_EQ(id.Data1, 0x7C2E5AD1U);
    EXPECT_EQ(id.Data2, 0x9F43);
    EXPECT_EQ(id.Data3, 0x4B6E);
}
