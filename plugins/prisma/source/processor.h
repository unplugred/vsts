#pragma once
#include "includes.h"
#include "dc_filter.h"

struct pluginmodule {
	SmoothedValue<float,ValueSmoothingTypes::Linear> value;
	SmoothedValue<float,ValueSmoothingTypes::Linear> valuey;
};

struct pluginband {
	pluginmodule modules[MAX_MOD];
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
	pluginband bands[BAND_COUNT];
	SmoothedValue<float,ValueSmoothingTypes::Linear> wet;
	bool oversampling = true;
	bool isb = false;
};

struct pluginpreset {
	String name = "";
	int id[BAND_COUNT][MAX_MOD];
	float values[BAND_COUNT][MAX_MOD];
	float valuesy[BAND_COUNT][MAX_MOD];
	float crossover[BAND_COUNT==1?1:BAND_COUNT-1];
	float gain[BAND_COUNT];
	float wet = 1;
	int modulecount = DEF_MOD;
	pluginpreset() {
		for(int b = 0; b < BAND_COUNT; ++b) {
			for(int m = 0; m < MAX_MOD; ++m) {
				id[b][m] = 0;
				values[b][m] = 0;
				valuesy[b][m] = 0;
			}
			if(b >= 1) crossover[b-1] = ((float)b)/BAND_COUNT;
			gain[b] = .5f;
		}
	}
};

class PrismaAudioProcessor : public plugmachine_dsp {
public:
	PrismaAudioProcessor();
	~PrismaAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void changechannelnum(int newchannelnum);
	void reseteverything();
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
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
	void setCurrentProgram(int index) override;
	const String getProgramName(int index) override;
	void changeProgramName(int index, const String& newName) override;

	void getStateInformation(MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;
	const String get_preset(int preset_id, const char delimiter = ',') override;
	void set_preset(const String& preset, int preset_id, const char delimiter = ',', bool print_errors = false) override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void calccross(float* input, float* output);
	double calcfilter(float val);

	void switchpreset(bool isbb);
	void copypreset(bool isbb);
	bool transition = false;
	
	void recalcactivebands();

	AudioProcessorValueTreeState::ParameterLayout create_parameters();
	AudioProcessorValueTreeState apvts;

	int version = 2;
	pluginparams pots;

	enum {
		fftOrder = 10,
		fftSize = 1 << fftOrder,
		scopeSize = 330
	};
	Atomic<bool> nextFFTBlockReady = false;
	Atomic<bool> dofft = false;
	float scopeData[scopeSize];

	int currentpreset = 0;
	Atomic<float> valuesy_gui[BAND_COUNT][MAX_MOD];
private:
	pluginpreset state[2];
	float crossovertruevalue[BAND_COUNT];

	pluginpreset presets[20];
	bool preparedtoplay = false;
	bool saved = false;

	std::unique_ptr<dsp::Oversampling<float>> os;
	AudioBuffer<float> osbuffer;
	std::vector<float*> ospointerarray;

	int channelnum = 0;
	int samplesperblock = 0;
	int samplerate = 44100;

	dc_filter dcfilter;
	bool removedc[BAND_COUNT];

	int filtercount = 0;
	std::vector<dsp::LinkwitzRileyFilter<float>> crossover;
	std::array<juce::AudioBuffer<float>,BAND_COUNT> filterbuffers;
	std::array<juce::AudioBuffer<float>,BAND_COUNT> wetbuffers;

	std::array<dsp::StateVariableTPTFilter<float>,BAND_COUNT*MAX_MOD> modulefilters;
	std::vector<dsp::IIR::Filter<float>> modulepeaks;

	std::vector<float> sampleandhold;
	float holdtime[BAND_COUNT*MAX_MOD];

	float ringmod[BAND_COUNT*MAX_MOD];

	std::vector<float> declick;
	float declickprogress[BAND_COUNT];

	float fifo[fftSize];
	float fftData[fftSize*2];
	int fifoIndex = 0;
	dsp::FFT forwardFFT;
	dsp::WindowingFunction<float> window;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismaAudioProcessor)
};

static std::function<String(float v, int max)> tonormalized = [](float v, int max) {
	return String(v,3);
};
static std::function<float(const String& s)> fromnormalized = [](const String& s) {
	return jlimit(0.f,1.f,s.getFloatValue());
};
static std::function<String(int v, int max)> toid = [](int v, int max) {
	switch(v) {
		case 0:
			return (String)"Off";
		case 1:
			return (String)"Soft clip";
		case 2:
			return (String)"Hard clip";
		case 3:
			return (String)"Heavy";
		case 4:
			return (String)"Asym";
		case 5:
			return (String)"Rectify";
		case 6:
			return (String)"Fold";
		case 7:
			return (String)"Sine fold";
		case 8:
			return (String)"Zero cross";
		case 9:
			return (String)"Bit crush";
		case 10:
			return (String)"Sample divide";
		case 11:
			return (String)"DC";
		case 12:
			return (String)"Width";
		case 13:
			return (String)"Gain";
		case 14:
			return (String)"Pan";
		case 15:
			return (String)"Lowpass";
		case 16:
			return (String)"Highpass";
		case 17:
			return (String)"Peak";
		case 18:
			return (String)"Dry/wet";
		case 19:
			return (String)"Stereo rectify";
		case 20:
			return (String)"Stereo DC";
		case 21:
			return (String)"Ring mod";
	}
	return String(v);
};
// Off  Empty
// Soft
// Hard  Clip
// HEavy
// ASym/.A
// REctify/.R
// FOld/.F
// SIne
// Zero
// Bit  CRush
// SAmple  DIvide
// DC/.D
// Width
// Gain
// PAn/.P
// LOwpass/.L
// HIghpass
// PEak
// DRy  WEt
// STereorectify S+R!RUS
// stereodc S+D
// RIng  MOd/.M
static std::function<int(const String& s)> fromid = [](const String& s) {
	String lower = s.toLowerCase();
	if(lower.contains("ri") || lower.contains("mo") || lower.startsWith("m"))
		return 21;
	if(lower.contains("s") && lower.contains("d") && !lower.contains("sa") && !lower.contains("di"))
		return 20;
	if(lower.contains("st") || (lower.contains("s") && lower.contains("r") && !lower.contains("rus")))
		return 19;
	if(lower.contains("dr") || lower.contains("we"))
		return 18;
	if(lower.contains("pe"))
		return 17;
	if(lower.contains("hi"))
		return 16;
	if(lower.contains("lo") || lower.startsWith("l"))
		return 15;
	if(lower.contains("pa") || lower.startsWith("p"))
		return 14;
	if(lower.contains("g"))
		return 13;
	if(lower.contains("w"))
		return 12;
	if(lower.contains("dc") || lower.startsWith("d"))
		return 11;
	if(lower.contains("sa") || lower.contains("di"))
		return 10;
	if(lower.contains("b") || lower.contains("cr"))
		return 9;
	if(lower.contains("z"))
		return 8;
	if(lower.contains("si"))
		return 7;
	if(lower.contains("fo") || lower.startsWith("f"))
		return 6;
	if(lower.contains("re") || lower.startsWith("r"))
		return 5;
	if(lower.contains("as") || lower.startsWith("a"))
		return 4;
	if(lower.contains("he"))
		return 3;
	if(lower.contains("h") || lower.contains("c"))
		return 2;
	if(lower.contains("s"))
		return 1;
	if(lower.contains("o") || lower.contains("e"))
		return 0;
	return jlimit(0,MODULE_COUNT,s.getIntValue());
};
static std::function<String(bool v, int max)> tobool = [](bool v, int max) {
	return v?"On":"Off";
};
static std::function<bool(const String& s)> frombool = [](const String& s) {
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return false;
};
static std::function<String(float v, int max)> tocross = [](float v, int max) {
	return String(round(mapToLog10(v,20.f,20000.f)))+"hz";
};
static std::function<float(const String& s)> fromcross = [](const String& s) {
	float val = s.getFloatValue();
	if(!s.containsIgnoreCase("k") && !s.containsIgnoreCase("hz") && val <= 1)
		return jlimit(0.f,1.f,val);
	if((s.containsIgnoreCase("k")) || (!s.containsIgnoreCase("hz") && val < 20))
		val *= 1000;
	return jlimit(0.f,1.f,mapFromLog10(val,20.f,20000.f));
};
static std::function<String(bool v, int max)> toquality = [](bool v, int max) {
	return v?"High":"Low";
};
static std::function<bool(const String& s)> fromquality = [](const String& s) {
	if(s.containsIgnoreCase("h") || s.containsIgnoreCase("i") || s.containsIgnoreCase("g")) return true;
	if(s.containsIgnoreCase("l") || s.containsIgnoreCase("w")) return false;
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return true;
};
static std::function<String(bool v, int max)> toab = [](bool v, int max) {
	return v?"B":"A";
};
static std::function<bool(const String& s)> fromab = [](const String& s) {
	if(s.containsIgnoreCase("b")) return true;
	if(s.containsIgnoreCase("a")) return false;
	if(s.containsIgnoreCase("n")) return true;
	if(s.containsIgnoreCase("f")) return false;
	if(s.containsIgnoreCase("1")) return true;
	if(s.containsIgnoreCase("0")) return false;
	return false;
};
