#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace digitalcam::core {

struct Result {
    bool ok = false;
    std::string message;

    static Result success() {
        return Result{true, {}};
    }

    static Result failure(std::string message) {
        return Result{false, std::move(message)};
    }

    explicit operator bool() const noexcept {
        return ok;
    }
};

enum class DecodeStatus : std::uint8_t {
    Ok,
    EndOfStream,
    NoData,
    Error
};

}
