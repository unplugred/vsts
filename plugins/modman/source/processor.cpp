#include "processor.h"
#include "editor.h"

ModManAudioProcessor::ModManAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	currentpreset = -1;
	presets[0].name = "default";
	set_preset("0,1,0.41,0.405,0.505,2,0,0,0.5,1,1,0.5,0,0.445,0.9575,0.705,0.3,2,0,0,0.5,1,1,0.5,1,0.2325,1,0.39,0,2,0,0,0.5,1,1,0.5,0,0,0.85,0.66,0.3,2,0,0,0.5,1,1,0.5,0,0.2425,1,0.595,1,2,0,0,0.5,1,1,0.5,0.5,",0);
	presets[1].name = "cheap tape";
	set_preset("0,1,1,0.88,0.625,5,0,0.483852,0.5,0.137037,0.480866,0.194963,0.237037,0.505993,0.5,0.707407,0.48203,0.5,1,0.495111,0.5,1,0.7875,0.935,0.61,0.375,13,0,0.0144644,1,0.0406938,0.0878419,1,0.132555,0.166626,1,0.224416,0.188107,1,0.316278,0.504791,1,0.408139,0.464529,1,0.5,0.576695,1,0.591861,0.614624,1,0.683722,0.685599,1,0.775584,0.607186,1,0.867445,0.830611,1,0.959306,0.858362,1,1,0.838254,1,1,0.0925,0.1925,0.375,0.3,2,0,0,0.5,1,1,0.5,1,0,0.0575,0.52,0.62,7,0,0.92563,0.279111,0.0962963,0.0349485,0.757704,0.318519,0.941347,0.121333,0.581481,0.855383,0.689333,0.696296,0.422954,0.599926,0.807407,0.969912,0.494741,1,1,0.5,1,0,1,0.495,0.3,5,0,0.957185,0.5,0.337037,0.895706,0.5,0.677778,0.967579,0.5,0.777778,0.757791,0.5,1,1,0.5,0.5,",1);
	presets[2].name = "unstable pad";
	set_preset("0,1,1,0.27,0.475,2,0,0,0.5,1,1,0.5,1,0.2625,0.6575,0.345,0.475,2,0,0,0.5,1,1,0.5,1,0.0775,0.505,0.39,0.525,2,0,0,0.5,1,1,0.5,0,0,0.3,0.5,0.3,2,0,0.883555,0.5,1,1,0.5,1,0.42,0.8825,0.245,0.205,2,0,0,0.5,1,1,0.5,0.5,",2);
	presets[3].name = "his guitar amp";
	set_preset("0,0,0.41,0.405,0.505,2,0,0,0.5,1,1,0.5,1,0.6975,0.6975,0.5,0.3,2,0,0,0.5,1,1,0.5,1,0.2525,0.57,0.46,0.715,3,0,0,0.85,0.5,0.5,0.15,1,1,0.5,1,1,1,0.5,0.3,2,0,0,0.5,1,1,0.5,0,0.2425,1,0.595,1,2,0,0,0.5,1,1,0.5,0.5,",3);
	presets[4].name = "first dub";
	set_preset("0,0,0.41,0.405,0.505,2,0,0,0.5,1,1,0.5,1,0.31,0.8625,0.63,0.3,3,0,0,0.15,0.5,0.5,0.85,1,1,0.5,1,0.4,0.7425,0.62,0.78,13,0,0,0.5,0.0623148,0.75339,0.5,0.138,0.229738,0.5,0.2285,0.352302,0.5,0.319,0.427579,0.5,0.4095,0.410268,0.5,0.5,0.560533,0.5,0.5905,0.696249,0.5,0.681,0.572321,0.5,0.7715,0.596401,0.5,0.862,0.786482,0.5,0.92287,0.477307,0.35763,1,1,0.5,0,0,1,0.5,0.3,2,0,0,0.5,1,1,0.5,0,0.2425,1,0.595,1,2,0,0,0.5,1,1,0.5,0.5,",4);
	presets[5].name = "eruption";
	set_preset("0,1,1,0.875,0.3,2,0,0,0.0105185,1,1,0.5,1,0.39,1,0.29,0.3,2,0,0,0.5,1,1,0.5,1,0.3675,1,0.5,0,13,0,0.0953367,1,0.0541937,0.0899692,1,0.143355,0.257013,1,0.232516,0.27234,1,0.321678,0.487043,1,0.410839,0.299614,1,0.5,0.384897,1,0.589161,0.414293,1,0.678322,0.719629,1,0.767484,0.809268,1,0.856645,0.811171,1,0.945806,0.950059,1,1,0.72759,1,1,0,0.385,0.375,0.3,2,0,0,0.5,1,1,0.5,0,0.2425,1,0.595,1,2,0,0,0.5,1,1,0.5,0.5,",5);
	presets[6].name = "ping pong scratching";
	set_preset("0,0,0.41,0.405,0.505,2,0,0,0.5,1,1,0.5,1,0.72,1,0.545,0.56,2,0,0,0.0424445,1,1,0.5,1,0.485,0.485,0.5,0.3,2,0,0,0.5,1,1,0.5,0,0,0.3,0.5,0.3,2,0,0,0.5,1,1,0.5,1,0,0.6525,0.565,0.3,3,0,0,0.0736296,0.5,1,1,1,0,0.5,0.5,",6);
	presets[7].name = "broken portamento";
	set_preset("0,1,1,0.475,1,5,0,0,0.5,0.177778,0,0.5,0.177778,1,0.879111,0.540741,0,0.136667,1,1,0.5,1,0.8375,1,0.965,0,13,0,0.149543,0.5,0.0475,0.105658,0.5,0.138,0.2127,0.5,0.2285,0.428855,0.5,0.319,0.393677,0.5,0.4095,0.500138,0.5,0.5,0.623489,0.5,0.5905,0.691118,0.5,0.681,0.625649,0.5,0.7715,0.735789,0.5,0.862,0.722408,0.5,0.9525,0.722345,0.5,1,0.798465,0.5,1,0.3,0.3,0.5,0.3,2,0,0,0.5,1,1,0.5,0,0,0.3,0.5,0.3,2,0,0,0.5,1,1,0.5,0,0.4225,0.9425,0.5,0.3,2,0,0,0.5,1,1,0.5,0.5,",7);
	presets[8].name = "the less lie";
	set_preset("0,1,0.2325,0.63,1,2,0,0,0.5,1,1,0.5,1,0.915,1,0.56,0,2,0,0,0.5,1,1,0.5,1,0.425,0.425,0.5,0,2,0,0,0.5,1,1,0.5,1,0.145,0.145,0.5,0.3,2,0,0,0.5,1,1,0.5,1,0.4,0.9575,0.47,0.51,3,0,0,0.15,0.5,1,0.85,1,0,0.5,0.5,",8);
	presets[9].name = "annoying wildlife";
	set_preset("0,1,0.16,0.555,0.93,3,0,0,0.1475,0.5,0.5,0.8525,1,1,0.5,1,0.78,1,0.64,0.425,2,0,0,0.5,1,1,0.5,1,0.3725,0.735,0.5,0.3,2,0,0,0.5,1,1,0.5,1,0,0.9725,0.425,1,3,0,0,0.0315556,0.896296,1,0.0315556,1,1,0.5,1,0.225,0.835,0.82,0.715,2,0,0,0.5,1,1,0.5,0.5,",9);
	presets[10].name = "crazy cat";
	set_preset("0,1,0.385,0.61,0.42,13,0,0.178691,0.5,0.0475,0.114013,0.5,0.138,0.24347,0.5,0.2285,0.160655,0.5,0.319,0.426177,0.5,0.4095,0.51448,0.5,0.5,0.512621,0.5,0.5905,0.557756,0.5,0.681,0.748026,0.5,0.7715,0.543361,0.5,0.862,0.864732,0.5,0.9525,0.903339,0.5,1,0.976119,0.5,1,0.4875,0.85,0.71,0.22,3,0,0,0.15,0.5,1,0.85,1,0,0.5,1,0.3275,0.92,0.5,0.3,3,0,0,0.155,0.5,0.5,0.845,1,1,0.5,1,0.58,0.8225,0.67,0.65,13,0,0.0711376,0.5,0.0475,0.11367,0.5,0.138,0.169817,0.5,0.2285,0.277909,0.5,0.319,0.231111,0.5,0.4095,0.355876,0.5,0.5,0.586744,0.5,0.5905,0.534028,0.5,0.681,0.554166,0.5,0.7715,0.731643,0.5,0.862,0.715541,0.5,0.9525,0.767356,0.5,1,0.70029,0.5,1,0.54,1,0.67,0.615,2,0,0,0.5,1,1,0.5,0.5,",10);
	presets[11].name = "life on mars";
	set_preset("0,1,1,0.535,0.395,3,0,0.504889,0.5,0.488889,0.425358,0.5,1,0.552963,0.5,1,0.1375,0.8075,0.435,0.18,13,0,0.743973,1,0.0586111,0.541428,1,0.138,0.258343,1,0.2285,0.381029,1,0.319,0.409338,1,0.394685,0.231706,1,0.474074,0.921448,1,0.5905,0.521888,1,0.681,0.568713,1,0.7715,0.733078,1,0.854593,0.148108,1,0.915463,0.922648,1,1,0.716807,1,1,0.2775,0.845,0.36,0,3,0,0,0.5,0.488889,1,0.5,1,0,0.5,1,0.1075,0.2975,0.625,0.3,13,0,0.106381,1,0.0475,0.321761,1,0.138,0.379836,1,0.2285,0.275218,1,0.319,0.340265,1,0.4095,0.433289,1,0.5,0.583963,1,0.5905,0.648189,1,0.681,0.713848,1,0.7715,0.590608,1,0.862,0.734829,1,0.9525,0.908464,1,1,0.73222,1,0,0.29,0.94,0.385,0.3,2,0,0,0.5,1,1,0.5,0.5,",11);
	presets[12].name = "we drift apart";
	set_preset("0,1,1,0.29,1,5,0,1,0.818444,0.237037,0,0.245185,0.355556,0.276367,0.85,0.611111,0.525926,0.15,1,1,0.5,1,0.3525,0.925,0.195,0.715,7,0,0.552222,0.5,0.22963,0,0.5,0.411111,0.387931,0.5,0.57037,0.203275,0.5,0.722222,1,0.5,0.855556,0.531697,0.5,1,0.400444,0.5,1,0.095,0.7175,0.375,0,2,0,0,0.5,1,1,0.5,0,0.35,0.54,0.35,0.75,13,0,0.195303,1,0.0475,0.135404,1,0.138,0.14604,1,0.2285,0.285271,1,0.319,0.301488,1,0.4095,0.472191,1,0.5,0.543018,1,0.5905,0.563741,1,0.681,0.514974,1,0.7715,0.822394,1,0.862,0.829151,1,0.9525,0.808214,1,1,0.986958,1,0,0.3375,0.81,0.7,0.565,2,0,0,0.5,1,1,0.5,0.5,",12);
	presets[13].name = "very unfaithful";
	set_preset("0,1,1,0.68,1,4,0,0.483852,0.5,0.207407,0.499565,0.5,0.688889,0.490215,0.5,1,0.484593,0.5,1,0.6575,1,0.505,0.3,3,0,0,0.5,0.488889,1,0.5,1,0,0.5,0,0,0.68,0.5,0.3,3,0,0.625852,0.5,0.388889,0.299096,0.5,1,1,0.5,1,0.9625,1,0.48,0.3,3,0,0,0.5,0.488889,1,0.5,1,0,0.5,1,0.76,0.94,0.805,0.38,3,0,0,0.5,0.488889,1,0.5,1,0,0.5,0.5,",13);
	currentpreset = 0;
	for(int i = 14; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "program "+((String)(i-13));
	}

	params.pots[0] = potentiometer("on"				,"on"			,0	,0.f	,1.f	,potentiometer::booltype);
	params.pots[1] = potentiometer("min range"		,"min"			,0	);
	params.pots[2] = potentiometer("max range"		,"max"			,0	);
	params.pots[3] = potentiometer("speed"			,"speed"		,0	);
	params.pots[4] = potentiometer("stereo"			,"stereo"		,0	);
	params.pots[5] = potentiometer("master speed"	,"masterspeed"	,0	);

	params.modulators[0].name = M1;
	params.modulators[1].name = M2;
	params.modulators[2].name = M3;
	params.modulators[3].name = M4;
	params.modulators[4].name = M5;

	for(int m = 0; m < MC; ++m) {
		time[m] = 0;
		params.modulators[m].id = m;
		for(int i = 0; i < (paramcount-1); i++) {
			if(i == 1 && m == 0) {
				params.modulators[m].defaults[i] = 0;
				state.values[m][i] = 0;
				presets[currentpreset].values[m][i] = 0;
				continue;
			}
			params.modulators[m].defaults[i] = presets[0].values[m][i];
			state.values[m][i] = params.pots[i].inflate(apvts.getParameter("m"+((String)m)+params.pots[i].id)->getValue());
			presets[currentpreset].values[m][i] = state.values[m][i];
			add_listener("m"+((String)m)+params.pots[i].id);
		}
	}
	state.masterspeed = params.pots[paramcount-1].inflate(apvts.getParameter(params.pots[paramcount-1].id)->getValue());
	presets[currentpreset].masterspeed = state.masterspeed;
	add_listener(params.pots[paramcount-1].id);

	for(int c = 0 ; c < 2; ++c)
		cuber_rot[c] = -.1f;

	prlin.init();

	updatedcurve = 1+2+4+8+16;
	updatevis = true;
}

ModManAudioProcessor::~ModManAudioProcessor(){
	close();
}

const String ModManAudioProcessor::getName() const { return "ModMan"; }
bool ModManAudioProcessor::acceptsMidi() const { return false; }
bool ModManAudioProcessor::producesMidi() const { return false; }
bool ModManAudioProcessor::isMidiEffect() const { return false; }
double ModManAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int ModManAudioProcessor::getNumPrograms() { return 20; }
int ModManAudioProcessor::getCurrentProgram() { return currentpreset; }
void ModManAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;

	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (paramcount-1); i++) {
			if(i == 1 && m == 0) continue;
			apvts.getParameter("m"+((String)m)+params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[m][i]));
		}
	}
	apvts.getParameter(params.pots[paramcount-1].id)->setValueNotifyingHost(params.pots[paramcount-1].normalize(presets[currentpreset].masterspeed));

	updatedcurve = 1+2+4+8+16;
	updatevis = true;
}
const String ModManAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void ModManAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void ModManAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void ModManAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void ModManAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	for(int m = 0; m < MC; ++m)
		state.curves[m].resizechannels(channelnum);

	modulator_data.resize(MC*channelnum*samplesperblock);
	for(int i = 0; i < (MC*channelnum*samplesperblock); ++i)
		modulator_data[i] = 0;

	drift_data.resize(channelnum*MAX_DRIFT*samplerate);
	for(int i = 0; i < (channelnum*MAX_DRIFT*samplerate); ++i)
		drift_data[i] = 0;

	smooth.resize(MC*channelnum);
	for(int i = 0; i < (MC*channelnum); ++i)
		smooth[i].reset(samplerate,i<channelnum?.01f:.001f);
	resetsmooth = true;

	dsp::ProcessSpec spec;
	spec.sampleRate = samplerate;
	spec.maximumBlockSize = samplesperblock;
	spec.numChannels = channelnum;
	lowpass.prepare(spec);
	lowpass.setType(dsp::StateVariableTPTFilterType::lowpass);
	lowpass.setCutoffFrequency(20000);
	lowpass.setResonance(1./MathConstants<double>::sqrt2);
	lowpass.reset();

	updatedcurve = 1+2+4+8+16;
}
void ModManAudioProcessor::releaseResources() { }

bool ModManAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void ModManAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	bool previson = ison[params.selectedmodulator.get()];

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);

	float* const* channel_data = buffer.getArrayOfWritePointers();

	if(updatedcurve.get() > 0) {
		int uc = updatedcurve.get();
		updatedcurve = 0;
		for(int m = 0; m < MC; ++m) if((uc&(1<<m)) > 0)
			state.curves[m].points = presets[currentpreset].curves[m].points;
	}

	for(int m = 0; m < MC; ++m) {
		float center = 0;
		float min = state.values[m][1];
		float max = state.values[m][2];
		float speed = state.values[m][3];
		float stereo = channelnum<2?0:pow(state.values[m][4],2);
		switch(m) {
			case 0: // DRIFT
				min = 0;
				driftmult = 1.f/fmax(max,0.0001f);
				max = pow(max,2);
				break;
			case 1: // LOW PASS
				center = 1;
				break;
			case 4: // AMPLITUDE
				center = .62996052493f;
				break;
		}

		ison[m] = state.values[m][0] > .5;
		if(!ison[m])
			for(int c = 0; c < channelnum; ++c)
				if(smooth[m*channelnum+c].getCurrentValue() != center)
					ison[m] = true;
		if(!ison[m]) continue;

		float v = 0;
		for(int s = 0; s < numsamples; ++s) {
			time[m] += pow(speed*2,4)*pow(state.masterspeed*2,4)*.00003f;
			for(int c = 0; c < channelnum; ++c) {
				v = (state.curves[m].process(prlin.noise(time[m],((((float)c)/fmax(channelnum-1,1))-.5)*stereo+m*10)*.5f+.5f,c)*(max-min)+min)*state.values[m][0]+center*(1-state.values[m][0]);
				if(s == 0 && resetsmooth)
					smooth[m*channelnum+c].setCurrentAndTargetValue(v);
				else
					smooth[m*channelnum+c].setTargetValue(v);
				modulator_data[(m*channelnum+c)*samplesperblock+s] = smooth[m*channelnum+c].getNextValue();
			}
		}

		float mono = 0;
		for(int c = 0; c < channelnum; ++c)
			mono += modulator_data[(m*channelnum+c)*samplesperblock];
		if(m == 0) flower_rot[0] = (mono/channelnum)*driftmult;
		else flower_rot[m] = mono/channelnum;
	}
	resetsmooth = false;

	if(previson) for(int c = 0 ; c < 2; ++c) {
		float r = modulator_data[(params.selectedmodulator.get()*channelnum+c*(channelnum-1))*samplesperblock];
		if(params.selectedmodulator.get() == 0) r *= driftmult;
		cuber_rot[c] = r;
	}

	//DRIFT
	for(int s = 0; s < numsamples; ++s) {
		driftindex = fmod(driftindex+1,MAX_DRIFT*samplerate);
		for(int c = 0; c < channelnum; ++c) {
			drift_data[c*MAX_DRIFT*samplerate+driftindex] = channel_data[c][s];
			if(ison[0])
				channel_data[c][s] = interpolatesamples(&drift_data[c*MAX_DRIFT*samplerate],driftindex+1+MAX_DRIFT*samplerate*(1-2.f/(samplerate*MAX_DRIFT))*(1-modulator_data[c*samplesperblock+s]),MAX_DRIFT*samplerate);
		}
	}

	//LOWPASS
	if(ison[1]) {
		if(!ison[2])
			lowpass.setResonance(calcresonance(0));
		for(int s = 0; s < numsamples; ++s) for(int c = 0; c < channelnum; ++c) {
			lowpass.setCutoffFrequency(calccutoff(modulator_data[(channelnum+c)*samplesperblock+s]));
			if(ison[2])
				lowpass.setResonance(calcresonance(modulator_data[(2*channelnum+c)*samplesperblock+s]));
			channel_data[c][s] = lowpass.processSample(c,channel_data[c][s]);
		}
	}

	//SATURATION
	if(ison[3]) for(int s = 0; s < numsamples; ++s) for(int c = 0; c < channelnum; ++c) {
		float satval = 1-(1-(pow(1-modulator_data[(3*channelnum+c)*samplesperblock+s],10)+(1-pow(modulator_data[(3*channelnum+c)*samplesperblock+s],.2)))*.5)*.99;
		channel_data[c][s] = (1-pow(1-fmin(fabs(channel_data[c][s]),1),1/satval))*(channel_data[c][s]>0?1:-1)*(1-(1-satval)*.92);
	}

	//AMPLITUDE
	if(ison[4]) for(int s = 0; s < numsamples; ++s) for(int c = 0; c < channelnum; ++c) {
		channel_data[c][s] *= 2*pow(modulator_data[(4*channelnum+c)*samplesperblock+s],1.5f);
	}
}
float ModManAudioProcessor::interpolatesamples(float* buffer, float position, int buffersize) {
	return buffer[((int)floor(position))%buffersize]*(1-fmod(position,1.f))+buffer[((int)ceil(position))%buffersize]*fmod(position,1.f);
}

bool ModManAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* ModManAudioProcessor::createEditor() {
	return new ModManAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void ModManAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter
		<< params.selectedmodulator.get() << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << delimiter;
		data << get_preset(i) << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void ModManAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		std::getline(ss, token, delimiter);
		params.selectedmodulator = std::stoi(token);

		for(int i = 0; i < getNumPrograms(); i++) {
			std::getline(ss,token,delimiter);
			presets[i].name = token;

			std::getline(ss, token, delimiter);
			set_preset(token,i,',',true);
		}
	} catch(const char* e) {
		debug((String)"Error loading saved data: "+(String)e);
	} catch(String e) {
		debug((String)"Error loading saved data: "+e);
	} catch(std::exception &e) {
		debug((String)"Error loading saved data: "+(String)e.what());
	} catch(...) {
		debug((String)"Error loading saved data");
	}
}
const String ModManAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (paramcount-1); i++) {
			if(i == 1 && m == 0) continue;
			data << presets[preset_id].values[m][i] << delimiter;
		}
		data << presets[preset_id].curves[m].points.size() << delimiter;
		for(int p = 0; p < presets[preset_id].curves[m].points.size(); ++p)
			data << presets[preset_id].curves[m].points[p].x << delimiter << presets[preset_id].curves[m].points[p].y << delimiter << presets[preset_id].curves[m].points[p].tension << delimiter;
	}
	data << presets[preset_id].masterspeed << delimiter;

	return data.str();
}
void ModManAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(preset_id);
	try {
		std::stringstream ss(preset.trim().toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int save_version = std::stoi(token);

		for(int m = 0; m < MC; ++m) {
			for(int i = 0; i < (paramcount-1); i++) {
				if(i == 1 && m == 0) continue;
				std::getline(ss, token, delimiter);
				presets[preset_id].values[m][i] = std::stof(token);
			}
			presets[preset_id].curves[m].points.clear();
			std::getline(ss, token, delimiter);
			int size = std::stof(token);
			if(size < 2) throw std::invalid_argument("Invalid point data");
			float prevx = 0;
			for(int p = 0; p < size; ++p) {
				std::getline(ss, token, delimiter);
				float x = std::stof(token);
				std::getline(ss, token, delimiter);
				float y = std::stof(token);
				std::getline(ss, token, delimiter);
				float tension = std::stof(token);
				if(x > 1 || x < prevx || y > 1 || y < 0 || tension > 1 || tension < 0 || (p == 0 && x != 0) || (p == (size-1) && x != 1))
					throw std::invalid_argument("Invalid point data");
				prevx = x;
				presets[preset_id].curves[m].points.push_back(point(x,y,tension));
			}
		}
		std::getline(ss, token, delimiter);
		presets[preset_id].masterspeed = std::stof(token);

	} catch(const char* e) {
		error = "Error loading saved data: "+(String)e;
	} catch(String e) {
		error = "Error loading saved data: "+e;
	} catch(std::exception &e) {
		error = "Error loading saved data: "+(String)e.what();
	} catch(...) {
		error = "Error loading saved data";
	}
	if(error != "") {
		if(print_errors)
			debug(error);
		set_preset(revert, preset_id);
		return;
	}

	if(currentpreset != preset_id) return;

	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (paramcount-1); i++) {
			if(i == 1 && m == 0) continue;
			apvts.getParameter("m"+((String)m)+params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[m][i]));
		}
	}
	apvts.getParameter(params.pots[paramcount-1].id)->setValueNotifyingHost(params.pots[paramcount-1].normalize(presets[currentpreset].masterspeed));
	updatedcurve = 1+2+4+8+16;
	updatevis = true;
}

void ModManAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == params.pots[paramcount-1].id) {
		state.masterspeed = newValue;
		presets[currentpreset].masterspeed = newValue;
	}
	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (paramcount-1); i++) {
			if(parameterID == ("m"+((String)m)+params.pots[i].id)) {
				state.values[m][i] = newValue;
				presets[currentpreset].values[m][i] = newValue;
				return;
			}
		}
	}
}
float ModManAudioProcessor::calccutoff(float val) {
	return mapToLog10(val,20.f,20000.f);
}
float ModManAudioProcessor::calcresonance(float val) {
	return mapToLog10(val,0.1f,40.f);
}

void ModManAudioProcessor::movepoint(int index, float x, float y) {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i].points[index].x = x;
	presets[currentpreset].curves[i].points[index].y = y;
	updatedcurve = updatedcurve.get()|(1<<i);
}
void ModManAudioProcessor::movetension(int index, float tension) {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i].points[index].tension = tension;
	updatedcurve = updatedcurve.get()|(1<<i);
}
void ModManAudioProcessor::addpoint(int index, float x, float y) {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i].points.insert(presets[currentpreset].curves[i].points.begin()+index,point(x,y,presets[currentpreset].curves[i].points[index-1].tension));
	updatedcurve = updatedcurve.get()|(1<<i);
}
void ModManAudioProcessor::deletepoint(int index) {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i].points.erase(presets[currentpreset].curves[i].points.begin()+index);
	updatedcurve = updatedcurve.get()|(1<<i);
}
const String ModManAudioProcessor::curvetostring(const char delimiter) {
	int i = params.selectedmodulator.get();
	return presets[currentpreset].curves[i].tostring(delimiter);
}
void ModManAudioProcessor::curvefromstring(String str, const char delimiter) {
	int i = params.selectedmodulator.get();
	String revert = presets[currentpreset].curves[i].tostring();
	try {
		presets[currentpreset].curves[i] = curve(str,delimiter);
	} catch(...) {
		presets[currentpreset].curves[i] = curve(revert);
	}
	updatevis = true;
	updatedcurve = updatedcurve.get()|(1<<i);
}
void ModManAudioProcessor::resetcurve() {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i] = curve("2,0,0,0.5,1,1,0.5");
	updatevis = true;
	updatedcurve = updatedcurve.get()|(1<<i);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ModManAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout ModManAudioProcessor::create_parameters() {
	float mins[5]	{0.f	,.445f	,.2325f	,0.f	,.2425f	};
	float maxs[5]	{.41f	,.9575f	,1.f	,.85f	,1.f	};
	float spds[5]	{.405f	,.705f	,.39f	,.66f	,.595f	};
	float strs[5]	{.505f	,.3f	,0.f	,.3f	,1.f	};
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	for(int m = 0; m < MC; ++m) {
		String name;
		switch(m) {
			case 0: name = M1; break;
			case 1: name = M2; break;
			case 2: name = M3; break;
			case 3: name = M4; break;
			case 4: name = M5; break;
		}
		parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"m"+((String)m)+"on"		,1},name+" on"															 ,m==0||m==2	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool			).withValueFromStringFunction(frombool			)));
		if(m == 0) {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" range"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),maxs[m]		,AudioParameterFloatAttributes().withStringFromValueFunction(toms			).withValueFromStringFunction(fromms			)));
		} else if(m == 1) {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"min"		,1},name+" min range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),mins[m]		,AudioParameterFloatAttributes().withStringFromValueFunction(tocutoff		).withValueFromStringFunction(fromcutoff		)));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" max range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),maxs[m]		,AudioParameterFloatAttributes().withStringFromValueFunction(tocutoff		).withValueFromStringFunction(fromcutoff		)));
		} else if(m == 2) {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"min"		,1},name+" min range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),mins[m]		,AudioParameterFloatAttributes().withStringFromValueFunction(toresonance	).withValueFromStringFunction(fromresonance		)));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" max range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),maxs[m]		,AudioParameterFloatAttributes().withStringFromValueFunction(toresonance	).withValueFromStringFunction(fromresonance		)));
		} else {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"min"		,1},name+" min range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),mins[m]		,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" max range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),maxs[m]		,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
		}
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"speed"	,1},name+" speed"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),spds[m]		,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"stereo"	,1},name+" stereo"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),strs[m]		,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
	}
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"masterspeed"				,1},"master speed"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.5f			,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
	return { parameters.begin(), parameters.end() };
}
