#include <QApplication>
#include <QCoreApplication>
#include <QTimer>

#include <chrono>
#include <iostream>
#include <memory>

#include <digitalcam/core/CoreEngine.h>
#include <digitalcam/logging/Logger.h>
#include <digitalcam/virtual_audio/NullAudioAdapter.h>
#include <digitalcam/virtual_camera/NullCameraAdapter.h>

#ifdef DIGITALCAM_WINDOWS_CAMERA
#include <digitalcam/virtual_camera/windows/WindowsVirtualCameraAdapter.h>
#endif

#include "CliOptions.h"
#include "MainWindow.h"

namespace {

#ifdef DIGITALCAM_WINDOWS_CAMERA
std::shared_ptr<digitalcam::core::ICameraAdapter> createCameraAdapter() {
    return std::make_shared<
        digitalcam::virtual_camera::windows::WindowsVirtualCameraAdapter>();
}
#else
std::shared_ptr<digitalcam::core::ICameraAdapter> createCameraAdapter() {
    return std::make_shared<digitalcam::virtual_camera::NullCameraAdapter>();
}
#endif

}

int main(int argc, char* argv[]) {
    digitalcam::app::CliOptions options;
    if (!digitalcam::app::parseCliOptions(argc, argv, options)) {
        digitalcam::logging::Logger::instance().initialize();
        digitalcam::logging::Logger::instance().error(options.error);
        std::cerr << options.error << '\n'
                  << digitalcam::app::cliUsage() << '\n';
        return 2;
    }

    digitalcam::logging::Logger::instance().initialize(options.logDirectory);
    if (options.verbose) {
        digitalcam::logging::Logger::instance().setLevel(
            digitalcam::logging::Level::Debug
        );
    }

    if (options.help) {
        const std::string usage = digitalcam::app::cliUsage();
        digitalcam::logging::Logger::instance().info(usage);
        std::cout << usage << '\n';
        return 0;
    }
    if (options.version) {
        digitalcam::logging::Logger::instance().info("DigitalCam 0.1.0");
        std::cout << "DigitalCam 0.1.0\n";
        return 0;
    }

    DCAM_LOG_INFO("DigitalCam Studio starting");
    std::unique_ptr<QCoreApplication> application;
    if (options.headless) {
        application = std::make_unique<QCoreApplication>(argc, argv);
    } else {
        application = std::make_unique<QApplication>(argc, argv);
    }
    QCoreApplication::setApplicationName("DigitalCam Studio");
    QCoreApplication::setApplicationVersion("0.1.0");

    auto camera = createCameraAdapter();
    auto audio = std::make_shared<digitalcam::virtual_audio::NullAudioAdapter>();
    auto engine = std::make_shared<digitalcam::core::CoreEngine>(camera, audio);

    std::unique_ptr<MainWindow> window;
    if (!options.headless) {
        window = std::make_unique<MainWindow>(engine);
        window->show();
    }

    if (!options.media.empty() || options.startCamera) {
        const digitalcam::core::Result result =
            digitalcam::app::applyCliOptions(options, *engine);
        if (!result) {
            DCAM_LOG_ERROR("CLI startup failed: " + result.message);
            if (window) {
                window->reportError(result.message);
            } else {
                std::cerr << result.message << '\n';
            }
        }
    }

    if (options.durationSeconds) {
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double>(*options.durationSeconds)
        );
        QTimer::singleShot(
            static_cast<int>(milliseconds.count()),
            application.get(),
            &QCoreApplication::quit
        );
    } else if (options.headless && !options.startCamera) {
        DCAM_LOG_INFO("Headless session has no camera; exiting");
        return 0;
    }

    return application->exec();
}
