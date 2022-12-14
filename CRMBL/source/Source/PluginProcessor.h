#pragma once
#include "includes.h"
#include "CoolLogger.h"
#include "functions.h"
#include "DCFilter.h"

#define MAX_DLY 6.0
#define MIN_DLY 0.02

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
	potentiometer pots[12];
	bool oversampling = true;
	float hold = 0;
	SmoothedValue<float,ValueSmoothingTypes::Linear> holdsmooth;
};

struct pluginpreset {
	String name = "";
	float values[12];
	pluginpreset(String pname = "", float val1 = .32f, float val2 = 0.f, float val3 = .15f, float val4 = .5f, float val5 = 0.f, float val6 = 1.f, float val7 = .5f, float val8 = 0.f, float val9 = 0.f, float val10 = 0.f, float val11 = .3f, float val12 = .4f) {
		name = pname;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
		values[3] = val4;
		values[4] = val5;
		values[5] = val6;
		values[6] = val7;
		values[7] = val8;
		values[8] = val9;
		values[9] = val10;
		values[10] = val11;
		values[11] = val12;
	}
};

class CRMBLAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener, private Timer {
public:
	CRMBLAudioProcessor();
	~CRMBLAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	double pnch(double source, float amount);
	float interpolatesamples(float* buffer, float position, int buffersize);

	void setoversampling(bool toggle);

	AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	const String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram (int index) override;
	const String getProgramName (int index) override;
	void changeProgramName (int index, const String& newName) override;

	void getStateInformation (MemoryBlock& destData) override;
	void setStateInformation (const void* data, int sizeInBytes) override;
	virtual void parameterChanged(const String& parameterID, float newValue);
	void randomize();

	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;
	Atomic<float> lastosc = 0;
	Atomic<float> lastmodamp = 0;
	Atomic<float> lastbpm = 120;

	int version = 1;
	const int paramcount = 12;

	pluginpreset state;
	pluginparams params;
	bool lerpchanged[12];

	CoolLogger logger;
private:
	AudioProcessorValueTreeState::ParameterLayout createParameters();
	pluginpreset presets[20];
	int currentpreset = 0;
	void timerCallback() override;
	float lerptable[12];
	float lerpstage = 0;
	bool preparedtoplay = false;
	bool saved = false;

	std::vector<float> pitchprocessbuffer;
	AudioBuffer<float> delaybuffer;
	AudioBuffer<float> delayprocessbuffer;
	std::vector<float*> delaypointerarray;
	AudioBuffer<float> delaytimelerp;
	functions::dampendvalue dampdelaytime;
	functions::dampendvalue dampchanneloffset;
	functions::dampendvalue dampamp;
	functions::dampendvalue damplimiter;
	functions::dampendvalue damppitchlatency;
	std::vector<int> prevclear;
	double crntsmpl = 0;
	int delaybufferindex = 0;
	float prevpitch = 0;
	std::vector<double> prevfilter;
	std::vector<int> reversecounter;
	bool resetdampenings = true;
	bool prevpitchbypass = true;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;
	int blocksizething = 0;

	soundtouch::SoundTouch pitchshift;
	DCFilter dcfilter;
	std::unique_ptr<dsp::Oversampling<float>> os;
	Random random;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CRMBLAudioProcessor)
};
