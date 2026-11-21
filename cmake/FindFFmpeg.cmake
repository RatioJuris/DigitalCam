set(_digitalcam_ffmpeg_hints
  ${FFMPEG_ROOT}
  $ENV{FFMPEG_ROOT}
  ${CMAKE_PREFIX_PATH}
)

find_path(
  FFMPEG_INCLUDE_DIR
  NAMES libavcodec/avcodec.h
  HINTS ${_digitalcam_ffmpeg_hints}
  PATH_SUFFIXES include
)

find_library(FFMPEG_LIBRARY_AVFORMAT NAMES avformat HINTS ${_digitalcam_ffmpeg_hints} PATH_SUFFIXES lib)
find_library(FFMPEG_LIBRARY_AVCODEC NAMES avcodec HINTS ${_digitalcam_ffmpeg_hints} PATH_SUFFIXES lib)
find_library(FFMPEG_LIBRARY_AVUTIL NAMES avutil HINTS ${_digitalcam_ffmpeg_hints} PATH_SUFFIXES lib)
find_library(FFMPEG_LIBRARY_SWSCALE NAMES swscale HINTS ${_digitalcam_ffmpeg_hints} PATH_SUFFIXES lib)
find_library(FFMPEG_LIBRARY_SWRESAMPLE NAMES swresample HINTS ${_digitalcam_ffmpeg_hints} PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  FFmpeg
  REQUIRED_VARS
    FFMPEG_INCLUDE_DIR
    FFMPEG_LIBRARY_AVFORMAT
    FFMPEG_LIBRARY_AVCODEC
    FFMPEG_LIBRARY_AVUTIL
    FFMPEG_LIBRARY_SWSCALE
    FFMPEG_LIBRARY_SWRESAMPLE
)

if(FFMPEG_FOUND)
  set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
  set(FFMPEG_LIBRARIES
    ${FFMPEG_LIBRARY_AVFORMAT}
    ${FFMPEG_LIBRARY_AVCODEC}
    ${FFMPEG_LIBRARY_AVUTIL}
    ${FFMPEG_LIBRARY_SWSCALE}
    ${FFMPEG_LIBRARY_SWRESAMPLE}
  )

  foreach(_component avformat avcodec avutil swscale swresample)
    if(NOT TARGET FFmpeg::${_component})
      string(TOUPPER ${_component} _component_upper)
      add_library(FFmpeg::${_component} UNKNOWN IMPORTED)
      set_target_properties(
        FFmpeg::${_component}
        PROPERTIES
          IMPORTED_LOCATION "${FFMPEG_LIBRARY_${_component_upper}}"
          INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
      )
    endif()
  endforeach()
endif()

mark_as_advanced(
  FFMPEG_INCLUDE_DIR
  FFMPEG_LIBRARY_AVFORMAT
  FFMPEG_LIBRARY_AVCODEC
  FFMPEG_LIBRARY_AVUTIL
  FFMPEG_LIBRARY_SWSCALE
  FFMPEG_LIBRARY_SWRESAMPLE
)
