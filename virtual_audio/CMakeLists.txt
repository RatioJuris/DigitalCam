add_library(digitalcam_audio_adapters STATIC
  src/AudioManager.cpp
  src/NullAudioAdapter.cpp
)

target_include_directories(digitalcam_audio_adapters
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(digitalcam_audio_adapters
  PUBLIC
    digitalcam::core
)

add_library(digitalcam::audio_adapters ALIAS digitalcam_audio_adapters)

if(DIGITALCAM_BUILD_PLATFORM_ADAPTERS)
  add_subdirectory(windows)
  add_subdirectory(linux)
  add_subdirectory(macos)
endif()
