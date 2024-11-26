#pragma once
#include "includes.h"

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
	potentiometer pots[10];
	bool oversampling = true;
};

struct pluginpreset {
	String name = "";
	float values[10];
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = .4f, float val3 = .4f, float val4 = 0.f, float val5 = 0.f, float val6 = 1.f, float val7 = 1.f, float val8 = .54f, float val9 = .9f, float val10 = .65f) {
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
	}
};

class ScopeAudioProcessor : public plugmachine_dsp {
public:
	ScopeAudioProcessor();
	~ScopeAudioProcessor() override;

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
	const String get_preset(int preset_id, const char delimiter = ',') override;
	void set_preset(const String& preset, int preset_id, const char delimiter = ',', bool print_errors = false) override;

	virtual void parameterChanged(const String& parameterID, float newValue);

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	int version = 1;
	const int paramcount = 10;

	std::vector<float> osci;
	std::vector<bool> oscimode;
	std::vector<int> osciscore;
	std::vector<float> line;
	bool linemode[800];
	int oscisize = 0;
	Atomic<bool> frameready = false;

	float oscskip = 0;
	int oscindex = 0;

	Atomic<int> sleep = 0;
	Atomic<int> height = 400;
	Atomic<int> settingsx = -999;
	Atomic<int> settingsy = -999;
	Atomic<bool> settingsopen = false;
	DocumentWindow* settingswindow = nullptr;

	pluginpreset state;
	pluginparams params;
	int currentpreset = 0;

	int channelnum = 0;
private:
	int samplesperblock = 0;
	int samplerate = 44100;

	pluginpreset presets[20];
	bool preparedtoplay = false;
	bool saved = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeAudioProcessor)
};
