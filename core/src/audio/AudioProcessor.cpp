#include <digitalcam/core/audio/AudioProcessor.h>

#include <algorithm>

namespace digitalcam::core {

void AudioProcessor::setVolume(float volume) noexcept {
    volume_ = std::clamp(volume, 0.0F, 4.0F);
}

float AudioProcessor::volume() const noexcept {
    return volume_;
}

void AudioProcessor::setMuted(bool muted) noexcept {
    muted_ = muted;
}

bool AudioProcessor::muted() const noexcept {
    return muted_;
}

void AudioProcessor::process(AudioBuffer& buffer) const {
    if (muted_) {
        std::fill(buffer.samples.begin(), buffer.samples.end(), 0.0F);
        return;
    }
    if (volume_ == 1.0F) {
        return;
    }
    for (float& sample : buffer.samples) {
        sample = std::clamp(sample * volume_, -1.0F, 1.0F);
    }
}

AudioBuffer AudioProcessor::silence(int sampleRate, int channels, Timestamp timestamp) {
    AudioBuffer buffer;
    buffer.sampleRate = sampleRate;
    buffer.channels = channels > 0 ? channels : 2;
    buffer.timestamp = timestamp;
    const std::size_t sampleCount =
        static_cast<std::size_t>(buffer.sampleRate) / 30U * buffer.channels;
    buffer.samples.assign(sampleCount, 0.0F);
    return buffer;
}

}
