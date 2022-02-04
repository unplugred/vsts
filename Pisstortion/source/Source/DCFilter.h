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
	float process(float in, int channel);
private:
	float R = .99714285714285714285714285714286f;
	std::vector<float> previn = {0.f,0.f};
	std::vector<float> prevout = {0.f,0.f};
};
