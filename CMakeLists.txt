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
        JUCE_DISPLAY_SPLASH_SCREEN=0
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
        Plugin_Host/Application.cpp
        Game/Game.cpp
        Game/Game_UI.cpp
        Game/Game_Mixer.cpp
        Game/Game_Mixer_UI.cpp
        shared/shared.cpp)
        
if(MSVC)
  target_compile_options(MixTrainer_Host PRIVATE /W4 /WX /wd4505 /wd4100 /wd4101 /wd4189)
else()
  target_compile_options(MixTrainer_Host PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function -Wno-unused-variable)
endif()

target_precompile_headers(MixTrainer_Host
    PRIVATE shared/pch.h)

target_compile_definitions(MixTrainer_Host
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
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
  target_compile_options(MixTrainer_Track PRIVATE /W4 /WX /wd4505 /wd4100 /wd4101 /wd4189)
else()
  target_compile_options(MixTrainer_Track PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function -Wno-unused-variable)
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
        Standalone/Application_Standalone.cpp
        Game/Game.cpp
        Game/Game_UI.cpp
        Game/Frequency_Game.cpp
        Game/Compressor_Game.cpp
        Game/Frequency_Game_UI.cpp
        Game/Compressor_Game_UI.cpp
        shared/shared.cpp)

if(MSVC)
  target_compile_options(MixTrainer_Standalone PRIVATE /W4 /WX /wd4505 /wd4100 /wd4101 /wd4189)
  message("msvc")
else()
  target_compile_options(MixTrainer_Standalone PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function -Wno-unused-variable)
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

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
set(RAYLIB_VERSION 4.2.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
# Our Project

add_executable(ray EXCLUDE_FROM_ALL ray/main.cpp ray/raygui.c ray/miniaudio.c)
set(raylib_VERBOSE 1)
target_link_libraries(ray raylib)

if (EMSCRIPTEN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY")
  set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so taht you can directly open it.
endif ()