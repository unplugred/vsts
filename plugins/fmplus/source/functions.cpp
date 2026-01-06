#include "functions.h"

void dampenedvalue::reset(float _smoothtime, int _samplerate, int _channelcount, float _current) {
	samplerate = _samplerate;

	setsmoothtime(_smoothtime);
	current.resize(_channelcount);
	velocity.resize(_channelcount);
	setallto(_current);
}
void dampenedvalue::setsmoothtime(float _smoothtime) {
	omega = 2.f/fmax(.0001f,_smoothtime);
	float x = omega/samplerate;
	exp = 1.f/(1+x+.48f*x*x+.235f*x*x*x);
}
float dampenedvalue::setto(float _target, int _channel) {
	current[_channel] = _target;
	velocity[_channel] = 0;
	return _target;
}
void dampenedvalue::setallto(float _target) {
	for(int c = 0; c < current.size(); ++c) {
		current[c] = _target;
		velocity[c] = 0;
	}
}
float dampenedvalue::nextvalue(float _target, int _channel) {
	float delta = current[_channel]-_target;
	float temp = (velocity[_channel]+omega*delta)/samplerate;
	current[_channel] = _target+(delta+temp)*exp;

	if((delta>0) == (current[_channel]>_target)) {
		velocity[_channel] = (velocity[_channel]-omega*temp)*exp;
	} else {
		current[_channel] = _target;
		velocity[_channel] = 0;
	}

	return current[_channel];
}

void onepolevalue::reset(float _smoothtime, int _samplerate, int _channelcount, float _current) {
	samplerate = _samplerate;

	setsmoothtime(_smoothtime);
	current.resize(_channelcount);
	setallto(_current);
}
void onepolevalue::setsmoothtime(float _smoothtime) {
	const float twopi = 6.283185307179586476925286766559f;
	a = exp(-twopi/(_smoothtime*samplerate));
	b = 1-a;
}
float onepolevalue::setto(float _target, int _channel) {
	current[_channel] = _target;
	return _target;
}
void onepolevalue::setallto(float _target) {
	for(int c = 0; c < current.size(); ++c)
		current[c] = _target;
}
float onepolevalue::nextvalue(float _target, int _channel) {
	current[_channel] = (_target*b)+(current[_channel]*a);
	return current[_channel];
}
