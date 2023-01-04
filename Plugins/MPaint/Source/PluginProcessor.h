#pragma once
#include "includes.h"
#include <future>

class MPaintAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	MPaintAudioProcessor();
	~MPaintAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
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

	Atomic<bool> error = false;
	Atomic<bool> loaded = false;

private:
	AudioProcessorValueTreeState::ParameterLayout createParameters();

	Synthesiser synth[15];
	const int numvoices = 8;
	AudioFormatManager formatmanager;

	int prevtime = -100000;
	int voicecycle[3] {0,0,0};
	bool voiceheld[3] {false,false,false};
	int currentvoice = 0;
	unsigned char prevsound = 0;
	int timesincesoundswitch = -100000;
	int timetillnoteplay = -100000;

	int samplerate = 44100;

	std::future<void> futurevoid;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPaintAudioProcessor)
};
