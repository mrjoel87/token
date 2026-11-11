cmake_minimum_required(VERSION 3.22)

project(TOTEM VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime: the plugin needs no Visual C++ redistributable on the target machine.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ---------------------------------------------------------------------------------------------
# JUCE
#   Option A: point at a local JUCE checkout:   -DTOTEM_JUCE_PATH=C:/path/to/JUCE
#   Option B: leave empty and CMake downloads the JUCE release zip below (no git required).
# ---------------------------------------------------------------------------------------------
set(TOTEM_JUCE_PATH "" CACHE PATH "Path to a local JUCE 8 checkout (optional)")
set(TOTEM_JUCE_VERSION "8.0.8" CACHE STRING "JUCE release tag to download when TOTEM_JUCE_PATH is empty")

if(TOTEM_JUCE_PATH)
    add_subdirectory("${TOTEM_JUCE_PATH}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        URL "https://github.com/juce-framework/JUCE/archive/refs/tags/${TOTEM_JUCE_VERSION}.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# IMPORTANT - plugin identity:
#   PLUGIN_MANUFACTURER_CODE (Smsw) and PLUGIN_CODE (Tokn) define the VST3 class ID that Ableton
#   uses to find the plugin in saved Live Sets. They are deliberately unchanged from TOKEN so
#   existing projects keep working. Only the visible name changed to "SMSW TOTEM".
#   The CMake target is also still called TOKEN (internal only) so existing GitHub workflows keep working.
juce_add_plugin(TOKEN
    COMPANY_NAME "SMSW"
    PLUGIN_MANUFACTURER_CODE Smsw
    PLUGIN_CODE Tokn
    FORMATS VST3
    PRODUCT_NAME "SMSW TOTEM"
    DESCRIPTION "Audio-reactive visualiser"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE      # Program Files needs admin rights - use install.ps1 instead
    VST3_CATEGORIES Fx Analyzer
    BUNDLE_ID com.smsw.token)          # unchanged on purpose (identity), only used on macOS

target_sources(TOKEN PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/AudioAnalyser.cpp
    Source/Theme.cpp
    Source/ColourEngine.cpp
    Source/Engine/SacredWorld.cpp
    Source/Engine/DreamWorld.cpp
    Source/Engine/SignalWorld.cpp
    Source/Engine/Alignment.cpp
    Source/Engine/TotemEngine.cpp
    Source/UI/TotemLookAndFeel.cpp
    Source/UI/Controls.cpp
    Source/Art/ArtKit.cpp
    Source/Art/ArtAncient.cpp
    Source/Art/ArtNature.cpp
    Source/Art/ArtCelestial.cpp
    Source/Art/ArtSentimental.cpp
    Source/Art/ArtSurreal.cpp
    Source/Art/ArtGeometric.cpp
    Source/Art/ArtMandala.cpp)

target_include_directories(TOKEN PRIVATE Source)

target_compile_definitions(TOKEN PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(TOKEN
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
