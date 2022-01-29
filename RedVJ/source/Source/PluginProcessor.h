/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;

struct inparameter {
	String name = "";
	float value = .5f;
	String midimapping = "";
};
class RedVJAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener
{
public:
	RedVJAudioProcessor();
	~RedVJAudioProcessor() override;

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	void pushNextSampleIntoFifo(float sample) noexcept;
	void drawNextFrameOfSpectrum();

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

	void parameterChanged(const String& parameterID, float newValue);

	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;

	int version = 1;

	String presetpath = "NULL";
	inparameter inparameters[16];

	enum {
		fftOrder = 11,
		fftSize = 1 << fftOrder,
		scopeSize = 512
	};
	float fifo[fftSize];
	float fftData[2*fftSize];
	int fifoIndex = 0;
	Atomic<bool> nextFFTBlockReady = false;
	float scopeData[scopeSize];
	int sampleRateSizeMaxInv;
	Atomic<bool> dofft = false;

private:
	dsp::FFT forwardFFT;
	dsp::WindowingFunction<float> window;

	AudioProcessorValueTreeState::ParameterLayout createParameters();
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RedVJAudioProcessor)
};
