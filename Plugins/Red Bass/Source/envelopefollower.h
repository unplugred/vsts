#pragma once
#include "includes.h"

class EnvelopeFollower {
public:
	double envelope = 0;

	double process(double in);
	void setattack(double atk, int samplerate);
	void setrelease(double rls, int samplerate);
	void setthreshold(double thrsh, int samplerate);
private:
	double attack = .9896117715;
	double attackms = 50;
	double release = .9997911706;
	double releasems = 100;
	double m = 1;
	double threshold = 0;
};
