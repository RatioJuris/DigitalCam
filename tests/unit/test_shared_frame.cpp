#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <digitalcam/virtual_camera/windows/SharedFrame.h>

using digitalcam::virtual_camera::windows::FrameHeader;
using digitalcam::virtual_camera::windows::FrameInfo;
using digitalcam::virtual_camera::windows::SharedFrameReader;
using digitalcam::virtual_camera::windows::SharedFrameWriter;
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

TEST(SharedFrame, WriterAndReaderExchangeFrames) {
    constexpr std::uint32_t width = 320;
    constexpr std::uint32_t height = 240;
    constexpr std::size_t stride = width * 4;
    std::vector<std::uint8_t> source(stride * height, 0xA5);

    SharedFrameWriter writer;
    ASSERT_TRUE(writer.open());
    ASSERT_TRUE(writer.configure(width, height, 30.0));
    ASSERT_TRUE(writer.publish(width, height, stride, source.data()));

    SharedFrameReader reader;
    ASSERT_TRUE(reader.open());

    FrameInfo info{};
    ASSERT_TRUE(reader.info(info));
    EXPECT_EQ(info.width, width);
    EXPECT_EQ(info.height, height);

    std::vector<std::uint8_t> output(source.size(), 0);
    ASSERT_TRUE(reader.copyTo(output.data(), stride, output.size(), info));
    EXPECT_EQ(output, source);
}
