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
	potentiometer pots[6];
	bool oversampling = true;
};

struct pluginpreset {
	String name = "";
	float values[6];
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = 0.f, float val3 = 0.f, float val4 = 0.f, float val5 = 0.f, float val6 = 0.f) {
		name = pname;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
		values[3] = val4;
		values[4] = val5;
		values[5] = val6;
	}
};

class PFAudioProcessor : public plugmachine_dsp, private Timer {
public:
	PFAudioProcessor();
	~PFAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
	float plasticfuneral(float source, int channel, int channelcount, pluginpreset stt, float nrm);
	float normalizegain(float fat, float dry);
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
	void setCurrentProgram(int index) override;
	const String getProgramName(int index) override;
	void changeProgramName(int index, const String& newName) override;

	void getStateInformation(MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;
	const String get_preset(int preset_id, const char delimiter = ',') override;
	void set_preset(const String& preset, int preset_id, const char delimiter = ',', bool print_errors = false) override;

	virtual void parameterChanged(const String& parameterID, float newValue);

	AudioProcessorValueTreeState apvts;
	AudioProcessorValueTreeState::ParameterLayout create_parameters();

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;

	int version = 4;
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
	bool preparedtoplay = false;
	bool saved = false;

	std::unique_ptr<dsp::Oversampling<float>> os;
	AudioBuffer<float> osbuffer;
	std::vector<float*> ospointerarray;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	float curfat = -1000;
	float curdry = -1000;
	double curnorm = 1;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PFAudioProcessor)
};
