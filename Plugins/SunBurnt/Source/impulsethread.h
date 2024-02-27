#pragma once
#include "includes.h"
#include "curves.h"

class createimpulse : public Thread {
public:
	createimpulse();
	~createimpulse();

	int samplerate = 44100;
	int channelnum = 0;
	Atomic<bool> done = false;
	Atomic<bool> active = false;
	int revlength = 0;
	int taillength = 0;
	std::vector<dsp::StateVariableFilter::Filter<float>> highpassfilters;
	std::vector<dsp::StateVariableFilter::Filter<float>> lowpassfilters;
	std::vector<AudioBuffer<float>> impulsebuffer;
	std::vector<AudioBuffer<float>> impulseeffectbuffer;
	std::vector<float*> impulsechanneldata;
	std::vector<float*> impulseeffectchanneldata;
	curveiterator iterator[5];
	double valuesraw[8];
	bool valuesactive[8];
	int64 seed;
	int enabledcurves[4];

	void run() override;
};
