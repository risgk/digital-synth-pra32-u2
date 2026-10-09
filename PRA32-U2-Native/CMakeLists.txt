cmake_minimum_required(VERSION 3.22)

# The version is the same as PRA32-U2 (PRA32_U2_VERSION in Digital-Synth-PRA32-U2.ino)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../Digital-Synth-PRA32-U2/Digital-Synth-PRA32-U2.ino"
     PRA32_U2_VERSION_LINE REGEX "^#define PRA32_U2_VERSION ")
if(NOT PRA32_U2_VERSION_LINE MATCHES "\"v([0-9]+\\.[0-9]+\\.[0-9]+)")
  message(FATAL_ERROR "PRA32_U2_VERSION is not found in Digital-Synth-PRA32-U2.ino")
endif()
set(PRA32_U2_NATIVE_VERSION "${CMAKE_MATCH_1}")

project(PRA32_U2_NATIVE VERSION ${PRA32_U2_NATIVE_VERSION} LANGUAGES C CXX)

# For packaging (e.g. the GitHub Actions workflow)
file(WRITE "${CMAKE_BINARY_DIR}/pra32-u2-native-version.txt" "${PRA32_U2_NATIVE_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows: link the C/C++ runtime statically, so that the Visual C++ Redistributable is not needed
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE: a local checkout (PRA32_U2_NATIVE_JUCE_DIR), or downloaded by FetchContent
set(PRA32_U2_NATIVE_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")

if(PRA32_U2_NATIVE_JUCE_DIR)
  add_subdirectory("${PRA32_U2_NATIVE_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

set(PRA32_U2_NATIVE_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND PRA32_U2_NATIVE_FORMATS AU)
endif()

juce_add_plugin(PRA32_U2_Native
  PRODUCT_NAME              "PRA32-U2 Native"
  VERSION                   ${PRA32_U2_NATIVE_VERSION}
  COMPANY_NAME              "ISGK Instruments"
  BUNDLE_ID                 com.isgk-instruments.pra32-u2-native
  PLUGIN_MANUFACTURER_CODE  Isgk
  PLUGIN_CODE               P32N
  FORMATS                   ${PRA32_U2_NATIVE_FORMATS}
  IS_SYNTH                  TRUE
  NEEDS_MIDI_INPUT          TRUE
  NEEDS_MIDI_OUTPUT         FALSE
  IS_MIDI_EFFECT            FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD   FALSE
  VST3_CATEGORIES           Instrument Synth
  AU_MAIN_TYPE              kAudioUnitType_MusicDevice)

target_sources(PRA32_U2_Native PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
  Source/PRA32U2Engine.cpp
  Source/PRA32U2Renderer.cpp
  Source/PRA32U2Resampler.cpp)

# The PRA32-U2 core (not modified), used by PRA32U2Engine.cpp only
set_source_files_properties(Source/PRA32U2Engine.cpp PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/../Digital-Synth-PRA32-U2")

target_compile_definitions(PRA32_U2_Native PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_FORCE_USE_LEGACY_PARAM_IDS=1)  # The VST3 parameter IDs are the parameter indices (= the control numbers)

target_link_libraries(PRA32_U2_Native
  PRIVATE
    juce::juce_audio_utils
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)
