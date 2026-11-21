add_library(digitalcam_camera_adapters STATIC
  src/CameraManager.cpp
  src/NullCameraAdapter.cpp
)

target_include_directories(digitalcam_camera_adapters
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(digitalcam_camera_adapters
  PUBLIC
    digitalcam::core
)

add_library(digitalcam::camera_adapters ALIAS digitalcam_camera_adapters)

if(DIGITALCAM_BUILD_PLATFORM_ADAPTERS)
  add_subdirectory(windows)
  add_subdirectory(linux)
  add_subdirectory(macos)
endif()
