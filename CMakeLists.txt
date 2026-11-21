cmake_minimum_required(VERSION 3.21)

project(
  DigitalCam
  VERSION 0.1.0
  DESCRIPTION "Software-based virtual camera for local image and video media"
  HOMEPAGE_URL "https://github.com/RatioJuris/DigitalCam"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(DIGITALCAM_BUILD_APP "Build the DigitalCam Studio Qt application" ON)
option(DIGITALCAM_BUILD_TESTS "Build DigitalCam unit tests" ON)
option(DIGITALCAM_BUILD_PLATFORM_ADAPTERS "Build native OS camera/audio adapters" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Dependencies)

add_subdirectory(logging)
add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(virtual_camera)
add_subdirectory(virtual_audio)

if(DIGITALCAM_BUILD_APP)
  add_subdirectory(app)
endif()

if(DIGITALCAM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
