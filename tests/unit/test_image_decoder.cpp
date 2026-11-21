#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <fstream>

#include <digitalcam/core/media/ImageDecoder.h>

namespace {

constexpr std::array<unsigned char, 69> kPng{{
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82,
    0, 0, 0, 1, 0, 0, 0, 1, 8, 2, 0, 0, 0, 144, 119, 83, 222, 0,
    0, 0, 12, 73, 68, 65, 84, 120, 156, 99, 224, 81, 178, 0, 0, 0,
    164, 0, 103, 152, 208, 224, 152, 0, 0, 0, 0, 73, 69, 78, 68,
    174, 66, 96, 130
}};

}

TEST(ImageDecoder, LoadsPngWithoutBestStreamParameters) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "digitalcam-test-image.png";
    {
        std::ofstream file(path, std::ios::binary);
        file.write(
            reinterpret_cast<const char*>(kPng.data()),
            static_cast<std::streamsize>(kPng.size())
        );
    }

    digitalcam::core::ImageDecoder decoder;
    digitalcam::core::VideoFrame frame;
    const auto result = decoder.load(path, {}, frame);

    std::error_code error;
    std::filesystem::remove(path, error);
    ASSERT_TRUE(result) << result.message;
    EXPECT_EQ(frame.size.width, 1);
    EXPECT_EQ(frame.size.height, 1);
    EXPECT_EQ(frame.format, digitalcam::core::PixelFormat::Bgra);
    EXPECT_FALSE(frame.data.empty());
}
