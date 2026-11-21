#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>

#include <digitalcam/core/CoreEngine.h>
#include <digitalcam/logging/Logger.h>
#include <digitalcam/virtual_audio/NullAudioAdapter.h>
#include <digitalcam/virtual_camera/NullCameraAdapter.h>

#ifdef DIGITALCAM_WINDOWS_CAMERA
#include <digitalcam/virtual_camera/windows/WindowsVirtualCameraAdapter.h>
#endif

#include "CliOptions.h"

namespace {

std::atomic<bool> interrupted{false};

void onSignal(int) {
    interrupted.store(true);
}

}

int main(int argc, char* argv[]) {
    digitalcam::app::CliOptions options;
    if (!digitalcam::app::parseCliOptions(argc, argv, options)) {
        std::cerr << options.error << '\n' << digitalcam::app::cliUsage() << '\n';
        return 2;
    }

    if (options.help) {
        std::cout << digitalcam::app::cliUsage() << '\n';
        return 0;
    }
    if (options.version) {
        std::cout << "DigitalCam 0.1.0\n";
        return 0;
    }
    if (options.media.empty() && !options.startCamera) {
        std::cerr << "Supply a media path or --start-camera\n"
                  << digitalcam::app::cliUsage() << '\n';
        return 2;
    }

    digitalcam::logging::Logger::instance().initialize(options.logDirectory);
    if (options.verbose) {
        digitalcam::logging::Logger::instance().setLevel(
            digitalcam::logging::Level::Debug
        );
    }

    DCAM_LOG_INFO("DigitalCam CLI started");
    DCAM_LOG_INFO("CLI options: media=" + options.media.string() +
        " width=" + std::to_string(options.width) +
        " height=" + std::to_string(options.height) +
        " fps=" + std::to_string(options.fps) +
        " startCamera=" + std::string(options.startCamera ? "true" : "false"));

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

#ifdef DIGITALCAM_WINDOWS_CAMERA
    auto camera = std::make_shared<
        digitalcam::virtual_camera::windows::WindowsVirtualCameraAdapter>();
#else
    auto camera = std::make_shared<digitalcam::virtual_camera::NullCameraAdapter>();
#endif
    auto audio = std::make_shared<digitalcam::virtual_audio::NullAudioAdapter>();
    digitalcam::core::CoreEngine engine(camera, audio);

    const digitalcam::core::Result result =
        digitalcam::app::applyCliOptions(options, engine);
    if (!result) {
        DCAM_LOG_ERROR("CLI startup failed: " + result.message);
        std::cerr << result.message << '\n';
        return 1;
    }

    if (options.startCamera) {
        const auto deadline = options.durationSeconds
            ? std::optional<std::chrono::steady_clock::time_point>{
                std::chrono::steady_clock::now() +
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(*options.durationSeconds)
                )}
            : std::nullopt;

        while (!interrupted.load()) {
            const auto now = std::chrono::steady_clock::now();
            if (deadline && now >= *deadline) {
                break;
            }
            const auto remaining = deadline
                ? std::min<std::chrono::steady_clock::duration>(
                    *deadline - now,
                    std::chrono::milliseconds(100)
                )
                : std::chrono::steady_clock::duration{std::chrono::milliseconds(100)};
            std::this_thread::sleep_for(remaining);
        }
    }

    engine.stopCamera();
    const auto status = engine.status();
    DCAM_LOG_INFO("DigitalCam CLI stopped: camera=" +
        std::string(status.cameraAvailable ? "available" : "stopped"));
    return 0;
}
