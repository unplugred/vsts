#pragma once
#include "includes.h"

struct potentiometer {
public:
	enum ptype {
		floattype = 0,
		inttype = 1,
		booltype = 2
	};
	String name = "";
	String id = "";
	float minimumvalue = 0;
	float maximumvalue = 1;
	float defaultvalue = 0;
	ptype ttype = ptype::floattype;
	potentiometer(String potname = "", String potid = "", float potdefault = 0.f, float potmin = 0.f, float potmax = 1.f, ptype pottype = ptype::floattype) {
		name = potname;
		id = potid;
		minimumvalue = potmin;
		maximumvalue = potmax;
		ttype = pottype;
	}
	float normalize(float val) {
		return (val-minimumvalue)/(maximumvalue-minimumvalue);
	}
	float inflate(float val) {
		return val*(maximumvalue-minimumvalue)+minimumvalue;
	}
};
struct pluginparams {
	potentiometer pots[4];
};

struct pluginpreset {
	float values[4];
	String text = "";
	pluginpreset(String str = "", float val1 = 0.f, float val2 = 0.f, float val3 = .45f, float val4 = 2.f) {
		text = str;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
		values[3] = val4;
	}
};

class MorseGateAudioProcessor : public plugmachine_dsp {
public:
	MorseGateAudioProcessor();
	~MorseGateAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
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
	const String get_preset(int preset_id, const char delimiter = ',') override;
	void set_preset(const String& preset, int preset_id, const char delimiter = ',', bool print_errors = false) override;
	void set_text(const String& text);

	virtual void parameterChanged(const String& parameterID, float newValue);

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	Atomic<bool> uiison = false;
	Atomic<int > uichar = 0;

	int version = 0;
	const int paramcount = 4;

	pluginpreset state;
	pluginparams params;
	int currentpreset = 0;

private:
	pluginpreset presets[20];

	bool lastretrigger = true;
	float lastbpm = 120;
	float lastppq = -10;

	float crntsmpl = 0;

	String prevtext = "`";
	std::vector<char> processedtext;
	int letterindex = 0;
	int currentletter = 0;
	int elementindex = 0;
	int currentelement = 2;
	int unitindex = 0;
	int currentunit = 0;
	float unitprogress = 0;
	float gainlerp = 0;

	// 0 - off
	// 1 - transition
	// 2 - on
	char elements[3][3] {
		{ 1, 1,0 }, // 0 - dit
		{ 2, 2,1 }, // 1 - dah
		{ 1, 0,0 }  // 2 - space
	};
	char letters[37][7] {
		{ 3, 2,2,2,2,2,2 }, // space

		{ 6, 1,1,1,1,1,2 }, // 0
		{ 6, 0,1,1,1,1,2 }, // 1
		{ 6, 0,0,1,1,1,2 }, // 2
		{ 6, 0,0,0,1,1,2 }, // 3
		{ 6, 0,0,0,0,1,2 }, // 4
		{ 6, 0,0,0,0,0,2 }, // 5
		{ 6, 1,0,0,0,0,2 }, // 6
		{ 6, 1,1,0,0,0,2 }, // 7
		{ 6, 1,1,1,0,0,2 }, // 8
		{ 6, 1,1,1,1,0,2 }, // 9

		{ 3, 0,1,2,2,2,2 }, // a
		{ 5, 1,0,0,0,2,2 }, // b
		{ 5, 1,0,1,0,2,2 }, // c
		{ 4, 1,0,0,2,2,2 }, // d
		{ 2, 0,2,2,2,2,2 }, // e
		{ 5, 0,0,1,0,2,2 }, // f
		{ 4, 1,1,0,2,2,2 }, // g
		{ 5, 0,0,0,0,2,2 }, // h
		{ 3, 0,0,2,2,2,2 }, // i
		{ 5, 0,1,1,1,2,2 }, // j
		{ 4, 1,0,1,2,2,2 }, // k
		{ 5, 0,1,0,0,2,2 }, // l
		{ 3, 1,1,2,2,2,2 }, // m
		{ 3, 1,0,2,2,2,2 }, // n
		{ 4, 1,1,1,2,2,2 }, // o
		{ 5, 0,1,1,0,2,2 }, // p
		{ 5, 1,1,0,1,2,2 }, // q
		{ 4, 0,1,0,2,2,2 }, // r
		{ 4, 0,0,0,2,2,2 }, // s
		{ 2, 1,2,2,2,2,2 }, // t
		{ 4, 0,0,1,2,2,2 }, // u
		{ 5, 0,0,0,1,2,2 }, // v
		{ 4, 0,1,1,2,2,2 }, // w
		{ 5, 1,0,0,1,2,2 }, // x
		{ 5, 1,0,1,1,2,2 }, // y
		{ 5, 1,1,0,0,2,2 }  // z
	};

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorseGateAudioProcessor)
};
static std::function<String(bool v, int max)> toretrigger = [](bool v, int max) {
	return (v?"on":"off");
};
static std::function<bool(const String& s)> fromretrigger = [](const String& s) {
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return false;
};
static std::function<String(float v, int max)> torelease = [](float v, int max) {
	return String(v*100,1)+"%";
};
static std::function<float(const String& s)> fromrelease = [](const String& s) {
	return jlimit(0.f,1.f,s.getFloatValue()/100.f);
};
static std::function<String(float v, int max)> tolength = [](float v, int max) {
	return String(round((pow(v,2)*(.5f-.05f)+.05f)*1000))+"ms";
};
static std::function<float(const String& s)> fromlength = [](const String& s) {
	float val = s.getFloatValue();
	if(!s.containsIgnoreCase("s") && val <= 1)
		return jlimit(0.f,1.f,val);
	if((s.containsIgnoreCase("s") && !s.containsIgnoreCase("ms")) || (!s.containsIgnoreCase("s") && val <= .5f))
		val *= 1000;
	return jlimit(0.f,1.f,(float)sqrt((val*.001f-.05f)/(.5f-.05f)));
};
static std::function<String(int v, int max)> tobpm = [](int v, int max) {
	if(v == 0) return (String)"off";
	return (String)"1/"+(String)powf(2,6-v);
};
static std::function<int(const String& s)> frombpm = [](const String& s) {
	if(s.containsIgnoreCase("f"))
		return 0;
	int val = 0;
	if(s.containsChar('/'))
		val = s.substring(1+s.lastIndexOfChar('/' )).getIntValue();
	else if(s.containsChar('\\'))
		val = s.substring(1+s.lastIndexOfChar('\\')).getIntValue();
	else
		val = s.getIntValue();
	if(val >= 4) val = 6-log2(val);
	return jlimit(0,4,val);
};
