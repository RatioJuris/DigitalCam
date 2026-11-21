#include <gtest/gtest.h>

#include <filesystem>

#include <digitalcam/config/AppConfig.h>

using digitalcam::config::AppConfig;
using digitalcam::config::AppConfigStore;
using digitalcam::config::ConfigAspectMode;
using digitalcam::config::ConfigPixelFormat;

TEST(AppConfig, SaveAndLoadRoundTrip) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "digitalcam_config_test.ini";

    AppConfig source;
    source.video.width = 1280;
    source.video.height = 720;
    source.video.frameRate = 60.0;
    source.video.pixelFormat = ConfigPixelFormat::Rgba;
    source.video.aspectMode = ConfigAspectMode::Fill;
    source.playback.loopVideos = false;
    source.playback.volume = 0.5F;
    source.playback.muted = true;

    AppConfigStore store;
    ASSERT_TRUE(store.save(path.string(), source));

    AppConfig loaded;
    ASSERT_TRUE(store.load(path.string(), loaded));
    EXPECT_EQ(loaded.video.width, 1280);
    EXPECT_EQ(loaded.video.height, 720);
    EXPECT_DOUBLE_EQ(loaded.video.frameRate, 60.0);
    EXPECT_EQ(loaded.video.pixelFormat, ConfigPixelFormat::Rgba);
    EXPECT_EQ(loaded.video.aspectMode, ConfigAspectMode::Fill);
    EXPECT_FALSE(loaded.playback.loopVideos);
    EXPECT_FLOAT_EQ(loaded.playback.volume, 0.5F);
    EXPECT_TRUE(loaded.playback.muted);

    std::error_code error;
    std::filesystem::remove(path, error);
}
