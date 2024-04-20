#pragma once
#include "includes.h"

class functions
{
public:
	static float smoothdamp(float current, float target, float* currentvelocity, float smoothtime, float maxspeed, float deltatime);
};
