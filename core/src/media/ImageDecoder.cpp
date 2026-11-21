#include <digitalcam/core/media/ImageDecoder.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

#include "FfmpegCommon.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace digitalcam::core {

namespace {

void rotateBgra(VideoFrame& frame, int quarter) {
    if (quarter == 0 || frame.size.width <= 0 || frame.size.height <= 0) {
        return;
    }

    const int width = frame.size.width;
    const int height = frame.size.height;
    const std::size_t sourceStride =
        frame.stride == 0 ? static_cast<std::size_t>(width) * 4U : frame.stride;
    const int targetStride = (quarter == 1 || quarter == 3) ? height : width;
    std::vector<std::uint8_t> rotated(frame.data.size());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int targetX = x;
            int targetY = y;
            if (quarter == 1) {
                targetX = height - 1 - y;
                targetY = x;
            } else if (quarter == 2) {
                targetX = width - 1 - x;
                targetY = height - 1 - y;
            } else if (quarter == 3) {
                targetX = y;
                targetY = width - 1 - x;
            }

            const std::size_t sourceIndex = static_cast<std::size_t>(y) * sourceStride + static_cast<std::size_t>(x) * 4U;
            const std::size_t targetIndex = (static_cast<std::size_t>(targetY) * targetStride + targetX) * 4U;
            for (std::size_t channel = 0; channel < 4; ++channel) {
                rotated[targetIndex + channel] = frame.data[sourceIndex + channel];
            }
        }
    }

    frame.data = std::move(rotated);
    frame.stride = static_cast<std::size_t>(targetStride) * 4U;
    if (quarter == 1 || quarter == 3) {
        std::swap(frame.size.width, frame.size.height);
    }
}


const AVCodec* decoderForExtension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char value) { return static_cast<char>(std::tolower(value)); }
    );

    if (extension == ".png") {
        return avcodec_find_decoder(AV_CODEC_ID_PNG);
    }
    if (extension == ".jpg" || extension == ".jpeg") {
        return avcodec_find_decoder(AV_CODEC_ID_MJPEG);
    }
    if (extension == ".bmp") {
        return avcodec_find_decoder(AV_CODEC_ID_BMP);
    }
    if (extension == ".webp") {
        return avcodec_find_decoder(AV_CODEC_ID_WEBP);
    }
    if (extension == ".gif") {
        return avcodec_find_decoder(AV_CODEC_ID_GIF);
    }
    return nullptr;
}

}

class ImageDecoderImpl {
public:
    ~ImageDecoderImpl() {
        close();
    }

    Result load(
        const std::filesystem::path& path,
        Size requestedSize,
        VideoFrame& frame
    ) {
        close();

        const std::string filePath = path.string();
        int result = avformat_open_input(&formatContext_, filePath.c_str(), nullptr, nullptr);
        if (result < 0) {
            return Result::failure("Unable to open image: " + ffmpeg::errorText(result));
        }

        result = avformat_find_stream_info(formatContext_, nullptr);
        if (result < 0) {
            close();
            return Result::failure("Unable to read image metadata: " + ffmpeg::errorText(result));
        }

        int streamIndex = av_find_best_stream(
            formatContext_,
            AVMEDIA_TYPE_VIDEO,
            -1,
            -1,
            nullptr,
            0
        );
        const AVCodec* codec = streamIndex >= 0
            ? avcodec_find_decoder(formatContext_->streams[streamIndex]->codecpar->codec_id)
            : nullptr;

        if (streamIndex < 0 || codec == nullptr) {
            streamIndex = -1;
            for (unsigned int index = 0; index < formatContext_->nb_streams; ++index) {
                if (formatContext_->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    streamIndex = static_cast<int>(index);
                    break;
                }
            }
        }
        if (streamIndex < 0) {
            close();
            return Result::failure("No image stream found");
        }
        if (codec == nullptr) {
            codec = avcodec_find_decoder(
                formatContext_->streams[streamIndex]->codecpar->codec_id
            );
        }
        if (codec == nullptr) {
            codec = decoderForExtension(path);
        }
        if (codec == nullptr) {
            close();
            return Result::failure("No decodable image stream found");
        }

        stream_ = formatContext_->streams[streamIndex];
        codecContext_ = avcodec_alloc_context3(codec);
        if (codecContext_ == nullptr) {
            close();
            return Result::failure("Unable to allocate image decoder");
        }

        result = avcodec_parameters_to_context(codecContext_, stream_->codecpar);
        if (result < 0) {
            close();
            return Result::failure("Unable to configure image decoder: " + ffmpeg::errorText(result));
        }

        result = avcodec_open2(codecContext_, codec, nullptr);
        if (result < 0) {
            close();
            return Result::failure("Unable to open image decoder: " + ffmpeg::errorText(result));
        }

        decodedFrame_ = av_frame_alloc();
        bgraFrame_ = av_frame_alloc();
        packet_ = av_packet_alloc();
        if (decodedFrame_ == nullptr || bgraFrame_ == nullptr || packet_ == nullptr) {
            close();
            return Result::failure("Unable to allocate FFmpeg frame buffers");
        }

        bool decoded = false;
        while (av_read_frame(formatContext_, packet_) >= 0) {
            if (packet_->stream_index == streamIndex) {
                result = avcodec_send_packet(codecContext_, packet_);
                av_packet_unref(packet_);
                if (result < 0) {
                    close();
                    return Result::failure("Image decode failed: " + ffmpeg::errorText(result));
                }

                result = avcodec_receive_frame(codecContext_, decodedFrame_);
                if (result == 0) {
                    decoded = true;
                    break;
                }
                if (result != AVERROR(EAGAIN)) {
                    close();
                    return Result::failure("Image decode failed: " + ffmpeg::errorText(result));
                }
            } else {
                av_packet_unref(packet_);
            }
        }

        if (!decoded) {
            close();
            return Result::failure("No image frame could be decoded");
        }

        int outputWidth = requestedSize.valid() ? requestedSize.width : decodedFrame_->width;
        int outputHeight = requestedSize.valid() ? requestedSize.height : decodedFrame_->height;
        const int quarter = ffmpeg::rotationQuarter(decodedFrame_);
        if (!requestedSize.valid() && (quarter == 1 || quarter == 3)) {
            std::swap(outputWidth, outputHeight);
        }

        bgraFrame_->format = AV_PIX_FMT_BGRA;
        bgraFrame_->width = outputWidth;
        bgraFrame_->height = outputHeight;
        result = av_frame_get_buffer(bgraFrame_, 32);
        if (result < 0) {
            close();
            return Result::failure("Unable to allocate converted image: " + ffmpeg::errorText(result));
        }

        swsContext_ = sws_getContext(
            decodedFrame_->width,
            decodedFrame_->height,
            static_cast<AVPixelFormat>(decodedFrame_->format),
            outputWidth,
            outputHeight,
            AV_PIX_FMT_BGRA,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );
        if (swsContext_ == nullptr) {
            close();
            return Result::failure("Unable to create image scaler");
        }

        sws_scale(
            swsContext_,
            decodedFrame_->data,
            decodedFrame_->linesize,
            0,
            decodedFrame_->height,
            bgraFrame_->data,
            bgraFrame_->linesize
        );

        frame.size = {outputWidth, outputHeight};
        frame.format = PixelFormat::Bgra;
        frame.stride = static_cast<std::size_t>(bgraFrame_->linesize[0]);
        frame.timestamp = Timestamp{0};
        frame.data.assign(
            bgraFrame_->data[0],
            bgraFrame_->data[0] + static_cast<std::size_t>(bgraFrame_->linesize[0]) * outputHeight
        );
        rotateBgra(frame, quarter);
        close();
        return Result::success();
    }

private:
    void close() {
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
    }

    AVFormatContext* formatContext_ = nullptr;
    AVCodecContext* codecContext_ = nullptr;
    const AVStream* stream_ = nullptr;
    AVFrame* decodedFrame_ = nullptr;
    AVFrame* bgraFrame_ = nullptr;
    AVPacket* packet_ = nullptr;
    SwsContext* swsContext_ = nullptr;
};

ImageDecoder::ImageDecoder() : impl_(std::make_unique<ImageDecoderImpl>()) {}
ImageDecoder::~ImageDecoder() = default;
ImageDecoder::ImageDecoder(ImageDecoder&&) noexcept = default;
ImageDecoder& ImageDecoder::operator=(ImageDecoder&&) noexcept = default;

Result ImageDecoder::load(
    const std::filesystem::path& path,
    Size requestedSize,
    VideoFrame& frame
) {
    if (!impl_) {
        return Result::failure("Image decoder is not initialised");
    }
    return impl_->load(path, requestedSize, frame);
}

}
