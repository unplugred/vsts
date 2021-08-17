/*
  ==============================================================================

    perlin.h
    Created: 16 Aug 2021 2:58:15pm
    Author:  thevoid stared back

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;

class perlin {
public:
	float noise(float xin, float yin);
	void init();
private:
	float dot(float g[], float x, float y);
	float grad3[12][3] {{1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},{1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},{0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}};
	int preperm[512];
	int perm[512];
};
