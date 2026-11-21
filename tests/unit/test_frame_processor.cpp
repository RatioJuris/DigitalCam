#include <gtest/gtest.h>

#include <digitalcam/core/video/FrameProcessor.h>

using digitalcam::core::AspectMode;
using digitalcam::core::FrameProcessor;
using digitalcam::core::FrameProcessorOptions;
using digitalcam::core::PixelFormat;
using digitalcam::core::VideoFrame;

TEST(FrameProcessor, FitsFrameIntoOutput) {
    VideoFrame input;
    input.size = {2, 2};
    input.format = PixelFormat::Bgra;
    input.stride = 8;
    input.data.assign(16, 0);
    for (std::size_t index = 0; index < input.data.size(); index += 4) {
        input.data[index] = 10;
        input.data[index + 1] = 20;
        input.data[index + 2] = 30;
        input.data[index + 3] = 255;
    }

    FrameProcessorOptions options;
    options.outputSize = {4, 4};
    options.outputFormat = PixelFormat::Bgra;
    options.aspectMode = AspectMode::Fit;

    FrameProcessor processor;
    processor.configure(options);

    VideoFrame output;
    ASSERT_TRUE(processor.process(input, output));
    EXPECT_EQ(output.size.width, 4);
    EXPECT_EQ(output.size.height, 4);
    EXPECT_EQ(output.format, PixelFormat::Bgra);
    EXPECT_EQ(output.data.size(), 64U);
}
