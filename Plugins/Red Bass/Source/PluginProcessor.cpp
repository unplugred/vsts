#include "PluginProcessor.h"
#include "PluginEditor.h"

RedBassAudioProcessor::RedBassAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	for(int i = 0; i < getNumPrograms(); i++)
		presets[i].name = "Program " + (String)(i+1);

	params.pots[0] = potentiometer("Frequency"			,"freq"		,.001f	,presets[0].values[0]	);
	params.pots[1] = potentiometer("Threshold"			,"threshold",0		,presets[0].values[1]	);
	params.pots[2] = potentiometer("Attack"				,"attack"	,0		,presets[0].values[2]	);
	params.pots[3] = potentiometer("Release"			,"release"	,0		,presets[0].values[3]	);
	params.pots[4] = potentiometer("Sidechain lowpass"	,"lowpass"	,0		,presets[0].values[4]		);
	params.pots[5] = potentiometer("Dry"				,"dry"		,.001f	,presets[0].values[5]		);
	params.pots[6] = potentiometer("Wet"				,"wet"		,.01f	,presets[0].values[6]	);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		add_listener(params.pots[i].id);
	}
	params.monitor = apvts.getParameter("monitor")->getValue() > .5 ? 1 : 0;
	params.monitorsmooth.setCurrentAndTargetValue(params.monitor);
	add_listener("monitor");
}

RedBassAudioProcessor::~RedBassAudioProcessor(){
	close();
}

const String RedBassAudioProcessor::getName() const { return "Red Bass"; }
bool RedBassAudioProcessor::acceptsMidi() const { return false; }
bool RedBassAudioProcessor::producesMidi() const { return false; }
bool RedBassAudioProcessor::isMidiEffect() const { return false; }
double RedBassAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int RedBassAudioProcessor::getNumPrograms() { return 20; }
int RedBassAudioProcessor::getCurrentProgram() { return currentpreset; }
void RedBassAudioProcessor::setCurrentProgram(int index) {
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
void RedBassAudioProcessor::timerCallback() {
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
const String RedBassAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void RedBassAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void RedBassAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void RedBassAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;
}
void RedBassAudioProcessor::reseteverything() {
	if(samplesperblock <= 0) return;

	for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
		params.pots[i].smooth.reset(samplerate, params.pots[i].smoothtime);
	params.monitorsmooth.reset(samplerate, .01f);

	envelopefollower.setattack(calculateattack(state.values[2]), samplerate);
	envelopefollower.setrelease(calculaterelease(state.values[3]), samplerate);
	envelopefollower.setthreshold(calculatethreshold(state.values[1]), samplerate);

	dsp::ProcessSpec spec;
	spec.sampleRate = samplerate;
	spec.maximumBlockSize = samplesperblock;
	spec.numChannels = 1;
	filter.reset();
	(*filter.parameters.get()).setCutOffFrequency(samplerate,calculatelowpass(state.values[4]));
	(*filter.parameters.get()).type = dsp::StateVariableFilter::Parameters<float>::Type::lowPass;
	filter.prepare(spec);
}
void RedBassAudioProcessor::releaseResources() { }

bool RedBassAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void RedBassAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	float prmsadd = rmsadd.get();
	int prmscount = rmscount.get();

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);

	float* const* channelData = buffer.getArrayOfWritePointers();

	double sidechain = 0;
	double osc = 0;
	for(int sample = 0; sample < numsamples; ++sample) {
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			state.values[i] = params.pots[i].smooth.getNextValue();
		params.monitor = params.monitorsmooth.getNextValue();

		sidechain = 0;

		double lvl = fmin(20*envelopefollower.envelope*state.values[6],1);

		if(prmscount < samplerate*2) {
			prmsadd += lvl;
			prmscount++;
		}

		crntsmpl = fmod(crntsmpl+(calculatefrequency(state.values[0]) / getSampleRate()), 1);
		if(params.monitor >= 1) osc = 0;
		else osc = sin(crntsmpl*MathConstants<double>::twoPi)*lvl;

		for(int channel = 0; channel < channelnum; ++channel) {
			sidechain += channelData[channel][sample];
			if(params.monitor < 1)
				channelData[channel][sample] = (osc+((double)channelData[channel][sample])*state.values[5])*(1-params.monitor);
			else channelData[channel][sample] = 0;
		}

		sidechain = sidechain/channelnum;

		if(state.values[4] < 1) sidechain = filter.processSample(sidechain);
		else filter.processSample(sidechain);
		if(params.monitor > 0) for(int channel = 0; channel < channelnum; ++channel)
			channelData[channel][sample] += sidechain*params.monitor;

		envelopefollower.process(sidechain);
	}

	rmsadd = prmsadd;
	rmscount = prmscount;
}
double RedBassAudioProcessor::calculateattack(double value) {
	double timestwo = value*15.2896119631+7.0710678119;
	return timestwo*timestwo;
}
double RedBassAudioProcessor::calculaterelease(double value) {
	double timestwo = value*26.4823847482+12.2474487139;
	return timestwo*timestwo;
}
double RedBassAudioProcessor::calculatelowpass(double value) {
	return mapToLog10(value,100.0,20000.0);
}
double RedBassAudioProcessor::calculatefrequency(double value) {
	return mapToLog10(value,20.0,100.0);
}
double RedBassAudioProcessor::calculatethreshold(double value) {
	return pow(value,2)*.7079f;
}

bool RedBassAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* RedBassAudioProcessor::createEditor() {
	return new RedBassAudioProcessorEditor(*this,paramcount,state,params);
}

void RedBassAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter
		<< params.monitorsmooth.getTargetValue() << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << delimiter;
		for(int v = 0; v < paramcount; v++)
			data << presets[i].values[v] << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void RedBassAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		if(saveversion >= 2) {
			std::getline(ss, token, delimiter);
			currentpreset = std::stoi(token);

			std::getline(ss, token, delimiter);
			params.monitor = std::stof(token) > .5 ? 1 : 0;

			for(int i = 0; i < getNumPrograms(); i++) {
				std::getline(ss, token, delimiter);
				presets[i].name = token;
				for(int v = 0; v < paramcount; v++) {
					std::getline(ss, token, delimiter);
					float val = std::stof(token);
					if(saveversion <= 2 && params.pots[v].id == "threshold") val = pow(pow(val,2)*7/7.079,.5);
					presets[i].values[v] = std::stof(token);
				}
			}
		} else {
			for(int i = 0; i < 8; i++) {
				std::getline(ss, token, delimiter);
				int ii = i<5?i:(i-1);
				if(i == 5) params.monitor = std::stof(token) > .5 ? 1 : 0;
				else {
					float val = std::stof(token);
					if(params.pots[ii].id == "threshold") {
						if(saveversion <= 0) val = pow(val/.7,.5);
						else val = pow(pow(val,2)*7/7.079,.5);
					}
					presets[currentpreset].values[ii] = val;
				}
			}
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

	for(int i = 0; i < paramcount; i++) {
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		if(params.pots[i].smoothtime > 0) {
			params.pots[i].smooth.setCurrentAndTargetValue(presets[currentpreset].values[i]);
			state.values[i] = presets[currentpreset].values[i];
		}
	}
	apvts.getParameter("monitor")->setValueNotifyingHost(params.monitor);
	params.monitorsmooth.setCurrentAndTargetValue(params.monitor);
}
const String RedBassAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int v = 0; v < paramcount; v++)
		data << presets[preset_id].values[v] << delimiter;

	return data.str();
}
void RedBassAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(preset_id);
	try {
		std::stringstream ss(preset.trim().toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int save_version = std::stoi(token);

		for(int v = 0; v < paramcount; v++) {
			std::getline(ss, token, delimiter);
			float val = std::stof(token);
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

void RedBassAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "monitor") {
		params.monitorsmooth.setTargetValue(newValue>.5?1:0);
		return;
	}
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else {
			state.values[i] = newValue;

			if(parameterID == "attack")
				envelopefollower.setattack(calculateattack(state.values[2]), samplerate);
			else if(parameterID == "release")
				envelopefollower.setrelease(calculaterelease(state.values[3]), samplerate);
			else if(parameterID == "threshold")
				envelopefollower.setthreshold(calculatethreshold(state.values[1]), samplerate);
			else if(parameterID == "lowpass")
				(*filter.parameters.get()).setCutOffFrequency(samplerate,calculatelowpass(state.values[4]));
		}
		if(lerpstage < .001 || lerpchanged[i]) presets[currentpreset].values[i] = newValue;
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RedBassAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout RedBassAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"freq"		,1},"Frequency"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.58f	,"",AudioProcessorParameter::genericParameter	,tofreq			,fromfreq		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"threshold"	,1},"Threshold"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.02f	,"",AudioProcessorParameter::genericParameter	,tothreshold	,fromthreshold	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"attack"		,1},"Attack"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.17f	,"",AudioProcessorParameter::genericParameter	,toattack		,fromattack		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"release"		,1},"Release"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.18f	,"",AudioProcessorParameter::genericParameter	,torelease		,fromrelease	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"lowpass"		,1},"Sidechain lowpass"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,"",AudioProcessorParameter::genericParameter	,tocutoff		,fromcutoff		));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"monitor"		,1},"Monitor sidechain"													 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"dry"			,1},"Dry"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"wet"			,1},"Wet"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.18f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	return { parameters.begin(), parameters.end() };
}
