#pragma once
#include "includes.h"
#include "functions.h"

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
	potentiometer(String potname = "", String potid = "", float potdefault = 0.f, float potmin = 0.f, float potmax = 1.f, ptype pottype = ptype::floattype) {
		name = potname;
		id = potid;
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
	float values[3];
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = 0.f, float val3 = 0.f) {
		name = pname;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
	}
};

class VUAudioProcessor : public plugmachine_dsp {
public:
	VUAudioProcessor();
	~VUAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
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

	Atomic<float> leftvu = 0;
	Atomic<float> rightvu = 0;
	Atomic<bool> leftpeak = false;
	Atomic<bool> rightpeak = false;
	Atomic<int> buffercount = 0;

	Atomic<int> height = 300;

	AudioProcessorValueTreeState apvts;
	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	int version = 2;

	pluginpreset presets[20];
	potentiometer pots[3];
	int currentpreset = 0;
	const int paramcount = 3;
private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUAudioProcessor)
};

static std::function<String(int v, int max)> todb = [](int v, int max) {
	return String(v)+"db";
};
static std::function<int(const String& s)> fromdb = [](const String& s) {
	return jlimit(-24,-6,s.getIntValue());
};
static std::function<String(bool v, int max)> tostereo = [](bool v, int max) {
	return v?"stereo":"mono";
};
static std::function<bool(const String& s)> fromstereo = [](const String& s) {
	if(s.containsIgnoreCase("s")) return true;
	if(s.containsIgnoreCase("m")) return false;
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return true;
};
