#pragma once
#include "includes.h"
#include "envelopefollower.h"

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

struct pluginpreset {
	String name = "";
	float values[7];
	pluginpreset(String pname = "", float val1 = .58f, float val2 = .02f, float val3 = .17f, float val4 = .18f, float val5 = 0.f, float val6 = 1.f, float val7 = .18f) {
		name = pname;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
		values[3] = val4;
		values[4] = val5;
		values[5] = val6;
		values[6] = val7;
	}
};
struct pluginparams {
	potentiometer pots[7];
	float monitor = 0;
	SmoothedValue<float,ValueSmoothingTypes::Linear> monitorsmooth;
};

class RedBassAudioProcessor : public plugmachine_dsp, private Timer {
public:
	RedBassAudioProcessor();
	~RedBassAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

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

	double calculateattack(double value);
	double calculaterelease(double value);
	double calculatelowpass(double value);
	double calculatefrequency(double value);
	double calculatethreshold(double value);
 
	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;

	int version = 3;
	const int paramcount = 7;

	pluginpreset state;
	pluginparams params;
	bool lerpchanged[7];
	int currentpreset = 0;

private:
	pluginpreset presets[20];
	void timerCallback() override;
	float lerptable[7];
	float lerpstage = 0;

	double crntsmpl = 0;
	EnvelopeFollower envelopefollower;
	dsp::StateVariableFilter::Filter<float> filter;

	int channelnum = 0;
	int samplesperblock = 0;
	float samplerate = 44100;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RedBassAudioProcessor)
};

static std::function<String(float v, int max)> tofreq = [](float v, int max) {
	return String(round(mapToLog10(v,20.f,100.f)))+"hz";
};
static std::function<float(const String& s)> fromfreq = [](const String& s) {
	float val = s.getFloatValue();
	if(val <= 1)
		return jlimit(0.f,1.f,val);
	return jlimit(0.f,1.f,mapFromLog10(val,20.f,100.f));
};
static std::function<String(float v, int max)> tothreshold = [](float v, int max) {
	float val = Decibels::gainToDecibels(fmax(.000001f,pow(v,2)*.7079f));
	if(val <= -96)
		return (String)"off";
	return String(val,2)+"db";
};
static std::function<float(const String& s)> fromthreshold = [](const String& s) {
	if(s.containsIgnoreCase("f"))
		return 0.f;
	float val = s.getFloatValue();
	if(val >= 0)
		return jlimit(0.f,1.f,val);
	return jlimit(0.f,1.f,(float)sqrt(Decibels::decibelsToGain(val)/.7079f));
};
static std::function<String(float v, int max)> toattack = [](float v, int max) {
	return String(round(pow(v*15.2896119631f+7.0710678119f,2)))+"ms";
};
static std::function<float(const String& s)> fromattack = [](const String& s) {
	float val = s.getFloatValue();
	if(val <= 1)
		return jlimit(0.f,1.f,val);
	return jlimit(0.f,1.f,((float)sqrt(val)-7.0710678119f)/15.2896119631f);
};
static std::function<String(float v, int max)> torelease = [](float v, int max) {
	return String(round(pow(v*26.4823847482f+12.2474487139f,2)))+"ms";
};
static std::function<float(const String& s)> fromrelease = [](const String& s) {
	float val = s.getFloatValue();
	if(val <= 1)
		return jlimit(0.f,1.f,val);
	return jlimit(0.f,1.f,((float)sqrt(val)-12.2474487139f)/26.4823847482f);
};
static std::function<String(float v, int max)> tocutoff = [](float v, int max) {
	if(v >= 1)
		return (String)"off";
	return String(round(mapToLog10(v,100.f,20000.f)))+"hz";
};
static std::function<float(const String& s)> fromcutoff = [](const String& s) {
	if(s.containsIgnoreCase("f"))
		return 1.f;
	float val = s.getFloatValue();
	if(!s.containsIgnoreCase("k") && !s.containsIgnoreCase("hz") && val <= 1)
		return jlimit(0.f,1.f,val);
	if((s.containsIgnoreCase("k")) || (!s.containsIgnoreCase("hz") && val < 20))
		val *= 1000;
	return jlimit(0.f,1.f,mapFromLog10(val,100.f,20000.f));
};
static std::function<String(bool v, int max)> tobool = [](bool v, int max) {
	return (String)(v?"on":"off");
};
static std::function<bool(const String& s)> frombool = [](const String& s) {
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return true;
};
static std::function<String(float v, int max)> tonormalized = [](float v, int max) {
	return String(v,3);
};
static std::function<float(const String& s)> fromnormalized = [](const String& s) {
	return jlimit(0.f,1.f,s.getFloatValue());
};
