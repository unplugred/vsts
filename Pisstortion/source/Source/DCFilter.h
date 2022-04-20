/*
  ==============================================================================

	DCFilter.h
	Created: 4 Feb 2022 9:10:54am
	Author:	 thevoid stared back

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>

class DCFilter {
public:
	void init(int samplerate, int channels);
	double process(double in, int channel);
private:
	double R = .99714285714285714285714285714286;
	std::vector<double> previn = {0.f,0.f};
	std::vector<double> prevout = {0.f,0.f};
};
