/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "functions.h"
using namespace juce;

class VuAudioProcessor	: public AudioProcessor
{
public:
	VuAudioProcessor();
	//static void crashhandler(void*);
	~VuAudioProcessor() override;

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

	Atomic<bool> stereo = false;
	Atomic<int> damping = 5;
	Atomic<int> nominal = -18;

	Atomic<float> leftvu = 0;
	Atomic<float> rightvu = 0;
	Atomic<bool> leftpeak = false;
	Atomic<bool> rightpeak = false;
	Atomic<int> buffercount = 0;

	Atomic<int> width = 505;
	Atomic<int> height = 300;

	AudioProcessorValueTreeState apvts;
	AudioProcessorValueTreeState::ParameterLayout createParameters();
	UndoManager undoManager;
	int version = 1;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VuAudioProcessor)
};
