#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include <digitalcam/core/IAudioAdapter.h>
#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::virtual_audio {

class NullAudioAdapter final : public digitalcam::core::IAudioAdapter {
public:
    std::string name() const override;
    digitalcam::core::Result install() override;
    digitalcam::core::Result start(int sampleRate, int channels) override;
    digitalcam::core::Result publishAudio(const digitalcam::core::AudioBuffer& buffer) override;
    void stop() override;
    bool isAvailable() const override;

    digitalcam::core::AudioBuffer lastBuffer() const;
    std::uint64_t publishedBuffers() const noexcept;

private:
    mutable std::mutex mutex_;
    digitalcam::core::AudioBuffer lastBuffer_;
    std::atomic<bool> installed_{false};
    std::atomic<bool> running_{false};
    std::atomic<std::uint64_t> publishedBuffers_{0};
};

}
