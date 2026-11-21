#include <digitalcam/core/media/VideoDecoder.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>

#include "FfmpegCommon.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/version.h>
#include <libswscale/swscale.h>
}

namespace digitalcam::core {

class VideoDecoderImpl {
public:
    ~VideoDecoderImpl() {
        close();
    }

    Result open(const std::filesystem::path& path, MediaMetadata& metadata) {
        close();

        const std::string filePath = path.string();
        int result = avformat_open_input(&formatContext_, filePath.c_str(), nullptr, nullptr);
        if (result < 0) {
            return Result::failure("Unable to open video: " + ffmpeg::errorText(result));
        }

        result = avformat_find_stream_info(formatContext_, nullptr);
        if (result < 0) {
            close();
            return Result::failure("Unable to read video metadata: " + ffmpeg::errorText(result));
        }

        const AVCodec* codec = nullptr;
        streamIndex_ = av_find_best_stream(
            formatContext_,
            AVMEDIA_TYPE_VIDEO,
            -1,
            -1,
            &codec,
            0
        );
        if (streamIndex_ < 0 || codec == nullptr) {
            close();
            return Result::failure("No decodable video stream found");
        }

        stream_ = formatContext_->streams[streamIndex_];
        codecContext_ = avcodec_alloc_context3(codec);
        if (codecContext_ == nullptr) {
            close();
            return Result::failure("Unable to allocate video decoder");
        }

        result = avcodec_parameters_to_context(codecContext_, stream_->codecpar);
        if (result < 0) {
            close();
            return Result::failure("Unable to configure video decoder: " + ffmpeg::errorText(result));
        }

        result = avcodec_open2(codecContext_, codec, nullptr);
        if (result < 0) {
            close();
            return Result::failure("Unable to open video decoder: " + ffmpeg::errorText(result));
        }

        decodedFrame_ = av_frame_alloc();
        bgraFrame_ = av_frame_alloc();
        packet_ = av_packet_alloc();
        if (decodedFrame_ == nullptr || bgraFrame_ == nullptr || packet_ == nullptr) {
            close();
            return Result::failure("Unable to allocate FFmpeg frame buffers");
        }

        sourceWidth_ = codecContext_->width;
        sourceHeight_ = codecContext_->height;
        sourcePixelFormat_ = codecContext_->pix_fmt;

        metadata = MediaMetadata{};
        metadata.path = path;
        metadata.type = MediaType::Video;
        metadata.size = {sourceWidth_, sourceHeight_};
        metadata.frameRate = ffmpeg::streamFrameRate(stream_);
        metadata.duration = Timestamp{
            static_cast<std::int64_t>(formatContext_->duration) * 1000000000LL / AV_TIME_BASE
        };
        metadata.hasVideo = true;
        metadata.videoCodec = codec->name == nullptr ? std::string() : std::string(codec->name);

        const AVCodec* audioCodec = nullptr;
        const int audioIndex = av_find_best_stream(
            formatContext_,
            AVMEDIA_TYPE_AUDIO,
            -1,
            streamIndex_,
            &audioCodec,
            0
        );
        if (audioIndex >= 0) {
            const AVStream* audioStream = formatContext_->streams[audioIndex];
            metadata.hasAudio = true;
            metadata.audioSampleRate = audioStream->codecpar->sample_rate;
#if LIBAVUTIL_VERSION_MAJOR >= 57
            metadata.audioChannels = audioStream->codecpar->ch_layout.nb_channels;
#else
            metadata.audioChannels = audioStream->codecpar->channels;
#endif
            metadata.audioCodec = audioCodec != nullptr && audioCodec->name != nullptr
                ? std::string(audioCodec->name)
                : std::string();
        }

        open_ = true;
        return Result::success();
    }

    bool isOpen() const noexcept {
        return open_;
    }

    DecodeStatus decodeNext(VideoFrame& frame, Result& result) {
        result = Result::success();
        if (!open_) {
            result = Result::failure("Video decoder is not open");
            return DecodeStatus::Error;
        }

        while (true) {
            const int readResult = av_read_frame(formatContext_, packet_);
            if (readResult < 0) {
                av_packet_unref(packet_);
                return DecodeStatus::EndOfStream;
            }

            if (packet_->stream_index != streamIndex_) {
                av_packet_unref(packet_);
                continue;
            }

            const int sendResult = avcodec_send_packet(codecContext_, packet_);
            av_packet_unref(packet_);
            if (sendResult < 0) {
                result = Result::failure("Video packet decode failed: " + ffmpeg::errorText(sendResult));
                return DecodeStatus::Error;
            }

            while (true) {
                const int receiveResult = avcodec_receive_frame(codecContext_, decodedFrame_);
                if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF) {
                    break;
                }
                if (receiveResult < 0) {
                    result = Result::failure("Video frame decode failed: " + ffmpeg::errorText(receiveResult));
                    return DecodeStatus::Error;
                }

                result = convertFrame(frame);
                if (!result) {
                    return DecodeStatus::Error;
                }
                return DecodeStatus::Ok;
            }
        }
    }

    Result seek(Timestamp position) {
        if (!open_ || stream_ == nullptr) {
            return Result::failure("Video decoder is not open");
        }

        const AVRational clockBase{1, 1000000000};
        const std::int64_t timestamp = av_rescale_q(position.count(), clockBase, stream_->time_base);
        const int result = av_seek_frame(formatContext_, streamIndex_, timestamp, AVSEEK_FLAG_BACKWARD);
        if (result < 0) {
            return Result::failure("Video seek failed: " + ffmpeg::errorText(result));
        }
        avcodec_flush_buffers(codecContext_);
        return Result::success();
    }

    Result reset() {
        if (!open_) {
            return Result::failure("Video decoder is not open");
        }
        const int result = av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD);
        if (result < 0) {
            return Result::failure("Video reset failed: " + ffmpeg::errorText(result));
        }
        avcodec_flush_buffers(codecContext_);
        return Result::success();
    }

private:
    Result ensureConverter() {
        if (swsContext_ != nullptr &&
            sourceWidth_ == decodedFrame_->width &&
            sourceHeight_ == decodedFrame_->height &&
            sourcePixelFormat_ == decodedFrame_->format) {
            return Result::success();
        }

        if (swsContext_ != nullptr) {
            sws_freeContext(swsContext_);
            swsContext_ = nullptr;
        }

        sourceWidth_ = decodedFrame_->width;
        sourceHeight_ = decodedFrame_->height;
        sourcePixelFormat_ = static_cast<AVPixelFormat>(decodedFrame_->format);

        swsContext_ = sws_getContext(
            sourceWidth_,
            sourceHeight_,
            sourcePixelFormat_,
            sourceWidth_,
            sourceHeight_,
            AV_PIX_FMT_BGRA,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );
        if (swsContext_ == nullptr) {
            return Result::failure("Unable to create video frame converter");
        }
        return Result::success();
    }

    Result convertFrame(VideoFrame& frame) {
        const Result converterResult = ensureConverter();
        if (!converterResult) {
            return converterResult;
        }

        if (bgraFrame_->width != sourceWidth_ || bgraFrame_->height != sourceHeight_) {
            av_frame_unref(bgraFrame_);
            bgraFrame_->format = AV_PIX_FMT_BGRA;
            bgraFrame_->width = sourceWidth_;
            bgraFrame_->height = sourceHeight_;
            const int result = av_frame_get_buffer(bgraFrame_, 32);
            if (result < 0) {
                return Result::failure("Unable to allocate converted video frame: " + ffmpeg::errorText(result));
            }
        }

        const int converted = sws_scale(
            swsContext_,
            decodedFrame_->data,
            decodedFrame_->linesize,
            0,
            decodedFrame_->height,
            bgraFrame_->data,
            bgraFrame_->linesize
        );
        if (converted <= 0) {
            return Result::failure("Video pixel conversion failed");
        }

        frame.size = {sourceWidth_, sourceHeight_};
        frame.format = PixelFormat::Bgra;
        frame.stride = static_cast<std::size_t>(bgraFrame_->linesize[0]);
        frame.timestamp = Timestamp{ffmpeg::toNanoseconds(
            decodedFrame_->best_effort_timestamp,
            stream_->time_base
        )};
        frame.data.assign(
            bgraFrame_->data[0],
            bgraFrame_->data[0] + frame.stride * static_cast<std::size_t>(sourceHeight_)
        );
        return Result::success();
    }

    void close() {
        open_ = false;
        if (swsContext_ != nullptr) {
            sws_freeContext(swsContext_);
            swsContext_ = nullptr;
        }
        if (packet_ != nullptr) {
            av_packet_free(&packet_);
        }
        if (decodedFrame_ != nullptr) {
            av_frame_free(&decodedFrame_);
        }
        if (bgraFrame_ != nullptr) {
            av_frame_free(&bgraFrame_);
        }
        if (codecContext_ != nullptr) {
            avcodec_free_context(&codecContext_);
        }
        if (formatContext_ != nullptr) {
            avformat_close_input(&formatContext_);
        }
        stream_ = nullptr;
        streamIndex_ = -1;
        sourceWidth_ = 0;
        sourceHeight_ = 0;
        sourcePixelFormat_ = AV_PIX_FMT_NONE;
    }

    AVFormatContext* formatContext_ = nullptr;
    AVCodecContext* codecContext_ = nullptr;
    const AVStream* stream_ = nullptr;
    AVFrame* decodedFrame_ = nullptr;
    AVFrame* bgraFrame_ = nullptr;
    AVPacket* packet_ = nullptr;
    SwsContext* swsContext_ = nullptr;
    int streamIndex_ = -1;
    int sourceWidth_ = 0;
    int sourceHeight_ = 0;
    AVPixelFormat sourcePixelFormat_ = AV_PIX_FMT_NONE;
    bool open_ = false;
};

VideoDecoder::VideoDecoder() : impl_(std::make_unique<VideoDecoderImpl>()) {}
VideoDecoder::~VideoDecoder() = default;
VideoDecoder::VideoDecoder(VideoDecoder&&) noexcept = default;
VideoDecoder& VideoDecoder::operator=(VideoDecoder&&) noexcept = default;

Result VideoDecoder::open(const std::filesystem::path& path, MediaMetadata& metadata) {
    if (!impl_) {
        return Result::failure("Video decoder is not initialised");
    }
    return impl_->open(path, metadata);
}

DecodeStatus VideoDecoder::decodeNext(VideoFrame& frame, Result& result) {
    if (!impl_) {
        result = Result::failure("Video decoder is not initialised");
        return DecodeStatus::Error;
    }
    return impl_->decodeNext(frame, result);
}

Result VideoDecoder::seek(Timestamp position) {
    if (!impl_) {
        return Result::failure("Video decoder is not initialised");
    }
    return impl_->seek(position);
}

Result VideoDecoder::reset() {
    if (!impl_) {
        return Result::failure("Video decoder is not initialised");
    }
    return impl_->reset();
}

bool VideoDecoder::isOpen() const noexcept {
    return impl_ != nullptr && impl_->isOpen();
}

}
