#pragma once
#include "includes.h"
#include "perlin.h"
#include "curves.h"

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
	ptype ttype = ptype::floattype;
	potentiometer(String potname = "", String potid = "", float smoothed = 0, float potmin = 0.f, float potmax = 1.f, ptype pottype = ptype::floattype) {
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
struct modulatorparams {
	String name = "";
	int id = 0;
	float defaults[5];
	modulatorparams() {}
};
struct pluginparams {
	potentiometer pots[6];
	modulatorparams modulators[MC];
	Atomic<int> selectedmodulator = 0;
};

struct pluginpreset {
	String name = "";
	curve curves[MC];
	float values[MC][5];
	float masterspeed = .5f;
	pluginpreset() {}
};

class ModManAudioProcessor : public plugmachine_dsp {
public:
	ModManAudioProcessor();
	~ModManAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
	float interpolatesamples(float* buffer, float position, int buffersize);

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

	virtual void parameterChanged(const String& parameterID, float newValue);
	float calccutoff(float val);
	float calcresonance(float val);

	void movepoint(int index, float x, float y);
	void movetension(int index, float tension);
	void addpoint(int index, float x, float y);
	void deletepoint(int index);
	const String curvetostring(const char delimiter = ',');
	void curvefromstring(String str, const char delimiter = ',');
	void resetcurve();
 
	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	Atomic<float> flower_rot[MC];
	Atomic<float> cuber_rot[2];
	float driftmult = 1;

	int version = 0;
	const int paramcount = 6;

	pluginpreset state;
	pluginpreset presets[20];
	int currentpreset = 0;
	pluginparams params;

	Atomic<bool> updatevis = false;
	Atomic<int> updatedcurve = 1+2+4+8+16;

private:

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	perlin prlin;
	double time[MC];
	std::vector<SmoothedValue<float,ValueSmoothingTypes::Linear>> smooth;
	bool resetsmooth = true;
	std::vector<float> modulator_data;
	bool ison[MC];

	std::vector<float> drift_data;
	int driftindex = 0;

	dsp::StateVariableTPTFilter<float> lowpass;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModManAudioProcessor)
};
static std::function<String(float v, int max)> toms = [](float v, int max) {
	return String(floor(v*v*MAX_DRIFT*1000))+"ms";
};
static std::function<float(const String& s)> fromms = [](const String& s) {
	float val = s.getFloatValue();
	if(s.containsIgnoreCase("ms") || val > 1)
		val /= MAX_DRIFT*1000;
	else if(s.containsIgnoreCase("s"))
		val /= MAX_DRIFT;
	return jlimit(0.f,1.f,(float)sqrt(val));
};
static std::function<String(float v, int max)> tocutoff = [](float v, int max) {
	return String(round(mapToLog10(v,20.f,20000.f)))+"hz";
};
static std::function<float(const String& s)> fromcutoff = [](const String& s) {
	float val = s.getFloatValue();
	if((s.containsIgnoreCase("k")) || (!s.containsIgnoreCase("hz") && val < 20))
		val *= 1000;
	return jlimit(0.f,1.f,mapFromLog10(val,20.f,20000.f));
};
static std::function<String(float v, int max)> toresonance = [](float v, int max) {
	return String(mapToLog10(v,.1f,40.f),1);
};
static std::function<float(const String& s)> fromresonance = [](const String& s) {
	return jlimit(0.f,1.f,mapFromLog10(s.getFloatValue(),.1f,40.f));
};
static std::function<String(bool v, int max)> tobool = [](bool v, int max) {
	return (String)(v?"on":"off");
};
static std::function<bool(const String& s)> frombool = [](const String& s) {
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return true;
};
static std::function<String(float v, int max)> tonormalized = [](float v, int max) {
	return String(v,3);
};
static std::function<float(const String& s)> fromnormalized = [](const String& s) {
	return jlimit(0.f,1.f,s.getFloatValue());
};
