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
    FORMATS AU AUv3 VST3
    PRODUCT_NAME "MixTrainer_Host")

target_sources(MixTrainer_Host
    PRIVATE
        Plugin_Host/Processor_Host.cpp
        Game/Game.cpp
        Game/Game_Mixer.cpp
        Application/Application.cpp
        shared/shared.cpp)
        
if(MSVC)
  target_compile_options(MixTrainer_Host PRIVATE /W4 /WX /wd4505)
else()
  target_compile_options(MixTrainer_Host PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function )
endif()

target_precompile_headers(MixTrainer_Host
    PRIVATE shared/pch.h)

target_compile_definitions(MixTrainer_Host
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixTrainer_Host 
    PUBLIC 
        juce_plugin_modules
    PRIVATE
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
    FORMATS AU AUv3 VST3
    PRODUCT_NAME "MixTrainer_Track")

target_sources(MixTrainer_Track
    PRIVATE
        Plugin_Track/Processor_Track.cpp
        shared/shared.cpp)
        
if(MSVC)
  target_compile_options(MixTrainer_Track PRIVATE /W4 /WX /wd4505)
else()
  target_compile_options(MixTrainer_Track PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function )
endif()

target_precompile_headers(MixTrainer_Track
    PRIVATE shared/pch.h)

target_compile_definitions(MixTrainer_Track
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixTrainer_Track 
    PUBLIC 
        juce_plugin_modules
    PRIVATE 
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

juce_add_gui_app(MixTrainer_Standalone
    PRODUCT_NAME "MixTrainer_Standalone")

target_sources(MixTrainer_Standalone
    PRIVATE
        Standalone/Standalone.cpp
        Game/Game.cpp
        Standalone/FrequencyGame.cpp
        Standalone/CompressorGame.cpp
        shared/shared.cpp)

if(MSVC)
  target_compile_options(MixTrainer_Standalone PRIVATE /W4 /WX /wd4505)
  message("msvc")
else()
  target_compile_options(MixTrainer_Standalone PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function )
  message("gcc")
endif()

target_precompile_headers(MixTrainer_Standalone
    PRIVATE shared/pch.h)

target_compile_definitions(MixTrainer_Standalone
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:MixTrainer_Standalone,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:MixTrainer_Standalone,JUCE_VERSION>")

target_link_libraries(MixTrainer_Standalone 
    PUBLIC 
        juce_plugin_modules
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)