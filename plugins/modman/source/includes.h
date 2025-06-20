#pragma once
#include <plugmachine/plugmachine.h>
#include <juce_dsp/juce_dsp.h>
#include "BinaryData.h"
using namespace juce;

#define MC 5
#define M1 "drift"
#define M2 "low pass"
#define M3 "low pass resonance"
#define M4 "saturation"
#define M5 "amplitude"

#define MAX_DRIFT 0.1
