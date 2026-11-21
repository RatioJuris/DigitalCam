#include <digitalcam/core/audio/AudioDecoder.h>

#include <cstdint>
#include <memory>

#include "../media/FfmpegCommon.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/version.h>
#include <libswresample/swresample.h>
}

namespace digitalcam::core {

class AudioDecoderImpl {
public:
    ~AudioDecoderImpl() {
        close();
    }

    Result open(const std::filesystem::path& path, MediaMetadata& metadata) {
        close();

        const std::string filePath = path.string();
        int result = avformat_open_input(&formatContext_, filePath.c_str(), nullptr, nullptr);
        if (result < 0) {
            return Result::failure("Unable to open audio: " + ffmpeg::errorText(result));
        }

        result = avformat_find_stream_info(formatContext_, nullptr);
        if (result < 0) {
            close();
            return Result::failure("Unable to read audio metadata: " + ffmpeg::errorText(result));
        }

        const AVCodec* codec = nullptr;
        streamIndex_ = av_find_best_stream(
            formatContext_,
            AVMEDIA_TYPE_AUDIO,
            -1,
            -1,
            &codec,
            0
        );
        if (streamIndex_ < 0 || codec == nullptr) {
            close();
            return Result::failure("No decodable audio stream found");
        }

        stream_ = formatContext_->streams[streamIndex_];
        codecContext_ = avcodec_alloc_context3(codec);
        if (codecContext_ == nullptr) {
            close();
            return Result::failure("Unable to allocate audio decoder");
        }

        result = avcodec_parameters_to_context(codecContext_, stream_->codecpar);
        if (result < 0) {
            close();
            return Result::failure("Unable to configure audio decoder: " + ffmpeg::errorText(result));
        }

        result = avcodec_open2(codecContext_, codec, nullptr);
        if (result < 0) {
            close();
            return Result::failure("Unable to open audio decoder: " + ffmpeg::errorText(result));
        }

        result = createResampler();
        if (result < 0) {
            close();
            return Result::failure("Unable to create audio resampler: " + ffmpeg::errorText(result));
        }

        decodedFrame_ = av_frame_alloc();
        packet_ = av_packet_alloc();
        if (decodedFrame_ == nullptr || packet_ == nullptr) {
            close();
            return Result::failure("Unable to allocate FFmpeg audio buffers");
        }

        metadata.hasAudio = true;
        metadata.audioSampleRate = codecContext_->sample_rate;
#if LIBAVUTIL_VERSION_MAJOR >= 57
        metadata.audioChannels = codecContext_->ch_layout.nb_channels;
#else
        metadata.audioChannels = codecContext_->channels;
#endif
        metadata.audioCodec = codec->name == nullptr ? std::string() : std::string(codec->name);

        open_ = true;
        return Result::success();
    }

    bool isOpen() const noexcept {
        return open_;
    }

    DecodeStatus decodeNext(AudioBuffer& buffer, Result& result) {
        result = Result::success();
        if (!open_) {
            result = Result::failure("Audio decoder is not open");
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
                result = Result::failure("Audio packet decode failed: " + ffmpeg::errorText(sendResult));
                return DecodeStatus::Error;
            }

            while (true) {
                const int receiveResult = avcodec_receive_frame(codecContext_, decodedFrame_);
                if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF) {
                    break;
                }
                if (receiveResult < 0) {
                    result = Result::failure("Audio frame decode failed: " + ffmpeg::errorText(receiveResult));
                    return DecodeStatus::Error;
                }

                result = convertFrame(buffer);
                if (!result) {
                    return DecodeStatus::Error;
                }
                return DecodeStatus::Ok;
            }
        }
    }

    Result reset() {
        if (!open_) {
            return Result::failure("Audio decoder is not open");
        }
        const int result = av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD);
        if (result < 0) {
            return Result::failure("Audio reset failed: " + ffmpeg::errorText(result));
        }
        avcodec_flush_buffers(codecContext_);
        return Result::success();
    }

private:
    int createResampler() {
#if LIBAVUTIL_VERSION_MAJOR >= 57
        AVChannelLayout outputLayout;
        av_channel_layout_default(&outputLayout, 2);
        const int result = swr_alloc_set_opts2(
            &swrContext_,
            &outputLayout,
            AV_SAMPLE_FMT_FLT,
            48000,
            &codecContext_->ch_layout,
            codecContext_->sample_fmt,
            codecContext_->sample_rate,
            0,
            nullptr
        );
        av_channel_layout_uninit(&outputLayout);
        return result;
#else
        const std::int64_t inputLayout = codecContext_->channel_layout != 0
            ? codecContext_->channel_layout
            : av_get_default_channel_layout(codecContext_->channels);
        swrContext_ = swr_alloc_set_opts(
            swrContext_,
            AV_CH_LAYOUT_STEREO,
            AV_SAMPLE_FMT_FLT,
            48000,
            inputLayout,
            codecContext_->sample_fmt,
            codecContext_->sample_rate,
            0,
            nullptr
        );
        return swrContext_ == nullptr ? AVERROR(ENOMEM) : 0;
#endif
    }

    Result convertFrame(AudioBuffer& buffer) {
        const int inputRate = codecContext_->sample_rate > 0 ? codecContext_->sample_rate : 48000;
        const int outputSamples = av_rescale_rnd(
            decodedFrame_->nb_samples,
            48000,
            inputRate,
            AV_ROUND_UP
        ) + 8;

        buffer.sampleRate = 48000;
        buffer.channels = 2;
        buffer.timestamp = Timestamp{ffmpeg::toNanoseconds(
            decodedFrame_->best_effort_timestamp,
            stream_->time_base
        )};
        buffer.samples.assign(static_cast<std::size_t>(outputSamples) * 2U, 0.0F);

        float* outputData = buffer.samples.data();
        const std::uint8_t* inputData[8]{};
        for (int index = 0; index < 8; ++index) {
            inputData[index] = decodedFrame_->data[index];
        }

        const int converted = swr_convert(
            swrContext_,
            reinterpret_cast<std::uint8_t**>(&outputData),
            outputSamples,
            inputData,
            decodedFrame_->nb_samples
        );
        if (converted <= 0) {
            return Result::failure("Audio resampling failed");
        }

        buffer.samples.resize(static_cast<std::size_t>(converted) * 2U);
        return Result::success();
    }

    void close() {
        open_ = false;
        if (packet_ != nullptr) {
            av_packet_free(&packet_);
        }
        if (decodedFrame_ != nullptr) {
            av_frame_free(&decodedFrame_);
        }
        if (swrContext_ != nullptr) {
            swr_free(&swrContext_);
        }
        if (codecContext_ != nullptr) {
            avcodec_free_context(&codecContext_);
        }
        if (formatContext_ != nullptr) {
            avformat_close_input(&formatContext_);
        }
        stream_ = nullptr;
        streamIndex_ = -1;
    }

    AVFormatContext* formatContext_ = nullptr;
    AVCodecContext* codecContext_ = nullptr;
    const AVStream* stream_ = nullptr;
    AVFrame* decodedFrame_ = nullptr;
    AVPacket* packet_ = nullptr;
    SwrContext* swrContext_ = nullptr;
    int streamIndex_ = -1;
    bool open_ = false;
};

AudioDecoder::AudioDecoder() : impl_(std::make_unique<AudioDecoderImpl>()) {}
AudioDecoder::~AudioDecoder() = default;
AudioDecoder::AudioDecoder(AudioDecoder&&) noexcept = default;
AudioDecoder& AudioDecoder::operator=(AudioDecoder&&) noexcept = default;

Result AudioDecoder::open(const std::filesystem::path& path, MediaMetadata& metadata) {
    if (!impl_) {
        return Result::failure("Audio decoder is not initialised");
    }
    return impl_->open(path, metadata);
}

DecodeStatus AudioDecoder::decodeNext(AudioBuffer& buffer, Result& result) {
    if (!impl_) {
        result = Result::failure("Audio decoder is not initialised");
        return DecodeStatus::Error;
    }
    return impl_->decodeNext(buffer, result);
}

Result AudioDecoder::reset() {
    if (!impl_) {
        return Result::failure("Audio decoder is not initialised");
    }
    return impl_->reset();
}

bool AudioDecoder::isOpen() const noexcept {
    return impl_ != nullptr && impl_->isOpen();
}

}
