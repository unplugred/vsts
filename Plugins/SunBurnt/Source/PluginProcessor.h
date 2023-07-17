#pragma once
#include "includes.h"
#include "functions.h"

#define BANNER
#define BETA

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
	std::vector<point> points;
	static double calctension(double interp, double tension);
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
};
struct pluginparams {
	int curveselection = 0;
	potentiometer pots[16];
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

		curves[0].points.push_back(point(0,0,.2f));
		curves[0].points.push_back(point(.07f,1,.7f));
		curves[0].points.push_back(point(1,0,.5f));

		curves[1].points.push_back(point(0,0,.5f));
		curves[1].points.push_back(point(1,0,.5f));

		curves[2].points.push_back(point(0,.3f,.5f));
		curves[2].points.push_back(point(1,.3f,.5f));

		curves[3].points.push_back(point(0,1,.5f));
		curves[3].points.push_back(point(1,1,.5f));

		curves[4].points.push_back(point(0,.3f,.5f));
		curves[4].points.push_back(point(1,.3f,.5f));

		curves[5].points.push_back(point(0,.5f,.5f));
		curves[5].points.push_back(point(1,.5f,.5f));

		curves[6].points.push_back(point(0,.5f,.5f));
		curves[6].points.push_back(point(1,.5f,.5f));

		curves[7].points.push_back(point(0,0,.35f));
		curves[7].points.push_back(point(1,.5f,.5f));
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
	virtual void parameterChanged(const String& parameterID, float newValue);
	int64 reseed();
	void movepoint(int index, float x, float y);
	void movetension(int index, float tension);
	void addpoint(int index, float x, float y);
	void deletepoint(int index);
	String curvetostring(const char linebreak = ',');
	void curvefromstring(String str, const char linebreak = ',');
	bool isvalidcurvestring(String str, const char linebreak = ',');
 
	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;

	int version = 0;
	const int paramcount = 16;

	pluginpreset state;
	pluginpreset presets[20];
	int currentpreset = 0;
	pluginparams params;
	const String curvename[8] { "None","High-pass","HP resonance","Low-pass","LP resonance","Pan","Density","Shimmer" };

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
	int vibratoindex = 0;
	double vibratophase = 0;
	functions::dampendvalue dampvibratodepth;
	int lastbpm = 120;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SunBurntAudioProcessor)
};
