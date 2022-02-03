/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class PNCHAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	PNCHAudioProcessor();
	~PNCHAudioProcessor() override;

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	float pnch(float source, float amount);
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

	int version = 1;
	Atomic<float> amount = 0.f;
	Atomic<int> oversampling = 1;

private:
	float oldamount = 0.f;

	AudioProcessorValueTreeState::ParameterLayout createParameters();
	bool preparedtoplay = false;
	bool saved = false;

	std::unique_ptr<dsp::Oversampling<float>> os[3];

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PNCHAudioProcessor)
};
