#pragma once
#include "includes.h"
#include "CoolLogger.h"
#include "functions.h"

class VuAudioProcessor	: public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	VuAudioProcessor();
	~VuAudioProcessor() override;

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

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
	void setCurrentProgram(int index) override;
	const String getProgramName(int index) override;
	void changeProgramName(int index, const String& newName) override;

	void getStateInformation (MemoryBlock& destData) override;
	void setStateInformation (const void* data, int sizeInBytes) override;
	virtual void parameterChanged(const String& parameterID, float newValue);

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

	CoolLogger logger;
private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VuAudioProcessor)
};
