#include "processor.h"
#include "editor.h"

DietAudioAudioProcessor::DietAudioAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts),
	fft(fftorder) {

	init();

	presets[0] = pluginpreset("Default"	,0.0f	,0.0f	,1.0f	,0.0f);
	for(int i = 1; i < getNumPrograms(); ++i) {
		presets[i] = presets[0];
		presets[i].name = "Program " + (String)(i-7);
	}

	params.pots[0] = potentiometer("Threshold"	,"thresh"		,0	,presets[0].values[0]	);
	params.pots[1] = potentiometer("Release"	,"release"		,0	,presets[0].values[1]	);
	params.pots[2] = potentiometer("Transients"	,"transients"	,0	,presets[0].values[2]	);
	params.pots[3] = potentiometer("The rest"	,"rest"			,0	,presets[0].values[3]	);

	for(int i = 0; i < paramcount; ++i) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		add_listener(params.pots[i].id);
	}

	dsp::WindowingFunction<float> window(fftsize, dsp::WindowingFunction<float>::hann, false);
	float windowresponse[fftsize];
	for(int i = 0; i < fftsize; ++i)
		windowresponse[i] = 1;
	window.multiplyWithWindowingTable(windowresponse,fftsize);
	for(int i = 0; i < (fftsize/2+1); ++i)
		sqrthann[i] = sqrt(windowresponse[i]);
}

DietAudioAudioProcessor::~DietAudioAudioProcessor(){
	close();
}

const String DietAudioAudioProcessor::getName() const { return "Diet Audio"; }
bool DietAudioAudioProcessor::acceptsMidi() const { return false; }
bool DietAudioAudioProcessor::producesMidi() const { return false; }
bool DietAudioAudioProcessor::isMidiEffect() const { return false; }
double DietAudioAudioProcessor::getTailLengthSeconds() const {
	return fftsize/samplerate;
}

int DietAudioAudioProcessor::getNumPrograms() { return 20; }
int DietAudioAudioProcessor::getCurrentProgram() { return currentpreset; }
void DietAudioAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	for(int i = 0; i < paramcount; i++) {
		if(lerpstage < .001 || lerpchanged[i])
			lerptable[i] = params.pots[i].normalize(presets[currentpreset].values[i]);
		lerpchanged[i] = false;
	}
	currentpreset = index;

	if(lerpstage <= 0) {
		lerpstage = 1;
		startTimerHz(30);
	} else lerpstage = 1;
}
void DietAudioAudioProcessor::timerCallback() {
	lerpstage *= .64f;
	if(lerpstage < .001) {
		for(int i = 0; i < paramcount; i++) if(!lerpchanged[i])
			apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		lerpstage = 0;
		undo_manager.beginNewTransaction();
		stopTimer();
		return;
	}
	for(int i = 0; i < paramcount; i++) if(!lerpchanged[i]) {
		lerptable[i] = (params.pots[i].normalize(presets[currentpreset].values[i])-lerptable[i])*.36f+lerptable[i];
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(lerptable[i]);
	}
}
const String DietAudioAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void DietAudioAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void DietAudioAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;
	setLatencySamples(fftsize);

	reseteverything();
}
void DietAudioAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void DietAudioAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
		params.pots[i].smooth.reset(samplerate, params.pots[i].smoothtime);

	preparedtoplay = true;

	envelopefollower.setchannelcount(channelnum*fftsize);
	envelopefollower.setsamplerate(samplerate/(fftsize/2));
	envelopefollower.setattack(0);
	envelopefollower.setrelease(calculaterelease(state.values[1]));
	prebuffer.resize(channelnum*fftbuffersize);
	postbuffer.resize(channelnum*fftbuffersize*2);
	for(int i = 0; i < channelnum*fftbuffersize; ++i)
		prebuffer[i] = 0;
	for(int i = 0; i < channelnum*fftbuffersize*2; ++i)
		postbuffer[i] = 0;
	altfifo = false;
	fifoindex = 0;
}
void DietAudioAudioProcessor::releaseResources() { }

bool DietAudioAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void DietAudioAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	saved = true;

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	float prmsadd = rmsadd.get();
	int prmscount = rmscount.get();

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);
	float* const* channeldata = buffer.getArrayOfWritePointers();
	for(int sample = 0; sample < numsamples; ++sample) {

		if((fifoindex >= (fftsize/2) && !altfifo) || fifoindex >= fftsize) {
			zeromem(&postbuffer[altfifo?(channelnum*fftbuffersize):0],sizeof(float)*channelnum*fftbuffersize);
			if(altfifo) {
				memcpy(&postbuffer[channelnum*fftbuffersize],&prebuffer[0],sizeof(float)*channelnum*fftbuffersize);
			} else {
				for(int channel = 0; channel < channelnum; ++channel) {
					memcpy(&postbuffer[channel*fftbuffersize          ],&prebuffer[fftsize/2],sizeof(float)*fftsize/2);
					memcpy(&postbuffer[channel*fftbuffersize+fftsize/2],&prebuffer[        0],sizeof(float)*fftsize/2);
				}
			}

			double threshold = calculatethreshold(state.values[0]);
			for(int channel = 0; channel < channelnum; ++channel) {
				for(int i = 0; i < fftsize; ++i)
					postbuffer[((altfifo?channelnum:0)+channel)*fftbuffersize+i] *= sqrthann[i<(fftsize/2)?i:(fftsize-i)];
				fft.performRealOnlyForwardTransform(&postbuffer[((altfifo?channelnum:0)+channel)*fftbuffersize]);

				for(int band = 0; band < fftsize; ++band) {
					float real = postbuffer[((altfifo?channelnum:0)+channel)*fftbuffersize+band];
					float imag = postbuffer[((altfifo?channelnum:0)+channel)*fftbuffersize+band+1];
					float magnitude = std::sqrt((real*real)+(imag*imag));
					//float phase = atan2(imag,real);
					float slope = Decibels::decibelsToGain(log2((band*samplerate*.5f)/fftsize)*4.5f);

					float amp = envelopefollower.process(((magnitude*slope)>threshold)?1:0,channel*fftsize+band);
					amp = amp*state.values[2]+(1-amp)*state.values[3];

					postbuffer[((altfifo?channelnum:0)+channel)*fftbuffersize+band  ] *= amp;
					postbuffer[((altfifo?channelnum:0)+channel)*fftbuffersize+band+1] *= amp;
					//postbuffer[((altfifo?channelnum:0)+channel)*fftbuffersize+band  ] = magnitude*cos(phase);
					//postbuffer[((altfifo?channelnum:0)+channel)*fftbuffersize+band+1] = magnitude*sin(phase);
				}

				fft.performRealOnlyInverseTransform(&postbuffer[((altfifo?channelnum:0)+channel)*fftbuffersize]);
			}

			if(altfifo) fifoindex = 0;
			altfifo = !altfifo;
		}

		for(int i = 0; i < paramcount; ++i) if(params.pots[i].smoothtime > 0)
			state.values[i] = params.pots[i].smooth.getNextValue();

		for(int channel = 0; channel < channelnum; ++channel) {
			prebuffer[channel*fftbuffersize+fifoindex] = channeldata[channel][sample];
			channeldata[channel][sample] = (
				postbuffer[channel*fftbuffersize+fifoindex+fftsize*(altfifo?-.5:.5)]*sqrthann[altfifo?(fifoindex-fftsize/2):(fftsize/2-fifoindex)]+
				postbuffer[channel*fftbuffersize+fifoindex+channelnum*fftbuffersize]*sqrthann[altfifo?(fftsize-fifoindex):fifoindex]);

			if(prmscount < samplerate*2) {
				prmsadd += channeldata[channel][sample]*channeldata[channel][sample];
				++prmscount;
			}
		}
		++fifoindex;
	}

	rmsadd = prmsadd;
	rmscount = prmscount;
}

bool DietAudioAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* DietAudioAudioProcessor::createEditor() {
	return new DietAudioAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void DietAudioAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << delimiter;
		data << get_preset(i) << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void DietAudioAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	saved = true;
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		for(int i = 0; i < getNumPrograms(); i++) {
			std::getline(ss, token, delimiter);
			presets[i].name = token;

			std::getline(ss, token, delimiter);
			set_preset(token, i, ',', true);
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
const String DietAudioAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int v = 0; v < paramcount; v++)
		data << presets[preset_id].values[v] << delimiter;

	return data.str();
}
void DietAudioAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(preset_id);
	try {
		std::stringstream ss(preset.trim().toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int save_version = std::stoi(token);

		for(int v = 0; v < paramcount; v++) {
			std::getline(ss, token, delimiter);
			presets[preset_id].values[v] = std::stof(token);
		}

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

	for(int i = 0; i < paramcount; i++) {
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
	}
}

void DietAudioAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else {
			state.values[i] = newValue;

			if(parameterID == "release")
				envelopefollower.setrelease(calculaterelease(newValue));
		}
		if(lerpstage < .001 || lerpchanged[i]) presets[currentpreset].values[i] = newValue;
		return;
	}
}
double DietAudioAudioProcessor::calculaterelease(double value) {
	double timestwo = value*38.7298334621;
	return timestwo*timestwo;
}
double DietAudioAudioProcessor::calculatethreshold(double value) {
	return pow(value,2)*10000;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DietAudioAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout DietAudioAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"thresh"		,1},"Threshold"		,juce::NormalisableRange<float>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tothreshold	).withValueFromStringFunction(fromthreshold	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"release"		,1},"Release"		,juce::NormalisableRange<float>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(torelease		).withValueFromStringFunction(fromrelease	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"transients"	,1},"Transients"	,juce::NormalisableRange<float>(0.0f	,1.0f	),1.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(todb			).withValueFromStringFunction(fromdb		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"rest"		,1},"The rest"		,juce::NormalisableRange<float>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(todb			).withValueFromStringFunction(fromdb		)));
	return { parameters.begin(), parameters.end() };
}
