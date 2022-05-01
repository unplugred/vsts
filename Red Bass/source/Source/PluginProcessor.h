#pragma once
#include <JuceHeader.h>
#include "envelopefollower.h"
#include "CoolLogger.h"

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
	bool savedinpreset = true;
	ptype ttype = ptype::floattype;
	SmoothedValue<float,ValueSmoothingTypes::Linear> smooth;
	float smoothtime = 0;
	potentiometer(String potname = "", String potid = "", float smoothed = 0, float potdefault = 0.f, float potmin = 0.f, float potmax = 1.f, bool potsaved = true, ptype pottype = ptype::floattype) {
		name = potname;
		id = potid;
		smoothtime = smoothed;
		if(smoothed > 0) smooth.setCurrentAndTargetValue(defaultvalue);
		defaultvalue = potdefault;
		minimumvalue = potmin;
		maximumvalue = potmax;
		savedinpreset = potsaved;
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
	float values[8];
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = 0.f, float val3 = 0.f, float val4 = 0.f, float val5 = 0.f, float val6 = 0.f, float val7 = 1.f, float val8 = 1.f) {
		name = pname;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
		values[3] = val4;
		values[4] = val5;
		values[5] = val6;
		values[6] = val7;
		values[7] = val8;
	}
};

class RedBassAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	RedBassAudioProcessor();
	~RedBassAudioProcessor() override;

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void resetfilter();
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

	int version = 1;
	const int paramcount = 8;

	pluginpreset state;
	potentiometer pots[8];

	CoolLogger logger;
private:
	AudioProcessorValueTreeState::ParameterLayout createParameters();

	double crntsmpl = 0;
	EnvelopeFollower envelopefollower;
	dsp::StateVariableFilter::Filter<float> filter;

	int channelnum = 0;
	int samplesperblock = 0;
	float samplerate = 44100;
	std::vector<float*> channelData;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RedBassAudioProcessor)
};
