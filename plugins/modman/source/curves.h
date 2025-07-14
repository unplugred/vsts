#pragma once
#include "includes.h"

struct point {
	point(float px, float py, float ptension = .5f) {
		x = px;
		y = py;
		tension = ptension;
	}
	float x = 0;
	float y = 0;
	float tension = .5f;
	bool enabled = true;
};
struct curve {
	curve() { }
	curve(String str, const char delimiter = ',');
	String tostring(const char delimiter = ',');
	std::vector<point> points;
	static double calctension(double interp, double tension);
	static bool isvalidcurvestring(String str, const char delimiter = ',');
	double process(double input, int channel);
	void resizechannels(int channelnum);
	std::vector<int> nextpoint;
	std::vector<int> currentpoint;
};
class curveiterator {
public:
	curveiterator() { }
	void reset(curve inputcurve, int wwidth);
	double next();
	bool pointhit = true;
	int width = 284;
	int x = 99999;
private:
	std::vector<point> points;
	int nextpoint = 1;
	int currentpoint = 0;
};
