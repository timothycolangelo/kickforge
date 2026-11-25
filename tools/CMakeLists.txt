add_executable(kick_render render_cli.cpp)
target_link_libraries(kick_render PRIVATE kick_dsp)
set_target_properties(kick_render PROPERTIES FOLDER "KICKFORGE")

