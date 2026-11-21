if(NOT WIN32)
  return()
endif()

find_package(WIL CONFIG REQUIRED)

add_library(digitalcam_camera_windows STATIC
  src/CameraInstaller.cpp
  src/SharedFrame.cpp
  src/WindowsVirtualCameraAdapter.cpp
)

target_include_directories(digitalcam_camera_windows
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(digitalcam_camera_windows
  PUBLIC
    digitalcam::core
    WIL::WIL
    mfplat.lib
    mf.lib
    mfuuid.lib
    ole32.lib
    advapi32.lib
)

add_library(digitalcam::camera_windows ALIAS digitalcam_camera_windows)

add_library(digitalcam_virtual_camera SHARED
  src/MediaFoundationVirtualCamera.cpp
  src/SharedFrame.cpp
  src/virtual_camera.def
)

target_include_directories(digitalcam_virtual_camera
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(digitalcam_virtual_camera
  PRIVATE
    WIL::WIL
    mfplat.lib
    mf.lib
    mfuuid.lib
    ole32.lib
    advapi32.lib
)

set_target_properties(digitalcam_virtual_camera PROPERTIES
  PREFIX ""
  OUTPUT_NAME DigitalCamVirtualCamera
)

add_executable(DigitalCamCameraInstaller
  src/installer_main.cpp
)

target_link_libraries(DigitalCamCameraInstaller
  PRIVATE
    digitalcam::camera_windows
)

add_dependencies(DigitalCamCameraInstaller digitalcam_virtual_camera)
