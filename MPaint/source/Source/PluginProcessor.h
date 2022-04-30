#pragma once
#include <JuceHeader.h>
#include "CoolLogger.h"

class MPaintAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	MPaintAudioProcessor();
	~MPaintAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

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
	CoolLogger logger;
	int version = 3;

	Atomic<unsigned char> sound = 0;
	Atomic<bool> limit = true;

private:
	AudioProcessorValueTreeState::ParameterLayout createParameters();

	Synthesiser synth[15];
	const int numvoices = 3;
	AudioFormatManager formatmanager;
	AudioFormatReader* formatreader = nullptr;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;
	std::vector<float*> channelData;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPaintAudioProcessor)
};
