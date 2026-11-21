#pragma once

#include <memory>
#include <string>

#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class IAudioAdapter {
public:
    virtual ~IAudioAdapter() = default;

    virtual std::string name() const = 0;
    virtual Result install() = 0;
    virtual Result start(int sampleRate, int channels) = 0;
    virtual Result publishAudio(const AudioBuffer& buffer) = 0;
    virtual void stop() = 0;
    virtual bool isAvailable() const = 0;
};

using AudioAdapterPtr = std::shared_ptr<IAudioAdapter>;

}
