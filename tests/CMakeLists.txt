add_executable(digitalcam_unit_tests
  unit/test_main.cpp
  unit/test_playback_state.cpp
  unit/test_loop_controller.cpp
  unit/test_playback_scheduler.cpp
  unit/test_frame_queue.cpp
  unit/test_pixel_converter.cpp
  unit/test_frame_processor.cpp
  unit/test_app_config.cpp
  unit/test_cli_options.cpp
  unit/test_image_decoder.cpp
)

target_link_libraries(digitalcam_unit_tests
  PRIVATE
    GTest::gtest
    digitalcam::app_cli
    digitalcam::config
    digitalcam::core
)

if(WIN32 AND DIGITALCAM_BUILD_PLATFORM_ADAPTERS)
  target_sources(digitalcam_unit_tests
    PRIVATE
      unit/test_shared_frame.cpp
  )
  target_include_directories(digitalcam_unit_tests
    PRIVATE
      ${CMAKE_SOURCE_DIR}/virtual_camera/windows/include
  )
  target_link_libraries(digitalcam_unit_tests
    PRIVATE
      digitalcam::camera_windows
  )
endif()

include(GoogleTest)
gtest_discover_tests(digitalcam_unit_tests)
