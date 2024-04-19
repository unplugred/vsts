#include "PluginProcessor.h"

createimpulse::createimpulse() : Thread("createimpulsethread") {}
createimpulse::~createimpulse() {}
void createimpulse::run() {
	active = true;

	if(channelnum > 2) {
		for(int c = 0; c < channelnum; ++c) {
			impulsebuffer[c] = AudioBuffer<float>(1,taillength);
			impulseeffectbuffer[c] = AudioBuffer<float>(1,taillength);
			impulsechanneldata[c] = impulsebuffer[c].getWritePointer(0);
			impulseeffectchanneldata[c] = impulseeffectbuffer[c].getWritePointer(0);
		}
	} else {
		impulsebuffer[0] = AudioBuffer<float>(channelnum,taillength);
		impulseeffectbuffer[0] = AudioBuffer<float>(channelnum,taillength);
		for(int c = 0; c < channelnum; ++c) {
			impulsechanneldata[c] = impulsebuffer[0].getWritePointer(c);
			impulseeffectchanneldata[c] = impulseeffectbuffer[0].getWritePointer(c);
		}
	}

	double out = 0;
	double agc = 0;
	highpassfilter.reset();
	lowpassfilter.reset();

	Random random(seed);
	double values[8];
	for (int s = 0; s < taillength; ++s) {
		valuesraw[0] = iterator[0].next();
		for(int i = 1; i < 5; ++i)
			valuesraw[enabledcurves[i-1]] = iterator[i].next();

		values[0] = valuesraw[0]*(random.nextFloat()>.5?1:-1);
		values[1] = mapToLog10(valuesraw[1],20.0,20000.0);
		values[2] = mapToLog10(valuesraw[2],0.1,40.0);
		values[3] = mapToLog10(valuesraw[3],20.0,20000.0);
		values[4] = mapToLog10(valuesraw[4],0.1,40.0);
		values[5] = valuesraw[5];
		values[6] = pow(valuesraw[6],4);
		values[7] = valuesraw[7];

		highpassfilter.setCutoffFrequency(values[1]);
		highpassfilter.setResonance(values[2]);
		lowpassfilter.setCutoffFrequency(values[3]);
		lowpassfilter.setResonance(values[4]);

		for (int c = 0; c < channelnum; ++c) {

			//vol
			if(s >= revlength) {
				out = 0;
			} else if(iterator[0].pointhit) {
				out = values[0];
			} else if(random.nextFloat() < values[6]) {
				float r = random.nextFloat();
				agc += r*r;
				out = values[0]*r;
			} else {
				out = 0;
			}

			//pan
			if(valuesactive[5] && channelnum > 1) {
				double linearpan = (((double)c)/(channelnum-1))*(1-values[5]*2)+values[5];
				out *= sin(linearpan*1.5707963268)*.7071067812+linearpan;
			}

			//low pass
			if(valuesactive[3]) {
				if(values[3] < 20000)
					out = lowpassfilter.processSample(c, out);
				else
					lowpassfilter.processSample(c, out);
			}

			//high pass
			if(valuesactive[1]) {
				if(values[1] > 20)
					out = highpassfilter.processSample(c, out);
				else
					highpassfilter.processSample(c, out);
			}

			if(!active.get()) return;
			impulsechanneldata[c][s] = out*(1-values[7]);
			impulseeffectchanneldata[c][s] = out*values[7];
		}
		iterator[0].pointhit = false;
	}
	agc = 1/fmax(sqrt(agc/channelnum),1);
	for (int c = 0; c < channelnum; ++c) {
		for (int s = 0; s < taillength; ++s) {
			if(!active.get()) return;
			impulsechanneldata[c][s] *= agc;
			impulseeffectchanneldata[c][s] *= agc;
		}
	}

	done = true;
	active = false;
	generated = true;
}
