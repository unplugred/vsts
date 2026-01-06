#pragma once
#include "includes.h"

struct point {
	point() {}
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
	curve() {}
	curve(String str, const char delimiter = ',', int channelnum = 0);
	void resizechannels(int channelnum);
	String tostring(const char delimiter = ',');
	static bool isvalidcurvestring(String str, const char delimiter = ',');
	static float calctension(float interp, float tension);
	float process(float input, int channel);

	std::vector<point> points;
	std::vector<int> nextpoint;
	std::vector<int> currentpoint;
};
class curveiterator {
public:
	curveiterator() { }
	void reset(curve inputcurve, int wwidth);
	float next();
	bool pointhit = true;
	int width = 284;
	int x = 99999;
private:
	std::vector<point> points;
	int nextpoint = 1;
	int currentpoint = 0;
};
