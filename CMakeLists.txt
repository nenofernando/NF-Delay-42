cmake_minimum_required(VERSION 3.22)
# macOS target / universal binary: must be set BEFORE project() or CMake ignores it
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS Universal Binary")
project(NFDelay42 VERSION 1.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(NFDelay42_FETCH_JUCE "Fetch JUCE when not installed" ON)
# The NF License System (mute + activation overlay) is ON by default so every release build, Windows and the
# macOS DMG, ships locked. Pass -DNFDelay42_LICENSE=OFF only for local development.
option(NFDelay42_LICENSE "Enforce the NF License System (mute + activation overlay)" ON)
option(NFDelay42_ENABLE_AAX "Build AAX (licensed SDK required)" OFF)
if(WIN32)
 set(_nfd_home "$ENV{USERPROFILE}")
else()
 set(_nfd_home "$ENV{HOME}")
endif()
set(NFDelay42_AAX_SDK_PATH "${_nfd_home}/Documents/AAX_SDK" CACHE PATH "Licensed AAX SDK path")

find_package(JUCE CONFIG QUIET)
if(NOT JUCE_FOUND)
 if(NOT NFDelay42_FETCH_JUCE)
  message(FATAL_ERROR "JUCE not found. Set JUCE_DIR or enable NFDelay42_FETCH_JUCE.")
 endif()
 include(FetchContent)
 FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
 FetchContent_MakeAvailable(JUCE)
endif()

set(NFDelay42_FORMATS VST3)
if(APPLE)
 list(APPEND NFDelay42_FORMATS AU)
endif()
if(NFDelay42_ENABLE_AAX)
 if(NOT EXISTS "${NFDelay42_AAX_SDK_PATH}")
  message(FATAL_ERROR "NFDelay42_ENABLE_AAX requires the AAX SDK at NFDelay42_AAX_SDK_PATH (${NFDelay42_AAX_SDK_PATH}).")
 endif()
 juce_set_aax_sdk_path("${NFDelay42_AAX_SDK_PATH}")
 list(APPEND NFDelay42_FORMATS AAX)
endif()

juce_add_plugin(NFDelay42
 COMPANY_NAME "NF Audio Tools"
 COMPANY_COPYRIGHT "NF Audio Tools by Nenno Fernando. (C) 2026 NF Audio Tools. All rights reserved."
 BUNDLE_ID "com.nfaudiotools.nfdelay42"
 AAX_IDENTIFIER "com.nfaudiotools.nfdelay42"
 PLUGIN_MANUFACTURER_CODE Nfat
 PLUGIN_CODE Nfd4
 FORMATS ${NFDelay42_FORMATS}
 PRODUCT_NAME "NF D-42"
 PLUGIN_NAME "NF D-42"
 DESCRIPTION "Digital delay processor with VCO sweep, clock and infinite repeat"
 IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
 EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE
 VST3_CATEGORIES Fx Delay
 AU_MAIN_TYPE kAudioUnitType_Effect
 AAX_CATEGORY AAX_ePlugInCategory_Effect)
juce_generate_juce_header(NFDelay42)
juce_add_binary_data(NFDelay42Assets HEADER_NAME NFDelay42BinaryData.h NAMESPACE NFDelay42BinaryData SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Logo/nf_audio_tools_logo.png" "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Manuals/NF_D42_Manual_English.pdf" "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Manuals/NF_D42_Manual_Portugues.pdf")

target_sources(NFDelay42 PRIVATE
 Source/PluginProcessor.cpp Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginEditor.h
 Source/PresetManager.cpp Source/PresetManager.h Source/ManualManager.cpp Source/ManualManager.h Source/DSP/DelayEngine.h Source/UI/PanelParts.h)
target_compile_definitions(NFDelay42 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
if(NFDelay42_LICENSE)
 target_compile_definitions(NFDelay42 PUBLIC NF_LICENSE_ENFORCE=1)
endif()
target_link_libraries(NFDelay42 PRIVATE NFDelay42Assets juce::juce_audio_utils juce::juce_dsp juce::juce_cryptography PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# DSP unit tests (plain C++, no JUCE)
add_executable(NFDelay42DspTests Tests/DelayEngineTests.cpp)
if(MSVC)
 target_compile_options(NFDelay42DspTests PRIVATE /W4)
else()
 target_compile_options(NFDelay42DspTests PRIVATE -Wall -Wextra -Wpedantic)
endif()
enable_testing()
add_test(NAME NFDelay42DspTests COMMAND NFDelay42DspTests)

# Developer tool: renders the editor to a PNG (never shipped). cmake -DNFDelay42_BUILD_SNAPSHOT=ON
option(NFDelay42_BUILD_SNAPSHOT "Build the UI snapshot tool" OFF)
if(NFDelay42_BUILD_SNAPSHOT)
 juce_add_console_app(NFDelay42Snapshot PRODUCT_NAME "NFDelay42Snapshot")
 juce_generate_juce_header(NFDelay42Snapshot)
 target_sources(NFDelay42Snapshot PRIVATE Tests/UiSnapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/PresetManager.cpp Source/ManualManager.cpp)
 target_compile_definitions(NFDelay42Snapshot PRIVATE JucePlugin_Name="NF D-42" JucePlugin_VersionString="1.0.1" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JucePlugin_Build_Standalone=0 JUCE_MODAL_LOOPS_PERMITTED=1)
 target_link_libraries(NFDelay42Snapshot PRIVATE NFDelay42Assets juce::juce_audio_utils juce::juce_dsp juce::juce_cryptography juce::juce_gui_extra juce::juce_recommended_config_flags)
 juce_add_console_app(NFDelay42Interaction PRODUCT_NAME "NFDelay42Interaction")
 juce_generate_juce_header(NFDelay42Interaction)
 target_sources(NFDelay42Interaction PRIVATE Tests/UiInteraction.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/PresetManager.cpp Source/ManualManager.cpp)
 target_compile_definitions(NFDelay42Interaction PRIVATE JucePlugin_Name="NF D-42" JucePlugin_VersionString="1.0.1" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JucePlugin_Build_Standalone=0 JUCE_MODAL_LOOPS_PERMITTED=1)
 target_link_libraries(NFDelay42Interaction PRIVATE NFDelay42Assets juce::juce_audio_utils juce::juce_dsp juce::juce_cryptography juce::juce_gui_extra juce::juce_recommended_config_flags)
 juce_add_console_app(NFDelay42HostLoad PRODUCT_NAME "NFDelay42HostLoad")
 juce_generate_juce_header(NFDelay42HostLoad)
 target_sources(NFDelay42HostLoad PRIVATE Tests/HostLoad.cpp)
 target_compile_definitions(NFDelay42HostLoad PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
 target_link_libraries(NFDelay42HostLoad PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra juce::juce_recommended_config_flags)
endif()

# "Check for updates" regression tests (version comparison, download-URL allow-list). No network.
add_executable(NFDelay42UpdateTests Tests/UpdateCheckTests.cpp)
target_compile_definitions(NFDelay42UpdateTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(NFDelay42UpdateTests PRIVATE juce::juce_core juce::juce_events juce::juce_recommended_config_flags)
add_test(NAME NFDelay42UpdateTests COMMAND NFDelay42UpdateTests)
