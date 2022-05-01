#pragma once
#include <JuceHeader.h>
#include "perlin.h"
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
	float values[9];
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = 0.f, float val3 = 0.f, float val4 = 0.f, float val5 = 0.f, float val6 = 0.f, float val7 = 0.f, float val8 = 0.f, float val9 = 0.f) {
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
	}
};

class ClickBoxAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	ClickBoxAudioProcessor();
	~ClickBoxAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
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
	virtual void parameterChanged(const String& parameterID, float newValue);

	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;
	
	Atomic<float> x = .5f;
	Atomic<float> y = .5f;
	Atomic<float> i = .0f;

	int version = 2;
	const int paramcount = 9;

	pluginpreset state;
	potentiometer pots[9];

	CoolLogger logger;
private:
	int channelnum = 0;
	std::vector<float*> channelData;

	float oldautomod = 0;
	float oldoverride = 1;

	Random random;
	perlin prlin;
	float time = 0;
	int samplessincelastperlin = 0;
	float oldi = .0f;
	float rms;

	AudioProcessorValueTreeState::ParameterLayout createParameters();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClickBoxAudioProcessor)
};
