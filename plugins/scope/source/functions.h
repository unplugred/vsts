#pragma once
#include "includes.h"
#include <vector>

class functions {
public:
	static double smoothdamp(double current, double target, double* currentvelocity, double smoothtime, double maxspeed, int samplerate);

	struct dampendvalue {
	public:
		double v_current;
		double v_velocity;
		double v_smoothtime = 1;
		double v_maxspeed = -1;
		int v_samplerate = 44100;
		void reset(double current, double smoothtime, double maxspeed, int samplerate);
		double nextvalue(double target);
	};
};
