#pragma once

#include <cstdint>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/rational.h>
}

namespace digitalcam::core::ffmpeg {

std::string errorText(int code);

std::int64_t toNanoseconds(std::int64_t value, AVRational timeBase);

double streamFrameRate(const AVStream* stream);

int rotationQuarter(const AVFrame* frame);

}
