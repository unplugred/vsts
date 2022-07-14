#include "functions.h"

double functions::smoothdamp(double current, double target, double* currentvelocity, double smoothtime, double maxspeed, int samplerate) {
	//if(abs(*currentvelocity) < 0.0000001) *currentvelocity = 0;
	if(abs(current) < 0.0000001 && target == 0) current = 0;
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
	if(abs(*currentvelocity) < 0.0000001) num6 = target;
	return num6;
}

double functions::inertiadamp(double current, double target, double* currentvelocity, double smoothtime, double maxspeed, int samplerate) {
	float differential = target-current;
	*currentvelocity = (*currentvelocity)*smoothtime+differential*(1-smoothtime);
	return current+(*currentvelocity);
}

void functions::dampendvalue::reset(double current, double smoothtime, double maxspeed, int samplerate, int channelcount) {
	v_current.resize(channelcount);
	v_velocity.resize(channelcount);
	for(int i = 0; i < channelcount; i++) {
		v_current[i] = current;
		v_velocity[i] = 0;
	}
	v_smoothtime = smoothtime;
	v_maxspeed = maxspeed;
	v_samplerate = samplerate;
}
double functions::dampendvalue::nextvalue(double target, int channel) {
	v_current[channel] = functions::smoothdamp(v_current[channel], target, &v_velocity[channel], v_smoothtime, v_maxspeed, v_samplerate);
	return v_current[channel];
}

void functions::inertiadampened::reset(double current, double smoothtime, double maxspeed, int samplerate, int channelcount) {
	v_current.resize(channelcount);
	v_velocity.resize(channelcount);
	for(int i = 0; i < channelcount; i++) {
		v_current[i] = current;
		v_velocity[i] = 0;
	}
	v_smoothtime = smoothtime;
	v_maxspeed = maxspeed;
	v_samplerate = samplerate;
}
double functions::inertiadampened::nextvalue(double target, int channel) {
	v_current[channel] = functions::inertiadamp(v_current[channel], target, &v_velocity[channel], v_smoothtime, v_maxspeed, v_samplerate);
	return v_current[channel];
}
