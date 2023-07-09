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
	if(xx >= points[nextpoint].x) {
		pointhit = true;
		currentpoint = nextpoint;
		++nextpoint;
		while(!points[nextpoint].enabled) ++nextpoint;
		return points[currentpoint].y;
	}
	double interp = curve::calctension((xx-points[currentpoint].x)/(points[nextpoint].x-points[currentpoint].x),points[currentpoint].tension);
	return points[currentpoint].y*(1-interp)+points[nextpoint].y*interp;
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
int SunBurntAudioProcessor::findcurve(int index) {
	for(int i = 1; i < 5; i++)
		if(presets[currentpreset].curveindex[i] == index)
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
	params.jpmode = (props.getUserSettings()->getIntValue("Language")%2) == 0;
	params.uiscale = props.getUserSettings()->getDoubleValue("UIScale");
	if(params.uiscale == 0) params.uiscale = 1.5f;

	//												dry		wet		density	length	depth	speed
	presets[0] = pluginpreset("Default"				,1.0f	,0.75f	,0.5f	,0.65f	,0.55f	,0.65f	);
	presets[1] = pluginpreset("Digital Driver"		,0.27f	,-11.36f,0.53f	,0.0f	,0.0f	,0.4f	);
	presets[2] = pluginpreset("Noisy Bass Pumper"	,0.55f	,20.0f	,0.59f	,0.77f	,0.0f	,0.4f	);
	presets[3] = pluginpreset("Broken Earphone"		,0.19f	,-1.92f	,1.0f	,0.0f	,0.88f	,0.4f	);
	presets[4] = pluginpreset("Fatass"				,0.62f	,-5.28f	,1.0f	,0.0f	,0.0f	,0.4f	);
	presets[5] = pluginpreset("Screaming Alien"		,0.63f	,5.6f	,0.2f	,0.0f	,0.0f	,0.4f	);
	presets[6] = pluginpreset("Bad Connection"		,0.25f	,-2.56f	,0.48f	,0.0f	,0.0f	,0.4f	);
	presets[7] = pluginpreset("Ouch"				,0.9f	,-20.0f	,0.0f	,0.0f	,0.69f	,0.4f	);
	for(int i = 8; i < getNumPrograms(); ++i) {
		presets[i] = presets[0];
		presets[i].name = "Program " + (String)(i-7);
	}

	params.pots[0] = potentiometer("Dry"			,"dry"		,.002f	,presets[0].values[0]	);
	params.pots[1] = potentiometer("Wet"			,"wet"		,.002f	,presets[0].values[1]	);
	params.pots[2] = potentiometer("Density"		,"density"	,0		,presets[0].values[2]	);
	params.pots[3] = potentiometer("Length"			,"length"	,0		,presets[0].values[3]	);
	params.pots[4] = potentiometer("Vibrato Depth"	,"depth"	,.002f	,presets[0].values[4]	);
	params.pots[5] = potentiometer("Vibrato Speed"	,"speed"	,.002f	,presets[0].values[5]	);

	for(int i = 0; i < paramcount; ++i) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(params.pots[i].id,this);
	}
	for(int i = 1; i < 5; ++i) {
		presets[currentpreset].curveindex[i] = apvts.getParameter("curve"+(String)i)->getValue()*7;
		apvts.addParameterListener("curve"+(String)(i),this);
	}
	presets[currentpreset].sync = apvts.getParameter("sync")->getValue()*16;
	apvts.addParameterListener("sync",this);
	presets[currentpreset].filter[0] = apvts.getParameter("highpass")->getValue();
	apvts.addParameterListener("highpass",this);
	presets[currentpreset].filter[1] = apvts.getParameter("lowpass")->getValue();
	apvts.addParameterListener("lowpass",this);
	presets[currentpreset].resonance[0] = apvts.getParameter("highpassres")->getValue();
	apvts.addParameterListener("highpassres",this);
	presets[currentpreset].resonance[1] = apvts.getParameter("lowpassres")->getValue();
	apvts.addParameterListener("lowpassres",this);
	presets[currentpreset].shimmerpitch = apvts.getParameter("shimmerpitch")->getValue()*48-24;
	apvts.addParameterListener("shimmerpitch",this);
}

SunBurntAudioProcessor::~SunBurntAudioProcessor(){
	for(int i = 0; i < paramcount; ++i) apvts.removeParameterListener(params.pots[i].id,this);
	for(int i = 1; i < 5; ++i) apvts.removeParameterListener("curve"+(String)i,this);
	apvts.removeParameterListener("sync",this);
	apvts.removeParameterListener("highpass",this);
	apvts.removeParameterListener("lowpass",this);
	apvts.removeParameterListener("highpassres",this);
	apvts.removeParameterListener("lowpassres",this);
	apvts.removeParameterListener("shimmerpitch",this);
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
	for(int i = 0; i < paramcount; ++i)
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
	updatedcurve = true;
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
	prevpitch = presets[currentpreset].shimmerpitch;
	pitchshift.setPitchSemiTones(presets[currentpreset].shimmerpitch);
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

	//update impulse
	if(updatedcurvecooldown.get() > 0) {
		updatedcurvecooldown = updatedcurvecooldown.get()-((float)numsamples)/samplerate;
		if(updatedcurvecooldown.get() <= 0)
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
		state.values[4] = params.pots[4].smooth.getNextValue();
		state.values[5] = params.pots[5].smooth.getNextValue();

		vibratoindex = (vibratoindex+1)%buffersize;
		double vibratospeed = pow(state.values[5]*2+.2,2);
		vibratophase = fmod(vibratophase+vibratospeed/samplerate,1);
		for (int c = 0; c < channelnum; ++c) {
			vibratobufferdata[c][vibratoindex] = drychanneldata[c][s];
			double vibratodepth = dampvibratodepth.nextvalue(pow(state.values[4],4)/vibratospeed,c);
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
	if(prevpitch != presets[currentpreset].shimmerpitch) {
		prevpitch = presets[currentpreset].shimmerpitch;
		pitchshift.setPitchSemiTones(presets[currentpreset].shimmerpitch);
	}
	if(pitchactive) {
		if(presets[currentpreset].shimmerpitch != 0) {
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
	dsp::AudioBlock<float> wetblock(wetbuffer);
	dsp::AudioBlock<float> effectblock(effectbuffer);
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

	revlength = (int)floor((pow(state.values[3],2)*(6-.1)+.1)*samplerate);

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

	int curveindexmap[8] = { 0,0,0,0,0,0,0,0 };
	iterator[0].reset(presets[currentpreset].curves[0],revlength);
	for(int i = 1; i < 5; ++i) {
		curveindexmap[presets[currentpreset].curveindex[i]] = i;
		iterator[i].reset(presets[currentpreset].curves[presets[currentpreset].curveindex[i]],revlength);
	}

	double out = 0;
	double agc = 0;

	double values[8];
	bool valuesactive[8] { false,false,false,false,false,false,false,false };
	for(int i = 1; i < 5; ++i)
		valuesactive[presets[currentpreset].curveindex[i]] = true;

	for (int s = 0; s < revlength; ++s) {
		values[0] = iterator[0].next();
		values[1] = presets[currentpreset].filter[0];
		values[2] = presets[currentpreset].resonance[0];
		values[3] = presets[currentpreset].filter[1];
		values[4] = presets[currentpreset].resonance[1];
		values[5] = .5;
		values[6] = pow(state.values[2],4);
		values[7] = 0;

		for(int i = 1; i < 5; ++i)
			values[presets[currentpreset].curveindex[i]] = iterator[i].next();

		values[0] *= (random.nextFloat()>.5?1:-1);
		values[1] = mapToLog10(values[1],20.0,20000.0);
		values[2] = mapToLog10(values[2],0.1,40.0);
		values[3] = mapToLog10(values[3],20.0,20000.0);
		values[4] = mapToLog10(values[4],0.1,40.0);

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
			if(valuesactive[3] || values[3] < 1) {
				(*lowpassfilters[c].parameters.get()).setCutOffFrequency(samplerate,values[3],values[4]);
				out = lowpassfilters[c].processSample(out);
			}

			//high pass
			if(valuesactive[1] || values[1] > 0) {
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
		data << presets[i].name << linebreak;
		for(int v = 0; v < paramcount; ++v)
			data << presets[i].values[v] << linebreak;
		data << presets[i].sync << linebreak << presets[i].filter[0] << linebreak << presets[i].filter[1] << linebreak << presets[i].resonance[0] << linebreak << presets[i].resonance[1] << linebreak << presets[i].shimmerpitch << linebreak;
		for(int c = 1; c < 5; ++c) {
			data << presets[i].curveindex[c] << linebreak;
		}
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
			for(int v = 0; v < paramcount; ++v) {
				std::getline(ss, token, '\n');
				presets[i].values[v] = std::stof(token);
			}
			std::getline(ss, token, '\n');
			presets[i].sync = std::stoi(token);
			std::getline(ss, token, '\n');
			presets[i].filter[0] = std::stof(token);
			std::getline(ss, token, '\n');
			presets[i].filter[1] = std::stof(token);
			std::getline(ss, token, '\n');
			presets[i].resonance[0] = std::stof(token);
			std::getline(ss, token, '\n');
			presets[i].resonance[1] = std::stof(token);
			std::getline(ss, token, '\n');
			presets[i].shimmerpitch = std::stoi(token);
			for(int c = 1; c < 5; ++c) {
				std::getline(ss, token, '\n');
				presets[i].curveindex[c] = std::stoi(token);
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
	for(int i = 1; i < 5; ++i) {
		apvts.getParameter("curve"+(String)i)->setValueNotifyingHost(presets[currentpreset].curveindex[i]/7.f);
	}
	apvts.getParameter("sync")->setValueNotifyingHost(presets[currentpreset].sync/16.f);
	apvts.getParameter("highpass")->setValueNotifyingHost(presets[currentpreset].filter[0]);
	apvts.getParameter("lowpass")->setValueNotifyingHost(presets[currentpreset].filter[1]);
	apvts.getParameter("highpassres")->setValueNotifyingHost(presets[currentpreset].resonance[0]);
	apvts.getParameter("lowpassres")->setValueNotifyingHost(presets[currentpreset].resonance[1]);
	apvts.getParameter("shimmerpitch")->setValueNotifyingHost((presets[currentpreset].shimmerpitch+24)/48.f);
	updatedcurve = true;
}
void SunBurntAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; ++i) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		presets[currentpreset].values[i] = newValue;
		if(parameterID == "length" || (parameterID == "density" && findcurve(6) == -1))
			updatedcurvecooldown = .5f;
		return;
	}
	for(int i = 1; i < 5; ++i) if(parameterID == ("curve"+(String)i)) {
		presets[currentpreset].curveindex[i] = newValue;
		updatedcurve = true;
		return;
	}
	if(parameterID == "sync") { //TODO
		presets[currentpreset].sync = newValue;
		updatedcurve = true;
		return;
	}
	if(parameterID == "highpass") {
		presets[currentpreset].filter[0] = newValue;
		if(findcurve(1) == -1)
			updatedcurvecooldown = .5f;
		return;
	}
	if(parameterID == "lowpass") {
		presets[currentpreset].filter[1] = newValue;
		if(findcurve(3) == -1)
			updatedcurvecooldown = .5f;
		return;
	}
	if(parameterID == "highpassres") {
		presets[currentpreset].resonance[0] = newValue;
		if(findcurve(2) == -1)
			updatedcurvecooldown = .5f;
		return;
	}
	if(parameterID == "lowpassres") {
		presets[currentpreset].resonance[1] = newValue;
		if(findcurve(4) == -1)
			updatedcurvecooldown = .5f;
		return;
	}
	if(parameterID == "shimmerpitch") {
		presets[currentpreset].shimmerpitch = newValue;
		return;
	}
}
void SunBurntAudioProcessor::movepoint(int index, float x, float y) {
	int i = presets[currentpreset].curveindex[params.curveselection];
	presets[currentpreset].curves[i].points[index].x = x;
	presets[currentpreset].curves[i].points[index].y = y;
	if(findcurve(i) != -1)
		updatedcurve = true;
}
void SunBurntAudioProcessor::movetension(int index, float tension) {
	int i = presets[currentpreset].curveindex[params.curveselection];
	presets[currentpreset].curves[i].points[index].tension = tension;
	if(findcurve(i) != -1)
		updatedcurve = true;
}
void SunBurntAudioProcessor::addpoint(int index, float x, float y) {
	int i = presets[currentpreset].curveindex[params.curveselection];
	presets[currentpreset].curves[i].points.insert(presets[currentpreset].curves[i].points.begin()+index,point(x,y,presets[currentpreset].curves[i].points[index-1].tension));
	if(findcurve(i) != -1)
		updatedcurve = true;
}
void SunBurntAudioProcessor::deletepoint(int index) {
	int i = presets[currentpreset].curveindex[params.curveselection];
	presets[currentpreset].curves[i].points.erase(presets[currentpreset].curves[i].points.begin()+index);
	if(findcurve(i) != -1)
		updatedcurve = true;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SunBurntAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout SunBurntAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>("dry"			,"Dry"					,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("wet"			,"wet"					,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.75f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("density"		,"Density"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("length"		,"Length"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.65f	));
	parameters.push_back(std::make_unique<AudioParameterInt		>("sync"		,"Time (Quarter note)"									,0.0f	,16.0f	 ,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("depth"		,"Vibrato Depth"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.55f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("speed"		,"Vibrato Speed"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.65f	));
	parameters.push_back(std::make_unique<AudioParameterChoice	>("curve1"		,"Curve 1",StringArray{"None","High Pass","HP Resonance","Low Pass","LP Resonance","Pan","Density","Shimmer"},1	));
	parameters.push_back(std::make_unique<AudioParameterChoice	>("curve2"		,"Curve 2",StringArray{"None","High Pass","HP Resonance","Low Pass","LP Resonance","Pan","Density","Shimmer"},3	));
	parameters.push_back(std::make_unique<AudioParameterChoice	>("curve3"		,"Curve 3",StringArray{"None","High Pass","HP Resonance","Low Pass","LP Resonance","Pan","Density","Shimmer"},5	));
	parameters.push_back(std::make_unique<AudioParameterChoice	>("curve4"		,"Curve 4",StringArray{"None","High Pass","HP Resonance","Low Pass","LP Resonance","Pan","Density","Shimmer"},7	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("highpass"	,"High Pass"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("highpassres"	,"HP Resonance"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("lowpass"		,"Low Pass"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("lowpassres"	,"LP Resonance"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	));
	parameters.push_back(std::make_unique<AudioParameterInt		>("shimmerpitch","Shimmer Pitch"										,-24.0f	,24.0f	 ,12.0f	));
	return { parameters.begin(), parameters.end() };
}
