#pragma once
#include <plugmachine/plugmachine.h>
#include <juce_dsp/juce_dsp.h>
#include "BinaryData.h"
using namespace juce;

#define MC 7
#define M1 "Drift"
#define M2 "Low Pass"
#define M3 "Low Pass Resonance"
#define M4 "Saturation"
#define M5 "Bitcrush" //?
#define M6 "Flange" //?
#define M7 "Amplitude"

#define MAX_DRIFT 0.1
#define MAX_FLANGE 0.01
