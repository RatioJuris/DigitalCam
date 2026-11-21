#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include <digitalcam/core/ICameraAdapter.h>
#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::virtual_camera {

class NullCameraAdapter final : public digitalcam::core::ICameraAdapter {
public:
    std::string name() const override;
    digitalcam::core::Result install() override;
    digitalcam::core::Result start(const digitalcam::core::VideoOutputFormat& format) override;
    digitalcam::core::Result publishFrame(const digitalcam::core::VideoFrame& frame) override;
    void stop() override;
    bool isAvailable() const override;

    digitalcam::core::VideoFrame lastFrame() const;
    std::uint64_t publishedFrames() const noexcept;
    digitalcam::core::VideoOutputFormat format() const;

private:
    mutable std::mutex mutex_;
    digitalcam::core::VideoOutputFormat format_;
    digitalcam::core::VideoFrame lastFrame_;
    std::atomic<bool> installed_{false};
    std::atomic<bool> running_{false};
    std::atomic<std::uint64_t> publishedFrames_{0};
};

}
