#pragma once
#include <vector>

class dc_filter {
public:
	void init(int samplerate, int channels);
	double process(double in, int channel);
	void reset(int channel);
private:
	double R = 1-(31.5/88200.0);
	std::vector<double> previn = {0.f,0.f};
	std::vector<double> prevout = {0.f,0.f};
};
