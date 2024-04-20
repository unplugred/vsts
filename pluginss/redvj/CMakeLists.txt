project(RedVJ VERSION 1.0.0)
set(BaseTargetName RedVJ)

juce_add_plugin("${BaseTargetName}"
	COMPANY_NAME "unplugred"
	COMPANY_WEBSITE "https://vst.unplug.red"
	COMPANY_EMAIL "arihanan@proton.me"
	FORMATS Standalone VST3 AU
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
	AU_MAIN_TYPE kAudioUnitType_Effect
	COPY_PLUGIN_AFTER_BUILD TRUE
	VST3_COPY_DIR "${outdir}"
	VST_COPY_DIR "${outdir}"
	AU_COPY_DIR "${outdir}")

target_compile_features("${BaseTargetName}" PRIVATE cxx_std_17)

clap_juce_extensions_plugin(TARGET "${BaseTargetName}"
	CLAP_ID "com.unplugred.${BaseTargetName}"
	CLAP_FEATURES analyzer utility mono stereo surround ambisonic
	CLAP_USE_JUCE_PARAMETER_RANGES ALL)

target_sources("${BaseTargetName}" PRIVATE
	source/lua-5.4.3/lapi.c
	source/lua-5.4.3/lauxlib.c
	source/lua-5.4.3/lbaselib.c
	source/lua-5.4.3/lcode.c
	source/lua-5.4.3/lcorolib.c
	source/lua-5.4.3/lctype.c
	source/lua-5.4.3/ldblib.c
	source/lua-5.4.3/ldebug.c
	source/lua-5.4.3/ldo.c
	source/lua-5.4.3/ldump.c
	source/lua-5.4.3/lfunc.c
	source/lua-5.4.3/lgc.c
	source/lua-5.4.3/linit.c
	source/lua-5.4.3/liolib.c
	source/lua-5.4.3/llex.c
	source/lua-5.4.3/lmathlib.c
	source/lua-5.4.3/lmem.c
	source/lua-5.4.3/loadlib.c
	source/lua-5.4.3/lobject.c
	source/lua-5.4.3/lopcodes.c
	source/lua-5.4.3/loslib.c
	source/lua-5.4.3/lparser.c
	source/lua-5.4.3/lstate.c
	source/lua-5.4.3/lstring.c
	source/lua-5.4.3/lstrlib.c
	source/lua-5.4.3/ltable.c
	source/lua-5.4.3/ltablib.c
	source/lua-5.4.3/ltm.c
	source/lua-5.4.3/lua.c
	source/lua-5.4.3/lua.hpp
	source/lua-5.4.3/luac.c
	source/lua-5.4.3/lundump.c
	source/lua-5.4.3/lutf8lib.c
	source/lua-5.4.3/lvm.c
	source/lua-5.4.3/lzio.c

	source/editor.cpp
	source/processor.cpp)

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
	source/txt.png)
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