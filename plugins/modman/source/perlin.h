#pragma once
#include "includes.h"

class perlin {
public:
	double noise(double xin, double yin);
	void init();
private:
	double dot(double g[], double x, double y);
	double grad3[12][3] {{1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},{1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},{0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}};
	int preperm[512];
	int perm[512];
	Random random;
};
