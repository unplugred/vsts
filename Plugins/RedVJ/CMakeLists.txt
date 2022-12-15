project(RedVJ VERSION 1.0.0)
set(BaseTargetName RedVJ)

juce_add_plugin("${BaseTargetName}"
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
	VST_COPY_DIR "${outdir}"
	VST3_COPY_DIR "${outdir}")

target_compile_features("${BaseTargetName}" PRIVATE cxx_std_17)

clap_juce_extensions_plugin(TARGET "${BaseTargetName}"
	CLAP_ID "com.unplugred.${BaseTargetName}"
	CLAP_FEATURES analyzer utility mono stereo surround ambisonic
	CLAP_USE_JUCE_PARAMETER_RANGES ALL)

target_sources("${BaseTargetName}" PRIVATE
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

target_compile_definitions("${BaseTargetName}" PUBLIC
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

juce_add_binary_data("${BaseTargetName}BinaryData" HEADER_NAME BinaryData.h NAMESPACE BinaryData SOURCES
	Source/txt.png)
set_target_properties("${BaseTargetName}BinaryData" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries("${BaseTargetName}" PRIVATE
	juce::juce_audio_devices
	juce::juce_audio_plugin_client
	juce::juce_audio_utils
	juce::juce_dsp
	juce::juce_opengl
	juce::juce_video
	"${BaseTargetName}BinaryData"
	PUBLIC
	juce::juce_recommended_config_flags
	juce::juce_recommended_lto_flags
	juce::juce_recommended_warning_flags)