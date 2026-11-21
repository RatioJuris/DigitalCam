#include <digitalcam/virtual_audio/NullAudioAdapter.h>

#include <mutex>
#include <utility>

namespace digitalcam::virtual_audio {

std::string NullAudioAdapter::name() const {
    return "Null Audio";
}

digitalcam::core::Result NullAudioAdapter::install() {
    installed_.store(true);
    return digitalcam::core::Result::success();
}

digitalcam::core::Result NullAudioAdapter::start(int sampleRate, int channels) {
    if (!installed_.load()) {
        return digitalcam::core::Result::failure("Null audio adapter is not installed");
    }
    if (sampleRate <= 0 || channels <= 0) {
        return digitalcam::core::Result::failure("Invalid audio format");
    }
    running_.store(true);
    return digitalcam::core::Result::success();
}

digitalcam::core::Result NullAudioAdapter::publishAudio(
    const digitalcam::core::AudioBuffer& buffer
) {
    if (!running_.load()) {
        return digitalcam::core::Result::failure("Null audio adapter is not running");
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lastBuffer_ = buffer;
    }
    publishedBuffers_.fetch_add(1);
    return digitalcam::core::Result::success();
}

void NullAudioAdapter::stop() {
    running_.store(false);
}

bool NullAudioAdapter::isAvailable() const {
    return installed_.load() && running_.load();
}

digitalcam::core::AudioBuffer NullAudioAdapter::lastBuffer() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastBuffer_;
}

std::uint64_t NullAudioAdapter::publishedBuffers() const noexcept {
    return publishedBuffers_.load();
}

}
