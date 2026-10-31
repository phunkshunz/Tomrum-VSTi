cmake_minimum_required(VERSION 3.22)

project(TOMRUM VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE hämtas automatiskt vid bygget (eller från JUCE_DIR om den finns lokalt)
if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory(${JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# TOMRUM är en egen trumsynt med inbyggd sekvens. Den skickar också varje slag
# som MIDI ut, så att den kan styra andra ljud.
juce_add_plugin(Tomrum
    COMPANY_NAME "Phunkshunz AUDIO"
    PRODUCT_NAME "TOMRUM"
    BUNDLE_ID "com.phunkshunz.tomrum"
    PLUGIN_MANUFACTURER_CODE Phnk
    PLUGIN_CODE Tmrm
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Drum Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS VST3 AU Standalone)

target_sources(Tomrum
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

target_compile_definitions(Tomrum
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Tomrum
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Linux används bara för utveckling och tester: inget ljudkort, inga X-tillägg
if(UNIX AND NOT APPLE AND TARGET Tomrum)
    target_compile_definitions(Tomrum PUBLIC JUCE_ALSA=0 JUCE_JACK=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0 JUCE_USE_XSHM=0)
endif()
