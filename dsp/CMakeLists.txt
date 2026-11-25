# kick_dsp: the whole sound engine, with no GUI dependency.
#
# The public headers in include/kick are plain C++ (no JUCE types), so tests and
# the plugin can use them without pulling in JUCE's include setup. JUCE's DSP
# modules are linked PRIVATE and only used inside the .cpp files.

add_library(kick_dsp STATIC
    src/KickDsp.cpp)

target_include_directories(kick_dsp PUBLIC include)

target_link_libraries(kick_dsp
    PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(kick_dsp
    PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0)

set_target_properties(kick_dsp PROPERTIES FOLDER "KICKFORGE")
