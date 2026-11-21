add_library(digitalcam_config STATIC
  src/AppConfig.cpp
)

target_include_directories(digitalcam_config
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(digitalcam_config
  PUBLIC
    digitalcam::logging
)

add_library(digitalcam::config ALIAS digitalcam_config)
