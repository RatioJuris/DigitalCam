find_package(Threads REQUIRED)
find_package(FFmpeg REQUIRED)

find_package(spdlog QUIET)

add_library(digitalcam_threads INTERFACE)
target_link_libraries(digitalcam_threads INTERFACE Threads::Threads)
add_library(digitalcam::threads ALIAS digitalcam_threads)

add_library(digitalcam_ffmpeg INTERFACE)
target_include_directories(digitalcam_ffmpeg INTERFACE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(
  digitalcam_ffmpeg
  INTERFACE
    FFmpeg::avformat
    FFmpeg::avcodec
    FFmpeg::swscale
    FFmpeg::swresample
    FFmpeg::avutil
)
add_library(digitalcam::ffmpeg ALIAS digitalcam_ffmpeg)

if(DIGITALCAM_BUILD_TESTS)
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    message(WARNING "GoogleTest was not found; tests will not be built.")
    set(DIGITALCAM_BUILD_TESTS OFF CACHE BOOL "GoogleTest not available" FORCE)
  endif()
endif()
