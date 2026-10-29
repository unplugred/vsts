#pragma once
#include "includes.h"
#include "midihandler.h"
#include "oscillator.h"
#include "curves.h"
#include "pages/000_general.h"
#include "pages/010_operator.h"
#include "pages/020_visualizer.h"
#include "pages/030_filter.h"
#include "pages/040_distortion.h"
#include "pages/050_phaser.h"
#include "pages/060_chorus.h"
#include "pages/070_equalizer.h"
#include "pages/080_compressor.h"
#include "pages/090_delay.h"
#include "pages/100_reverb.h"

struct pluginparams {
	Atomic<signed char> selectedtab = 3;
	Atomic<signed char> fxtab = 3;
	File tuningdir = File();
	File themedir = File();
	unsigned char theme[14] = "             ";
};

struct pluginpreset {
	String name = "";
	char fxorder[FXC] { 3,4,5,6,7,8,9,10 };
	short oppos[(MC+FXC+1)*2];
	signed char inputs   [MC+1][3];
	signed char outputs  [MC+1][3];
	float       influence[MC+1][3];
	pluginpreset(String pname = "") {
		name = pname;
	}
};

struct oscillator {
	void reset(int channelnum, int samplesperblock);
	std::vector<float> fb; // feedback  len =   c
	std::vector<float> p ; // phase     len =   c
	std::vector<float> a ; // amplitude len = s*c
	std::vector<float> f ; // frequency len = s
	std::vector<float> w ; // waveform  len = s
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

	void updateconnectors(signed char o, bool outputs, signed char* ids);

	void movepoint				(int page, int curve, int instance, int index, float x, float y);
	void movetension			(int page, int curve, int instance, int index, float tension);
	void addpoint				(int page, int curve, int instance, int index, float x, float y);
	void deletepoint			(int page, int curve, int instance, int index);
	const String curvetostring	(int page, int curve, int instance, const char delimiter = ',');
	void curvefromstring		(int page, int curve, int instance, String str, const char delimiter = ',');
	void resetcurve				(int page, int curve, int instance);
	Atomic<bool> updatevis = false;

	float indicators[MC+1];
	Atomic<bool> getframe = false;
	Atomic<bool> updatepreset = false;
	Atomic<float> framedata[VC];

	void resettuning();
	void updatetuning(File file);
	void resettheme();
	void updatetheme(File file);

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	int version = 0;

	pluginparams params;
	int currentpreset = 0;
	pluginpreset presets[20];

	char fxorder[FXC] { 3,4,5,6,7,8,9,10 };
	Atomic<bool> updatefxorder = true;
	Atomic<bool> updateconnections = true;

	int tabp[TABC] = { 0,2,3, 1,1,1,1, 1,1,1,1 };
	int tabo[TABC] = { 0,0,0, 0,1,2,3, 4,5,6,7 };
	FMPlusPage* pages[PC];
	FMPlusPageGeneral    page_gen;
	FMPlusPageOperator   page_op;
	FMPlusPageVisualizer page_vis;
	FMPlusPageFilter     page_filter;
	FMPlusPageDistortion page_dist;
	FMPlusPagePhaser     page_phaser;
	FMPlusPageChorus     page_chorus;
	FMPlusPageEqualizer  page_eq;
	FMPlusPageCompressor page_comp;
	FMPlusPageDelay      page_delay;
	FMPlusPageReverb     page_reverb;

private:
	bool preparedtoplay = false;
	bool saved = false;

	signed char oporder[MC];
	signed char voiceorder[VC];
	int opcount = 0;
	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;
	double bpm = 120;

	int osindex = 0;
	std::unique_ptr<dsp::Oversampling<float>> os[3]; // 2 4 8
	AudioBuffer<float> osbuffer;
	std::vector<float*> ospointerarray;

	midihandler midihandle;
	void updatearpspeed();

	void updatevibspeed();
	float vibphase = 0;
	float vibdelta = 0;
	float vibattack[VC];

	void updatea(int o);
	void updated(int o);
	void updater(int o);
	char egstage[MC*VC];
	float egprog[MC*VC];
	float eglast[MC*VC];
	float egsustain[MC*VC];
	float egd[MC*5];
	float ega[MC*5];

	void updatelfospeed(int o);
	void updatelfoattackspeed(int o);
	Random random;
	float lfoattack[MC*VC];
	float lfoattackdelta[MC];
	float lfophase[MC*VC];
	float lfophasedelta[MC];
	onepolevalue lfodamp;

	onepolevalue mutedamp;

	morphosc generator[MC];
	oscillator osc[MC][VC];

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusAudioProcessor)
};

static float sinapprox(float x) {
	return (x+x*x*x*(.5f*x*x-1.5f))*3.04f;
}
static float freqaddinflate(float value) {
	return (mapToLog10((float)fabs(value*2-1),1.f,10000.f)-1)*(round(value)*2-1);
};
static float freqaddnormalize(float value) {
	return mapFromLog10((float)fabs(value)+1,1.f,10000.f)*(value>=0?.5f:-.5f)+.5f;
};
static String format_time(float value, bool ms = false) {
	if(value <= .00005f)
		return "0ms";
	if(value < .00995f)
		return String(value*1000,1)+"ms";
	if(value < .9995f || ms)
		return (String)round(value*1000)+"ms";
	if(value < 9.995f)
		return String(value,2)+"s";
	if(value < 99.95f)
		return String(value,1)+"s";
	return (String)round(value)+"s";
};
//                             0     1      2      3       4      5      6      7     8      9      10    11     12    13    14     15
const String bpmsyncs_s  [13] {"Off","1/32","1/16","1/16d", "1/8","1/8t","1/8d","1/4","1/4t","1/4d","1/2","1/2d","1/1"};
const float  bpmsyncs_f  [13] {    0, .125f,  .25f,  .375f,   .5f, 2/3.f,  .75f,    1, 4/3.f,  1.5f,    2,     3,    4};
const String bpmsynclfo_s[16] {"Off", "1/8", "1/4", "1/4t","1/4d", "1/2","1/2d","1/1","1/1d", "2/1","3/1", "4/1","6/1","8/1","12/1","16/1"};
const float  bpmsynclfo_f[16] {    0,   .5f,     1,  4/3.f,  1.5f,     2,     3,    4,     6,     8,   12,    16,   24,   32,    48,    64};
static String get_string(int param, float value, int page) {
	  if(page == 0) {
		  if(param ==  0) {
			if(value == 1)
				return "Mono";
			return (String)value;
		} if(param ==  1) {
			if(value == 0)
				return "Off";
			return (String)value+"ST";
		} if(param ==  2) {
			if(value == 0)
				return "Off";
			return format_time(value*value*MAXGLIDE);
		} if(param ==  3) {
			return value>.5?"On":"Off";
		} if(param ==  4) {
			if(value == 0)
				return "Note";
			if(value == 1)
				return "Rand";
			if(value == 2)
				return "Free";
			if(value == 3)
				return "Trig";
		} if(param ==  6) {
			if(value == 0)
				return "Sequen";
			if(value == 1)
				return "Up";
			if(value == 2)
				return "Down";
			if(value == 3)
				return "U&D";
			if(value == 4)
				return "D&U";
			if(value == 5)
				return "Random";
		} if(param ==  7) {
			return (String)round(value);
		} if(param ==  8) {
			return (String)round(value*100)+'%';
		} if(param ==  9) {
			return format_time(value*value*(MAXARP-MINARP)+MINARP,true);
		} if(param == 12) {
			if(value == 0)
				return "Flutter";
			if(value == 1)
				return "Sine";
			if(value == 2)
				return "Square";
			if(value == 3)
				return "S&H";
		} if(param == 13) {
			return format_time(value*value*(MAXVIB-MINVIB)+MINVIB,true);
		} if(param == 11 || param == 14) {
			return bpmsyncs_s[(int)round(value)];
		} if(param == 15) {
			return (String)round(value*value*100)+'C';
		} if(param == 16) {
			return format_time(value*value*MAXVIBATT);
		} if(param == 17) {
			if(value < .5)
				return (String)floor(value*200)+'%';
			return (String)pow(2,fmin(3,fmax(0,floor(value*8-4))))+'x';
		}
	} if(page == 1) {
		  if(param ==  1) {
			if(round(value*200) == 100)
				return "0\%C";
			return (String)round(fabs(value*200-100))+'%'+(value<.5?'L':'R');
		} if(param ==  2) {
			return (String)round(value*200)+'%';
		} if(param ==  4 || param ==  5) {
			return (String)round(value*100)+'%';
		} if(param ==  6) {
			return value>.5?"*":"/";
		} if(param ==  7) {
			return String(value,2).substring(0,4);
		} if(param ==  8) {
			String s = String(fabs(freqaddinflate(value)),2).substring(0,4);
			if(value < .5) s = "-"+s;
			return s;
		} if(param ==  9) {
			return format_time(value*value*(MAXA-MINA)+MINA);
		} if(param == 10) {
			return format_time(value*value*(MAXD-MIND)+MIND);
		} if(param == 11) {
			return String(value,2);
		} if(param == 12) {
			return format_time(value*value*(MAXR-MINR)+MINR);
		} if(param == 14) {
			if(value == 0)
				return "Amp";
			if(value == 1)
				return "Pitch";
			if(value == 2)
				return "Pan";
			if(value == 3)
				return "Tone";
		} if(param == 15) {
			if(value >= 1)
				return "Fixed";
			return format_time(value*value*(MAXLFO-MINLFO)+MINLFO);
		} if(param == 16) {
			return bpmsynclfo_s[(int)round(value)];
		} if(param == 17) {
			return (String)round(value*200-100)+'%';
		} if(param == 18) {
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
static std::function<String(int v, int max)> topitchbend = [](int v, int max) {
	return get_string(1,v,0);
};
static std::function<int(const String& s)> frompitchbend = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toglide = [](float v, int max) {
	return get_string(2,v,0);
};
static std::function<float(const String& s)> fromglide = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> tolfosync = [](int v, int max) {
	return get_string(4,v,0);
};
static std::function<int(const String& s)> fromlfosync = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> toarpdirection = [](int v, int max) {
	return get_string(6,v,0);
};
static std::function<int(const String& s)> fromarpdirection = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> toarpoctave = [](int v, int max) {
	return get_string(7,v,0);
};
static std::function<int(const String& s)> fromarpoctave = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toarplength = [](float v, int max) {
	return get_string(8,v,0);
};
static std::function<float(const String& s)> fromarplength = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toarpspeed = [](float v, int max) {
	return get_string(9,v,0);
};
static std::function<float(const String& s)> fromarpspeed = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> toarpbpm = [](int v, int max) {
	return get_string(10,v,0);
};
static std::function<int(const String& s)> fromarpbpm = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> tovibratoshape = [](int v, int max) {
	return get_string(12,v,0);
};
static std::function<int(const String& s)> fromvibratoshape = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tovibratorate = [](float v, int max) {
	return get_string(13,v,0);
};
static std::function<float(const String& s)> fromvibratorate = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> tovibratobpm = [](int v, int max) {
	return get_string(14,v,0);
};
static std::function<int(const String& s)> fromvibratobpm = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tovibratoamount = [](float v, int max) {
	return get_string(15,v,0);
};
static std::function<float(const String& s)> fromvibratoamount = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tovibratoattack = [](float v, int max) {
	return get_string(16,v,0);
};
static std::function<float(const String& s)> fromvibratoattack = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toantialias = [](float v, int max) {
	return get_string(17,v,0);
};
static std::function<float(const String& s)> fromantialias = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> topan = [](float v, int max) {
	return get_string(1,v,1);
};
static std::function<float(const String& s)> frompan = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toamp = [](float v, int max) {
	return get_string(2,v,1);
};
static std::function<float(const String& s)> fromamp = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tovelocity = [](float v, int max) {
	return get_string(4,v,1);
};
static std::function<float(const String& s)> fromvelocity = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tomodat = [](float v, int max) {
	return get_string(5,v,1);
};
static std::function<float(const String& s)> frommodat = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(bool v, int max)> tofreqmode = [](bool v, int max) {
	return get_string(6,v,1);
};
static std::function<bool(const String& s)> fromfreqmode = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tofreqmult = [](float v, int max) {
	return get_string(7,v,1);
};
static std::function<float(const String& s)> fromfreqmult = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tofreqadd = [](float v, int max) {
	return get_string(8,v,1);
};
static std::function<float(const String& s)> fromfreqadd = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> toattack = [](float v, int max) {
	return get_string(9,v,1);
};
static std::function<float(const String& s)> fromattack = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> todecay = [](float v, int max) {
	return get_string(10,v,1);
};
static std::function<float(const String& s)> fromdecay = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> torelease = [](float v, int max) {
	return get_string(12,v,1);
};
static std::function<float(const String& s)> fromrelease = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> tolfotarget = [](int v, int max) {
	return get_string(14,v,1);
};
static std::function<int(const String& s)> fromlfotarget = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tolforate = [](float v, int max) {
	return get_string(15,v,1);
};
static std::function<float(const String& s)> fromlforate = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> tolfobpm = [](int v, int max) {
	return get_string(16,v,1);
};
static std::function<int(const String& s)> fromlfobpm = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tolfoinfluence = [](float v, int max) {
	return get_string(17,v,1);
};
static std::function<float(const String& s)> fromlfoinfluence = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(float v, int max)> tolfoattack = [](float v, int max) {
	return get_string(18,v,1);
};
static std::function<float(const String& s)> fromlfoattack = [](const String& s) { // TODO
	return jlimit(0.f,1.f,s.getFloatValue());
};
