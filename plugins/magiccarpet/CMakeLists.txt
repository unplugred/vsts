project(MagicCarpet VERSION 1.0.0)
set(BaseTargetName MagicCarpet)

juce_add_plugin("${BaseTargetName}"
	PRODUCT_NAME "Magic Carpet"
	COMPANY_NAME "unplugred"
	COMPANY_WEBSITE "https://vst.unplug.red"
	COMPANY_EMAIL "melody@unplug.red"
	FORMATS Standalone VST3 AU
	IS_SYNTH FALSE
	NEEDS_MIDI_INPUT FALSE
	NEEDS_MIDI_OUTPUT FALSE
	IS_MIDI_EFFECT FALSE
	EDITOR_WANTS_KEYBOARD_FOCUS FALSE
	DESCRIPTION "Three delays feedbacking into the atmosphere."
	PLUGIN_MANUFACTURER_CODE Ured
	PLUGIN_CODE Mgct
	VST3_CATEGORIES Delay
	VST2_CATEGORY kPlugCategEffect
	AU_MAIN_TYPE kAudioUnitType_Effect
	COPY_PLUGIN_AFTER_BUILD TRUE
	VST3_COPY_DIR "${outdir}"
	VST_COPY_DIR "${outdir}"
	AU_COPY_DIR "${outdir}")

target_compile_features("${BaseTargetName}" PRIVATE cxx_std_17)

clap_juce_extensions_plugin(TARGET "${BaseTargetName}"
	CLAP_ID "com.unplugred.${BaseTargetName}"
	CLAP_FEATURES audio-effect delay mono stereo surround ambisonic
	CLAP_USE_JUCE_PARAMETER_RANGES ALL)

target_sources("${BaseTargetName}" PRIVATE
	source/dc_filter.cpp
	source/functions.cpp
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
	JUCE_USE_CAMERA=0)

juce_add_binary_data("${BaseTargetName}BinaryData" HEADER_NAME BinaryData.h NAMESPACE BinaryData SOURCES
	resources/text.png
	resources/noise.png
	resources/logo.png
	resources/carpet.png)
set_target_properties("${BaseTargetName}BinaryData" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries("${BaseTargetName}" PRIVATE
	plugmachine
	"${BaseTargetName}BinaryData"
	PUBLIC
	juce::juce_recommended_lto_flags
	juce::juce_recommended_config_flags)