#include <digitalcam/virtual_camera/windows/CameraInstaller.h>

#define NOMINMAX

#include <windows.h>
#include <objbase.h>
#include <aclapi.h>
#include <sddl.h>

#include <cstdio>
#include <iterator>
#include <string>

#include <digitalcam/virtual_camera/windows/SharedFrame.h>

namespace digitalcam::virtual_camera::windows {

namespace {

constexpr wchar_t kSecurityDescriptor[] =
    L"D:P(A;;GA;;;SY)(A;;GA;;;LS)(A;;GA;;;NS)(A;;GA;;;BA)(A;;GA;;;BU)(A;;GA;;;WD)";

std::string lastError() {
    const DWORD code = GetLastError();
    wchar_t* message = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        0,
        reinterpret_cast<LPWSTR>(&message),
        0,
        nullptr
    );
    if (length == 0 || !message) {
        return "Windows error " + std::to_string(code);
    }

    std::wstring wide(message, length);
    LocalFree(message);
    while (!wide.empty() && (wide.back() == L'\r' || wide.back() == L'\n')) {
        wide.pop_back();
    }
    return std::string(wide.begin(), wide.end());
}

std::string hresultMessage(HRESULT value) {
    if (HRESULT_FACILITY(value) == FACILITY_WIN32) {
        const DWORD code = HRESULT_CODE(value);
        SetLastError(code);
        return lastError();
    }
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "HRESULT 0x%08X", static_cast<unsigned>(value));
    return buffer;
}

bool applySecurity(const wchar_t* path) {
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            kSecurityDescriptor,
            SDDL_REVISION_1,
            &descriptor,
            nullptr
        )) {
        return false;
    }

    BOOL present = FALSE;
    BOOL defaulted = FALSE;
    PACL dacl = nullptr;
    const bool valid = GetSecurityDescriptorDacl(descriptor, &present, &dacl, &defaulted) && present;
    const LSTATUS status = valid
        ? SetNamedSecurityInfoW(
              const_cast<wchar_t*>(path),
              SE_FILE_OBJECT,
              DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
              nullptr,
              nullptr,
              dacl,
              nullptr
          )
        : ERROR_INVALID_PARAMETER;
    LocalFree(descriptor);
    return status == ERROR_SUCCESS;
}

bool ensureDirectory(const wchar_t* path) {
    if (CreateDirectoryW(path, nullptr)) {
        return applySecurity(path);
    }
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
        return false;
    }
    return applySecurity(path);
}

std::wstring sourcePath(const wchar_t* name) {
    std::wstring executable(MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    while (length >= executable.size()) {
        executable.resize(executable.size() * 2);
        length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    }
    executable.resize(length);

    const std::size_t separator = executable.find_last_of(L"\\/");
    if (separator == std::wstring::npos) {
        return name;
    }
    return executable.substr(0, separator + 1) + name;
}

std::wstring targetPath() {
    return std::wstring(installDirectory()) + L"\\" + std::wstring(virtualCameraDllName());
}

core::Result elevatedOnly() {
    if (!isElevated()) {
        return core::Result::failure(
            "Administrator rights are required. Run DigitalCamCameraInstaller.exe as Administrator."
        );
    }
    return core::Result::success();
}

}

bool isElevated() {
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID administrators = nullptr;
    if (!AllocateAndInitializeSid(
            &authority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0,
            0,
            0,
            0,
            0,
            0,
            &administrators
        )) {
        return false;
    }

    BOOL member = FALSE;
    const bool result = CheckTokenMembership(nullptr, administrators, &member) && member;
    FreeSid(administrators);
    return result;
}

core::Result installCamera() {
    core::Result result = elevatedOnly();
    if (!result) {
        return result;
    }

    const std::wstring source = sourcePath(virtualCameraDllName().data());
    const std::wstring target = targetPath();
    if (GetFileAttributesW(source.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return core::Result::failure("DigitalCamVirtualCamera.dll is missing from the application folder.");
    }
    if (!ensureDirectory(installDirectory().data())) {
        return core::Result::failure("Cannot prepare the DigitalCam install directory: " + lastError());
    }
    if (!CopyFileW(source.c_str(), target.c_str(), FALSE)) {
        return core::Result::failure("Cannot copy the virtual camera DLL: " + lastError());
    }
    if (!applySecurity(target.c_str())) {
        return core::Result::failure("Cannot secure the virtual camera DLL: " + lastError());
    }
    if (!ensureSharedFrameStorage()) {
        return core::Result::failure("Cannot prepare the frame bridge: " + lastError());
    }

    HMODULE module = LoadLibraryW(target.c_str());
    if (!module) {
        return core::Result::failure("Cannot load the virtual camera DLL: " + lastError());
    }
    using RegisterFunction = HRESULT (*)();
    auto registerFunction = reinterpret_cast<RegisterFunction>(
        GetProcAddress(module, "DllRegisterServer")
    );
    HRESULT hr = registerFunction ? registerFunction() : HRESULT_FROM_WIN32(GetLastError());
    FreeLibrary(module);
    if (FAILED(hr)) {
        return core::Result::failure("Cannot register the virtual camera: " + hresultMessage(hr));
    }
    return core::Result::success();
}

core::Result uninstallCamera() {
    core::Result result = elevatedOnly();
    if (!result) {
        return result;
    }

    const std::wstring target = targetPath();
    HMODULE module = LoadLibraryW(target.c_str());
    if (module) {
        using UnregisterFunction = HRESULT (*)();
        auto unregisterFunction = reinterpret_cast<UnregisterFunction>(
            GetProcAddress(module, "DllUnregisterServer")
        );
        if (unregisterFunction) {
            const HRESULT hr = unregisterFunction();
            if (FAILED(hr)) {
                FreeLibrary(module);
                return core::Result::failure(
                    "Cannot unregister the virtual camera: " + hresultMessage(hr)
                );
            }
        }
        FreeLibrary(module);
    } else {
        wchar_t clsid[64]{};
        const GUID id = virtualCameraClsid();
        StringFromGUID2(id, clsid, static_cast<int>(std::size(clsid)));
        const std::wstring registryPath =
            L"SOFTWARE\\Classes\\CLSID\\" + std::wstring(clsid);
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, registryPath.c_str());
    }

    DeleteFileW(target.c_str());
    DeleteFileW(frameFilePath().data());
    RemoveDirectoryW(programDataDirectory().data());
    return core::Result::success();
}

}
