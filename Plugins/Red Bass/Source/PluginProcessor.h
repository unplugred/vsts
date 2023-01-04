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

class RedBassAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener, private Timer {
public:
	RedBassAudioProcessor();
	~RedBassAudioProcessor() override;

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

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

	double calculateattack(double value);
	double calculaterelease(double value);
	double calculatelowpass(double value);
	double calculatefrequency(double value);
	double calculatethreshold(double value);
 
	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;

	int version = 2;
	const int paramcount = 7;

	pluginpreset state;
	pluginparams params;
	bool lerpchanged[7];

	CoolLogger logger;
private:
	AudioProcessorValueTreeState::ParameterLayout createParameters();
	pluginpreset presets[20];
	int currentpreset = 0;
	void timerCallback() override;
	float lerptable[7];
	float lerpstage = 0;

	double crntsmpl = 0;
	EnvelopeFollower envelopefollower;
	dsp::StateVariableFilter::Filter<float> filter;

	int channelnum = 0;
	int samplesperblock = 0;
	float samplerate = 44100;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RedBassAudioProcessor)
};
