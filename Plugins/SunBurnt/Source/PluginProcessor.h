#pragma once
#include "includes.h"
#include "functions.h"

#define MAX_VIBRATO 1.0

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
	curve(String str, const char linebreak = ',');
	String tostring(const char linebreak = ',');
	std::vector<point> points;
	static double calctension(double interp, double tension);
	static bool isvalidcurvestring(String str, const char linebreak = ',');
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
struct potentiometer {
public:
	enum ptype {
		floattype = 0,
		inttype = 1,
		booltype = 2
	};
	String name = "";
	String id = "";
	float minimumvalue = 0;
	float maximumvalue = 1;
	float defaultvalue = 0;
	ptype ttype = ptype::floattype;
	SmoothedValue<float,ValueSmoothingTypes::Linear> smooth;
	float smoothtime = 0;
	potentiometer(String potname = "", String potid = "", float smoothed = 0, float potdefault = 0.f, float potmin = 0.f, float potmax = 1.f, ptype pottype = ptype::floattype) {
		name = potname;
		id = potid;
		smoothtime = smoothed;
		if(smoothed > 0) smooth.setCurrentAndTargetValue(defaultvalue);
		defaultvalue = potdefault;
		minimumvalue = potmin;
		maximumvalue = potmax;
		ttype = pottype;
	}
	float normalize(float val) {
		return (val-minimumvalue)/(maximumvalue-minimumvalue);
	}
	float inflate(float val) {
		return val*(maximumvalue-minimumvalue)+minimumvalue;
	}
	std::vector<int> showon { };
	std::vector<int> dontshowif { };
};
struct curveparams {
	String name = "";
	String defaultvalue = "";
	curveparams(String potname = "", String potdefault = "") {
		name = potname;
		defaultvalue = potdefault;
	}
};
struct pluginparams {
	int curveselection = 0;
	potentiometer pots[16];
	curveparams curves[8];
	double uiscale = 1.5;
	bool jpmode = false;
};

struct pluginpreset {
	String name = "";
	curve curves[8];
	float values[16];
	int64 seed = 0;
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = 0.f, float val3 = 0.f, float val4 = 0.f, float val5 = 0.f, float val6 = 0.f, float val7 = 0.f, float val8 = 0.f, float val9 = 0.f, float val10 = 0.f, float val11 = 0.f, float val12 = 0.f, float val13 = 0.f, float val14 = 0.f, float val15 = 0.f, float val16 = 0.f) {
		name = pname;

		values[ 0] = val1;
		values[ 1] = val2;
		values[ 2] = val3;
		values[ 3] = val4;
		values[ 4] = val5;
		values[ 5] = val6;
		values[ 6] = val7;
		values[ 7] = val8;
		values[ 8] = val9;
		values[ 9] = val10;
		values[10] = val11;
		values[11] = val12;
		values[12] = val13;
		values[13] = val14;
		values[14] = val15;
		values[15] = val16;
	}
};

class SunBurntAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	SunBurntAudioProcessor();
	~SunBurntAudioProcessor() override;

	int findcurve(int index);

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void resetconvolution();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
	void genbuffer();
	float interpolatesamples(float* buffer, float position, int buffersize);

	AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;
	void setLang(bool isjp);
	void setUIScale(double uiscale);

	const String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const String getProgramName(int index) override;
	void changeProgramName(int index, const String& newName) override;

	void getStateInformation(MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;
	const String getpreset(int presetid, const char delimiter = ',');
	void setpreset(const String& preset, int presetid, const char delimiter = ',', bool printerrors = false);
	virtual void parameterChanged(const String& parameterID, float newValue);
	int64 reseed();
	void movepoint(int index, float x, float y);
	void movetension(int index, float tension);
	void addpoint(int index, float x, float y);
	void deletepoint(int index);
	const String curvetostring(const char linebreak = ',');
	void curvefromstring(String str, const char linebreak = ',');
	void resetcurve();

	AudioProcessorValueTreeState apvts;

	UndoManager undoManager;

	int version = 0;
	const int paramcount = 16;

	pluginpreset state;
	pluginpreset presets[20];
	int currentpreset = 0;
	pluginparams params;

	Atomic<bool> updatevis = false;

	CoolLogger logger;
private:
	AudioProcessorValueTreeState::ParameterLayout createParameters();
	ApplicationProperties props;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	curveiterator iterator[5];

	std::vector<dsp::StateVariableFilter::Filter<float>> highpassfilters;
	std::vector<dsp::StateVariableFilter::Filter<float>> lowpassfilters;
	std::vector<std::unique_ptr<dsp::Convolution>> convolver;

	std::vector<std::unique_ptr<dsp::Convolution>> convolvereffect;
	soundtouch::SoundTouch pitchshift;
	std::vector<float> pitchprocessbuffer;
	int prevpitch = 0;

	Atomic<bool> updatedcurve = true;
	Atomic<float> updatedcurvecooldown = -1;
	float updatedcurvebpmcooldown = -1;
	AudioBuffer<float> vibratobuffer;
	std::vector<AudioBuffer<float>> impulsebuffer;
	std::vector<AudioBuffer<float>> impulseeffectbuffer;
	std::vector<float*> impulsechanneldata;
	std::vector<float*> impulseeffectchanneldata;
	AudioBuffer<float> wetbuffer;
	AudioBuffer<float> effectbuffer;
	int revlength = 0;
	int taillength = 0;
	int vibratoindex = 0;
	double vibratophase = 0;
	functions::dampendvalue dampvibratodepth;
	int lastbpm = 120;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SunBurntAudioProcessor)
};

static std::function<String(float v, int max)> topercent = [](float v, int max) {
	return String(v*100.f,1)+"%";
};
static std::function<float(const String& s)> frompercent = [](const String& s) {
	return jlimit(0.f,1.f,s.getFloatValue()*.01f);
};
static std::function<String(float v, int max)> tonormalized = [](float v, int max) {
	return String(v,3);
};
static std::function<float(const String& s)> fromnormalized = [](const String& s) {
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tolength = [](float v, int max) {
	return String(round(pow(v,2)*5900+100))+"ms";
};
static std::function<float(const String& s)> fromlength = [](const String& s) {
	float val = s.getFloatValue();
	if((s.containsIgnoreCase("s") && !s.containsIgnoreCase("ms")) || (!s.containsIgnoreCase("s") && val <= 6))
		val *= 1000;
	return jlimit(0.f,1.f,(float)sqrt((val-100)/5900.f));
};
static std::function<String(int v, int max)> toqn = [](int v, int max) {
	if(v <= 0)
		return (String)"off";
	return String(v)+"qn";
};
static std::function<int(const String& s)> fromqn = [](const String& s) {
	if(s.containsIgnoreCase("f"))
		return 0;
	return jlimit(0,16,s.getIntValue());
};
static std::function<String(float v, int max)> tospeed = [](float v, int max) {
	return String(pow(v*2+.2f,2),1)+"hz";
};
static std::function<float(const String& s)> fromspeed = [](const String& s) {
	return jlimit(0.f,1.f,((float)sqrt(s.getFloatValue())-.2f)*.5f);
};
static std::function<String(float v, int max)> tocutoff = [](float v, int max) {
	return String(round(mapToLog10(v,20.f,20000.f)))+"hz";
};
static std::function<float(const String& s)> fromcutoff = [](const String& s) {
	float val = s.getFloatValue();
	if((s.containsIgnoreCase("k")) || (!s.containsIgnoreCase("hz") && val < 20))
		val *= 1000;
	return jlimit(0.f,1.f,mapFromLog10(val,20.f,20000.f));
};
static std::function<String(float v, int max)> toresonance = [](float v, int max) {
	return String(mapToLog10(v,.1f,40.f),1);
};
static std::function<float(const String& s)> fromresonance = [](const String& s) {
	return jlimit(0.f,1.f,mapFromLog10(s.getFloatValue(),.1f,40.f));
};
static std::function<String(int v, int max)> topitch = [](int v, int max) {
	if(v == 0)
		return (String)"off";
	if(max < 12)
		return String(v)+"st";
	return String(v)+" semitones";
};
static std::function<int(const String& s)> frompitch = [](const String& s) {
	if(s.containsIgnoreCase("f"))
		return 0;
	return jlimit(-24,24,s.getIntValue());
};
