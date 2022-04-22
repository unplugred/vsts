/*
  ==============================================================================

	envelopefollower.cpp
	Created: 5 Feb 2022 1:01:20pm
	Author: unplugred 

  ==============================================================================
*/
//adapted from github.com/arkogelul/pizmidi
#include "envelopefollower.h"

double EnvelopeFollower::process(double in) {
	double s = fmax(abs(in)-threshold,0.)*m;
	if(in > envelope) envelope = attack *(envelope-s)+s;
	else              envelope = release*(envelope-s)+s;
	if(envelope <= .00001) envelope = 0;
	if(envelope > 1.) envelope = 1;
	return envelope;
}

void EnvelopeFollower::setattack(double atk, int samplerate) {
	attackms = atk;
	attack  = pow(.01,1./(attackms*m*samplerate*.001));
}
void EnvelopeFollower::setrelease(double rls, int samplerate) {
	releasems = rls;
	release  = pow(.01,1./(releasems*m*samplerate*.001));
}
void EnvelopeFollower::setthreshold(double thrsh, int samplerate) {
	threshold = thrsh;
	m = 1./fmin(1.001-threshold,1.);
	attack  = pow(.01,1./(attackms*m*samplerate*.001));
	release  = pow(.01,1./(releasems*m*samplerate*.001));
}
