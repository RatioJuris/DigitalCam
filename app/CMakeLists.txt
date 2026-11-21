find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_library(digitalcam_app_cli STATIC
  CliOptions.cpp
  CliOptions.h
)

target_include_directories(digitalcam_app_cli
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(digitalcam_app_cli
  PUBLIC
    digitalcam::core
    digitalcam::logging
)

add_library(digitalcam::app_cli ALIAS digitalcam_app_cli)

qt_add_executable(DigitalCamStudio
  WIN32
  MACOSX_BUNDLE
  main.cpp
  MainWindow.cpp
  MainWindow.h
)

target_link_libraries(DigitalCamStudio
  PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    digitalcam::app_cli
    digitalcam::config
    digitalcam::core
    digitalcam::camera_adapters
    digitalcam::audio_adapters
    digitalcam::logging
)

add_executable(DigitalCamCli
  cli_main.cpp
)

target_link_libraries(DigitalCamCli
  PRIVATE
    digitalcam::app_cli
    digitalcam::config
    digitalcam::core
    digitalcam::camera_adapters
    digitalcam::audio_adapters
    digitalcam::logging
)

if(WIN32 AND DIGITALCAM_BUILD_PLATFORM_ADAPTERS)
  target_link_libraries(DigitalCamStudio
    PRIVATE
      digitalcam::camera_windows
  )
  target_compile_definitions(DigitalCamStudio
    PRIVATE
      DIGITALCAM_WINDOWS_CAMERA=1
  )
  target_link_libraries(DigitalCamCli
    PRIVATE
      digitalcam::camera_windows
  )
  target_compile_definitions(DigitalCamCli
    PRIVATE
      DIGITALCAM_WINDOWS_CAMERA=1
  )
endif()

set_target_properties(DigitalCamStudio PROPERTIES
  OUTPUT_NAME DigitalCamStudio
)

set_target_properties(DigitalCamCli PROPERTIES
  OUTPUT_NAME DigitalCamCli
)
