#include <QApplication>

#include <memory>

#include <digitalcam/core/CoreEngine.h>
#include <digitalcam/logging/Logger.h>
#include <digitalcam/virtual_audio/NullAudioAdapter.h>
#include <digitalcam/virtual_camera/NullCameraAdapter.h>

#ifdef DIGITALCAM_WINDOWS_CAMERA
#include <digitalcam/virtual_camera/windows/WindowsVirtualCameraAdapter.h>
#endif

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName("DigitalCam Studio");
    QCoreApplication::setApplicationVersion("0.1.0");

    digitalcam::logging::Logger::instance().initialize();

#ifdef DIGITALCAM_WINDOWS_CAMERA
    auto camera = std::make_shared<
        digitalcam::virtual_camera::windows::WindowsVirtualCameraAdapter>();
#else
    auto camera = std::make_shared<digitalcam::virtual_camera::NullCameraAdapter>();
#endif
    auto audio = std::make_shared<digitalcam::virtual_audio::NullAudioAdapter>();
    auto engine = std::make_shared<digitalcam::core::CoreEngine>(camera, audio);

    MainWindow window(engine);
    window.show();
    return application.exec();
}
