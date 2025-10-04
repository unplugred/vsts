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
	SmoothedValue<float,ValueSmoothingTypes::Linear> smooth[MC];
	float smoothtime = 0;
	potentiometer(String potname = "", String potid = "", float smoothed = 0, float potdefault = 0.f, float potmin = 0.f, float potmax = 1.f, ptype pottype = ptype::floattype) {
		name = potname;
		id = potid;
		smoothtime = smoothed;
		if(smoothed > 0) for(int o = 0; o < MC; ++o)
			smooth[o].setCurrentAndTargetValue(defaultvalue);
		defaultvalue = potdefault;
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
	potentiometer general[15];
	potentiometer values[19];
	float antialiasing = .7f; // 0.0 aa, 0.5 1x, 0.625 2x, 0.75 4x, 0.875 8x
	Atomic<int> selectedtab = 3;
	bool presetunsaved = false;
	String tuningfile = "Standard";
	String themefile = "Default";
	float theme[9*3];
};

struct connection {
	int input = 0;
	int output = 0;
	float influence = .5f;
	connection(int pinput, int poutput, float pinfluence) {
		input = pinput;
		output = poutput;
		influence = pinfluence;
	}
};
struct pluginpreset {
	String name = "";
	float general[16];
	float values[MC][19];
	int oppos[(MC+1)*2];
	std::vector<connection> opconnections[9];
	//curve curves[MC]; TODO
	pluginpreset(String pname = "") {
		name = pname;
	}
};

class FMPlusAudioProcessor : public plugmachine_dsp {
public:
	FMPlusAudioProcessor();
	~FMPlusAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
	void setoversampling();

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

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	Atomic<float> rmsadd = 0;
	Atomic<int> rmscount = 0;

	int version = 0;
	const int generalcount = 15;
	const int paramcount = 19;

	pluginpreset state;
	pluginparams params;
	int currentpreset = 0;
	pluginpreset presets[20];

private:
	bool preparedtoplay = false;
	bool saved = false;

	int osindex = 0;
	std::unique_ptr<dsp::Oversampling<float>> os[3]; // 2 4 8
	AudioBuffer<float> osbuffer;
	std::vector<float*> ospointerarray;

	float pitches[128];

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusAudioProcessor)
};

static String format_time(float value, bool ms = false) {
	if(time) {
		     if(value <= .00005f)
			return "0ms";
		else if(value < .00995f)
			return String(value*1000,1)+"ms";
		else if(value < .9995f || ms)
			return (String)round(value*1000)+"ms";
		else if(value < 9.995f)
			return String(value,2)+"s";
		else if(value < 99.95f)
			return String(value,1)+"s";
		else
			return (String)round(value)+"s";
	}
	return "meow";
};
//                            0      1        2       3        4      5       6      7      8      9      10     11     12     13     14     15     16     17      18
const String bpmsyncs_s[19] { "Off",  "1/32", "1/16",  "3/32", "1/8", "3/16", "1/4", "3/8", "1/2", "3/4", "1/1", "3/2", "2/1", "3/1", "4/1", "6/1", "8/1", "12/1", "16/1" };
const float  bpmsyncs_f[19] {     0, .03125f, .0625f, .09375f, .125f, .1875f,  .25f, .375f,   .5f,  .75f,     1,  1.5f,     2,     3,     4,     6,     8,     12,     16 };
static String get_string(int param, float value, int tab) {
	if(tab == 0) {
		       if(param ==  0) {
			if(value == 1)
				return "Mono";
			else
				return (String)value;
		} else if(param ==  1) {
			return format_time(value*value*MAXPORT);
		} else if(param ==  2) {
			return value>.5?"On":"Off";
		} else if(param ==  3) {
			     if(value == 0)
				return "Note";
			else if(value == 1)
				return "Rand";
			else if(value == 2)
				return "Uni";
			else if(value == 3)
				return "Trig";
		} else if(param ==  4) {
			if(value == 0)
				return "Off";
			else
				return (String)value+"ST";
		} else if(param ==  6) {
			     if(value == 0)
				return "Up";
			else if(value == 1)
				return "Down";
			else if(value == 2)
				return "U&D";
			else if(value == 3)
				return "D&U";
			else if(value == 4)
				return "Rand";
		} else if(param ==  7) {
			return (String)round(value*100)+'%';
		} else if(param ==  8) {
			return format_time(value*value*MAXARP,true);
		} else if(param ==  9 || param == 12) {
			return bpmsyncs_s[(int)round(value)];
		} else if(param == 11) {
			return format_time(value*value*MAXVIB,true);
		} else if(param == 13) {
			return (String)round(pow(value,2)*100)+'C';
		} else if(param == 14) {
			return format_time(value*value*MAXVIBATT);
		} else if(param == 15) {
			if(value < .5)
				return (String)floor(value*200)+'%';
			else
				return (String)pow(2,fmin(3,fmax(0,floor(value*8-4))))+'x';
		}
	} else if(tab >= 3) {
		       if(param ==  1) {
			if(round(value*200) == 100)
				return "0\%C";
			else
				return (String)round(fabs(value*200-100))+'%'+(value<.5?'L':'R');
		} else if(param ==  2) {
			return (String)round(value*200)+'%';
		} else if(param ==  4 || param ==  5) {
			return (String)round(value*100)+'%';
		} else if(param ==  6) {
			return value>.5?"*":"/";
		} else if(param ==  7) {
			return String(value,2).substring(0,4);
		} else if(param ==  8) {
			String s = String(mapToLog10((float)fabs(value*2-1),1.f,10000.f)-1,2).substring(0,4);
			if(value < .5) s = "-"+s;
			return s;
		} else if(param ==  9) {
			return format_time(value*value*MAXA);
		} else if(param == 10) {
			return format_time(value*value*MAXD);
		} else if(param == 11) {
			return String(value,2);
		} else if(param == 12) {
			return format_time(value*value*MAXR);
		} else if(param == 14) {
			     if(value == 0)
				return "Amp";
			else if(value == 1)
				return "Pitch";
			else if(value == 2)
				return "Pan";
			else if(value == 3)
				return "Tone";
		} else if(param == 15) {
			return format_time(value*value*MAXLFO);
		} else if(param == 16) {
			return bpmsyncs_s[value==0?0:(value==1?4:((int)round(value)+4))];
		} else if(param == 17) {
			return (String)round(value*100)+'%';
		} else if(param == 18) {
			return format_time(value*value*MAXLFOATT);
		}
	}
	return "meow";
};

static std::function<String(float v, int max)> tonormalized = [](float v, int max) {
	return String(v,3);
};
static std::function<float(const String& s)> fromnormalized = [](const String& s) {
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(bool v, int max)> tobool = [](bool v, int max) {
	return get_string(2,v,0);
};
static std::function<bool(const String& s)> frombool = [](const String& s) {
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return false;
};
static std::function<String(float v, int max)> toportamento = [](float v, int max) {
	return get_string(1,v,0);
};
static std::function<float(const String& s)> fromportamento = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> tolfosync = [](int v, int max) {
	return get_string(3,v,0);
};
static std::function<int(const String& s)> fromlfosync = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> topitchbend = [](int v, int max) {
	return get_string(4,v,0);
};
static std::function<int(const String& s)> frompitchbend = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> toarpdirection = [](int v, int max) {
	return get_string(6,v,0);
};
static std::function<int(const String& s)> fromarpdirection = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toarplength = [](float v, int max) {
	return get_string(7,v,0);
};
static std::function<float(const String& s)> fromarplength = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toarpspeed = [](float v, int max) {
	return get_string(8,v,0);
};
static std::function<float(const String& s)> fromarpspeed = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> toarpbpm = [](int v, int max) {
	return get_string(9,v,0);
};
static std::function<int(const String& s)> fromarpbpm = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tovibratorate = [](float v, int max) {
	return get_string(11,v,0);
};
static std::function<float(const String& s)> fromvibratorate = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> tovibratobpm = [](int v, int max) {
	return get_string(12,v,0);
};
static std::function<int(const String& s)> fromvibratobpm = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tovibratoamount = [](float v, int max) {
	return get_string(13,v,0);
};
static std::function<float(const String& s)> fromvibratoamount = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tovibratoattack = [](float v, int max) {
	return get_string(14,v,0);
};
static std::function<float(const String& s)> fromvibratoattack = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toantialias = [](float v, int max) {
	return get_string(15,v,0);
};
static std::function<float(const String& s)> fromantialias = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> topan = [](float v, int max) {
	return get_string(1,v,3);
};
static std::function<float(const String& s)> frompan = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toamp = [](float v, int max) {
	return get_string(2,v,3);
};
static std::function<float(const String& s)> fromamp = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tovelocity = [](float v, int max) {
	return get_string(4,v,3);
};
static std::function<float(const String& s)> fromvelocity = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toaftertouch = [](float v, int max) {
	return get_string(5,v,3);
};
static std::function<float(const String& s)> fromaftertouch = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(bool v, int max)> tofreqmode = [](bool v, int max) {
	return get_string(6,v,3);
};
static std::function<bool(const String& s)> fromfreqmode = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tofreqmult = [](float v, int max) {
	return get_string(7,v,3);
};
static std::function<float(const String& s)> fromfreqmult = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tofreqadd = [](float v, int max) {
	return get_string(8,v,3);
};
static std::function<float(const String& s)> fromfreqadd = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toattack = [](float v, int max) {
	return get_string(9,v,3);
};
static std::function<float(const String& s)> fromattack = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> todecay = [](float v, int max) {
	return get_string(10,v,3);
};
static std::function<float(const String& s)> fromdecay = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> torelease = [](float v, int max) {
	return get_string(12,v,3);
};
static std::function<float(const String& s)> fromrelease = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> tolfotarget = [](int v, int max) {
	return get_string(14,v,3);
};
static std::function<int(const String& s)> fromlfotarget = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tolforate = [](float v, int max) {
	return get_string(15,v,3);
};
static std::function<float(const String& s)> fromlforate = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> tolfobpm = [](int v, int max) {
	return get_string(16,v,3);
};
static std::function<int(const String& s)> fromlfobpm = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tolfoamount = [](float v, int max) {
	return get_string(17,v,3);
};
static std::function<float(const String& s)> fromlfoamount = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tolfoattack = [](float v, int max) {
	return get_string(18,v,3);
};
static std::function<float(const String& s)> fromlfoattack = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
