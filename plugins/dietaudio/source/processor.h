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
struct pluginparams {
	potentiometer pots[4];
};

struct pluginpreset {
	String name = "";
	float values[4];
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = 0.f, float val3 = 0.f, float val4 = 0.f) {
		name = pname;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
		values[3] = val4;
	}
};

class DietAudioAudioProcessor : public plugmachine_dsp, private Timer {
public:
	DietAudioAudioProcessor();
	~DietAudioAudioProcessor() override;

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
	double calculaterelease(double value);
	double calculatethreshold(double value);

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;

	int version = 0;
	const int paramcount = 4;

	pluginpreset state;
	pluginparams params;
	bool lerpchanged[4];
	int currentpreset = 0;

private:
	pluginpreset presets[20];
	void timerCallback() override;
	float lerptable[4];
	float lerpstage = 0;
	bool preparedtoplay = false;
	bool saved = false;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	float curfat = -1000;
	float curdry = -1000;
	double curnorm = 1;

	static const int fftorder = 10;
	static const int fftsize = 1 << fftorder;
	static const int fftbuffersize = fftsize*2;
	int fifoindex = 0;
	bool altfifo = false;
	std::vector<float> prebuffer;
	std::vector<float> postbuffer;
	float sqrthann[fftsize/2+1];
	dsp::FFT fft;
	EnvelopeFollower envelopefollower;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DietAudioAudioProcessor)
};

static std::function<String(float v, int max)> tothreshold = [](float v, int max) {
	return String(fmax(-96,Decibels::gainToDecibels(fmax(.000001f,pow(v,2)*10000))-58.4),2)+"dB";
};
static std::function<float(const String& s)> fromthreshold = [](const String& s) {
	if(s.containsIgnoreCase("f"))
		return 0.f;
	float val = s.getFloatValue();
	if(val >= 0)
		return jlimit(0.f,1.f,val);
	return jlimit(0.f,1.f,(float)sqrt(Decibels::decibelsToGain(val+58.4)/10000));
};
static std::function<String(float v, int max)> torelease = [](float v, int max) {
	return String(round(pow(v*38.7298334621,2)))+"ms";
};
static std::function<float(const String& s)> fromrelease = [](const String& s) {
	float val = s.getFloatValue();
	if(val <= 1)
		return jlimit(0.f,1.f,val);
	return jlimit(0.f,1.f,((float)sqrt(val)-12.2474487139f)/26.4823847482f);
};
static std::function<String(float v, int max)> todb = [](float v, int max) {
	float val = Decibels::gainToDecibels(fmax(.000001f,v));
	if(val <= -96)
		return (String)"Off";
	return String(val,2)+"dB";
};
static std::function<float(const String& s)> fromdb = [](const String& s) {
	if(s.containsIgnoreCase("f"))
		return 0.f;
	float val = s.getFloatValue();
	if(val >= 0)
		return jlimit(0.f,1.f,val);
	return jlimit(0.f,1.f,(float)Decibels::decibelsToGain(val));
};
