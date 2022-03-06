/* ==============================================================================

	DCFilter.cpp
	Created: 4 Feb 2022 9:10:54am
	Author:	 red

  ==============================================================================
*/
//adapted from https://musicdsp.org/en/latest/Filters/135-dc-filter.html
#include "DCFilter.h"

void DCFilter::init(int samplerate, int channels) {
	R = 1-(126.f/samplerate);

	if(prevout.size() != channels || previn.size() != channels) {
		previn.clear();
		prevout.clear();
		for (int i = 0; i < channels; i++) {
			previn.push_back(0.f);
			prevout.push_back(0.f);
		}
	}
}

float DCFilter::process(float in, int channel) {
	if(channel > (previn.size()-1)) DBG("HEYYYYYYYY");
	float out = in-previn[channel]+prevout[channel]*R;
	previn[channel] = in;
	prevout[channel] = out;
	return out;
}