add_library(digitalcam_logging STATIC
  src/Logger.cpp
)

target_include_directories(digitalcam_logging
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(digitalcam_logging
  PUBLIC
    digitalcam::threads
)

if(spdlog_FOUND)
  target_link_libraries(digitalcam_logging PRIVATE spdlog::spdlog)
  target_compile_definitions(digitalcam_logging PUBLIC DIGITALCAM_HAS_SPDLOG=1)
endif()

add_library(digitalcam::logging ALIAS digitalcam_logging)
