/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "CoolLogger.h"

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
	float pnch(float source, float amountt);
	void setoversampling(bool toggle);

	AudioProcessorEditor* createEditor() override;
	void changechannelnum(int newchannelnum);
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

	int version = 2;
	float amount = 0.f;
	Atomic<bool> oversampling = 1;

	CoolLogger logger;
private:
	AudioProcessorValueTreeState::ParameterLayout createParameters();
	bool preparedtoplay = false;
	bool saved = false;

	std::unique_ptr<dsp::Oversampling<float>> os;
	AudioBuffer<float> osbuffer;
	std::vector<float*> ospointerarray;

	int channelnum = 0;
	int samplesperblock = 512;
	int samplerate = 44100;
	std::vector<float*> channelData;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PNCHAudioProcessor)
};
