/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "perlin.h"
using namespace juce;

class ClickBoxAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	ClickBoxAudioProcessor();
	~ClickBoxAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
	float generateclick();

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
	virtual void parameterChanged(const String& parameterID, float newValue);

	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;
	
	Atomic<float> x = .5f;
	Atomic<float> y = .5f;
	Atomic<float> oldi = .0f;

	int version = 1;
private:
	float intensity = .5f, amount = .5f, stereo = .28f, automod = 0;
	bool sidechain = false, dry = true;
	float oldintensity = 0.f, oldstereo = .28f, olddry = 0, oldautomod = 0;
	bool oldoverride = true, overridee = false;

	Random random;
	perlin prlin;
	float xval = .5;
	float yval = .5;
	float time = 0;

	AudioProcessorValueTreeState::ParameterLayout createParameters();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClickBoxAudioProcessor)
};
