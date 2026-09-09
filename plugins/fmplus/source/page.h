#pragma once
#include "includes.h"
#include "functions.h"
#include "curves.h"

struct potentiometer {
public:
	enum ptype {
		floattype = 0,
		inttype = 1,
		booltype = 2
	};

	potentiometer(String potname = "", String potid = "", String guiname = "", float smoothed = 0, float potmin = 0.f, float potmax = 1.f, ptype pottype = ptype::floattype) {
		name = potname;
		nameui = guiname;
		id = potid;
		smoothtime = smoothed;
		minimumvalue = potmin;
		maximumvalue = potmax;
		ttype = pottype;
	}

	String name = "";
	String nameui = "";
	String id = "";
	ptype ttype = ptype::floattype;
	float minimumvalue = 0.f;
	float maximumvalue = 1.f;
	float defaultvalue = 0.f;
	float normalize(float val) {
		return (val-minimumvalue)/(maximumvalue-minimumvalue);
	}
	float inflate(float val) {
		return val*(maximumvalue-minimumvalue)+minimumvalue;
	}
	float smoothtime = 0;
	float defaultvaluenorm = 0.f;
	float uismooth = 0;

	bool saved = true;
	signed char updateline = -1;
	signed char bpmsync = 0;
	bool defaultbpm = false;
	signed char adsr = -1;
	signed char adsrstage = -1;
};
struct curvedata {
	std::vector<curve> c;
	std::vector<curve> cui;
	std::vector<Atomic<bool>> updatedcurve;
	char channels = 1;
	char linenum = 0;
	char animid = 0;
};
struct box {
	unsigned char x = 0;
	unsigned char y = 0;
	unsigned char w = 0;
	unsigned char h = 0;

	signed char type = 0; // -1 - inactive, 0 - line, 1 - centered, 2 - noline, 3 - tick, 4 - burger
	signed char curve = -1;
	signed char knob = -1;
	signed char textamount = 0;

	bool corner = false;
	char meow   = 0; // make data type the
	char meoww  = 0; // size of 16 bytes to
	char meowww = 0; // improve caching

	signed short mesh = -1;
	signed short textmesh = -1;

	box(int pknob = -1, int px = 0, int py = 0, int pw = 0, int ph = 0, int ptype = -1, int ptextamount = 0, bool pcorner = false) {
		knob = pknob;
		x = px;
		y = py;
		w = pw;
		h = ph;
		type = ptype;
		textamount = ptextamount;
		corner = pcorner;
	}
};
struct meshdata {
	float text[(1937+1)*4]; // X Y COL CHAR
	int textlength = -1;
	void addtext(float x, float y, String txt, float color = 0, bool rotated = false);
	void replacetext(int id, String txt, int length = -1);

	box boxes[24];
	int boxnum = 0;

	float square[(233+1)*4];
	int squarelength = -1;
	void addsquare(float x, float y, float w, float h, float color = 0, bool corner = false);

	float line[(479+1)*3+1000]; // TODO
	int linelength = -1;
	int connectorlength = 0;
	float lineprevx = 0;
	float lineprevy = 020;
	float linecurrentx = 0;
	float linecurrenty = 0;
	bool linebegun = true;
	int linewritepos = -1;
	int linesegments[4];
	float lineht[4] = { 10, 10, 10, 10 };
	void beginline(float x, float y);
	void endline();
	void nextpoint(float x, float y, bool knee = false);

	int width = 0;
	int height = 0;
	float scaled_dpi = 0;

	int tab = -1;
	int o = -1;
	int p =  -1;

	float framedata[VC];
};
struct preset {
	int id;
	String name;
	int version = 0;
	//virtual const String get_preset(char delimiter = ',');
	//virtual void set_preset(const String& preset, char delimiter = ',');
};
struct presetpage : public preset {
	std::vector<std::vector<float>> value; // [i][o]
	std::vector<std::vector<std::vector<point>>> points; // [i][o][p]
	// TODO get / set
};
class FMPlusPage {
public:

	FMPlusPage();
	~FMPlusPage();

	virtual void calcvis(int lineupdated);
	virtual void rebuildtab();
	virtual void addtext(int i);
	virtual bool updatevalue(int param);
	virtual void getframe();

	meshdata* mesh = nullptr;
	int linesnum = 0;
	int paramstart = 0;
	int sections[4] = { -1, -1, -1, -1 };

	String tabname;

	dampenedvalue knobsmooth;
	std::vector<potentiometer> params;
	std::vector<presetpage> presets;
	presetpage state;
	std::vector<std::vector<SmoothedValue<float,ValueSmoothingTypes::Linear>>> smooth;
	int instances = 1;
	std::vector<std::vector<float>> valuenorm;
	std::vector<signed char> knobbox;

	std::vector<curvedata> curves;
	int curvesnum = 0;

	std::vector<std::vector<float>> adsr; // [a,d,s,r]*[t,b]*[c,t]
	int adsrnum = 0;
	dampenedvalue dampadsr; // currently not supporting variable stage num within page

	float dividerx[4] { 1.f,1.f,1.f,1.f };
	float dividery[3] { 1.f,1.f,1.f };
	int framedatavals = 0;
};
