#pragma once
#include "includes.h"
#include "functions.h"
#include "dc_filter.h"

#define MAX_DLY 3.0
#define MIN_DLY 0.02
#define DLINES 3

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
	potentiometer pots[2+DLINES];
};

struct pluginpreset {
	String name = "";
	float values[2+DLINES];
	pluginpreset(String pname = "", float val1 = 0.6f, float val2 = 0.66f) {
		name = pname;
		values[0] = val1;
		values[1] = val2;
	}
};

class TripleDAudioProcessor : public plugmachine_dsp, private Timer {
public:
	TripleDAudioProcessor();
	~TripleDAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
	float interpolatesamples(float* buffer, float position, int buffersize);

	AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

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
	const String get_preset(int preset_id, const char delimiter = ',') override;
	void set_preset(const String& preset, int preset_id, const char delimiter = ',', bool print_errors = false) override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void randomize();

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;

	int version = 0;
	const int paramcount = 2+DLINES;

	pluginpreset state;
	pluginparams params;
	bool lerpchanged[2+DLINES];
	int currentpreset = 0;

private:
	pluginpreset presets[20];
	void timerCallback() override;
	float lerptable[2+DLINES];
	float lerpstage = 0;
	bool preparedtoplay = false;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	functions::dampendvalue delayamt;
	float dindex[DLINES];
	int readpos = 0;
	AudioBuffer<float> delaybuffer;
	int delaybuffersize = 0;
	dc_filter dcfilter;
	bool resetdampenings = false;
	Random random;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TripleDAudioProcessor)
};

static std::function<String(float v, int max)> tolength = [](float v, int max) {
	return String(round((pow(v,2)*(MAX_DLY-MIN_DLY)+MIN_DLY)*1000))+"ms";
};
static std::function<float(const String& s)> fromlength = [](const String& s) {
	float val = s.getFloatValue();
	if(!s.containsIgnoreCase("s") && val <= 1)
		return jlimit(0.f,1.f,val);
	if((s.containsIgnoreCase("s") && !s.containsIgnoreCase("ms")) || (!s.containsIgnoreCase("s") && val <= MAX_DLY))
		val *= 1000;
	return jlimit(0.f,1.f,(float)sqrt((val*.001f-MIN_DLY)/(MAX_DLY-MIN_DLY)));
};
static std::function<String(float v, int max)> tonormalized = [](float v, int max) {
	return String(v,3);
};
static std::function<float(const String& s)> fromnormalized = [](const String& s) {
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(bool v, int max)> tobool = [](bool v, int max) {
	return v?"on":"off";
};
static std::function<bool(const String& s)> frombool = [](const String& s) {
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return true;
};
