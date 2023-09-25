#include "PluginProcessor.h"
#include "PluginEditor.h"

void curveiterator::reset(curve inputcurve, int wwidth) {
	points = inputcurve.points;
	width = wwidth;
	x = -1;
	nextpoint = 0;
	points[0].x = 0;
	points[points.size()-1].x = 1;
	pointhit = true;
	while(!points[nextpoint].enabled) ++nextpoint;
}
double curveiterator::next() {
	double xx = ((double)++x)/(width-1);
	if(xx >= 1) {
		if((((double)x-1)/(width-1)) < 1) pointhit = true;
		return points[points.size()-1].y;
	}
	if(xx >= points[nextpoint].x || (width-x) <= (points.size()-nextpoint)) {
		pointhit = true;
		currentpoint = nextpoint;
		++nextpoint;
		while(!points[nextpoint].enabled) ++nextpoint;
		return points[currentpoint].y;
	}
	double interp = curve::calctension((xx-points[currentpoint].x)/(points[nextpoint].x-points[currentpoint].x),points[currentpoint].tension);
	return points[currentpoint].y*(1-interp)+points[nextpoint].y*interp;
}
curve::curve(String str, const char linebreak) {
	std::stringstream ss(str.trim().toRawUTF8());
	std::string token;
	std::getline(ss, token, linebreak);
	int size = std::stof(token);
	for(int p = 0; p < size; ++p) {
		std::getline(ss, token, linebreak);
		float x = std::stof(token);
		std::getline(ss, token, linebreak);
		float y = std::stof(token);
		std::getline(ss, token, linebreak);
		float tension = std::stof(token);
		points.push_back(point(x,y,tension));
	}
}
String curve::tostring(const char linebreak) {
	std::ostringstream data;
	data << points.size() << linebreak;
	for(int p = 0; p < points.size(); ++p)
		data << points[p].x << linebreak << points[p].y << linebreak << points[p].tension << linebreak;
	return (String)data.str();
}
double curve::calctension(double interp, double tension) {
	if(tension == .5)
		return interp;
	else {
		tension = tension*.99+.005;
		if(tension < .5)
			return pow(interp,.5/tension)*(1-tension)+(1-pow(1-interp,2*tension))*tension;
		else
			return pow(interp,2-2*tension)*(1-tension)+(1-pow(1-interp,.5/(1-tension)))*tension;
	}
}
bool curve::isvalidcurvestring(String str, const char linebreak) {
	String trimstring = str.trim();
	if(trimstring.isEmpty())
		return false;
	if(!trimstring.containsOnly(String(&linebreak,1)+".0123456789"))
		return false;
	if(!trimstring.endsWithChar(linebreak))
		return false;
	if(trimstring.startsWithChar(linebreak))
		return false;
	return true;
}
int SunBurntAudioProcessor::findcurve(int index) {
	if(index == 0) return 0;
	for(int i = 1; i < 5; i++)
		if(((int)state.values[6+i]) == index)
			return i;
	return -1;
}
SunBurntAudioProcessor::SunBurntAudioProcessor() :
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	PropertiesFile::Options options;
	options.applicationName = getName();
	options.filenameSuffix = ".settings";
	options.osxLibrarySubFolder = "Application Support";
	options.folderName = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile(getName()).getFullPathName();
	options.storageFormat = PropertiesFile::storeAsXML;
	props.setStorageParameters(options);
	PropertiesFile* usersettings = props.getUserSettings();
	if(usersettings->containsKey("Language"))
		params.jpmode = (usersettings->getIntValue("Language")%2) == 0;
	if(usersettings->containsKey("UIScale"))
		params.uiscale = usersettings->getDoubleValue("UIScale");

	params.curves[0] = curveparams("None"			,"3,0,0,0.2,0.07,1,0.7,1,0,0.5,");
	params.curves[1] = curveparams("High-pass"		,"2,0,0,0.5,1,0,0.5,"			);
	params.curves[2] = curveparams("HP resonance"	,"2,0,0.3,0.5,1,0.3,0.5,"		);
	params.curves[3] = curveparams("Low-pass"		,"2,0,1,0.5,1,1,0.5,"			);
	params.curves[4] = curveparams("LP resonance"	,"2,0,0.3,0.5,1,0.3,0.5,"		);
	params.curves[5] = curveparams("Pan"			,"2,0,0.5,0.5,1,0.5,0.5,"		);
	params.curves[6] = curveparams("Density"		,"2,0,0.5,0.5,1,0.5,0.5,"		);
	params.curves[7] = curveparams("Shimmer"		,"2,0,0,0.35,1,0.5,0.5,"		);

	//									dry		wet		density	length	sync	depth	speed	curve1	curve2	curve3	curve4	hp		hp res	lp		lp res	shimmer
	presets[0] = pluginpreset("Default"	,1.0f	,0.75f	,0.5f	,0.65f	,0		,0.55f	,0.65f	,1		,3		,5		,7		,0.0f	,0.3f	,1.0f	,0.3f	,12.0f	);
	for(int i = 1; i < getNumPrograms(); ++i) {
		presets[i] = presets[0];
		presets[i].name = "Program " + (String)(i-7);
	}
	for(int i = 0; i < getNumPrograms(); ++i)
		for(int c = 0; c < 8; ++c)
			presets[i].curves[c] = params.curves[c].defaultvalue;
	presets[currentpreset].seed = Time::currentTimeMillis();

	params.pots[ 0] = potentiometer("Dry"				,"dry"			,.002f	,presets[0].values[ 0]	);
	params.pots[ 1] = potentiometer("Wet"				,"wet"			,.002f	,presets[0].values[ 1]	);
	params.pots[ 2] = potentiometer("Density"			,"density"		,0		,presets[0].values[ 2]	);
	params.pots[ 3] = potentiometer("Length"			,"length"		,0		,presets[0].values[ 3]	);
	params.pots[ 4] = potentiometer("Length (quarter note)","sync"		,0		,presets[0].values[ 4]	,0	,16	,potentiometer::inttype);
	params.pots[ 5] = potentiometer("Vibrato depth"		,"depth"		,.002f	,presets[0].values[ 5]	);
	params.pots[ 6] = potentiometer("Vibrato speed"		,"speed"		,.002f	,presets[0].values[ 6]	);
	params.pots[ 7] = potentiometer("Curve 1"			,"curve1"		,0		,presets[0].values[ 7]	,0	,7	,potentiometer::inttype);
	params.pots[ 8] = potentiometer("Curve 2"			,"curve2"		,0		,presets[0].values[ 8]	,0	,7	,potentiometer::inttype);
	params.pots[ 9] = potentiometer("Curve 3"			,"curve3"		,0		,presets[0].values[ 9]	,0	,7	,potentiometer::inttype);
	params.pots[10] = potentiometer("Curve 4"			,"curve4"		,0		,presets[0].values[10]	,0	,7	,potentiometer::inttype);
	params.pots[11] = potentiometer("High-pass"			,"highpass"		,0		,presets[0].values[11]	);
	params.pots[12] = potentiometer("HP resonance"		,"highpassres"	,0		,presets[0].values[12]	);
	params.pots[13] = potentiometer("Low-pass"			,"lowpass"		,0		,presets[0].values[13]	);
	params.pots[14] = potentiometer("LP resonance"		,"lowpassres"	,0		,presets[0].values[14]	);
	params.pots[15] = potentiometer("Shimmer pitch"		,"shimmerpitch"	,0		,presets[0].values[15]	,-24,24	,potentiometer::inttype);

	params.pots[11].showon.push_back(0);
	params.pots[11].dontshowif.push_back(1);
	params.pots[12].showon.push_back(0);
	params.pots[12].showon.push_back(1);
	params.pots[12].dontshowif.push_back(1);
	params.pots[12].dontshowif.push_back(2);
	params.pots[13].showon.push_back(0);
	params.pots[13].dontshowif.push_back(3);
	params.pots[14].showon.push_back(0);
	params.pots[14].showon.push_back(3);
	params.pots[14].dontshowif.push_back(3);
	params.pots[14].dontshowif.push_back(4);
	params.pots[15].showon.push_back(7);

	for(int i = 0; i < paramcount; ++i) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(params.pots[i].id,this);
	}
}

SunBurntAudioProcessor::~SunBurntAudioProcessor(){
	for(int i = 0; i < paramcount; ++i) apvts.removeParameterListener(params.pots[i].id,this);
}

const String SunBurntAudioProcessor::getName() const { return "SunBurnt"; }
bool SunBurntAudioProcessor::acceptsMidi() const { return false; }
bool SunBurntAudioProcessor::producesMidi() const { return false; }
bool SunBurntAudioProcessor::isMidiEffect() const { return false; }
double SunBurntAudioProcessor::getTailLengthSeconds() const {
	if(samplerate <= 0) return 0;
	return ((double)revlength)/samplerate;
}

int SunBurntAudioProcessor::getNumPrograms() { return 20; }
int SunBurntAudioProcessor::getCurrentProgram() { return currentpreset; }
void SunBurntAudioProcessor::setCurrentProgram (int index) {
	if(currentpreset == index) return;
	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;

	if(presets[currentpreset].seed == 0)
		presets[currentpreset].seed = Time::currentTimeMillis();
	state.seed = presets[currentpreset].seed;
	for(int i = 0; i < 8; ++i)
		state.curves[i] = presets[currentpreset].curves[i];
	for(int i = 0; i < paramcount; ++i)
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));

	updatedcurve = true;
	updatevis = true;
	undoManager.beginNewTransaction();
}
const String SunBurntAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void SunBurntAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
}

void SunBurntAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void SunBurntAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void SunBurntAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	for(int i = 0; i < paramcount; ++i) if(params.pots[i].smoothtime > 0)
		params.pots[i].smooth.reset(samplerate, params.pots[i].smoothtime);

	dampvibratodepth.reset(0,.2,-1,samplerate,channelnum);

	dsp::ProcessSpec spec;
	dsp::ProcessSpec monospec;
	spec.sampleRate = samplerate;
	monospec.sampleRate = samplerate;
	spec.maximumBlockSize = samplesperblock;
	monospec.maximumBlockSize = samplesperblock;
	spec.numChannels = channelnum;
	monospec.numChannels = 1;

	vibratobuffer.setSize(channelnum,(int)floor(MAX_VIBRATO*samplerate));
	vibratobuffer.clear();
	highpassfilters.resize(channelnum);
	lowpassfilters.resize(channelnum);
	impulsechanneldata.resize(channelnum);
	impulseeffectchanneldata.resize(channelnum);
	wetbuffer.setSize(channelnum,samplesperblock);
	effectbuffer.setSize(channelnum,samplesperblock);
	effectbuffer.clear();

	revlength = 0;
	if(channelnum > 2) {
		impulsebuffer.resize(channelnum);
		impulseeffectbuffer.resize(channelnum);
		convolver.resize(channelnum);
		convolvereffect.resize(channelnum);
	} else {
		impulsebuffer.resize(1);
		impulseeffectbuffer.resize(1);
		convolver.resize(1);
		convolvereffect.resize(1);
		convolver[0].reset(new dsp::Convolution);
		convolvereffect[0].reset(new dsp::Convolution);
		convolver[0]->prepare(spec);
		convolvereffect[0]->prepare(spec);
		convolver[0]->reset();
		convolvereffect[0]->reset();
	}

	for(int c = 0; c < channelnum; ++c) {
		highpassfilters[c].reset();
		lowpassfilters[c].reset();
		(*highpassfilters[c].parameters.get()).type = dsp::StateVariableFilter::Parameters<float>::Type::highPass;
		(*lowpassfilters[c].parameters.get()).type = dsp::StateVariableFilter::Parameters<float>::Type::lowPass;
		highpassfilters[c].prepare(monospec);
		lowpassfilters[c].prepare(monospec);

		if(channelnum > 2) {
			convolver[c].reset(new dsp::Convolution);
			convolvereffect[c].reset(new dsp::Convolution);
			convolver[c]->prepare(monospec);
			convolvereffect[c]->prepare(monospec);
			convolver[c]->reset();
			convolvereffect[c]->reset();
		}
	}

	pitchshift.setSampleRate(fmin(samplerate,192000));
	pitchshift.setChannels(channelnum);
	prevpitch = state.values[15];
	pitchshift.setPitchSemiTones(state.values[15]);
	pitchprocessbuffer.resize(channelnum*samplesperblock);

	genbuffer();
	if(channelnum > 2) {
		for(int c = 0; c < channelnum; ++c) {
			convolver[c]->loadImpulseResponse(std::move(impulsebuffer[c]),samplerate,dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
			convolvereffect[c]->loadImpulseResponse(std::move(impulseeffectbuffer[c]),samplerate,dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
		}
	} else {
		convolver[0]->loadImpulseResponse(std::move(impulsebuffer[0]),samplerate,channelnum==2?dsp::Convolution::Stereo::yes:dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
		convolvereffect[0]->loadImpulseResponse(std::move(impulseeffectbuffer[0]),samplerate,channelnum==2?dsp::Convolution::Stereo::yes:dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
	}
}
void SunBurntAudioProcessor::releaseResources() { }

bool SunBurntAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void SunBurntAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	int numsamples = buffer.getNumSamples();

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, numsamples);

	float* const* drychanneldata = buffer.getArrayOfWritePointers();
	float* const* wetchanneldata = wetbuffer.getArrayOfWritePointers();
	float* const* effectchanneldata = effectbuffer.getArrayOfWritePointers();

	//get bpm
	double bpm = 120;
	if(state.values[4] > 0) {
		if(getPlayHead() != nullptr) {
			AudioPlayHead::CurrentPositionInfo cpi;
			getPlayHead()->getCurrentPosition(cpi);
			if(cpi.bpm != lastbpm) {
				lastbpm = cpi.bpm;
				updatedcurvebpmcooldown = .5f;
			}
			bpm = cpi.bpm;
		} else bpm = lastbpm;
	}

	//update impulse
	if(updatedcurvecooldown.get() > 0) {
		updatedcurvecooldown = updatedcurvecooldown.get()-((float)numsamples)/samplerate;
		if(updatedcurvecooldown.get() <= 0)
			updatedcurve = true;
	}
	if(updatedcurvebpmcooldown > 0) {
		updatedcurvebpmcooldown = updatedcurvebpmcooldown-((float)numsamples)/samplerate;
		if(updatedcurvebpmcooldown <= 0)
			updatedcurve = true;
	}
	if(updatedcurve.get()) {
		genbuffer();

		if(channelnum > 2) {
			for(int c = 0; c < channelnum; ++c) {
				convolver[c]->loadImpulseResponse(std::move(impulsebuffer[c]),samplerate,dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
				convolvereffect[c]->loadImpulseResponse(std::move(impulseeffectbuffer[c]),samplerate,dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
			}
		} else {
			convolver[0]->loadImpulseResponse(std::move(impulsebuffer[0]),samplerate,channelnum==2?dsp::Convolution::Stereo::yes:dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
			convolvereffect[0]->loadImpulseResponse(std::move(impulseeffectbuffer[0]),samplerate,channelnum==2?dsp::Convolution::Stereo::yes:dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
		}
	}

	//vibrato
	int buffersize = (int)floor(MAX_VIBRATO*samplerate);
	float* const* vibratobufferdata = vibratobuffer.getArrayOfWritePointers();
	for (int s = 0; s < numsamples; ++s) {
		state.values[5] = params.pots[5].smooth.getNextValue();
		state.values[6] = params.pots[6].smooth.getNextValue();

		vibratoindex = (vibratoindex+1)%buffersize;
		double vibratospeed = pow(state.values[6]*2+.2,2);
		vibratophase = fmod(vibratophase+vibratospeed/samplerate,1);
		for (int c = 0; c < channelnum; ++c) {
			vibratobufferdata[c][vibratoindex] = drychanneldata[c][s];
			double vibratodepth = dampvibratodepth.nextvalue(pow(state.values[5],4)/vibratospeed,c);
			if(vibratodepth > 0.0000000001) {
				double channeloffset = 0;
				if(channelnum > 1)
					channeloffset = (((float)c)/(channelnum-1))*.25;
				double readpos = vibratoindex-(sin((vibratophase+channeloffset)*MathConstants<double>::twoPi)*.5+.5)*vibratodepth*.04*(buffersize-1);
				wetchanneldata[c][s] = interpolatesamples(vibratobufferdata[c],readpos,buffersize);
			} else {
				wetchanneldata[c][s] = drychanneldata[c][s];
			}
		}
	}

	//shimmer
	bool pitchactive = findcurve(7) != -1;
	if(prevpitch != state.values[15]) {
		prevpitch = state.values[15];
		pitchshift.setPitchSemiTones(state.values[15]);
	}
	if(pitchactive) {
		if(state.values[15] != 0) {
			using Format = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

			AudioData::interleaveSamples(
				AudioData::NonInterleavedSource<Format>{wetbuffer.getArrayOfReadPointers(),channelnum},
				AudioData::InterleavedDest<Format>{&pitchprocessbuffer.front(),channelnum},numsamples);
			pitchshift.putSamples(pitchprocessbuffer.data(),numsamples);
			pitchshift.receiveSamples(pitchprocessbuffer.data(),numsamples);
			AudioData::deinterleaveSamples(
				AudioData::InterleavedSource<Format>{&pitchprocessbuffer.front(),channelnum},
				AudioData::NonInterleavedDest<Format>{effectbuffer.getArrayOfWritePointers(),channelnum},numsamples);
		} else {
			for (int c = 0; c < channelnum; ++c)
				for (int s = 0; s < numsamples; ++s)
					effectchanneldata[c][s] = wetchanneldata[c][s];
		}
	}

	//convolve
	dsp::AudioBlock<float> wetblock(wetchanneldata,channelnum,numsamples);
	dsp::AudioBlock<float> effectblock(effectchanneldata,channelnum,numsamples);
	if(channelnum > 2) {
		for(int c = 0; c < channelnum; ++c) {
			dsp::AudioBlock<float> wetblocksinglechannel = wetblock.getSingleChannelBlock(c);
			dsp::ProcessContextReplacing<float> context(wetblocksinglechannel);
			convolver[c]->process(context);
			if(pitchactive) {
				dsp::AudioBlock<float> effectblocksinglechannel = effectblock.getSingleChannelBlock(c);
				dsp::ProcessContextReplacing<float> effectcontext(effectblocksinglechannel);
				convolvereffect[c]->process(effectcontext);
			}
		}
	} else {
		dsp::ProcessContextReplacing<float> context(wetblock);
		convolver[0]->process(context);
		if(pitchactive) {
			dsp::ProcessContextReplacing<float> effectcontext(effectblock);
			convolvereffect[0]->process(effectcontext);
		}
	}

	for (int s = 0; s < numsamples; ++s) {
		state.values[0] = params.pots[0].smooth.getNextValue();
		state.values[1] = params.pots[1].smooth.getNextValue();

		for (int c = 0; c < channelnum; ++c) {
			//dry wet
			if(pitchactive)
				drychanneldata[c][s] = drychanneldata[c][s]*state.values[0]+(wetchanneldata[c][s]+effectchanneldata[c][s])*pow(state.values[1]*2,2);
			else
				drychanneldata[c][s] = drychanneldata[c][s]*state.values[0]+wetchanneldata[c][s]*pow(state.values[1]*2,2);
		}
	}
}
void SunBurntAudioProcessor::genbuffer() {
	if(channelnum <= 0) return;
	updatedcurve = false;
	updatedcurvecooldown = -1;
	updatedcurvebpmcooldown = -1;

	if(state.values[4] == 0)
		revlength = (int)round((pow(state.values[3],2)*(6-.1)+.1)*samplerate);
	else
		revlength = (int)round(((60.*state.values[4])/lastbpm)*samplerate);

	if(channelnum > 2) {
		for(int c = 0; c < channelnum; ++c) {
			impulsebuffer[c] = AudioBuffer<float>(1,revlength);
			impulseeffectbuffer[c] = AudioBuffer<float>(1,revlength);
			impulsechanneldata[c] = impulsebuffer[c].getWritePointer(0);
			impulseeffectchanneldata[c] = impulseeffectbuffer[c].getWritePointer(0);
		}
	} else {
		impulsebuffer[0] = AudioBuffer<float>(channelnum,revlength);
		impulseeffectbuffer[0] = AudioBuffer<float>(channelnum,revlength);
		for(int c = 0; c < channelnum; ++c) {
			impulsechanneldata[c] = impulsebuffer[0].getWritePointer(c);
			impulseeffectchanneldata[c] = impulseeffectbuffer[0].getWritePointer(c);
		}
	}

	bool valuesactive[8] { false,false,false,false,false,false,false,false };
	iterator[0].reset(presets[currentpreset].curves[0],revlength);
	for(int i = 1; i < 5; ++i) {
		iterator[i].reset(presets[currentpreset].curves[(int)state.values[6+i]],revlength);
		valuesactive[(int)state.values[6+i]] = true;
	}

	double out = 0;
	double agc = 0;

	Random random(presets[currentpreset].seed);
	double values[8];
	for (int s = 0; s < revlength; ++s) {
		values[0] = iterator[0].next();
		values[1] = state.values[11];
		values[2] = state.values[12];
		values[3] = state.values[13];
		values[4] = state.values[14];
		values[5] = .5;
		values[6] = state.values[2];
		values[7] = 0;

		for(int i = 1; i < 5; ++i)
			values[(int)state.values[6+i]] = iterator[i].next();

		values[0] *= (random.nextFloat()>.5?1:-1);
		values[1] = mapToLog10(values[1],20.0,20000.0);
		values[2] = mapToLog10(values[2],0.1,40.0);
		values[3] = mapToLog10(values[3],20.0,20000.0);
		values[4] = mapToLog10(values[4],0.1,40.0);
		values[6] = pow(values[6],4);

		for (int c = 0; c < channelnum; ++c) {

			//vol
			if(iterator[0].pointhit) {
				out = values[0];
			} else if(random.nextFloat() < values[6]) {
				float r = random.nextFloat();
				agc += r*r;
				out = values[0]*r;
			} else {
				out = 0;
			}

			//pan
			if(valuesactive[5] && channelnum > 1) {
				double linearpan = (((double)c)/(channelnum-1))*(1-values[5]*2)+values[5];
				out *= sin(linearpan*1.5707963268)*.7071067812+linearpan;
			}

			//low pass
			if(valuesactive[3] || values[3] < 20000) {
				(*lowpassfilters[c].parameters.get()).setCutOffFrequency(samplerate,values[3],values[4]);
				out = lowpassfilters[c].processSample(out);
			}

			//high pass
			if(valuesactive[1] || values[1] > 20) {
				(*highpassfilters[c].parameters.get()).setCutOffFrequency(samplerate,values[1],values[2]);
				out = highpassfilters[c].processSample(out);
			}

			impulsechanneldata[c][s] = out*(1-values[7]);
			impulseeffectchanneldata[c][s] = out*values[7];
		}
		iterator[0].pointhit = false;
	}
	agc = 1/fmax(sqrt(agc/channelnum),1);
	for (int c = 0; c < channelnum; ++c) {
		for (int s = 0; s < revlength; ++s) {
			impulsechanneldata[c][s] *= agc;
			impulseeffectchanneldata[c][s] *= agc;
		}
	}
}
float SunBurntAudioProcessor::interpolatesamples(float* buffer, float position, int buffersize) {
	return buffer[((int)floor(position+buffersize))%buffersize]*(1-fmod(position,1.f))+buffer[((int)ceil(position+buffersize))%buffersize]*fmod(position,1.f);
}

bool SunBurntAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* SunBurntAudioProcessor::createEditor() {
	return new SunBurntAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}
void SunBurntAudioProcessor::setLang(bool isjp) {
	params.jpmode = isjp;
	props.getUserSettings()->setValue("Language",isjp?2:1);
}
void SunBurntAudioProcessor::setUIScale(double uiscale) {
	params.uiscale = uiscale;
	props.getUserSettings()->setValue("UIScale",uiscale);
}

void SunBurntAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;

	data << version << linebreak
		<< currentpreset << linebreak << params.curveselection << linebreak;

	for(int i = 0; i < getNumPrograms(); ++i) {
		data << presets[i].name << linebreak << presets[i].seed << linebreak;
		for(int v = 0; v < paramcount; ++v)
			data << presets[i].values[v] << linebreak;
		for(int c = 0; c < 8; ++c) {
			data << presets[i].curves[c].points.size() << linebreak;
			for(int p = 0; p < presets[i].curves[c].points.size(); ++p)
				data << presets[i].curves[c].points[p].x << linebreak << presets[i].curves[c].points[p].y << linebreak << presets[i].curves[c].points[p].tension << linebreak;
		}
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void SunBurntAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	try {
		//*/
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, '\n');
		int saveversion = std::stoi(token);

		std::getline(ss, token, '\n');
		currentpreset = std::stoi(token);

		std::getline(ss, token, '\n');
		params.curveselection = std::stoi(token);

		for(int i = 0; i < getNumPrograms(); ++i) {
			std::getline(ss, token, '\n');
			presets[i].name = token;
			std::getline(ss, token, '\n');
			presets[i].seed = std::stoll(token);
			for(int v = 0; v < paramcount; ++v) {
				std::getline(ss, token, '\n');
				presets[i].values[v] = std::stof(token);
			}
			for(int c = 0; c < 8; ++c) {
				presets[i].curves[c].points.clear();
				std::getline(ss, token, '\n');
				int size = std::stof(token);
				for(int p = 0; p < size; ++p) {
					std::getline(ss, token, '\n');
					float x = std::stof(token);
					std::getline(ss, token, '\n');
					float y = std::stof(token);
					std::getline(ss, token, '\n');
					float tension = std::stof(token);
					presets[i].curves[c].points.push_back(point(x,y,tension));
				}
			}
		}
		//*/
	} catch (const char* e) {
		logger.debug((String)"Error loading saved data: "+(String)e);
	} catch(String e) {
		logger.debug((String)"Error loading saved data: "+e);
	} catch(std::exception &e) {
		logger.debug((String)"Error loading saved data: "+(String)e.what());
	} catch(...) {
		logger.debug((String)"Error loading saved data");
	}

	for(int i = 0; i < paramcount; ++i) {
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		if(params.pots[i].smoothtime > 0) {
			params.pots[i].smooth.setCurrentAndTargetValue(presets[currentpreset].values[i]);
			state.values[i] = presets[currentpreset].values[i];
		}
	}
	updatedcurve = true;
}
void SunBurntAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; ++i) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		presets[currentpreset].values[i] = newValue;

		if(parameterID.startsWith("curve"))
			updatedcurve = true;
		else if(parameterID == "length" || parameterID == "sync"
				|| (findcurve(1) == -1 && parameterID == "highpass")
				|| (findcurve(2) == -1 && parameterID == "highpassres")
				|| (findcurve(3) == -1 && parameterID == "lowpass")
				|| (findcurve(4) == -1 && parameterID == "lowpassres")
				|| (findcurve(6) == -1 && parameterID == "density"))
			updatedcurvecooldown = .5f;
		return;
	}
}
int64 SunBurntAudioProcessor::reseed() {
	presets[currentpreset].seed = Time::currentTimeMillis();
	state.seed = presets[currentpreset].seed;
	updatedcurve = true;
	return state.seed;
}
void SunBurntAudioProcessor::movepoint(int index, float x, float y) {
	int i = 0;
	if(params.curveselection > 0)
		i = (int)state.values[6+params.curveselection];
	presets[currentpreset].curves[i].points[index].x = x;
	presets[currentpreset].curves[i].points[index].y = y;
	if(findcurve(i) != -1)
		updatedcurve = true;
}
void SunBurntAudioProcessor::movetension(int index, float tension) {
	int i = 0;
	if(params.curveselection > 0)
		i = (int)state.values[6+params.curveselection];
	presets[currentpreset].curves[i].points[index].tension = tension;
	if(findcurve(i) != -1)
		updatedcurve = true;
}
void SunBurntAudioProcessor::addpoint(int index, float x, float y) {
	int i = 0;
	if(params.curveselection > 0)
		i = (int)state.values[6+params.curveselection];
	presets[currentpreset].curves[i].points.insert(presets[currentpreset].curves[i].points.begin()+index,point(x,y,presets[currentpreset].curves[i].points[index-1].tension));
	if(findcurve(i) != -1)
		updatedcurve = true;
}
void SunBurntAudioProcessor::deletepoint(int index) {
	int i = 0;
	if(params.curveselection > 0)
		i = (int)state.values[6+params.curveselection];
	presets[currentpreset].curves[i].points.erase(presets[currentpreset].curves[i].points.begin()+index);
	if(findcurve(i) != -1)
		updatedcurve = true;
}
String SunBurntAudioProcessor::curvetostring(const char linebreak) {
	int i = 0;
	if(params.curveselection > 0)
		i = (int)state.values[6+params.curveselection];
	return presets[currentpreset].curves[i].tostring();
}
void SunBurntAudioProcessor::curvefromstring(String str, const char linebreak) {
	int i = 0;
	if(params.curveselection > 0)
		i = (int)state.values[6+params.curveselection];
	presets[currentpreset].curves[i] = curve(str);
	updatevis = true;
	if(findcurve(i) != -1)
		updatedcurve = true;
}
void SunBurntAudioProcessor::resetcurve() {
	int i = 0;
	if(params.curveselection > 0)
		i = (int)state.values[6+params.curveselection];
	presets[currentpreset].curves[i] = curve(params.curves[i].defaultvalue);
	updatevis = true;
	if(findcurve(i) != -1)
		updatedcurve = true;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SunBurntAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout SunBurntAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"dry"			,1}	,"Dry"					,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"wet"			,1}	,"wet"					,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.75f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"density"		,1}	,"Density"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"length"		,1},"Length"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.65f	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"sync"		,1},"Length (quarter note)"									,0.0f	,16.0f	 ,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"depth"		,1},"Vibrato depth"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.55f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"speed"		,1},"Vibrato speed"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.65f	));
	parameters.push_back(std::make_unique<AudioParameterChoice	>(ParameterID{"curve1"		,1},"Curve 1",StringArray{"None","High-pass","HP resonance","Low-pass","LP resonance","Pan","Density","Shimmer"},1	));
	parameters.push_back(std::make_unique<AudioParameterChoice	>(ParameterID{"curve2"		,1},"Curve 2",StringArray{"None","High-pass","HP resonance","Low-pass","LP resonance","Pan","Density","Shimmer"},3	));
	parameters.push_back(std::make_unique<AudioParameterChoice	>(ParameterID{"curve3"		,1},"Curve 3",StringArray{"None","High-pass","HP resonance","Low-pass","LP resonance","Pan","Density","Shimmer"},5	));
	parameters.push_back(std::make_unique<AudioParameterChoice	>(ParameterID{"curve4"		,1},"Curve 4",StringArray{"None","High-pass","HP resonance","Low-pass","LP resonance","Pan","Density","Shimmer"},7	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"highpass"	,1},"High-pass"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"highpassres"	,1}	,"HP resonance"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"lowpass"		,1}	,"Low-Pass"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"lowpassres"	,1},"LP resonance"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"shimmerpitch",1},"Shimmer pitch"											,-24.0f	,24.0f	 ,12.0f	));
	return { parameters.begin(), parameters.end() };
}
