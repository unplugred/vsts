#pragma once
#include <vector>

class DCFilter {
public:
	void init(int samplerate, int channels);
	double process(double in, int channel);
	void reset(int channel);
private:
	double R = .99714285714285714285714285714286;
	std::vector<double> previn = {0.f,0.f};
	std::vector<double> prevout = {0.f,0.f};
};
