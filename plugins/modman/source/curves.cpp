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
	int c = fmin(channel,nextpoint.size());
	if(input <= 0) return points[0].y;
	if(input >= 1) return points[points.size()-1].y;
	if(nextpoint[c] >= points.size()) {
		currentpoint[c] = points.size()-1;
		nextpoint[c] = currentpoint[c]--;
		while(!points[currentpoint[c]].enabled) --currentpoint[c];
	}
	while(input < points[currentpoint[c]].x) {
		nextpoint[c] = currentpoint[c]--;
		while(!points[currentpoint[c]].enabled) --currentpoint[c];
	}
	while(input >= points[nextpoint[c]].x) {
		currentpoint[c] = nextpoint[c]++;
		while(!points[nextpoint[c]].enabled) ++nextpoint[c];
	}
	double interp = .5f;
	if((points[nextpoint[c]].x-points[currentpoint[c]].x) >= .0001f)
		interp = curve::calctension((input-points[currentpoint[c]].x)/(points[nextpoint[c]].x-points[currentpoint[c]].x),points[currentpoint[c]].tension);
	return points[currentpoint[c]].y*(1-interp)+points[nextpoint[c]].y*interp;
}
void curve::resizechannels(int channelnum) {
	currentpoint.resize(channelnum);
	nextpoint.resize(channelnum);

	currentpoint[0] = 0;
	nextpoint[0] = 1;
	while(!points[nextpoint[0]].enabled) ++nextpoint[0];

	for(int i = 0; i < channelnum; ++i) {
		currentpoint[i] = currentpoint[0];
		nextpoint[i] = nextpoint[0];
	}
}
curve::curve(String str, const char delimiter) {
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
