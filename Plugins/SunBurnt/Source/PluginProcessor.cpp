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
	double xx = ((double)++x)/width;
	if(xx >= 1) return points[points.size()-1].y;
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

	presets[0] = pluginpreset("Default"				,1.0f	,0.75f	,0.5f	,0.65f	,0.0f	,.5f	);
	presets[1] = pluginpreset("Digital Driver"		,0.27f	,-11.36f,0.53f	,0.0f	,0.0f	,.4f	);
	presets[2] = pluginpreset("Noisy Bass Pumper"	,0.55f	,20.0f	,0.59f	,0.77f	,0.0f	,.4f	);
	presets[3] = pluginpreset("Broken Earphone"		,0.19f	,-1.92f	,1.0f	,0.0f	,0.88f	,.4f	);
	presets[4] = pluginpreset("Fatass"				,0.62f	,-5.28f	,1.0f	,0.0f	,0.0f	,.4f	);
	presets[5] = pluginpreset("Screaming Alien"		,0.63f	,5.6f	,0.2f	,0.0f	,0.0f	,.4f	); 
	presets[6] = pluginpreset("Bad Connection"		,0.25f	,-2.56f	,0.48f	,0.0f	,0.0f	,.4f	);
	presets[7] = pluginpreset("Ouch"				,0.9f	,-20.0f	,0.0f	,0.0f	,0.69f	,.4f	);
	for(int i = 8; i < getNumPrograms(); ++i) {
		presets[i] = presets[0];
		presets[i].name = "Program " + (String)(i-7);
	}

	params.pots[0] = potentiometer("Dry"			,"dry"		,.002f	,presets[0].values[0]	);
	params.pots[1] = potentiometer("Wet"			,"wet"		,.002f	,presets[0].values[1]	);
	params.pots[2] = potentiometer("Density"		,"density"	,0		,presets[0].values[2]	);
	params.pots[3] = potentiometer("Length"			,"length"	,0		,presets[0].values[3]	);
	params.pots[4] = potentiometer("Vibrato Depth"	,"depth"	,.001f	,presets[0].values[4]	);
	params.pots[5] = potentiometer("Vibrato Speed"	,"speed"	,.001f	,presets[0].values[5]	);

	for(int i = 0; i < paramcount; ++i) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(params.pots[i].id, this);
	}
	for(int i = 1; i < 5; ++i) {
		state.curves[i].enabled = apvts.getParameter(curveid[i])->getValue()>.5;
		apvts.addParameterListener(curveid[i],this);
	}
	apvts.addParameterListener("highpassres",this);
	apvts.addParameterListener("lowpassres",this);
}

SunBurntAudioProcessor::~SunBurntAudioProcessor(){
	for(int i = 0; i < paramcount; ++i) apvts.removeParameterListener(params.pots[i].id, this);
	for(int i = 1; i < 5; ++i) apvts.removeParameterListener(curveid[i],this);
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

	dsp::ProcessSpec spec;
	dsp::ProcessSpec monospec;
	spec.sampleRate = samplerate;
	monospec.sampleRate = samplerate;
	spec.maximumBlockSize = samplesperblock;
	monospec.maximumBlockSize = samplesperblock;
	spec.numChannels = channelnum;
	monospec.numChannels = 1;

	highpassfilters.resize(channelnum);
	lowpassfilters.resize(channelnum);
	impulsechanneldata.resize(channelnum);
	wetbuffer.setSize(channelnum,samplesperblock);

	revlength = 0;
	if(channelnum > 2) {
		impulsebuffer.resize(channelnum);
		convolver.resize(channelnum);
	} else {
		impulsebuffer.resize(1);
		convolver.resize(1);
		convolver[0].reset(new dsp::Convolution);
		convolver[0]->prepare(spec);
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
			convolver[c]->prepare(spec);
		}
	}

	updatedcurve = true;
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

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	int numsamples = buffer.getNumSamples();

	float* const* drychanneldata = buffer.getArrayOfWritePointers();
	float* const* wetchanneldata = wetbuffer.getArrayOfWritePointers();

	//update buffer if necessary
	if(updatedcurve.get()) {
		genbuffer();

		if(channelnum > 2) {
			for(int c = 0; c < channelnum; ++c)
				convolver[c]->loadImpulseResponse(std::move(impulsebuffer[c]),samplerate,dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
		} else {
			convolver[0]->loadImpulseResponse(std::move(impulsebuffer[0]),samplerate,channelnum==2?dsp::Convolution::Stereo::yes:dsp::Convolution::Stereo::no,dsp::Convolution::Trim::no,dsp::Convolution::Normalise::no);
		}
	}

	//pitch up if necessary

	//convolve
	if(channelnum > 2) {
		dsp::AudioBlock<float> dryblock(buffer);
		dsp::AudioBlock<float> wetblock(wetbuffer);
		for(int c = 0; c < channelnum; ++c) {
			dsp::AudioBlock<float> wetblocksinglechannel = wetblock.getSingleChannelBlock(c);
			dsp::ProcessContextNonReplacing<float> context(dryblock.getSingleChannelBlock(c),wetblocksinglechannel);
			convolver[c]->process(context);
		}
	} else {
		dsp::AudioBlock<float> wetblock(wetbuffer);
		dsp::ProcessContextNonReplacing<float> context(dsp::AudioBlock<float>(buffer),wetblock);
		convolver[0]->process(context);
	}

	for (int s = 0; s < numsamples; ++s) {
		for(int i = 0; i < paramcount; ++i) if(params.pots[i].smoothtime > 0)
			state.values[i] = params.pots[i].smooth.getNextValue();

		for (int c = 0; c < channelnum; ++c) {
			//dry wet
			drychanneldata[c][s] = drychanneldata[c][s]*state.values[0]+wetchanneldata[c][s]*pow(state.values[1]*2,2);
		}
	}
}
//TODO: move the bitch onto a different thread
void SunBurntAudioProcessor::genbuffer() {
	if(channelnum <= 0) return;
	updatedcurve = false;

	revlength = (int)floor((pow(state.values[3],2)*(6-.1)+.1)*samplerate);

	if(channelnum > 2) {
		for(int c = 0; c < channelnum; ++c) {
			impulsebuffer[c] = AudioBuffer<float>(1,revlength);
			impulsechanneldata[c] = impulsebuffer[c].getWritePointer(0);
		}
	} else {
		impulsebuffer[0] = AudioBuffer<float>(channelnum,revlength);
		for(int c = 0; c < channelnum; ++c) {
			impulsechanneldata[c] = impulsebuffer[0].getWritePointer(c);
		}
	}

	for(int i = 0; i < 5; ++i)
		iterator[i].reset(presets[currentpreset].curves[i],revlength);

	double density = pow(state.values[2],4);
	double out = 0;
	double agc = 0;
	for (int s = 0; s < revlength; ++s) {
		double highpass = mapToLog10(iterator[1].next(),20.0,20000.0);
		double lowpass = mapToLog10(iterator[2].next(),20.0,20000.0);
		double pan = iterator[3].next();
		//double vol = iterator[0].next()*(1-pow(density,.04)*pow(fmax(1-density*200,0),.5)*.98)*(random.nextFloat()>.5?1:-1);
		double vol = iterator[0].next()*(random.nextFloat()>.5?1:-1);

		for (int c = 0; c < channelnum; ++c) {

			//vol
			if(iterator[0].pointhit) {
				out = vol;
			} else if(random.nextFloat() < density) {
				float r = random.nextFloat();
				agc += r*r;
				out = vol*r;
			} else {
				out = 0;
			}

			//pan
			if(presets[currentpreset].curves[3].enabled && channelnum > 1) {
				double linearpan = (((double)c)/(channelnum-1))*(1-pan*2)+pan;
				out *= sin(linearpan*1.5707963268)*.7071067812+linearpan;
			}

			//low pass
			if(presets[currentpreset].curves[2].enabled) {
				(*lowpassfilters[c].parameters.get()).setCutOffFrequency(samplerate,lowpass,mapToLog10((double)presets[currentpreset].resonance[1],0.1,40.0));
				out = lowpassfilters[c].processSample(out);
			}

			//high pass
			if(presets[currentpreset].curves[1].enabled) {
				(*highpassfilters[c].parameters.get()).setCutOffFrequency(samplerate,highpass,mapToLog10((double)presets[currentpreset].resonance[0],0.1,40.0));
				out = highpassfilters[c].processSample(out);
			}

			impulsechanneldata[c][s] = out;
		}
		iterator[0].pointhit = false;
	}
	agc = 1/fmax(sqrt(agc/channelnum),1);
	for (int c = 0; c < channelnum; ++c)
		for (int s = 0; s < revlength; ++s)
			impulsechanneldata[c][s] *= agc;
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
		data << presets[i].resonance[0] << linebreak << presets[i].resonance[1] << linebreak;
		for(int c = 0; c < 5; ++c) {
			data << (presets[i].curves[c].enabled?1:0) << linebreak << presets[i].curves[c].points.size() << linebreak;
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
			presets[i].resonance[0] = std::stof(token);
			std::getline(ss, token, '\n');
			presets[i].resonance[1] = std::stof(token);
			for(int c = 0; c < 5; ++c) {
				std::getline(ss, token, '\n');
				presets[i].curves[c].enabled = std::stof(token) == 1;
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
		if(parameterID == "length" || parameterID == "density")
			updatedcurve = true;
		return;
	}
	for(int i = 1; i < 5; ++i) {
		if(parameterID == curveid[i]) {
			presets[currentpreset].curves[i].enabled = newValue>.5;
			updatedcurve = true;
			return;
		}
		if(i < 3 && parameterID == (curveid[i]+"res")) {
			presets[currentpreset].resonance[i-1] = newValue;
			updatedcurve = true;
			return;
		}
	}
}
void SunBurntAudioProcessor::movepoint(int index, float x, float y) {
	presets[currentpreset].curves[params.curveselection].points[index].x = x;
	presets[currentpreset].curves[params.curveselection].points[index].y = y;
	if(presets[currentpreset].curves[params.curveselection].enabled)
		updatedcurve = true;
}
void SunBurntAudioProcessor::movetension(int index, float tension) {
	presets[currentpreset].curves[params.curveselection].points[index].tension = tension;
	if(presets[currentpreset].curves[params.curveselection].enabled)
		updatedcurve = true;
}
void SunBurntAudioProcessor::addpoint(int index, float x, float y) {
	presets[currentpreset].curves[params.curveselection].points.insert(presets[currentpreset].curves[params.curveselection].points.begin()+index,point(x,y,presets[currentpreset].curves[params.curveselection].points[index-1].tension));
	if(presets[currentpreset].curves[params.curveselection].enabled)
		updatedcurve = true;
}
void SunBurntAudioProcessor::deletepoint(int index) {
	presets[currentpreset].curves[params.curveselection].points.erase(presets[currentpreset].curves[params.curveselection].points.begin()+index);
	if(presets[currentpreset].curves[params.curveselection].enabled)
		updatedcurve = true;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SunBurntAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout SunBurntAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>("dry"			,"Dry"					,juce::NormalisableRange<float>(0.0f,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("wet"			,"wet"					,juce::NormalisableRange<float>(0.0f,1.0f	),0.75f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("density"		,"Density"				,juce::NormalisableRange<float>(0.0f,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("length"		,"Length"				,juce::NormalisableRange<float>(0.0f,1.0f	),0.65f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("depth"		,"Vibrato Depth"		,juce::NormalisableRange<float>(0.0f,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("speed"		,"Vibrato Speed"		,juce::NormalisableRange<float>(0.0f,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("highpass"	,"High Pass Enable"													 ,true	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("highpassres"	,"High Pass Resonance"	,juce::NormalisableRange<float>(0.0f,1.0f	),0.3f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("lowpass"		,"Low Pass Enable"													 ,true	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("lowpassres"	,"Low Pass Resonance"	,juce::NormalisableRange<float>(0.0f,1.0f	),0.3f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("pan"			,"Pan Enable"														 ,true	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("shimmer"		,"Shimmer Enable"													 ,false	));
	return { parameters.begin(), parameters.end() };
}
