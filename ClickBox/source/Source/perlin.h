#pragma once
#include "includes.h"

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
