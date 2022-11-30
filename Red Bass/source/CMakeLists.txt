#[[
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
]]

cmake_minimum_required(VERSION 3.15)
project(RedBass VERSION 1.0.0)

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
	JUCE
	GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
	GIT_TAG 7.0.2
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(JUCE)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_BUILD_EXTRAS "Build extras" OFF)
option(JUCE_BUILD_EXAMPLES "Build examples" OFF)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Enable module source groups" ON)
option(JUCE_COPY_PLUGIN_AFTER_BUILD "Copy plugin after build" ON)

if(MSVC)
	juce_set_vst2_sdk_path(C:/vstsdk2.4)
	set(vstdir "C:/docs/MEGA/vsts/64")
	set(vstthreedir "C:/Program Files/Common Files/VST3")
endif(MSVC)
if(UNIX AND NOT APPLE)
	juce_set_vst2_sdk_path(/home/unplugred/projects/JUUUCE/vstsdk2.4)
	set(vstdir "/home/unplugred/MEGA/vsts/linux/vst2")
	set(vstthreedir "/home/unplugred/MEGA/vsts/linux/vst3")
endif()

juce_add_plugin("${PROJECT_NAME}"
	PRODUCT_NAME "Red Bass"
	COMPANY_NAME "unplugred"
	COMPANY_WEBSITE "https://vst.unplug.red"
	COMPANY_EMAIL "arihanan@proton.me"
	FORMATS VST Standalone VST3
	IS_SYNTH FALSE
	NEEDS_MIDI_INPUT FALSE
	NEEDS_MIDI_OUTPUT FALSE
	IS_MIDI_EFFECT FALSE
	EDITOR_WANTS_KEYBOARD_FOCUS FALSE
	DESCRIPTION "Low-end enchancer excellent for kicks."
	PLUGIN_MANUFACTURER_CODE Ured
	PLUGIN_CODE Rdbs
	VST3_CATEGORY Fx
	VST2_CATEGORY kPlugCategEffect
	COPY_PLUGIN_AFTER_BUILD TRUE
	VST_COPY_DIR "${vstdir}"
	VST3_COPY_DIR "${vstthreedir}")

target_sources("${PROJECT_NAME}" PRIVATE
	Source/CoolLogger.cpp
	Source/envelopefollower.cpp
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
	Resources/txt.png)
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