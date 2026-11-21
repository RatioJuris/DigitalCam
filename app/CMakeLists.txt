find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

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
    digitalcam::config
    digitalcam::core
    digitalcam::camera_adapters
    digitalcam::audio_adapters
    digitalcam::logging
)

set_target_properties(DigitalCamStudio
  PROPERTIES
    OUTPUT_NAME DigitalCamStudio
)
