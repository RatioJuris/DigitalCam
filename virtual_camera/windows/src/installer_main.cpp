#include <iostream>
#include <string>

#include <digitalcam/virtual_camera/windows/CameraInstaller.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: DigitalCamCameraInstaller.exe --install|--uninstall\n";
        return 2;
    }

    const std::string command = argv[1];
    if (command == "--install") {
        const auto result = digitalcam::virtual_camera::windows::installCamera();
        if (!result) {
            std::cerr << result.message << '\n';
            return 1;
        }
        std::cout << "DigitalCam Virtual Camera installed.\n";
        return 0;
    }

    if (command == "--uninstall") {
        const auto result = digitalcam::virtual_camera::windows::uninstallCamera();
        if (!result) {
            std::cerr << result.message << '\n';
            return 1;
        }
        std::cout << "DigitalCam Virtual Camera uninstalled.\n";
        return 0;
    }

    std::cerr << "Unknown command: " << command << '\n';
    return 2;
}
