#include <digitalcam/virtual_camera/windows/WindowsVirtualCameraAdapter.h>

#include <windows.h>
#include <mfapi.h>
#include <mfvirtualcamera.h>

#include <cstdio>
#include <iterator>
#include <vector>

namespace digitalcam::virtual_camera::windows {

namespace {

std::string narrow(const std::wstring& value) {
    const int bytes = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (bytes <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(bytes), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        result.data(),
        bytes,
        nullptr,
        nullptr
    );
    return result;
}

std::string hresultMessage(HRESULT value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "HRESULT 0x%08X", static_cast<unsigned>(value));
    return buffer;
}

std::string registrationPath() {
    wchar_t clsid[64]{};
    StringFromGUID2(virtualCameraClsid(), clsid, static_cast<int>(std::size(clsid)));
    return narrow(L"SOFTWARE\\Classes\\CLSID\\" + std::wstring(clsid) + L"\\InprocServer32");
}

}

WindowsVirtualCameraAdapter::WindowsVirtualCameraAdapter() = default;

WindowsVirtualCameraAdapter::~WindowsVirtualCameraAdapter() {
    stop();
}

std::string WindowsVirtualCameraAdapter::name() const {
    return "Windows Media Foundation virtual camera";
}

core::Result WindowsVirtualCameraAdapter::install() {
    HKEY key = nullptr;
    const LSTATUS status = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        registrationPath().c_str(),
        0,
        KEY_READ,
        &key
    );
    if (status != ERROR_SUCCESS) {
        return core::Result::failure(
            "DigitalCam Virtual Camera is not installed. Run "
            "DigitalCamCameraInstaller.exe as Administrator, then start the camera again."
        );
    }

    wchar_t dllPath[MAX_PATH]{};
    DWORD size = sizeof(dllPath);
    const LSTATUS queryStatus = RegQueryValueExW(
        key,
        nullptr,
        nullptr,
        nullptr,
        reinterpret_cast<BYTE*>(dllPath),
        &size
    );
    RegCloseKey(key);
    if (queryStatus != ERROR_SUCCESS ||
        GetFileAttributesW(dllPath) == INVALID_FILE_ATTRIBUTES) {
        return core::Result::failure(
            "Virtual camera registration is broken. Run "
            "DigitalCamCameraInstaller.exe as Administrator again."
        );
    }
    return core::Result::success();
}

core::Result WindowsVirtualCameraAdapter::start(const core::VideoOutputFormat& format) {
    std::lock_guard<std::mutex> lock(mutex_);
    stopLocked();

    if (!format.size.valid() || format.frameRate <= 0.0 ||
        static_cast<std::uint32_t>(format.size.width) > kMaxFrameWidth ||
        static_cast<std::uint32_t>(format.size.height) > kMaxFrameHeight) {
        return core::Result::failure("Unsupported virtual camera output format");
    }

    HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        return core::Result::failure("Media Foundation startup failed: " + hresultMessage(hr));
    }
    mfStarted_ = true;

    if (!writer_.open() || !writer_.configure(
            static_cast<std::uint32_t>(format.size.width),
            static_cast<std::uint32_t>(format.size.height),
            format.frameRate
        )) {
        stopLocked();
        return core::Result::failure("Cannot open the DigitalCam frame bridge");
    }

    const std::size_t frameBytes = static_cast<std::size_t>(format.size.width) *
        static_cast<std::size_t>(format.size.height) * 4;
    std::vector<std::uint8_t> initialFrame(frameBytes, 0);
    for (std::size_t offset = 3; offset < initialFrame.size(); offset += 4) {
        initialFrame[offset] = 255;
    }
    if (!writer_.publish(
            static_cast<std::uint32_t>(format.size.width),
            static_cast<std::uint32_t>(format.size.height),
            static_cast<std::size_t>(format.size.width) * 4,
            initialFrame.data()
        )) {
        stopLocked();
        return core::Result::failure("Cannot publish the initial virtual camera frame");
    }

    wchar_t clsid[64]{};
    StringFromGUID2(virtualCameraClsid(), clsid, static_cast<int>(std::size(clsid)));
    const std::wstring friendlyName(virtualCameraName());
    hr = MFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource,
        MFVirtualCameraLifetime_Session,
        MFVirtualCameraAccess_CurrentUser,
        friendlyName.c_str(),
        clsid,
        nullptr,
        0,
        camera_.put()
    );
    if (FAILED(hr)) {
        stopLocked();
        return core::Result::failure("Cannot create the virtual camera: " + hresultMessage(hr));
    }

    hr = camera_->Start(nullptr);
    if (FAILED(hr)) {
        stopLocked();
        return core::Result::failure("Cannot start the virtual camera: " + hresultMessage(hr));
    }

    available_ = true;
    return core::Result::success();
}

core::Result WindowsVirtualCameraAdapter::publishFrame(const core::VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!available_ || frame.format != core::PixelFormat::Bgra || !frame.size.valid()) {
        return core::Result::failure("Virtual camera is not ready for this frame");
    }
    if (!writer_.publish(
            static_cast<std::uint32_t>(frame.size.width),
            static_cast<std::uint32_t>(frame.size.height),
            frame.stride,
            frame.data.data()
        )) {
        return core::Result::failure("Cannot publish a virtual camera frame");
    }
    return core::Result::success();
}

void WindowsVirtualCameraAdapter::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopLocked();
}

bool WindowsVirtualCameraAdapter::isAvailable() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return available_;
}

void WindowsVirtualCameraAdapter::stopLocked() {
    available_ = false;
    if (camera_) {
        camera_->Remove();
        camera_.reset();
    }
    writer_.close();
    if (mfStarted_) {
        MFShutdown();
        mfStarted_ = false;
    }
}

}
