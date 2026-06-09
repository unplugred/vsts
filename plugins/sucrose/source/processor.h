#pragma once
#include "includes.h"
#include "dsp/core.h"

struct potentiometer
{
public:
	enum ptype
	{
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
	SmoothedValue<float, ValueSmoothingTypes::Linear> smooth;
	float smoothtime = 0;
	potentiometer(String potname = "", String potid = "", float smoothed = 0, float potdefault = 0.f, float potmin = 0.f, float potmax = 1.f, ptype pottype = ptype::floattype)
	{
		name = potname;
		id = potid;
		smoothtime = smoothed;
		if (smoothed > 0)
			smooth.setCurrentAndTargetValue(defaultvalue);
		defaultvalue = potdefault;
		minimumvalue = potmin;
		maximumvalue = potmax;
		ttype = pottype;
	}
	float normalize(float val)
	{
		return (val - minimumvalue) / (maximumvalue - minimumvalue);
	}
	float inflate(float val)
	{
		return val * (maximumvalue - minimumvalue) + minimumvalue;
	}
};
struct pluginparams
{
	potentiometer pots[7];
};

struct pluginpreset
{
	String name = "";
	float values[7];
	pluginpreset(String pname = "", float val1 = 0.f, float val2 = .5f, float val3 = 0.f, float val4 = 0.f, float val5 = 0.f, float val6 = 0.f, int val7 = 1)
	{
		name = pname;
		values[0] = val1;
		values[1] = val2;
		values[2] = val3;
		values[3] = val4;
		values[4] = val5;
		values[5] = val6;
		values[6] = val7;
	}
};

class SucroseAudioProcessor : public plugmachine_dsp
{
public:
	SucroseAudioProcessor();
	~SucroseAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

	void processBlock(AudioBuffer<float> &, MidiBuffer &) override;

	AudioProcessorEditor *createEditor() override;
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
	void changeProgramName(int index, const String &newName) override;

	void getStateInformation(MemoryBlock &destData) override;
	void setStateInformation(const void *data, int sizeInBytes) override;
	const String get_preset(int preset_id, const char delimiter = ',') override;
	void set_preset(const String &preset, int preset_id, const char delimiter = ',', bool print_errors = false) override;

	void randomize();

	virtual void parameterChanged(const String &parameterID, float newValue);

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	int version = 0;
	const int paramcount = 7;

	pluginpreset state;
	pluginparams params;
	int currentpreset = 0;

private:
	pluginpreset presets[20];

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	DspEngine dsp;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SucroseAudioProcessor)
};
static float tolc(float v)
{
	return mapToLog10((double)v, 20.0, 8000.0);
};
static float fromlc(float v)
{
	return mapFromLog10(v, 20.f, 8000.f);
};
static float tohc(float v)
{
	return mapToLog10((double)v, 50.0, 20000.0);
};
static float fromhc(float v)
{
	return mapFromLog10(v, 50.f, 20000.f);
};
static float togain(float v)
{
	return v * v * 4.f;
};
static float fromgain(float v)
{
	return (float)sqrt(v * .25f);
};
static float todb(float v)
{
	return Decibels::gainToDecibels(fmax(.000001f, togain(v)));
};
static float fromdb(float v)
{
	return fromgain(Decibels::decibelsToGain(v));
};
static std::function<String(float v, int max)> stolc = [](float v, int max)
{
	if (v <= 0)
		return (String) "off";
	return String(round(tolc(v))) + "hz";
};
static std::function<float(const String &s)> sfromlc = [](const String &s)
{
	if (s.containsIgnoreCase("f"))
		return 0.f;
	float val = s.getFloatValue();
	if (!s.containsIgnoreCase("k") && !s.containsIgnoreCase("hz") && val <= 1)
		return jlimit(0.f, 1.f, val);
	if ((s.containsIgnoreCase("k")) || (!s.containsIgnoreCase("hz") && val < 20))
		val *= 1000;
	return jlimit(0.f, 1.f, fromlc(val));
};
static std::function<String(float v, int max)> stohc = [](float v, int max)
{
	if (v >= 1)
		return (String) "off";
	return String(round(tohc(v))) + "hz";
};
static std::function<float(const String &s)> sfromhc = [](const String &s)
{
	if (s.containsIgnoreCase("f"))
		return 1.f;
	float val = s.getFloatValue();
	if (!s.containsIgnoreCase("k") && !s.containsIgnoreCase("hz") && val <= 1)
		return jlimit(0.f, 1.f, val);
	if ((s.containsIgnoreCase("k")) || (!s.containsIgnoreCase("hz") && val < 20))
		val *= 1000;
	return jlimit(0.f, 1.f, fromhc(val));
};
static std::function<String(float v, int max)> stodb = [](float v, int max)
{
	float val = todb(v);
	if (val <= -96)
		return (String) "off";
	return String(val, 2) + "db";
};
static std::function<float(const String &s)> sfromdb = [](const String &s)
{
	if (s.containsIgnoreCase("f"))
		return 0.f;
	float val = s.getFloatValue();
	if (s.containsIgnoreCase("d") || val < 0.f || val > 1.f)
		return jlimit(0.f, 1.f, fromdb(val));
	return jlimit(0.f, 1.f, val);
};
static std::function<String(int v, int max)> toalgo = [](int v, int max)
{
	if (v == 0)
		return "dirty";
	if (v == 1)
		return "clean8";
	return "clean16";
};
static std::function<int(const String &s)> fromalgo = [](const String &s)
{
	if (s.containsIgnoreCase("d"))
		return 0;
	if (s.containsIgnoreCase("8"))
		return 1;
	if (s.containsIgnoreCase("1"))
		return 2;
	return 1;
};
