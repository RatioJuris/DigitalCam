#include "FfmpegCommon.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

extern "C" {
#include <libavutil/display.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
}

namespace digitalcam::core::ffmpeg {

std::string errorText(int code) {
    char buffer[AV_ERROR_MAX_STRING_SIZE]{};
    av_strerror(code, buffer, sizeof(buffer));
    return std::string(buffer);
}

std::int64_t toNanoseconds(std::int64_t value, AVRational timeBase) {
    if (timeBase.num == 0 || timeBase.den == 0) {
        return 0;
    }
    const double seconds = static_cast<double>(value) * av_q2d(timeBase);
    return static_cast<std::int64_t>(seconds * 1000000000.0);
}

double streamFrameRate(const AVStream* stream) {
    if (stream == nullptr) {
        return 0.0;
    }
    const double rate = av_q2d(stream->avg_frame_rate);
    if (rate > 0.0 && std::isfinite(rate)) {
        return rate;
    }
    const double fallback = av_q2d(stream->r_frame_rate);
    return fallback > 0.0 && std::isfinite(fallback) ? fallback : 0.0;
}

int rotationQuarter(const AVFrame* frame) {
    if (frame == nullptr) {
        return 0;
    }
    const AVFrameSideData* sideData =
        av_frame_get_side_data(const_cast<AVFrame*>(frame), AV_FRAME_DATA_DISPLAYMATRIX);
    if (sideData == nullptr || sideData->data == nullptr) {
        return 0;
    }
    const auto* matrix = reinterpret_cast<const std::int32_t*>(sideData->data);
    const double rotation = av_display_rotation_get(matrix);
    int quarter = static_cast<int>(std::lround(rotation / 90.0)) % 4;
    if (quarter < 0) {
        quarter += 4;
    }
    return quarter;
}

}
