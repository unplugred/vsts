#pragma once
#include "includes.h"
#include <future>

class MPaintAudioProcessor : public plugmachine_dsp {
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

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	int version = 3;

	Atomic<unsigned char> sound = 0;
	Atomic<bool> limit = true;

	Atomic<bool> error = false;
	Atomic<bool> loaded = false;

private:
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

static std::function<String(int v, int max)> tosound = [](int v, int max) {
	switch(v) {
		case 0:
			return (String)"mario";
			break;
		case 1:
			return (String)"mushroom";
			break;
		case 2:
			return (String)"yoshi";
			break;
		case 3:
			return (String)"star";
			break;
		case 4:
			return (String)"flower";
			break;
		case 5:
			return (String)"gameboy";
			break;
		case 6:
			return (String)"dog";
			break;
		case 7:
			return (String)"cat";
			break;
		case 8:
			return (String)"pig";
			break;
		case 9:
			return (String)"duck";
			break;
		case 10:
			return (String)"face";
			break;
		case 11:
			return (String)"plane";
			break;
		case 12:
			return (String)"ship";
			break;
		case 13:
			return (String)"car";
			break;
		case 14:
			return (String)"heart";
			break;
	}
	return String(v);
};
/*
00 MArio
01 MUshroom tOAD
02 Yoshi
03 STar
04 FLower
05 GAmeboy COnsole
06 DOg
07 CAT
08 PIg
09 DUck SWan
10 FAce CHild
11 PLane
12 shIP BOAt
13 CAR Vehicle
14 HEart
*/
static std::function<int(const String& s)> fromsound = [](const String& s) {
	String lower = s.toLowerCase();
	if(lower.startsWith("h") || lower.contains("he"))
		return 14;
	if(lower.contains("car") || lower.contains("v"))
		return 13;
	if(lower.startsWith("sh") || lower.contains("ip") || lower.startsWith("b") || lower.contains("boa"))
		return 12;
	if(lower.contains("pl"))
		return 11;
	if(lower.contains("fa") || lower.contains("ch"))
		return 10;
	if(lower.contains("du") || lower.contains("sw"))
		return 9;
	if(lower.contains("p"))
		return 8;
	if(lower.contains("ca"))
		return 7;
	if(lower.contains("do"))
		return 6;
	if(lower.contains("g") || lower.contains("c"))
		return 5;
	if(lower.contains("f"))
		return 4;
	if(lower.startsWith("s") || lower.contains("st"))
		return 3;
	if(lower.contains("y"))
		return 2;
	if(lower.contains("mu") || lower.contains("t"))
		return 1;
	if(lower.contains("m"))
		return 0;
	return jlimit(0,14,s.getIntValue());
};
static std::function<String(bool v, int max)> tobool = [](bool v, int max) {
	return v?"on":"off";
};
static std::function<bool(const String& s)> frombool = [](const String& s) {
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return true;
};
