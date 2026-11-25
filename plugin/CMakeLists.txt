set(KF_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND KF_FORMATS AU)
endif()

juce_add_plugin(Kickforge
    PRODUCT_NAME "KICKFORGE"
    COMPANY_NAME "Kickforge"
    BUNDLE_ID "com.kickforge.kickforge"
    PLUGIN_MANUFACTURER_CODE Kkfg
    PLUGIN_CODE Kfg1
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS ${KF_FORMATS})

target_sources(Kickforge PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(Kickforge PRIVATE Source)

target_compile_definitions(Kickforge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Kickforge
    PRIVATE
        kick_dsp
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
