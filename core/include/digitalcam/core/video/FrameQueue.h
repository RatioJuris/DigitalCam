#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace digitalcam::core {

template <typename T>
class FrameQueue {
public:
    explicit FrameQueue(std::size_t capacity) : capacity_(capacity == 0 ? 1 : capacity) {}

    bool push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= capacity_) {
            queue_.pop_front();
        }
        queue_.push_back(std::move(value));
        return true;
    }

    bool pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    std::size_t capacity() const noexcept {
        return capacity_;
    }

    bool empty() const {
        return size() == 0;
    }

private:
    mutable std::mutex mutex_;
    std::deque<T> queue_;
    std::size_t capacity_ = 1;
};

}
