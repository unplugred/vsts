#[[
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
]]

cmake_minimum_required(VERSION 3.21)
project(Prisma VERSION 1.0.0)

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
	JUCE
	GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
	GIT_TAG 7.0.3
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(JUCE)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_BUILD_EXTRAS "Build extras" OFF)
option(JUCE_BUILD_EXAMPLES "Build examples" OFF)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Enable module source groups" ON)
option(JUCE_COPY_PLUGIN_AFTER_BUILD "Copy plugin after build" ON)

set(vstdir "../../../Setup/build/free")
set(vstthreedir "../../../Setup/build/free")

juce_add_plugin("${PROJECT_NAME}"
	COMPANY_NAME "unplugred"
	COMPANY_WEBSITE "https://vst.unplug.red"
	COMPANY_EMAIL "arihanan@proton.me"
	FORMATS Standalone VST3
	IS_SYNTH FALSE
	NEEDS_MIDI_INPUT FALSE
	NEEDS_MIDI_OUTPUT FALSE
	IS_MIDI_EFFECT FALSE
	EDITOR_WANTS_KEYBOARD_FOCUS FALSE
	DESCRIPTION "Modular multiband distortion plugin."
	PLUGIN_MANUFACTURER_CODE Ured
	PLUGIN_CODE Prsm
	VST3_CATEGORY Distortion
	VST2_CATEGORY kPlugCategEffect
	COPY_PLUGIN_AFTER_BUILD TRUE
	VST_COPY_DIR "${vstdir}"
	VST3_COPY_DIR "${vstthreedir}")

target_sources("${PROJECT_NAME}" PRIVATE
	Source/CoolLogger.cpp
	Source/DCFilter.cpp
	Source/PluginEditor.cpp
	Source/PluginProcessor.cpp)

target_compile_definitions("${PROJECT_NAME}" PUBLIC
	JUCE_DISPLAY_SPLASH_SCREEN=0
	JUCE_USE_FLAC=0
	JUCE_USE_OGGVORBIS=0
	JUCE_USE_MP3AUDIOFORMAT=0
	JUCE_USE_LAME_AUDIO_FORMAT=0
	JUCE_USE_WINDOWS_MEDIA_FORMAT=0
	JUCE_VST3_CAN_REPLACE_VST2=0
	JUCE_USE_CURL=0
	JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
	JUCE_STRICT_REFCOUNTEDPOINTER=1
	JUCE_WEB_BROWSER=0
	JUCE_USE_WIN_WEBVIEW2=0
	JUCE_USE_CAMERA=0)

juce_add_binary_data(BinaryData HEADER_NAME BinaryData.h NAMESPACE BinaryData SOURCES
	Resources/banner.png
	Resources/base.png
	Resources/dbg_txt.png
	Resources/elements.png
	Resources/modules.png
	Resources/selector.png)
set_target_properties(BinaryData PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries("${PROJECT_NAME}" PRIVATE
	juce::juce_audio_devices
	juce::juce_audio_plugin_client
	juce::juce_audio_utils
	juce::juce_dsp
	juce::juce_opengl
	BinaryData
	PUBLIC
	juce::juce_recommended_config_flags
	juce::juce_recommended_lto_flags
	juce::juce_recommended_warning_flags)