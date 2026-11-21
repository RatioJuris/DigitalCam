#pragma once

#include <cstddef>
#include <cstdint>
#include <guiddef.h>
#include <string_view>

namespace digitalcam::virtual_camera::windows {

inline constexpr std::uint32_t kSharedFrameMagic = 0x44434D43;
inline constexpr std::uint32_t kSharedFrameVersion = 1;
inline constexpr std::uint32_t kSlotCount = 2;
inline constexpr std::uint32_t kMaxFrameWidth = 1920;
inline constexpr std::uint32_t kMaxFrameHeight = 1080;
inline constexpr std::uint32_t kPixelFormatBgra = 1;
inline constexpr std::size_t kFrameHeaderSize = 128;
inline constexpr std::size_t kFrameSlotSize =
    static_cast<std::size_t>(kMaxFrameWidth) * kMaxFrameHeight * 4;

struct FrameHeader {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t fpsNumerator;
    std::uint32_t fpsDenominator;
    std::uint32_t pixelFormat;
    std::uint32_t slotCount;
    std::uint64_t sequences[kSlotCount];
    std::uint32_t sizes[kSlotCount];
    std::uint64_t published;
    std::uint64_t reserved[8];
};

static_assert(sizeof(FrameHeader) == kFrameHeaderSize);

struct FrameInfo {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t fpsNumerator = 30000;
    std::uint32_t fpsDenominator = 1000;
    std::uint32_t pixelFormat = kPixelFormatBgra;
};

GUID virtualCameraClsid() noexcept;
std::wstring_view virtualCameraName() noexcept;
std::wstring_view frameFilePath() noexcept;
std::wstring_view programDataDirectory() noexcept;
std::wstring_view installDirectory() noexcept;
std::wstring_view virtualCameraDllName() noexcept;
std::size_t frameSlotOffset(std::uint32_t index) noexcept;
std::size_t sharedFrameFileSize() noexcept;

bool ensureSharedFrameStorage();

class SharedFrameWriter {
public:
    SharedFrameWriter() noexcept = default;
    ~SharedFrameWriter();

    SharedFrameWriter(const SharedFrameWriter&) = delete;
    SharedFrameWriter& operator=(const SharedFrameWriter&) = delete;

    bool open();
    void close() noexcept;
    bool configure(std::uint32_t width, std::uint32_t height, double frameRate);
    bool publish(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t stride,
        const std::uint8_t* data
    );
    bool isOpen() const noexcept;

private:
    void* handle_ = nullptr;
    void* view_ = nullptr;
    std::uint32_t slot_ = 0;
};

class SharedFrameReader {
public:
    SharedFrameReader() noexcept = default;
    ~SharedFrameReader();

    SharedFrameReader(const SharedFrameReader&) = delete;
    SharedFrameReader& operator=(const SharedFrameReader&) = delete;

    bool open();
    void close() noexcept;
    bool info(FrameInfo& result) const;
    bool copyTo(
        std::uint8_t* destination,
        std::size_t destinationStride,
        std::size_t destinationSize,
        FrameInfo& result
    ) const;
    bool isOpen() const noexcept;

private:
    void* handle_ = nullptr;
    void* view_ = nullptr;
};

}
