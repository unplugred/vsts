#include "curves.h"

void curveiterator::reset(curve inputcurve, int wwidth) {
	points = inputcurve.points;
	width = wwidth;
	x = -1;
	nextpoint = 0;
	points[0].x = 0;
	points[points.size()-1].x = 1;
	pointhit = true;
	while(!points[nextpoint].enabled) ++nextpoint;
}
double curveiterator::next() {
	double xx = ((double)++x)/(width-1);
	if(xx >= 1) {
		if((((double)x-1)/(width-1)) < 1) pointhit = true;
		return points[points.size()-1].y;
	}
	if(xx >= points[nextpoint].x || (width-x) <= (points.size()-nextpoint)) {
		pointhit = true;
		currentpoint = nextpoint;
		++nextpoint;
		while(!points[nextpoint].enabled) ++nextpoint;
		return points[currentpoint].y;
	}
	double interp = curve::calctension((xx-points[currentpoint].x)/(points[nextpoint].x-points[currentpoint].x),points[currentpoint].tension);
	return points[currentpoint].y*(1-interp)+points[nextpoint].y*interp;
}
double curve::process(double input, int channel) {
	if(input <= 0) return points[0].y;
	if(input >= 1) return points[points.size()-1].y;
	
	if(nextpoint[channel] >= points.size()) {
		nextpoint[channel] = points.size()-1;
		currentpoint[channel] = nextpoint[channel]-1;
	}

	while(input < points[currentpoint[channel]].x)
		nextpoint[channel] = currentpoint[channel]--;
	while(input >= points[nextpoint[channel]].x)
		currentpoint[channel] = nextpoint[channel]++;

	double interp = .5f;
	if((points[nextpoint[channel]].x-points[currentpoint[channel]].x) >= .0001f)
		interp = curve::calctension((input-points[currentpoint[channel]].x)/(points[nextpoint[channel]].x-points[currentpoint[channel]].x),points[currentpoint[channel]].tension);
	return points[currentpoint[channel]].y*(1-interp)+points[nextpoint[channel]].y*interp;
}
void curve::resizechannels(int channelnum) {
	currentpoint.resize(channelnum);
	nextpoint.resize(channelnum);

	currentpoint[0] = 0;
	nextpoint[0] = 1;

	for(int i = 1; i < channelnum; ++i) {
		currentpoint[i] = currentpoint[0];
		nextpoint[i] = nextpoint[0];
	}
}
curve::curve(String str, const char delimiter, int channelnum) {
	if(channelnum > 0) {
		currentpoint.resize(channelnum);
		nextpoint.resize(channelnum);
		for(int i = 0; i < channelnum; ++i) {
			currentpoint[i] = 0;
			nextpoint[i] = 1;
		}
	}

	std::stringstream ss(str.trim().toRawUTF8());
	std::string token;
	std::getline(ss, token, delimiter);
	int size = std::stof(token);
	if(size < 2) throw std::invalid_argument("Invalid point data");
	float prevx = 0;
	for(int p = 0; p < size; ++p) {
		std::getline(ss, token, delimiter);
		float x = std::stof(token);
		std::getline(ss, token, delimiter);
		float y = std::stof(token);
		std::getline(ss, token, delimiter);
		float tension = std::stof(token);
		if(x > 1 || x < prevx || y > 1 || y < 0 || tension > 1 || tension < 0 || (p == 0 && x != 0) || (p == (size-1) && x != 1))
			throw std::invalid_argument("Invalid point data");
		prevx = x;
		points.push_back(point(x,y,tension));
	}
}
String curve::tostring(const char delimiter) {
	std::ostringstream data;
	data << points.size() << delimiter;
	for(int p = 0; p < points.size(); ++p)
		data << points[p].x << delimiter << points[p].y << delimiter << points[p].tension << delimiter;
	return (String)data.str();
}
double curve::calctension(double interp, double tension) {
	if(tension == .5)
		return interp;
	else {
		tension = tension*.99+.005;
		if(tension < .5)
			return pow(interp,.5/tension)*(1-tension)+(1-pow(1-interp,2*tension))*tension;
		else
			return pow(interp,2-2*tension)*(1-tension)+(1-pow(1-interp,.5/(1-tension)))*tension;
	}
}
bool curve::isvalidcurvestring(String str, const char delimiter) {
	String trimstring = str.trim();
	if(trimstring.isEmpty())
		return false;
	if(!trimstring.containsOnly(String(&delimiter,1)+"-.0123456789"))
		return false;
	if(!trimstring.endsWithChar(delimiter))
		return false;
	if(trimstring.startsWithChar(delimiter))
		return false;
	return true;
}
