#pragma once
#include "includes.h"

class EnvelopeFollower {
public:
	std::vector<double> envelope;

	double process(double in, int channel);
	void setattack(double atk);
	void setrelease(double rls);
	void setsamplerate(int smplrt);
	void setchannelcount(int channelcount);
	double release = .9997911706;
	double releasems = 100;
	int samplerate = 44100;
private:
	double attack = .9896117715;
	double attackms = 50;
};
