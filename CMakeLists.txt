# Example Audio Plugin CMakeLists.txt

# To get started on a new plugin, copy this entire folder (containing this file and C++ sources) to
# a convenient location, and then start making modifications.

# The first line of any CMake project should be a call to `cmake_minimum_required`, which checks
# that the installed CMake will be able to understand the following CMakeLists, and ensures that
# CMake's behaviour is compatible with the named version. This is a standard CMake command, so more
# information can be found in the CMake docs.

cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
project(MixTrainer VERSION 0.0.1)

find_package(JUCE CONFIG REQUIRED)

juce_add_plugin(MixTrainer_Master
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
    PLUGIN_CODE MTM0
    FORMATS AU VST3
    PRODUCT_NAME "MixTrainer Master")


target_sources(MixTrainer_Master
    PRIVATE
        Processor_Master.cpp)

target_compile_definitions(MixTrainer_Master
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
        
target_link_libraries(MixTrainer_Master
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

juce_add_plugin(MixTrainer_Slave
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
    PLUGIN_CODE MTS0
    FORMATS AU VST3
    PRODUCT_NAME "MixTrainer Slave")


target_sources(MixTrainer_Slave
    PRIVATE
        Processor_Slave.cpp)

target_compile_definitions(MixTrainer_Slave
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixTrainer_Slave
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

