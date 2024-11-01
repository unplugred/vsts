#include "functions.h"

double functions::smoothdamp(double current, double target, double* currentvelocity, double smoothtime, double maxspeed, int samplerate) {
	smoothtime = fmax(.0001, smoothtime);
	double num1 = 2./smoothtime;
	double num2 = num1/((double)samplerate);
	double num3 = 1./(1+num2+.48*num2*num2+.235*num2*num2*num2);
	double num4 = current-target;
	if(maxspeed > 0) num4 = fmax(fmin(num4,maxspeed*smoothtime),-maxspeed*smoothtime);
	double num5 = (*currentvelocity+num1*num4)/((double)samplerate);
	target = current-num4;
	double num6 = target+(num4+num5)*num3;
	if((target-current > 0) == (num6 > target)) {
		num6 = target;
		*currentvelocity = 0;
	} else *currentvelocity = (*currentvelocity-num1*num5)*num3;
	return num6;
}

void functions::dampendvalue::reset(double current, double smoothtime, double maxspeed, int samplerate) {
	v_current = current;
	v_velocity = 0;
	v_smoothtime = smoothtime;
	v_maxspeed = maxspeed;
	v_samplerate = samplerate;
}
double functions::dampendvalue::nextvalue(double target) {
	v_current = functions::smoothdamp(v_current, target, &v_velocity, v_smoothtime, v_maxspeed, v_samplerate);
	return v_current;
}
