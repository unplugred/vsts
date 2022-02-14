/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;

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
	potentiometer(String potname = "", String potid = "", float potdefault = 0.f, float potmin = 0.f, float potmax = 1.f, bool potsaved = true, ptype pottype = ptype::floattype) {
		name = potname;
		id = potid;
		minimumvalue = potmin;
		maximumvalue = potmax;
		defaultvalue = potdefault;
		savedinpreset = potsaved;
		ttype = pottype;
	}
	float normalize(float value) {
		return (value-minimumvalue)/(maximumvalue-minimumvalue);
	}
	float inflate(float value) {
		return value*(maximumvalue-minimumvalue)+minimumvalue;
	}
};

struct pluginpreset {
	String name = "";
	float values[7];
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = 0.f, float val3 = 0.f, float val4 = 0.f, float val5 = 0.f, float val6 = 0.f, float val7 = 0.f) {
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

class PFAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener, private Timer {
public:
	PFAudioProcessor();
	~PFAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	float plasticfuneral(float source, int channel, int channelcount, pluginpreset stt);
	void normalizegain();
	void setoversampling(int factor);

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
 
	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;
	Atomic<bool> updatevis;

	int version = 2;
	Atomic<float> norm = 1;
	const int paramcount = 7;

	pluginpreset state;
	potentiometer pots[7];

private:
	float oldnorm = 1;

	AudioProcessorValueTreeState::ParameterLayout createParameters();
	pluginpreset presets[8];
	pluginpreset oldstate;
	int currentpreset = 0;
	void timerCallback() override;
	void lerpValue(StringRef, float&, float);
	float lerptable[6];
	float lerpstage = 0;
	bool boot = false;
	bool preparedtoplay = false;

	std::unique_ptr<dsp::Oversampling<float>> os;
	AudioBuffer<float> osbuffer;
	int channelnum = 0;
	int samplesperblock = 512;
	bool changingoversampling = false;
	std::vector<float*> ospointerarray;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PFAudioProcessor)
};
