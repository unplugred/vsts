/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;

//==============================================================================
/**
*/
struct PluginPreset {
	String name;
	float freq, fat, drive, dry, stereo, gain;
};

class FmerAudioProcessor  : public juce::AudioProcessor
{
public:
	//==============================================================================
	FmerAudioProcessor();
	~FmerAudioProcessor() override;

	//==============================================================================
	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

	void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
	float plasticfuneral(float source, int channel, float freq, float fat, float drive, float dry, float stereo, float gain);

	//==============================================================================
	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	//==============================================================================
	const juce::String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram (int index) override;
	const juce::String getProgramName (int index) override;
	void changeProgramName (int index, const juce::String& newName) override;

	//==============================================================================
	void getStateInformation (juce::MemoryBlock& destData) override;
	void setStateInformation (const void* data, int sizeInBytes) override;
 
	AudioProcessorValueTreeState apvts;
	AudioProcessorValueTreeState::ParameterLayout createParameters();
	UndoManager undoManager;

	void normalizegain();
	float newnorm;
	float oldfreq = 0, oldfat = 0, olddrive = 0, olddry = 1, oldstereo = 0, oldgain = 1, oldnorm = 1;
	float freq = 0, fat = 0, drive = 0, dry = 1, stereo = 0, gain = 1, norm = 1;

	PluginPreset presets[10];
	int currentpreset = 0;
	void lerpPreset(float);
	void lerpValue(StringRef, float&, float);
	float lerptable[6];
private:

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FmerAudioProcessor)
};
