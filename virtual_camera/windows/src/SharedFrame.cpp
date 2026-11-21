#include <digitalcam/virtual_camera/windows/SharedFrame.h>

#include <windows.h>
#include <aclapi.h>
#include <sddl.h>

#include <algorithm>
#include <cmath>

namespace digitalcam::virtual_camera::windows {

namespace {

constexpr wchar_t kProgramDataDirectory[] = L"C:\\ProgramData\\DigitalCam";
constexpr wchar_t kFrameFilePath[] = L"C:\\ProgramData\\DigitalCam\\frame-bridge.bin";
constexpr wchar_t kInstallDirectory[] = L"C:\\Program Files\\DigitalCam";
constexpr wchar_t kVirtualCameraName[] = L"DigitalCam Virtual Camera";
constexpr wchar_t kVirtualCameraDllName[] = L"DigitalCamVirtualCamera.dll";

constexpr wchar_t kSecurityDescriptor[] =
    L"D:P(A;;GA;;;SY)(A;;GA;;;LS)(A;;GA;;;NS)(A;;GA;;;BA)(A;;GA;;;BU)(A;;GA;;;WD)";

class LocalSecurityDescriptor {
public:
    LocalSecurityDescriptor() {
        ConvertStringSecurityDescriptorToSecurityDescriptorW(
            kSecurityDescriptor,
            SDDL_REVISION_1,
            &value_,
            nullptr
        );
    }

    ~LocalSecurityDescriptor() {
        if (value_) {
            LocalFree(value_);
        }
    }

    LocalSecurityDescriptor(const LocalSecurityDescriptor&) = delete;
    LocalSecurityDescriptor& operator=(const LocalSecurityDescriptor&) = delete;

    PSECURITY_DESCRIPTOR get() const noexcept {
        return value_;
    }

    SECURITY_ATTRIBUTES attributes() const noexcept {
        SECURITY_ATTRIBUTES result{};
        result.nLength = sizeof(result);
        result.lpSecurityDescriptor = value_;
        result.bInheritHandle = FALSE;
        return result;
    }

private:
    PSECURITY_DESCRIPTOR value_ = nullptr;
};

bool ensureDirectory(const wchar_t* path) {
    LocalSecurityDescriptor security;
    SECURITY_ATTRIBUTES attributes = security.attributes();
    if (CreateDirectoryW(path, &attributes)) {
        return true;
    }
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

volatile LONG* asLong32(std::uint32_t* value) noexcept {
    return reinterpret_cast<volatile LONG*>(value);
}

volatile LONG64* asLong64(std::uint64_t* value) noexcept {
    return reinterpret_cast<volatile LONG64*>(value);
}

void fillBlack(
    std::uint8_t* destination,
    std::size_t stride,
    std::size_t destinationSize,
    std::uint32_t width,
    std::uint32_t height
) {
    if (!destination || stride == 0) {
        return;
    }

    const std::size_t rowBytes = std::min(static_cast<std::size_t>(width) * 4, stride);
    const std::size_t rows = std::min(
        static_cast<std::size_t>(height),
        destinationSize / stride
    );

    for (std::size_t row = 0; row < rows; ++row) {
        std::uint8_t* current = destination + row * stride;
        for (std::size_t column = 0; column < rowBytes; column += 4) {
            current[column] = 0;
            current[column + 1] = 0;
            current[column + 2] = 0;
            current[column + 3] = 255;
        }
        if (stride > rowBytes) {
            std::fill(current + rowBytes, current + stride, 0);
        }
    }
}

}

GUID virtualCameraClsid() noexcept {
    return GUID{
        0x7c2e5ad1,
        0x9f43,
        0x4b6e,
        {0xa1, 0x2d, 0x8c, 0x64, 0x3e, 0x51, 0xb7, 0x0a}
    };
}

std::wstring_view virtualCameraName() noexcept {
    return kVirtualCameraName;
}

std::wstring_view frameFilePath() noexcept {
    return kFrameFilePath;
}

std::wstring_view programDataDirectory() noexcept {
    return kProgramDataDirectory;
}

std::wstring_view installDirectory() noexcept {
    return kInstallDirectory;
}

std::wstring_view virtualCameraDllName() noexcept {
    return kVirtualCameraDllName;
}

std::size_t frameSlotOffset(std::uint32_t index) noexcept {
    return kFrameHeaderSize + static_cast<std::size_t>(index) * kFrameSlotSize;
}

std::size_t sharedFrameFileSize() noexcept {
    return kFrameHeaderSize + static_cast<std::size_t>(kSlotCount) * kFrameSlotSize;
}

bool ensureSharedFrameStorage() {
    if (!ensureDirectory(kProgramDataDirectory)) {
        return false;
    }

    LocalSecurityDescriptor security;
    SECURITY_ATTRIBUTES attributes = security.attributes();
    HANDLE file = CreateFileW(
        kFrameFilePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &attributes,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER size{};
    size.QuadPart = static_cast<LONGLONG>(sharedFrameFileSize());
    const bool resized = SetFilePointerEx(file, size, nullptr, FILE_BEGIN) &&
        SetEndOfFile(file);
    CloseHandle(file);
    return resized;
}

SharedFrameWriter::~SharedFrameWriter() {
    close();
}

bool SharedFrameWriter::open() {
    close();
    if (!ensureSharedFrameStorage()) {
        return false;
    }

    LocalSecurityDescriptor security;
    SECURITY_ATTRIBUTES attributes = security.attributes();
    HANDLE file = CreateFileW(
        kFrameFilePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &attributes,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    HANDLE mapping = CreateFileMappingW(
        file,
        &attributes,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(sharedFrameFileSize()),
        nullptr
    );
    CloseHandle(file);
    if (!mapping) {
        return false;
    }

    void* view = MapViewOfFile(
        mapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sharedFrameFileSize()
    );
    CloseHandle(mapping);
    if (!view) {
        return false;
    }

    view_ = view;
    handle_ = view;
    auto* header = static_cast<FrameHeader*>(view_);
    if (header->magic != kSharedFrameMagic ||
        header->version != kSharedFrameVersion ||
        header->slotCount != kSlotCount) {
        std::fill_n(static_cast<std::uint8_t*>(view_), kFrameHeaderSize, 0);
        header->magic = kSharedFrameMagic;
        header->version = kSharedFrameVersion;
        header->slotCount = kSlotCount;
        header->fpsNumerator = 30000;
        header->fpsDenominator = 1000;
        header->pixelFormat = kPixelFormatBgra;
    }
    return true;
}

void SharedFrameWriter::close() noexcept {
    if (view_) {
        UnmapViewOfFile(view_);
        view_ = nullptr;
    }
    handle_ = nullptr;
    slot_ = 0;
}

bool SharedFrameWriter::isOpen() const noexcept {
    return view_ != nullptr;
}

bool SharedFrameWriter::configure(
    std::uint32_t width,
    std::uint32_t height,
    double frameRate
) {
    if (!view_ || width == 0 || height == 0 ||
        width > kMaxFrameWidth || height > kMaxFrameHeight ||
        !std::isfinite(frameRate) || frameRate <= 0.0) {
        return false;
    }

    auto* header = static_cast<FrameHeader*>(view_);
    const auto fps = std::max(1LL, std::llround(frameRate * 1000.0));
    InterlockedExchange(asLong32(&header->width), static_cast<LONG>(width));
    InterlockedExchange(asLong32(&header->height), static_cast<LONG>(height));
    InterlockedExchange(asLong32(&header->fpsNumerator), static_cast<LONG>(fps));
    InterlockedExchange(asLong32(&header->fpsDenominator), 1000);
    InterlockedExchange(asLong32(&header->pixelFormat), static_cast<LONG>(kPixelFormatBgra));
    return true;
}

bool SharedFrameWriter::publish(
    std::uint32_t width,
    std::uint32_t height,
    std::size_t stride,
    const std::uint8_t* data
) {
    if (!view_ || !data || width == 0 || height == 0 ||
        width > kMaxFrameWidth || height > kMaxFrameHeight) {
        return false;
    }

    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4;
    if (stride < rowBytes) {
        stride = rowBytes;
    }

    auto* header = static_cast<FrameHeader*>(view_);
    const std::uint32_t slot = slot_++ % kSlotCount;
    auto* sequence = asLong64(&header->sequences[slot]);
    InterlockedIncrement64(sequence);

    auto* destination = static_cast<std::uint8_t*>(view_) + frameSlotOffset(slot);
    for (std::uint32_t row = 0; row < height; ++row) {
        std::copy_n(data + row * stride, rowBytes, destination + row * rowBytes);
    }

    InterlockedExchange(asLong32(&header->sizes[slot]), static_cast<LONG>(rowBytes * height));
    InterlockedIncrement64(sequence);
    InterlockedExchange64(
        asLong64(&header->published),
        static_cast<LONG64>(InterlockedCompareExchange64(asLong64(&header->published), 0, 0) + 1)
    );
    return true;
}

SharedFrameReader::~SharedFrameReader() {
    close();
}

bool SharedFrameReader::open() {
    close();
    if (!ensureSharedFrameStorage()) {
        return false;
    }

    HANDLE file = CreateFileW(
        kFrameFilePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    HANDLE mapping = CreateFileMappingW(
        file,
        nullptr,
        PAGE_READWRITE,
        0,
        0,
        nullptr
    );
    CloseHandle(file);
    if (!mapping) {
        return false;
    }

    void* view = MapViewOfFile(
        mapping,
        FILE_MAP_READ | FILE_MAP_WRITE,
        0,
        0,
        sharedFrameFileSize()
    );
    CloseHandle(mapping);
    if (!view) {
        return false;
    }

    view_ = view;
    handle_ = view;
    return true;
}

void SharedFrameReader::close() noexcept {
    if (view_) {
        UnmapViewOfFile(view_);
        view_ = nullptr;
    }
    handle_ = nullptr;
}

bool SharedFrameReader::isOpen() const noexcept {
    return view_ != nullptr;
}

bool SharedFrameReader::info(FrameInfo& result) const {
    if (!view_) {
        return false;
    }

    auto* header = static_cast<const FrameHeader*>(view_);
    if (InterlockedCompareExchange(
            asLong32(const_cast<std::uint32_t*>(&header->magic)), 0, 0
        ) != static_cast<LONG>(kSharedFrameMagic) ||
        InterlockedCompareExchange(
            asLong32(const_cast<std::uint32_t*>(&header->version)), 0, 0
        ) != static_cast<LONG>(kSharedFrameVersion)) {
        return false;
    }

    result.width = static_cast<std::uint32_t>(InterlockedCompareExchange(
        asLong32(const_cast<std::uint32_t*>(&header->width)), 0, 0
    ));
    result.height = static_cast<std::uint32_t>(InterlockedCompareExchange(
        asLong32(const_cast<std::uint32_t*>(&header->height)), 0, 0
    ));
    result.fpsNumerator = static_cast<std::uint32_t>(InterlockedCompareExchange(
        asLong32(const_cast<std::uint32_t*>(&header->fpsNumerator)), 0, 0
    ));
    result.fpsDenominator = static_cast<std::uint32_t>(InterlockedCompareExchange(
        asLong32(const_cast<std::uint32_t*>(&header->fpsDenominator)), 0, 0
    ));
    result.pixelFormat = static_cast<std::uint32_t>(InterlockedCompareExchange(
        asLong32(const_cast<std::uint32_t*>(&header->pixelFormat)), 0, 0
    ));

    if (result.width == 0 || result.height == 0 ||
        result.width > kMaxFrameWidth || result.height > kMaxFrameHeight ||
        result.fpsNumerator == 0 || result.fpsDenominator == 0) {
        result = FrameInfo{};
        return false;
    }
    return true;
}

bool SharedFrameReader::copyTo(
    std::uint8_t* destination,
    std::size_t destinationStride,
    std::size_t destinationSize,
    FrameInfo& result
) const {
    if (!view_ || !destination || destinationStride == 0) {
        return false;
    }

    FrameInfo current{};
    const bool haveInfo = info(current);
    result = current;
    if (!haveInfo) {
        fillBlack(destination, destinationStride, destinationSize, 1280, 720);
        return false;
    }

    auto* header = static_cast<const FrameHeader*>(view_);
    const std::uint64_t published = static_cast<std::uint64_t>(InterlockedCompareExchange64(
        asLong64(const_cast<std::uint64_t*>(&header->published)), 0, 0
    ));
    if (published == 0) {
        fillBlack(destination, destinationStride, destinationSize, current.width, current.height);
        return false;
    }

    const std::uint32_t slot = static_cast<std::uint32_t>((published - 1) % kSlotCount);
    auto* sequence = asLong64(const_cast<std::uint64_t*>(&header->sequences[slot]));

    const std::size_t rowBytes = static_cast<std::size_t>(current.width) * 4;
    const std::size_t rows = std::min(
        static_cast<std::size_t>(current.height),
        destinationSize / destinationStride
    );
    const std::size_t copyBytes = std::min(rowBytes, destinationStride);
    const auto* source = static_cast<const std::uint8_t*>(view_) + frameSlotOffset(slot);

    for (int attempt = 0; attempt < 4; ++attempt) {
        const std::int64_t before = InterlockedCompareExchange64(sequence, 0, 0);
        if (before == 0 || (before & 1) != 0) {
            break;
        }

        for (std::size_t row = 0; row < rows; ++row) {
            std::copy_n(
                source + row * rowBytes,
                copyBytes,
                destination + row * destinationStride
            );
            if (destinationStride > copyBytes) {
                std::fill(
                    destination + row * destinationStride + copyBytes,
                    destination + (row + 1) * destinationStride,
                    0
                );
            }
        }

        const std::int64_t after = InterlockedCompareExchange64(sequence, 0, 0);
        if (before == after) {
            return true;
        }
    }

    fillBlack(destination, destinationStride, destinationSize, current.width, current.height);
    return false;
}

}
