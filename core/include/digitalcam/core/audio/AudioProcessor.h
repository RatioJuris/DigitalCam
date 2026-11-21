#pragma once

#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class AudioProcessor {
public:
    void setVolume(float volume) noexcept;
    float volume() const noexcept;

    void setMuted(bool muted) noexcept;
    bool muted() const noexcept;

    void process(AudioBuffer& buffer) const;
    static AudioBuffer silence(int sampleRate, int channels, Timestamp timestamp);

private:
    float volume_ = 1.0F;
    bool muted_ = false;
};

}
