/*
  ==============================================================================

	envelopefollower.h
	Created: 5 Feb 2022 1:01:20pm
	Author:	 thevoid stared back

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class EnvelopeFollower {
public:
	float envelope = 0;

	float process(float in);
	void setattack(float atk, float samplerate);
	void setrelease(float rls, float samplerate);
	void setthreshold(float thrsh, float samplerate);
private:
	float attack = .9896117715f;
	float attackms = 50;
	float release = .9997911706f;
	float releasems = 100;
	float m = 1;
	float threshold = 0;
};
