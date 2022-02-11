/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;

struct PluginPreset {
	String name;
	float freq, fat, drive, dry, stereo;
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
	float plasticfuneral(float source, int channel, float freq, float fat, float drive, float dry, float stereo, float gain);
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
	Atomic<float> freq = 0.32, fat = 0, drive = 0, dry = 0, stereo = 0.37, gain = .4, norm = 1;
	Atomic<int> oversampling = 1;

private:
	float oldfreq = 0, oldfat = 0, olddrive = 0, olddry = 0, oldstereo = 0, oldgain = 1, oldnorm = 1;

	AudioProcessorValueTreeState::ParameterLayout createParameters();
	PluginPreset presets[8];
	int currentpreset = 0;
	void timerCallback() override;
	void lerpValue(StringRef, float&, float);
	float lerptable[6];
	float lerpstage = 0;
	bool boot = false;
	bool preparedtoplay = false;

	std::unique_ptr<dsp::Oversampling<float>> os[3];
	AudioBuffer<float> osbuffer;
	int channelnum = 0;
	int samplesperblock = 512;
	std::vector<float*> ospointerarray;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PFAudioProcessor)
};
