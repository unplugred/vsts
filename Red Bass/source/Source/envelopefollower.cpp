/*
  ==============================================================================

	envelopefollower.cpp
	Created: 5 Feb 2022 1:01:20pm
	Author: unplugred 

  ==============================================================================
*/
//adapted from github.com/arkogelul/pizmidi
#include "envelopefollower.h"

float EnvelopeFollower::process(float in) {
	float s = fmax(fabs(in)-threshold,0.f)*m;
	if(in > envelope) envelope = attack *(envelope-s)+s;
	else              envelope = release*(envelope-s)+s;
	if (envelope <= .00001f) envelope = 0;
	if (envelope > 1.f) envelope = 1;
	return envelope;
}

void EnvelopeFollower::setattack(float atk, float samplerate) {
	attackms = atk;
	attack  = powf(.01f,1.f/(attackms*m*samplerate*.001f));
}
void EnvelopeFollower::setrelease(float rls, float samplerate) {
	releasems = rls;
	release  = powf(.01f,1.f/(releasems*m*samplerate*.001f));
}
void EnvelopeFollower::setthreshold(float thrsh, float samplerate) {
	threshold = thrsh;
	m = 1.f/fmin(1.001f-threshold,1.f);
	attack  = powf(.01f,1.f/(attackms*m*samplerate*.001f));
	release  = powf(.01f,1.f/(releasems*m*samplerate*.001f));
}
