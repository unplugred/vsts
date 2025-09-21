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
	SmoothedValue<float,ValueSmoothingTypes::Linear> smooth[MC];
	float smoothtime = 0;
	potentiometer(String potname = "", String potid = "", float smoothed = 0, float potdefault = 0.f, float potmin = 0.f, float potmax = 1.f, ptype pottype = ptype::floattype) {
		name = potname;
		id = potid;
		smoothtime = smoothed;
		if(smoothed > 0) for(int o = 0; o < MC; ++o)
			smooth[o].setCurrentAndTargetValue(defaultvalue);
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
	potentiometer general[15];
	potentiometer values[19];
	float antialiasing = .7f; // 0.0 aa, 0.5 1x, 0.625 2x, 0.75 4x, 0.875 8x
	Atomic<int> selectedtab = 3;
	String tuningfile = "Standard";
	String themefile = "Default";
	float theme[9*3];
};

struct connection {
	int input = 0;
	int output = 0;
	float influence = .5f;
	connection(int pinput, int poutput, float pinfluence) {
		input = pinput;
		output = poutput;
		influence = pinfluence;
	}
};
struct pluginpreset {
	String name = "";
	bool unsaved = false;
	float general[16];
	float values[MC][19];
	int oppos[(MC+1)*2];
	std::vector<connection> opconnections[9];
	//curve curves[MC]; TODO
	pluginpreset(String pname = "") {
		name = pname;
	}
};

class FMPlusAudioProcessor : public plugmachine_dsp {
public:
	FMPlusAudioProcessor();
	~FMPlusAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
	void setoversampling();

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

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;

	int version = 0;
	const int generalcount = 15;
	const int paramcount = 19;

	pluginpreset state;
	pluginparams params;
	int currentpreset = 0;
	pluginpreset presets[20];

private:
	bool preparedtoplay = false;
	bool saved = false;

	int osindex = 0;
	std::unique_ptr<dsp::Oversampling<float>> os[3]; // 2 4 8
	AudioBuffer<float> osbuffer;
	std::vector<float*> ospointerarray;

	float pitches[128];

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusAudioProcessor)
};
