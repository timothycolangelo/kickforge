add_executable(kick_dsp_tests
    test_smoke.cpp)

target_link_libraries(kick_dsp_tests PRIVATE kick_dsp Catch2::Catch2WithMain)

set_target_properties(kick_dsp_tests PROPERTIES FOLDER "KICKFORGE")

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(kick_dsp_tests)
