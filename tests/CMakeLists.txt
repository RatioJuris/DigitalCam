add_executable(digitalcam_unit_tests
  unit/test_main.cpp
  unit/test_playback_state.cpp
  unit/test_loop_controller.cpp
  unit/test_playback_scheduler.cpp
  unit/test_frame_queue.cpp
  unit/test_pixel_converter.cpp
  unit/test_frame_processor.cpp
  unit/test_app_config.cpp
)

target_link_libraries(digitalcam_unit_tests
  PRIVATE
    GTest::gtest
    digitalcam::config
    digitalcam::core
)

include(GoogleTest)
gtest_discover_tests(digitalcam_unit_tests)
