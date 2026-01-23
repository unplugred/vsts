#pragma once
#include <plugmachine/plugmachine.h>
#include <tuning-library/tuning-library.h>
//#include <juce_audio_basics/juce_audio_basics.h>
//#include <juce_audio_devices/juce_audio_devices.h>
//#include <juce_audio_formats/juce_audio_formats.h>
//#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
//#include <juce_audio_processors/juce_audio_processors.h>
//#include <juce_audio_utils/juce_audio_utils.h>
//#include <juce_core/juce_core.h>
//#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
//#include <juce_events/juce_events.h>
#include "BinaryData.h"
using namespace juce;

#define MC 8
#define VC 24
#define MINA 0
#define MAXA 8
#define MIND .005f
#define MAXD 8
#define MINR .005f
#define MAXR 8
#define MAXGLIDE 2
#define MINARP .01f
#define MAXARP 1
#define MINVIB .05f
#define MAXVIB 4
#define MAXVIBATT 2
#define MAXLFO 24
#define MAXLFOATT 4
