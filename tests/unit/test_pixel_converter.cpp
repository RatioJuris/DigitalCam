#include <gtest/gtest.h>

#include <digitalcam/core/video/PixelConverter.h>

using digitalcam::core::PixelConverter;
using digitalcam::core::PixelFormat;
using digitalcam::core::VideoFrame;

TEST(PixelConverter, ConvertsBgraToRgba) {
    VideoFrame source;
    source.size = {1, 1};
    source.format = PixelFormat::Bgra;
    source.stride = 4;
    source.data = {10, 20, 30, 40};

    VideoFrame output;
    ASSERT_TRUE(PixelConverter::convert(source, PixelFormat::Rgba, output));
    ASSERT_EQ(output.data.size(), 4U);
    EXPECT_EQ(output.data[0], 30);
    EXPECT_EQ(output.data[1], 20);
    EXPECT_EQ(output.data[2], 10);
    EXPECT_EQ(output.data[3], 40);
}

TEST(PixelConverter, RejectsPlanarFormats) {
    EXPECT_FALSE(PixelConverter::canConvert(PixelFormat::I420, PixelFormat::Bgra));
}
