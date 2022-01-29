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
	float freq, piss, noise, harm, stereo;
};

class PisstortionAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener, private Timer {
public:
	PisstortionAudioProcessor();
	~PisstortionAudioProcessor() override;

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	float pisstortion(float source, int channel, float freq, float piss, float noise, float harm, float stereo, float gain);
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

	int version = 0;
	Atomic<float> freq = 0.17f, piss = 1.f, noise = .35f, harm = .31f, stereo = .1f, gain = 1.f, norm = 1.f;
	Atomic<int> oversampling = 1;

private:
	float oldfreq = 0, oldpiss = 0, oldnoise = 0, oldharm = .5f, oldstereo = 0, oldgain = 1, oldnorm = 1;

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

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PisstortionAudioProcessor)
};
