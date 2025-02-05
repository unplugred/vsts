//adapted from https://musicdsp.org/en/latest/Filters/135-dc-filter.html
#include "dc_filter.h"

void dc_filter::init(int samplerate, int channels) {
	R = 1-(31.5/(double)samplerate); //CUTOFF 5Hz

	if(prevout.size() != channels || previn.size() != channels) {
		previn.clear();
		prevout.clear();
		for (int i = 0; i < channels; i++) {
			previn.push_back(0.f);
			prevout.push_back(0.f);
		}
	}
}

double dc_filter::process(double in, int channel) {
	//if(channel > (previn.size()-1)) DBG("HEYYYYYYYY");
	double out = in-previn[channel]+prevout[channel]*R;
	previn[channel] = in;
	prevout[channel] = out;
	return out;
}

void dc_filter::reset(int channel) {
	previn[channel] = 0;
	prevout[channel] = 0;
}
