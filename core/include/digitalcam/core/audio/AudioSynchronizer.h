#pragma once

#include <digitalcam/core/Types.h>

namespace digitalcam::core {

class AudioSynchronizer {
public:
    void setVideoTimestamp(Timestamp timestamp) noexcept;
    Timestamp videoTimestamp() const noexcept;

    Timestamp align(Timestamp audioTimestamp) const noexcept;
    void reset() noexcept;

private:
    Timestamp videoTimestamp_{0};
};

}
