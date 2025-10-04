#pragma once
#include "includes.h"
#include <vector>

class functions {
public:
	static double smoothdamp(double current, double target, double* currentvelocity, double smoothtime, double maxspeed, int samplerate);

	struct dampendvalue {
	public:
		std::vector<double> v_current;
		std::vector<double> v_velocity;
		double v_smoothtime = 1;
		double v_maxspeed = -1;
		int v_samplerate = 44100;
		int v_channelcount = 2;
		void reset(double current, double smoothtime, double maxspeed, int samplerate, int channelcount);
		double nextvalue(double target, int channel = 0);
	};
};
