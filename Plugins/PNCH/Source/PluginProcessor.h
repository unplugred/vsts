#pragma once
#include "includes.h"

class PNCHAudioProcessor : public plugmachine_dsp {
public:
	PNCHAudioProcessor();
	~PNCHAudioProcessor() override;

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	float pnch(float source, float amountt);
	void setoversampling(bool toggle);

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
	const String get_preset(int preset_id, const char delimiter = ',') override;
	void set_preset(const String& preset, int preset_id, const char delimiter = ',', bool print_errors = false) override;

	virtual void parameterChanged(const String& parameterID, float newValue);

	AudioProcessorValueTreeState::ParameterLayout createParameters();
	AudioProcessorValueTreeState apvts;

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;

	int version = 2;
	SmoothedValue<float,ValueSmoothingTypes::Linear> amount;
	Atomic<bool> oversampling = 1;

private:
	bool preparedtoplay = false;
	bool saved = false;

	std::unique_ptr<dsp::Oversampling<float>> os;
	AudioBuffer<float> osbuffer;
	std::vector<float*> ospointerarray;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PNCHAudioProcessor)
};
