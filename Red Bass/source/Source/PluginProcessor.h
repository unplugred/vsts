/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "envelopefollower.h"

class RedBassAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	RedBassAudioProcessor();
	~RedBassAudioProcessor() override;

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

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
	virtual void parameterChanged(const String& parameterID, float newValue);

	float calculateattack(float value);
	float calculaterelease(float value);
	float calculatelowpass(float value);
	float calculatefrequency(float value);
 
	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;

	int version = 0;
	Atomic<float> freq = 0.48, threshold = 0.02, attack = 0.17, release = 0.18, lowpass = 0, dry = 1, wet = 0.18;
	Atomic<bool> monitor = false;

	Atomic<int> viscount = 0;
	Atomic<float> visamount = 0;

private:
	float oldfreq = 0.48, oldthreshold = 0, oldattack = 0.16, oldrelease = 0.17, oldlowpass = 0, olddry = 1, oldwet = 0;
	bool oldmonitor = false;
	float crntsmpl = 0;

	float samplerate = 44100;
	EnvelopeFollower envelopefollower;
	std::vector<float*> channelData;
	dsp::StateVariableFilter::Filter<float> filter;

	AudioProcessorValueTreeState::ParameterLayout createParameters();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RedBassAudioProcessor)
};
