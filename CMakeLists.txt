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
        Plugin_Host/Processor_Host.cpp
        Game/Game.cpp
        Game/Game_Mixer.cpp
        Game/Game_Mixer_Tries.cpp
        Game/Game_Mixer_Timer.cpp
        Application/Application.cpp)

target_compile_definitions(MixTrainer_Host
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
        
target_compile_options(MixTrainer_Host PRIVATE /wd4505)

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
        Plugin_Track/Processor_Track.cpp)

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

juce_add_gui_app(Standalone
    # VERSION ...                       # Set this if the app version is different to the project version
    # ICON_BIG ...                      # ICON_* arguments specify a path to an image file to use as an icon
    # ICON_SMALL ...
    # DOCUMENT_EXTENSIONS ...           # Specify file extensions that should be associated with this app
    # COMPANY_NAME ...                  # Specify the name of the app's author
    PRODUCT_NAME "Gui App Example")     # The name of the final executable, which can differ from the target name

juce_generate_juce_header(Standalone)

target_sources(Standalone
    PRIVATE
        Standalone/Standalone.cpp)

target_compile_definitions(Standalone
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Standalone,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Standalone,JUCE_VERSION>")


target_link_libraries(Standalone
    PRIVATE
        # GuiAppData            # If we'd created a binary data target, we'd link to it here
        juce::juce_gui_extra
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)