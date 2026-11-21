#include <digitalcam/core/CoreEngine.h>

#include <utility>

#include <digitalcam/core/media/MediaLoader.h>

#include <digitalcam/logging/Logger.h>

namespace digitalcam::core {

CoreEngine::CoreEngine(CameraAdapterPtr cameraAdapter, AudioAdapterPtr audioAdapter)
    : cameraAdapter_(std::move(cameraAdapter)),
      audioAdapter_(std::move(audioAdapter)) {
    workerRunning_.store(true);
    stopRequested_.store(false);
    worker_ = std::thread(&CoreEngine::workerLoop, this);
}

CoreEngine::~CoreEngine() {
    stopRequested_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
    workerRunning_.store(false);
    if (cameraAdapter_) {
        cameraAdapter_->stop();
    }
    if (audioAdapter_) {
        audioAdapter_->stop();
    }
}

Result CoreEngine::startCamera(const VideoOutputFormat& format) {
    if (!cameraAdapter_) {
        return Result::failure("No camera adapter configured");
    }
    if (!format.size.valid() || format.frameRate <= 0.0) {
        return Result::failure("Invalid camera output format");
    }

    DCAM_LOG_INFO("Starting camera adapter: " + cameraAdapter_->name());
    const Result installResult = cameraAdapter_->install();
    if (!installResult) {
        DCAM_LOG_ERROR("Camera adapter install failed: " + installResult.message);
        return installResult;
    }
    DCAM_LOG_INFO("Camera adapter install check passed");
    const Result startResult = cameraAdapter_->start(format);
    if (!startResult) {
        DCAM_LOG_ERROR("Camera adapter start failed: " + startResult.message);
        return startResult;
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    outputFormat_ = format;
    cameraStarted_ = true;
    scheduler_.setFrameRate(format.frameRate);
    FrameProcessorOptions options;
    options.outputSize = format.size;
    options.outputFormat = format.format;
    options.aspectMode = frameProcessor_.options().aspectMode;
    options.background = frameProcessor_.options().background;
    frameProcessor_.configure(options);
    emitStatus();
    DCAM_LOG_INFO("Camera adapter started: " + cameraAdapter_->name());
    return Result::success();
}

void CoreEngine::stopCamera() {
    CameraAdapterPtr adapter;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        cameraStarted_ = false;
        adapter = cameraAdapter_;
        emitStatus();
    }
    if (adapter) {
        adapter->stop();
        DCAM_LOG_INFO("Camera adapter stopped");
    }
}

Result CoreEngine::startAudio(int sampleRate, int channels) {
    if (!audioAdapter_) {
        return Result::failure("No audio adapter configured");
    }
    if (sampleRate <= 0 || channels <= 0) {
        return Result::failure("Invalid audio format");
    }

    const Result installResult = audioAdapter_->install();
    if (!installResult) {
        return installResult;
    }
    const Result startResult = audioAdapter_->start(sampleRate, channels);
    if (!startResult) {
        return startResult;
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    audioSampleRate_ = sampleRate;
    audioChannels_ = channels;
    audioStarted_ = true;
    emitStatus();
    DCAM_LOG_INFO("Audio adapter started: " + audioAdapter_->name());
    return Result::success();
}

void CoreEngine::stopAudio() {
    AudioAdapterPtr adapter;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        audioStarted_ = false;
        adapter = audioAdapter_;
        emitStatus();
    }
    if (adapter) {
        adapter->stop();
        DCAM_LOG_INFO("Audio adapter stopped");
    }
}

Result CoreEngine::loadMedia(const std::filesystem::path& path, bool autoPlay) {
    DCAM_LOG_INFO("Loading media: " + path.string());
    MediaLoader loader;
    MediaMetadata metadata;
    Result result = loader.probe(path, metadata);
    if (!result) {
        DCAM_LOG_ERROR("Media probe failed: " + result.message);
        return result;
    }

    VideoFrame imageFrame;
    if (metadata.type == MediaType::Image) {
        ImageDecoder decoder;
        result = decoder.load(path, {}, imageFrame);
        if (!result) {
            return result;
        }
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    audioDecoder_ = AudioDecoder{};
    videoDecoder_ = VideoDecoder{};
    imageDecoder_ = ImageDecoder{};
    mediaPath_ = path;
    mediaType_ = metadata.type;
    position_ = Timestamp{0};
    lastFrame_ = {};
    hasFrame_ = false;
    audioSynchronizer_.reset();
    scheduler_.reset();
    loopController_.reset();

    if (metadata.type == MediaType::Image) {
        lastFrame_ = std::move(imageFrame);
        hasFrame_ = true;
    } else {
        result = videoDecoder_.open(path, metadata);
        if (!result) {
            emitError(result.message);
            return result;
        }
        if (metadata.hasAudio) {
            const Result audioResult = audioDecoder_.open(path, metadata);
            if (!audioResult) {
                DCAM_LOG_WARNING(audioResult.message);
            }
        }
    }

    result = playback_.markReady();
    if (!result) {
        emitError(result.message);
        return result;
    }

    if (autoPlay) {
        result = playback_.markPlaying();
        if (!result) {
            emitError(result.message);
            return result;
        }
    }

    emitStatus();
    DCAM_LOG_INFO("Media loaded: " + path.string());
    return Result::success();
}

void CoreEngine::play() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const Result result = playback_.markPlaying();
    if (!result) {
        emitError(result.message);
        return;
    }
    emitStatus();
}

void CoreEngine::pause() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const Result result = playback_.markPaused();
    if (!result) {
        emitError(result.message);
        return;
    }
    emitStatus();
}

void CoreEngine::stop() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const Result result = playback_.markStopped();
    if (!result) {
        emitError(result.message);
        return;
    }
    emitStatus();
}

Result CoreEngine::seek(Timestamp position) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (mediaType_ == MediaType::Video && videoDecoder_.isOpen()) {
        const Result result = videoDecoder_.seek(position);
        if (!result) {
            return result;
        }
        if (audioDecoder_.isOpen()) {
            audioDecoder_.reset();
        }
    }
    position_ = position;
    return Result::success();
}

void CoreEngine::setLooping(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    loopController_.setEnabled(enabled);
}

void CoreEngine::setVolume(float volume) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    audioProcessor_.setVolume(volume);
}

void CoreEngine::setMuted(bool muted) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    audioProcessor_.setMuted(muted);
}

void CoreEngine::setAspectMode(AspectMode mode) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    FrameProcessorOptions options = frameProcessor_.options();
    options.aspectMode = mode;
    frameProcessor_.configure(options);
}

void CoreEngine::setStatusCallback(StatusCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    statusCallback_ = std::move(callback);
}

void CoreEngine::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    errorCallback_ = std::move(callback);
}

void CoreEngine::setFrameCallback(FrameCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    frameCallback_ = std::move(callback);
}

CoreEngine::Status CoreEngine::status() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    Status status;
    status.state = playback_.state();
    status.cameraAvailable = cameraStarted_ && cameraAdapter_ && cameraAdapter_->isAvailable();
    status.audioAvailable = audioStarted_ && audioAdapter_ && audioAdapter_->isAvailable();
    status.workerRunning = workerRunning_.load();
    status.looping = loopController_.isEnabled();
    status.muted = audioProcessor_.muted();
    status.volume = audioProcessor_.volume();
    status.mediaName = mediaPath_.filename().string();
    status.position = position_;
    return status;
}

void CoreEngine::workerLoop() {
    while (!stopRequested_.load()) {
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            const PlaybackState state = playback_.state();

            if (state == PlaybackState::Playing && mediaType_ == MediaType::Video && videoDecoder_.isOpen()) {
                VideoFrame frame;
                Result decodeResult;
                const DecodeStatus status = videoDecoder_.decodeNext(frame, decodeResult);
                if (status == DecodeStatus::Ok) {
                    lastFrame_ = std::move(frame);
                    hasFrame_ = true;
                    position_ = lastFrame_.timestamp;
                    audioSynchronizer_.setVideoTimestamp(position_);
                } else if (status == DecodeStatus::EndOfStream) {
                    loopController_.onEndReached();
                    if (loopController_.shouldLoop()) {
                        videoDecoder_.reset();
                        if (audioDecoder_.isOpen()) {
                            audioDecoder_.reset();
                        }
                    } else {
                        playback_.markPaused();
                        emitStatus();
                    }
                } else {
                    emitError(decodeResult.message);
                }
            } else if (state == PlaybackState::Playing && mediaType_ == MediaType::Image) {
                position_ += scheduler_.interval();
            }

            publishCurrentFrame();
            publishCurrentAudio();
        }
        scheduler_.markPublished();
        scheduler_.waitUntilNext();
    }
}

void CoreEngine::publishCurrentFrame() {
    const VideoFrame source = hasFrame_ ? lastFrame_ : blackFrame();
    VideoFrame output;
    const Result result = frameProcessor_.process(source, output);
    if (!result) {
        return;
    }

    if (cameraStarted_ && cameraAdapter_ && cameraAdapter_->isAvailable()) {
        cameraAdapter_->publishFrame(output);
    }
    if (frameCallback_) {
        frameCallback_(output);
    }
}

void CoreEngine::publishCurrentAudio() {
    if (!audioStarted_ || !audioAdapter_ || !audioAdapter_->isAvailable()) {
        return;
    }

    AudioBuffer buffer = AudioProcessor::silence(audioSampleRate_, audioChannels_, position_);
    if (playback_.state() == PlaybackState::Playing && audioDecoder_.isOpen()) {
        AudioBuffer decoded;
        Result decodeResult;
        if (audioDecoder_.decodeNext(decoded, decodeResult) == DecodeStatus::Ok) {
            decoded.timestamp = audioSynchronizer_.align(decoded.timestamp);
            audioProcessor_.process(decoded);
            buffer = std::move(decoded);
        }
    }
    audioAdapter_->publishAudio(buffer);
}

void CoreEngine::emitStatus() {
    if (!statusCallback_) {
        return;
    }

    Status status;
    status.state = playback_.state();
    status.cameraAvailable = cameraStarted_ && cameraAdapter_ && cameraAdapter_->isAvailable();
    status.audioAvailable = audioStarted_ && audioAdapter_ && audioAdapter_->isAvailable();
    status.workerRunning = workerRunning_.load();
    status.looping = loopController_.isEnabled();
    status.muted = audioProcessor_.muted();
    status.volume = audioProcessor_.volume();
    status.mediaName = mediaPath_.filename().string();
    status.position = position_;
    statusCallback_(status);
}

void CoreEngine::emitError(const std::string& message) {
    DCAM_LOG_ERROR(message);
    if (errorCallback_) {
        errorCallback_(message);
    }
}

VideoFrame CoreEngine::blackFrame() const {
    VideoFrame frame;
    const FrameProcessorOptions options = frameProcessor_.options();
    frame.size = options.outputSize;
    frame.format = options.outputFormat;
    frame.timestamp = position_;
    if (!frame.size.valid()) {
        return frame;
    }

    frame.stride = static_cast<std::size_t>(frame.size.width) * bytesPerPixel(frame.format);
    frame.data.assign(frame.stride * static_cast<std::size_t>(frame.size.height), 0);
    if (frame.stride % 4 == 0) {
        for (std::size_t offset = 3; offset < frame.data.size(); offset += 4) {
            frame.data[offset] = 255;
        }
    }
    return frame;
}

}
