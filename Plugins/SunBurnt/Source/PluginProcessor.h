#pragma once
#include "includes.h"

#define BANNER
#define BETA

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
	bool enabled = true;
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
	potentiometer pots[6];
	double uiscale = 1.5;
	bool jpmode = false;
};

struct pluginpreset {
	String name = "";
	curve curves[5];
	int sync = 0;
	float values[6];
	float resonance[2] = {.3f,.3f};
	int shimmerpitch = 12;
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = 0.f, float val3 = 0.f, float val4 = 0.f, float val5 = 0.f, float val6 = 0.f, bool enablehighpass = true, bool enablelowpass = true, bool enablepan = true, bool enableshimmer = false) {
		name = pname;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
		values[3] = val4;
		values[4] = val5;
		values[5] = val6;
		curves[1].enabled = enablehighpass;
		curves[2].enabled = enablelowpass;
		curves[3].enabled = enablepan;
		curves[4].enabled = enableshimmer;
		curves[0].points.push_back(point(0,0,.2f));
		curves[0].points.push_back(point(.07f,1,.7f));
		curves[0].points.push_back(point(1,0,.5f));
		curves[1].points.push_back(point(0,0,.5f));
		curves[1].points.push_back(point(1,0,.5f));
		curves[2].points.push_back(point(0,1,.5f));
		curves[2].points.push_back(point(1,1,.5f));
		curves[3].points.push_back(point(0,.5f,.5f));
		curves[3].points.push_back(point(1,.5f,.5f));
		curves[4].points.push_back(point(0,0,.2f));
		curves[4].points.push_back(point(1,1,.5f));
	}
};

class SunBurntAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	SunBurntAudioProcessor();
	~SunBurntAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
	void genbuffer();

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
	void movepoint(int index, float x, float y);
	void movetension(int index, float tension);
	void addpoint(int index, float x, float y);
	void deletepoint(int index);
 
	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;

	int version = 0;
	const int paramcount = 6;

	pluginpreset state;
	pluginparams params;
	const String curvename[5] { "Volume","High Pass","Low Pass","Pan","Shimmer" };
	const String curveid[5] { "volume","highpass","lowpass","pan","shimmer" };

	CoolLogger logger;
private:
	AudioProcessorValueTreeState::ParameterLayout createParameters();
	pluginpreset presets[20];
	int currentpreset = 0;
	ApplicationProperties props;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	Random random;
	curveiterator iterator[5];

	std::vector<dsp::StateVariableFilter::Filter<float>> highpassfilters;
	std::vector<dsp::StateVariableFilter::Filter<float>> lowpassfilters;
	std::vector<std::unique_ptr<dsp::Convolution>> convolver;

	std::vector<std::unique_ptr<dsp::Convolution>> convolvereffect;
	soundtouch::SoundTouch pitchshift;
	std::vector<float> pitchprocessbuffer;
	int prevpitch = 0;

	Atomic<bool> updatedcurve;
	std::vector<AudioBuffer<float>> impulsebuffer;
	std::vector<AudioBuffer<float>> impulseeffectbuffer;
	std::vector<float*> impulsechanneldata;
	std::vector<float*> impulseeffectchanneldata;
	AudioBuffer<float> wetbuffer;
	AudioBuffer<float> effectbuffer;
	int revlength = 0;
	int readx = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SunBurntAudioProcessor)
};
