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
	ptype ttype = ptype::floattype;
	SmoothedValue<float,ValueSmoothingTypes::Linear> smooth[MC];
	float smoothtime = 0;
	potentiometer(String potname = "", String potid = "", float smoothed = 0, float potmin = 0.f, float potmax = 1.f, ptype pottype = ptype::floattype) {
		name = potname;
		id = potid;
		smoothtime = smoothed;
		if(smoothed > 0)
			for(int m = 0; m < MC; ++m)
				smooth[m].setCurrentAndTargetValue(0);
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
struct modulatorparams {
	String name = "";
	int id = 0;
	float defaults[5];
	modulatorparams() {}
};
struct pluginparams {
	potentiometer pots[6];
	modulatorparams modulators[MC];
	int selectedmodulator = 0;
};

struct pluginpreset {
	String name = "";
	float values[MC][5];
	float masterspeed = .5f;
	pluginpreset() {}
};

class ModManAudioProcessor : public plugmachine_dsp {
public:
	ModManAudioProcessor();
	~ModManAudioProcessor() override;

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
	float calccutoff(float val);
	float calcresonance(float val);
 
	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;

	int version = 0;
	const int paramcount = 6;

	pluginpreset state;
	pluginparams params;
	int currentpreset = 0;

private:
	pluginpreset presets[20];

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	perlin prlin;
	double time[MC];
	std::vector<float> modulator_data;
	bool ison[MC];

	std::vector<float> drift_data;
	std::vector<float> flange_data;
	int driftindex = 0;
	int flangeindex = 0;

	dsp::StateVariableTPTFilter<float> lowpass;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModManAudioProcessor)
};
