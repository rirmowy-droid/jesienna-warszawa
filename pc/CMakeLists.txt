cmake_minimum_required(VERSION 3.22)
project(JesiennaWarszawa VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# JUCE: a local checkout (-DJUCE_DIR=...) or downloaded at configure time
if(JUCE_DIR)
  add_subdirectory(${JUCE_DIR} juce)
else()
  include(FetchContent)
  FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(juce)
endif()

option(JW_ASIO "Build the standalone app with ASIO (needs the ASIO SDK in ASIO_SDK_DIR)" OFF)

juce_add_plugin(JesiennaWarszawa
  COMPANY_NAME "Jesienna Warszawa"
  COMPANY_WEBSITE "https://github.com/rirmowy-droid/jesienna-warszawa"
  PRODUCT_NAME "Jesienna Warszawa"
  BUNDLE_ID "pl.jesiennawarszawa.gitara"
  PLUGIN_MANUFACTURER_CODE Jswa
  PLUGIN_CODE Jwgt
  FORMATS VST3 Standalone
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Fx Distortion
  MICROPHONE_PERMISSION_ENABLED TRUE
  MICROPHONE_PERMISSION_TEXT "Wejście gitary")

target_sources(JesiennaWarszawa PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(JesiennaWarszawa PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_USE_MP3AUDIOFORMAT=1 JUCE_DISPLAY_SPLASH_SCREEN=0)
if(JW_ASIO AND ASIO_SDK_DIR)
  target_compile_definitions(JesiennaWarszawa PUBLIC JUCE_ASIO=1)
  target_include_directories(JesiennaWarszawa PUBLIC ${ASIO_SDK_DIR}/common)
endif()
target_link_libraries(JesiennaWarszawa PRIVATE
  juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
