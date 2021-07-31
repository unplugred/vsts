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

class FmerAudioProcessor : public AudioProcessor, private Timer {
public:
	FmerAudioProcessor();
	~FmerAudioProcessor() override;

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	float plasticfuneral(float source, int channel, float freq, float fat, float drive, float dry, float stereo, float gain);
	void normalizegain();

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
 
	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;

	int version = 1;
	float freq = 0.32, fat = 0, drive = 0, dry = 0, stereo = 0.37, gain = .4, norm = 1;

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

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FmerAudioProcessor)
};
