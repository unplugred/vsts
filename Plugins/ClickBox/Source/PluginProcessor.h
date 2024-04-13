#pragma once
#include "includes.h"
#include "perlin.h"

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
	potentiometer pots[6];
	bool overridee = false;
	float x = .5f;
	float y = .5f;
	SmoothedValue<float,ValueSmoothingTypes::Linear> xsmooth;
	SmoothedValue<float,ValueSmoothingTypes::Linear> ysmooth;
};

struct pluginpreset {
	String name = "";
	float values[6];
	pluginpreset(String pname = "", float val1 = .5f, float val2 = .5f, float val3 = .28f, float val4 = 0.f, float val5 = 1.f, float val6 = 0.f) {
		name = pname;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
		values[3] = val4;
		values[4] = val5;
		values[5] = val6;
	}
};

class ClickBoxAudioProcessor : public plugmachine_dsp, private Timer {
public:
	ClickBoxAudioProcessor();
	~ClickBoxAudioProcessor() override;

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
	const String get_preset(int preset_id, const char delimiter = ',') override;
	void set_preset(const String& preset, int preset_id, const char delimiter = ',', bool print_errors = false) override;

	virtual void parameterChanged(const String& parameterID, float newValue);

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;
	
	Atomic<float> x = .5f;
	Atomic<float> y = .5f;
	Atomic<float> i = .0f;

	int version = 3;
	const int paramcount = 6;

	pluginpreset state;
	pluginparams params;
	bool lerpchanged[6];
	int currentpreset = 0;

private:
	pluginpreset presets[20];
	void timerCallback() override;
	float lerptable[6];
	float lerpstage = 0;

	float oldautomod = 0;
	float oldoverride = 1;

	Random random;
	perlin prlin;
	float time = 0;
	int samplessincelastperlin = 0;
	float oldi = .0f;
	float rms;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClickBoxAudioProcessor)
};
