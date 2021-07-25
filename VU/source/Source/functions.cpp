/*
  ==============================================================================

    functions.cpp
    Created: 8 Jul 2021 4:43:14pm
    Author:  thevoid stared back

  ==============================================================================
*/

#include "functions.h"

float functions::smoothdamp(float current, float target, float* currentvelocity, float smoothtime, float maxspeed, float deltatime)
{
	smoothtime = fmax(.0001f, smoothtime);
	float num1 = 2.f/smoothtime;
	float num2 = num1*deltatime;
	float num3 = 1.f/(1+num2+.48f*num2*num2+.235f*num2*num2*num2);
	float num4 = current-target;
	if(maxspeed > 0) num4 = fmax(fmin(num4,maxspeed*smoothtime),-maxspeed*smoothtime);
	float num5 = (*currentvelocity+num1*num4)*deltatime;
	target = current-num4;
	float num6 = target+(num4+num5)*num3;
	if ((target-current > 0) == (num6 > target)) {
		num6 = target;
		*currentvelocity = 0;
	} else *currentvelocity = (*currentvelocity-num1*num5)*num3;
	return num6;
}
