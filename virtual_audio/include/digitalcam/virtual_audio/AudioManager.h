#pragma once

#include <memory>
#include <string>

#include <digitalcam/core/IAudioAdapter.h>
#include <digitalcam/core/Result.h>

namespace digitalcam::virtual_audio {

class AudioManager {
public:
    explicit AudioManager(digitalcam::core::AudioAdapterPtr adapter);

    digitalcam::core::Result install();
    digitalcam::core::Result start(int sampleRate, int channels);
    digitalcam::core::Result publish(const digitalcam::core::AudioBuffer& buffer);
    void stop();

    bool isAvailable() const;
    std::string name() const;

    digitalcam::core::IAudioAdapter& adapter() const;

private:
    digitalcam::core::AudioAdapterPtr adapter_;
};

}
