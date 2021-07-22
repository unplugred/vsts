/*
  ==============================================================================

    functions.h
    Created: 8 Jul 2021 4:43:14pm
    Author:  thevoid stared back

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;

class functions
{
public:
	static float smoothdamp(float current, float target, float* currentvelocity, float smoothtime, float maxspeed, float deltatime);
};
