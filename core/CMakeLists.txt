add_library(digitalcam_core STATIC
  src/media/MediaLoader.cpp
  src/media/ImageDecoder.cpp
  src/media/VideoDecoder.cpp
  src/media/MediaMetadata.cpp
  src/media/FfmpegCommon.cpp
  src/playback/PlaybackController.cpp
  src/playback/PlaybackScheduler.cpp
  src/playback/LoopController.cpp
  src/playback/PlaybackState.cpp
  src/video/FrameProcessor.cpp
  src/video/PixelConverter.cpp
  src/video/FrameQueue.cpp
  src/audio/AudioDecoder.cpp
  src/audio/AudioProcessor.cpp
  src/audio/AudioSynchronizer.cpp
  src/CoreEngine.cpp
)

target_include_directories(digitalcam_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(digitalcam_core
  PUBLIC
    digitalcam::logging
    digitalcam::threads
  PRIVATE
    digitalcam::ffmpeg
)

add_library(digitalcam::core ALIAS digitalcam_core)
