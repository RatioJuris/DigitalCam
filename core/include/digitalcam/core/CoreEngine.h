#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <digitalcam/core/IAudioAdapter.h>
#include <digitalcam/core/ICameraAdapter.h>
#include <digitalcam/core/MediaMetadata.h>
#include <digitalcam/core/Result.h>
#include <digitalcam/core/Types.h>
#include <digitalcam/core/audio/AudioDecoder.h>
#include <digitalcam/core/audio/AudioProcessor.h>
#include <digitalcam/core/audio/AudioSynchronizer.h>
#include <digitalcam/core/media/ImageDecoder.h>
#include <digitalcam/core/media/VideoDecoder.h>
#include <digitalcam/core/playback/LoopController.h>
#include <digitalcam/core/playback/PlaybackController.h>
#include <digitalcam/core/playback/PlaybackScheduler.h>
#include <digitalcam/core/playback/PlaybackState.h>
#include <digitalcam/core/video/FrameProcessor.h>

namespace digitalcam::core {

class CoreEngine {
public:
    struct Status {
        PlaybackState state = PlaybackState::Idle;
        bool cameraAvailable = false;
        bool audioAvailable = false;
        bool workerRunning = false;
        bool looping = true;
        bool muted = false;
        float volume = 1.0F;
        std::string mediaName;
        Timestamp position{0};
    };

    using StatusCallback = std::function<void(const Status&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using FrameCallback = std::function<void(const VideoFrame&)>;

    CoreEngine(CameraAdapterPtr cameraAdapter, AudioAdapterPtr audioAdapter);
    ~CoreEngine();

    CoreEngine(const CoreEngine&) = delete;
    CoreEngine& operator=(const CoreEngine&) = delete;

    Result startCamera(const VideoOutputFormat& format);
    void stopCamera();

    Result startAudio(int sampleRate = 48000, int channels = 2);
    void stopAudio();

    Result loadMedia(const std::filesystem::path& path, bool autoPlay = false);
    void play();
    void pause();
    void stop();
    Result seek(Timestamp position);

    void setLooping(bool enabled);
    void setVolume(float volume);
    void setMuted(bool muted);

    void setAspectMode(AspectMode mode);
    void setStatusCallback(StatusCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setFrameCallback(FrameCallback callback);

    Status status() const;

private:
    void workerLoop();
    void publishCurrentFrame();
    void publishCurrentAudio();
    void emitStatus();
    void emitError(const std::string& message);
    VideoFrame blackFrame() const;

    CameraAdapterPtr cameraAdapter_;
    AudioAdapterPtr audioAdapter_;

    mutable std::recursive_mutex mutex_;
    PlaybackController playback_;
    PlaybackScheduler scheduler_;
    LoopController loopController_;
    FrameProcessor frameProcessor_;
    AudioProcessor audioProcessor_;
    AudioSynchronizer audioSynchronizer_;

    VideoDecoder videoDecoder_;
    ImageDecoder imageDecoder_;
    AudioDecoder audioDecoder_;

    VideoOutputFormat outputFormat_;
    VideoFrame lastFrame_;
    bool hasFrame_ = false;
    std::filesystem::path mediaPath_;
    MediaType mediaType_ = MediaType::Unknown;
    Timestamp position_{0};
    bool cameraStarted_ = false;
    bool audioStarted_ = false;
    int audioSampleRate_ = 48000;
    int audioChannels_ = 2;

    std::atomic<bool> workerRunning_{false};
    std::atomic<bool> stopRequested_{false};
    std::thread worker_;

    StatusCallback statusCallback_;
    ErrorCallback errorCallback_;
    FrameCallback frameCallback_;
};

}
