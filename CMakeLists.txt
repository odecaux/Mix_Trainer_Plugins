# Example Audio Plugin CMakeLists.txt

cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)

project(MixTrainer VERSION 0.0.1)

find_package(JUCE CONFIG REQUIRED)

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
        Processor_Host.cpp
        Game.cpp
        Game_Mixer.cpp
        Game_Mixer_Tries.cpp
        Game_Mixer_Timer.cpp
        Application.cpp)

target_compile_definitions(MixTrainer_Host
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
        
target_link_libraries(MixTrainer_Host
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

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
    FORMATS AU VST3
    PRODUCT_NAME "MixTrainer Track")

target_sources(MixTrainer_Track
    PRIVATE
        Processor_Track.cpp)

target_compile_definitions(MixTrainer_Track
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixTrainer_Track
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

