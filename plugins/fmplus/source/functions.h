#pragma once
#include "includes.h"
#include <vector>

struct dampenedvalue {
public:
	std::vector<float> current;
	std::vector<float> velocity;
	int samplerate = 44100;
	float omega = 0;
	float exp = 0;
	void reset(float _smoothtime, int _samplerate, int _channelcount = 1, float _current = 0);
	void setsmoothtime(float _smoothtime);
	float setto(float _target = 0, int _channel = 0);
	void setallto(float _target = 0);
	float nextvalue(float _target, int _channel = 0);
};
struct onepolevalue {
public:
	std::vector<float> current;
	int samplerate = 44100;
	float a = 0;
	float b = 0;
	void reset(float _smoothtime, int _samplerate, int _channelcount = 1, float _current = 0);
	void setsmoothtime(float _smoothtime);
	float setto(float _target = 0, int _channel = 0);
	void setallto(float _target = 0);
	float nextvalue(float _target, int _channel = 0);
};
