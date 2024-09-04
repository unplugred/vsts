//adapted from github.com/arkogelul/pizmidi
#include "envelopefollower.h"

double EnvelopeFollower::process(double in, int channel) {
	if(in > envelope[channel]) envelope[channel] = attack *(envelope[channel]-in)+in;
	else                       envelope[channel] = release*(envelope[channel]-in)+in;
	if(envelope[channel] <= .00001) envelope[channel] = 0;
	if(envelope[channel] > 1.     ) envelope[channel] = 1;
	return envelope[channel];
}

void EnvelopeFollower::setattack (double atk) {
	attackms = atk;
	if( attackms <= 0) attack  = 0;
	else attack  = pow(.01,1./( attackms*samplerate*.0027));
}
void EnvelopeFollower::setrelease(double rls) {
	releasems = rls;
	if(releasems <= 0) release = 0;
	else release = pow(.01,1./(releasems*samplerate*.0027));
}
void EnvelopeFollower::setsamplerate(int smplrt) {
	samplerate = smplrt;
	setattack ( attackms);
	setrelease(releasems);
}
void EnvelopeFollower::setchannelcount(int channelcount) {
	envelope.resize(channelcount);
	for(int i = 0; i < channelcount; ++i)
		envelope[i] = 0;
}
