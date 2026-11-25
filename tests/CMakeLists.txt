add_executable(kick_dsp_tests
    test_smoke.cpp
    test_render.cpp)

target_link_libraries(kick_dsp_tests PRIVATE kick_dsp Catch2::Catch2WithMain)

set_target_properties(kick_dsp_tests PROPERTIES FOLDER "KICKFORGE")

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(kick_dsp_tests)

# Plugin integration tests: link the plugin's shared code directly.
add_executable(kickforge_plugin_tests
    test_plugin.cpp)

target_link_libraries(kickforge_plugin_tests PRIVATE Kickforge Catch2::Catch2WithMain)
# The JUCE module sources are compiled into Kickforge; borrow its include
# paths and defines rather than compiling the modules a second time.
target_include_directories(kickforge_plugin_tests PRIVATE
    $<TARGET_PROPERTY:Kickforge,INCLUDE_DIRECTORIES>)
target_compile_definitions(kickforge_plugin_tests PRIVATE
    $<TARGET_PROPERTY:Kickforge,COMPILE_DEFINITIONS>)
set_target_properties(kickforge_plugin_tests PROPERTIES FOLDER "KICKFORGE")
catch_discover_tests(kickforge_plugin_tests)
