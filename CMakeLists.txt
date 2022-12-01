# Example Audio Plugin CMakeLists.txt

cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)

project(MixTrainer VERSION 0.0.1)

find_package(JUCE CONFIG REQUIRED)

add_library(juce_plugin_modules STATIC)

target_link_libraries(juce_plugin_modules
    PRIVATE
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(juce_plugin_modules
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,COMPILE_DEFINITIONS>)

target_include_directories(juce_plugin_modules
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,INCLUDE_DIRECTORIES>)

set_target_properties(juce_plugin_modules PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    VISIBILITY_INLINES_HIDDEN TRUE
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden)

juce_add_plugin(MixTrainer_Host
    VERSION 0.0.1
    # ICON_BIG ...
    # ICON_SMALL ...
    COMPANY_NAME Octave
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE               # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT FALSE              # Does the plugin need midi output?
    IS_MIDI_EFFECT FALSE                 # Is this plugin a MIDI effect?
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE    # Does the editor need keyboard focus?
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Octa
    PLUGIN_CODE MTH0
    FORMATS AU VST3
    PRODUCT_NAME "MixTrainer Host")

target_sources(MixTrainer_Host
    PRIVATE
        Plugin_Host/Processor_Host.cpp
        Game/Game.cpp
        Game/Game_Mixer.cpp
        Application/Application.cpp)

target_compile_definitions(MixTrainer_Host
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixTrainer_Host PUBLIC juce_plugin_modules)

juce_add_plugin(MixTrainer_Track
    VERSION 0.0.1
    # ICON_BIG ...
    # ICON_SMALL ...
    COMPANY_NAME Octave
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE               # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT FALSE              # Does the plugin need midi output?
    IS_MIDI_EFFECT FALSE                 # Is this plugin a MIDI effect?
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE    # Does the editor need keyboard focus?
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Octa
    PLUGIN_CODE MTT0
    FORMATS AU AUv3 VST3
    PRODUCT_NAME "MixTrainer Track")

target_sources(MixTrainer_Track
    PRIVATE
        Plugin_Track/Processor_Track.cpp)

target_compile_definitions(MixTrainer_Track
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixTrainer_Track PUBLIC juce_plugin_modules)

juce_add_gui_app(Standalone
    PRODUCT_NAME "Mix Trainer")

target_sources(Standalone
    PRIVATE
        Standalone/Standalone.cpp
        Game/Game.cpp)

target_precompile_headers(Standalone
    PRIVATE Standalone/pch.h)

target_compile_definitions(Standalone
    PRIVATE
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Standalone,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Standalone,JUCE_VERSION>")

target_link_libraries(Standalone PUBLIC juce_plugin_modules)