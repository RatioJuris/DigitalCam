#include <digitalcam/virtual_audio/AudioManager.h>

#include <utility>

namespace digitalcam::virtual_audio {

AudioManager::AudioManager(digitalcam::core::AudioAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

digitalcam::core::Result AudioManager::install() {
    if (!adapter_) {
        return digitalcam::core::Result::failure("No audio adapter configured");
    }
    return adapter_->install();
}

digitalcam::core::Result AudioManager::start(int sampleRate, int channels) {
    if (!adapter_) {
        return digitalcam::core::Result::failure("No audio adapter configured");
    }
    return adapter_->start(sampleRate, channels);
}

digitalcam::core::Result AudioManager::publish(
    const digitalcam::core::AudioBuffer& buffer
) {
    if (!adapter_) {
        return digitalcam::core::Result::failure("No audio adapter configured");
    }
    return adapter_->publishAudio(buffer);
}

void AudioManager::stop() {
    if (adapter_) {
        adapter_->stop();
    }
}

bool AudioManager::isAvailable() const {
    return adapter_ != nullptr && adapter_->isAvailable();
}

std::string AudioManager::name() const {
    return adapter_ ? adapter_->name() : std::string();
}

digitalcam::core::IAudioAdapter& AudioManager::adapter() const {
    return *adapter_;
}

}
