cmake_minimum_required(VERSION 3.22)

project(UB_Poly16 VERSION 0.7.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE 8 checkout (https://github.com/juce-framework/JUCE), used under AGPLv3.
# Not vendored in this repo — override with -DUB_JUCE_DIR=<path>.
set(UB_JUCE_DIR "D:/Audio/JUCE" CACHE PATH "Path to a local JUCE checkout")
add_subdirectory("${UB_JUCE_DIR}" JUCE)

# ASIO support (optional, Windows): the Steinberg ASIO SDK is proprietary and
# NOT redistributable, so it is never part of this repo and public binaries
# are built without it. Local builds only:
#   -DUB_ASIO=ON -DUB_ASIO_SDK_DIR=<path to the SDK's 'common' directory>
option(UB_ASIO "Enable ASIO support (requires the Steinberg ASIO SDK)" OFF)
set(UB_ASIO_SDK_DIR "" CACHE PATH "Path to the ASIO SDK 'common' directory")

juce_add_plugin(UB_Poly16
    COMPANY_NAME            "TheUNBoRN"
    PLUGIN_MANUFACTURER_CODE Ubrn
    PLUGIN_CODE             Up16
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS                 VST3 Standalone
    ICON_BIG                "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png"
    ICON_SMALL              "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon_small.png"
    PRODUCT_NAME            "UB_Poly16")

juce_generate_juce_header(UB_Poly16)

target_sources(UB_Poly16 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SynthVoice.cpp
    Source/Arpeggiator.cpp
    Source/PresetManager.cpp)

target_compile_definitions(UB_Poly16 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

if(UB_ASIO)
    if(NOT EXISTS "${UB_ASIO_SDK_DIR}/iasiodrv.h")
        message(FATAL_ERROR "UB_ASIO=ON but UB_ASIO_SDK_DIR does not point to the ASIO SDK 'common' directory")
    endif()
    target_compile_definitions(UB_Poly16 PUBLIC JUCE_ASIO=1)
    target_include_directories(UB_Poly16 PRIVATE "${UB_ASIO_SDK_DIR}")
endif()

target_link_libraries(UB_Poly16 PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
