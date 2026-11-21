#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "CliOptions.h"

namespace {

std::vector<char*> makeArgv(std::vector<std::string>& arguments) {
    std::vector<char*> result;
    result.reserve(arguments.size());
    for (auto& argument : arguments) {
        result.push_back(argument.data());
    }
    return result;
}

}

TEST(CliOptions, Defaults) {
    std::vector<std::string> arguments{"DigitalCamStudio"};
    auto argv = makeArgv(arguments);
    digitalcam::app::CliOptions options;

    ASSERT_TRUE(digitalcam::app::parseCliOptions(
        static_cast<int>(argv.size()), argv.data(), options
    ));
    EXPECT_EQ(options.width, 1280);
    EXPECT_EQ(options.height, 720);
    EXPECT_DOUBLE_EQ(options.fps, 30.0);
    EXPECT_FALSE(options.startCamera);
    EXPECT_FALSE(options.headless);
}

TEST(CliOptions, ParsesLongAndInlineOptions) {
    std::vector<std::string> arguments{
        "DigitalCamStudio",
        "--media=clip.mp4",
        "--width", "640",
        "--height=480",
        "--fps", "29.97",
        "--duration=12.5",
        "--start-camera",
        "--headless",
        "--verbose",
        "--log-dir", "diagnostics"
    };
    auto argv = makeArgv(arguments);
    digitalcam::app::CliOptions options;

    ASSERT_TRUE(digitalcam::app::parseCliOptions(
        static_cast<int>(argv.size()), argv.data(), options
    ));
    EXPECT_EQ(options.media, "clip.mp4");
    EXPECT_EQ(options.width, 640);
    EXPECT_EQ(options.height, 480);
    EXPECT_DOUBLE_EQ(options.fps, 29.97);
    ASSERT_TRUE(options.durationSeconds.has_value());
    EXPECT_DOUBLE_EQ(*options.durationSeconds, 12.5);
    EXPECT_TRUE(options.startCamera);
    EXPECT_TRUE(options.headless);
    EXPECT_TRUE(options.verbose);
    EXPECT_EQ(options.logDirectory, "diagnostics");
}

TEST(CliOptions, ParsesPositionalMediaAndShortOptions) {
    std::vector<std::string> arguments{
        "DigitalCamCli", "still.png", "-w640", "-H360", "-f60", "-s"
    };
    auto argv = makeArgv(arguments);
    digitalcam::app::CliOptions options;

    ASSERT_TRUE(digitalcam::app::parseCliOptions(
        static_cast<int>(argv.size()), argv.data(), options
    ));
    EXPECT_EQ(options.media, "still.png");
    EXPECT_EQ(options.width, 640);
    EXPECT_EQ(options.height, 360);
    EXPECT_DOUBLE_EQ(options.fps, 60.0);
    EXPECT_TRUE(options.startCamera);
}

TEST(CliOptions, RejectsInvalidValues) {
    const std::vector<std::vector<std::string>> invalidSets{
        {"DigitalCamStudio", "--width", "0"},
        {"DigitalCamStudio", "--width", "1921"},
        {"DigitalCamStudio", "--height", "0"},
        {"DigitalCamStudio", "--fps", "0"},
        {"DigitalCamStudio", "--duration", "-1"},
        {"DigitalCamStudio", "--width", "wide"},
        {"DigitalCamStudio", "--unknown"}
    };

    for (auto arguments : invalidSets) {
        auto argv = makeArgv(arguments);
        digitalcam::app::CliOptions options;
        EXPECT_FALSE(digitalcam::app::parseCliOptions(
            static_cast<int>(argv.size()), argv.data(), options
        ));
        EXPECT_FALSE(options.error.empty());
    }
}

TEST(CliOptions, HelpAndVersion) {
    std::vector<std::string> arguments{"DigitalCamStudio", "--help", "--version"};
    auto argv = makeArgv(arguments);
    digitalcam::app::CliOptions options;

    ASSERT_TRUE(digitalcam::app::parseCliOptions(
        static_cast<int>(argv.size()), argv.data(), options
    ));
    EXPECT_TRUE(options.help);
    EXPECT_TRUE(options.version);
    EXPECT_FALSE(digitalcam::app::cliUsage().empty());
}
