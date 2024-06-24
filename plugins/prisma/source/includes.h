#pragma once
#include <plugmachine/plugmachine.h>
#include <juce_dsp/juce_dsp.h>
#include "BinaryData.h"
using namespace juce;

#define MODULE_COUNT 21
#define MIN_MOD 1
#define MAX_MOD 8
#define DEF_MOD 4

#ifdef PRISMON
#define BAND_COUNT 1
#else
#define BAND_COUNT 4
#endif
