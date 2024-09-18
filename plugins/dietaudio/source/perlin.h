#pragma once
#include "includes.h"

class perlin {
public:
	void init();
	float noise(float xin);
	float noise(float xin, float yin);
	float noise(float xin, float yin, float zin);
private:
	uint8_t perm[256];
	float grad(int32_t hash, float xin);
	float grad(int32_t hash, float xin, float yin);
	float grad(int32_t hash, float xin, float yin, float zin);
	uint8_t hash(int32_t i);
};
