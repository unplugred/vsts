#[[
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
]]

cmake_minimum_required(VERSION 3.21)
project(RedVJ VERSION 1.0.0)

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
	BUNDLE_ID com.unplugred.redvj
	COMPANY_NAME "unplugred"
	COMPANY_WEBSITE "https://vst.unplug.red"
	COMPANY_EMAIL "arihanan@proton.me"
	FORMATS Standalone VST3
	IS_SYNTH FALSE
	NEEDS_MIDI_INPUT TRUE
	NEEDS_MIDI_OUTPUT FALSE
	IS_MIDI_EFFECT FALSE
	EDITOR_WANTS_KEYBOARD_FOCUS TRUE
	DESCRIPTION "An advanced VJing software programmable in GLSL and Lua."
	PLUGIN_MANUFACTURER_CODE Ured
	PLUGIN_CODE Rdvj
	VST3_CATEGORY External
	VST2_CATEGORY kPlugCategEffect
	COPY_PLUGIN_AFTER_BUILD TRUE
	VST_COPY_DIR "${vstdir}"
	VST3_COPY_DIR "${vstthreedir}")

target_sources("${PROJECT_NAME}" PRIVATE
	Source/lua-5.4.3/lapi.c
	Source/lua-5.4.3/lauxlib.c
	Source/lua-5.4.3/lbaselib.c
	Source/lua-5.4.3/lcode.c
	Source/lua-5.4.3/lcorolib.c
	Source/lua-5.4.3/lctype.c
	Source/lua-5.4.3/ldblib.c
	Source/lua-5.4.3/ldebug.c
	Source/lua-5.4.3/ldo.c
	Source/lua-5.4.3/ldump.c
	Source/lua-5.4.3/lfunc.c
	Source/lua-5.4.3/lgc.c
	Source/lua-5.4.3/linit.c
	Source/lua-5.4.3/liolib.c
	Source/lua-5.4.3/llex.c
	Source/lua-5.4.3/lmathlib.c
	Source/lua-5.4.3/lmem.c
	Source/lua-5.4.3/loadlib.c
	Source/lua-5.4.3/lobject.c
	Source/lua-5.4.3/lopcodes.c
	Source/lua-5.4.3/loslib.c
	Source/lua-5.4.3/lparser.c
	Source/lua-5.4.3/lstate.c
	Source/lua-5.4.3/lstring.c
	Source/lua-5.4.3/lstrlib.c
	Source/lua-5.4.3/ltable.c
	Source/lua-5.4.3/ltablib.c
	Source/lua-5.4.3/ltm.c
	Source/lua-5.4.3/lua.c
	Source/lua-5.4.3/lua.hpp
	Source/lua-5.4.3/luac.c
	Source/lua-5.4.3/lundump.c
	Source/lua-5.4.3/lutf8lib.c
	Source/lua-5.4.3/lvm.c
	Source/lua-5.4.3/lzio.c

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
	JUCE_USE_CAMERA=1)

juce_add_binary_data(BinaryData HEADER_NAME BinaryData.h NAMESPACE BinaryData SOURCES
	Source/txt.png)
set_target_properties(BinaryData PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries("${PROJECT_NAME}" PRIVATE
	juce::juce_audio_devices
	juce::juce_audio_plugin_client
	juce::juce_audio_utils
	juce::juce_dsp
	juce::juce_opengl
	juce::juce_video
	BinaryData
	PUBLIC
	juce::juce_recommended_config_flags
	juce::juce_recommended_lto_flags
	juce::juce_recommended_warning_flags)