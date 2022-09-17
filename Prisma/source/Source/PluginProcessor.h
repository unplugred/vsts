#pragma once
#include "includes.h"
#include "DCFilter.h"
#include "CoolLogger.h"

struct pluginmodule {
	SmoothedValue<float,ValueSmoothingTypes::Linear> value;
};

struct pluginband {
	pluginmodule modules[4];
	SmoothedValue<float,ValueSmoothingTypes::Linear> gain;
	SmoothedValue<float,ValueSmoothingTypes::Linear> bandactive;
	SmoothedValue<float,ValueSmoothingTypes::Linear> bandbypass;
	float activesmooth = 1;
	float bypasssmooth = 0;
	bool mute = false;
	bool solo = false;
	bool bypass = false;
};

struct pluginparams {
	pluginband bands[4];
	SmoothedValue<float,ValueSmoothingTypes::Linear> wet;
	bool oversampling = true;
	bool isb = false;
};

struct pluginpreset {
	String name = "";
	float values[4][4] = {
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0}};
	int id[4][4] = {
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0}};
	float crossover[3] {.25f,.5f,.75f};
	float gain[4] {.5f,.5f,.5f,.5f};
	float wet = 1;
};

class PrismaAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	PrismaAudioProcessor();
	~PrismaAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	void setoversampling(bool toggle);

	void pushNextSampleIntoFifo(float sample) noexcept;
	void drawNextFrameOfSpectrum(int fallfactor);

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
	double calcfilter(float val);

	void switchpreset(bool isbb);
	void copypreset(bool isbb);
	bool transition = false;
	
	void recalcactivebands();
 
	AudioProcessorValueTreeState apvts;
	UndoManager undoManager;

	int version = 0;
	pluginparams pots;
	CoolLogger logger;

	enum {
		fftOrder = 10,
		fftSize = 1 << fftOrder,
		scopeSize = 330
	};
	Atomic<bool> nextFFTBlockReady = false;
	Atomic<bool> dofft = false;
	float scopeData[scopeSize];
private:
	pluginpreset state[2];
	float crossovertruevalue[3];

	AudioProcessorValueTreeState::ParameterLayout createParameters();
	pluginpreset presets[20];
	int currentpreset = 0;
	bool preparedtoplay = false;
	bool saved = false;

	std::unique_ptr<dsp::Oversampling<float>> os;
	AudioBuffer<float> osbuffer;
	std::vector<float*> ospointerarray;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	DCFilter dcfilter;
	bool removedc[4] { false,false,false,false };

	dsp::LinkwitzRileyFilter<float> crossover[9];
	std::array<juce::AudioBuffer<float>,4> filterbuffers;
	std::array<juce::AudioBuffer<float>,4> wetbuffers;

	std::vector<dsp::StateVariableFilter::Filter<float>> modulefilters;

	std::vector<float> sampleandhold;
	float holdtime[16];

	std::vector<float> declick;
	float declickprogress[4] { 1,1,1,1 };

	float fifo[fftSize];
	float fftData[fftSize*2];
	int fifoIndex = 0;
	dsp::FFT forwardFFT;
	dsp::WindowingFunction<float> window;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismaAudioProcessor)
};
