#pragma once
#include <JuceHeader.h>
using namespace juce;

class functions
{
public:
	static float smoothdamp(float current, float target, float* currentvelocity, float smoothtime, float maxspeed, float deltatime);
};
