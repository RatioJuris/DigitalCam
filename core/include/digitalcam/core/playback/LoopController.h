#pragma once

#include <cstdint>
#include <mutex>

namespace digitalcam::core {

class LoopController {
public:
    explicit LoopController(bool enabled = true);

    void setEnabled(bool enabled);
    bool isEnabled() const;
    bool shouldLoop() const;

    void onEndReached();
    std::uint64_t loopCount() const;
    void reset();

private:
    mutable std::mutex mutex_;
    bool enabled_ = true;
    std::uint64_t loopCount_ = 0;
};

}
