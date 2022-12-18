#include "PluginProcessor.h"
#include "PluginEditor.h"

RedBassAudioProcessor::RedBassAudioProcessor() :
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
	apvts(*this, &undoManager, "Parameters", createParameters())
{
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
		apvts.addParameterListener(params.pots[i].id, this);
	}
	params.monitor = apvts.getParameter("monitor")->getValue() > .5 ? 1 : 0;
	params.monitorsmooth.setCurrentAndTargetValue(params.monitor);
	apvts.addParameterListener("monitor", this);
}

RedBassAudioProcessor::~RedBassAudioProcessor(){
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(params.pots[i].id, this);
	apvts.removeParameterListener("monitor", this);
}

const String RedBassAudioProcessor::getName() const { return "Red Bass"; }
bool RedBassAudioProcessor::acceptsMidi() const { return false; }
bool RedBassAudioProcessor::producesMidi() const { return false; }
bool RedBassAudioProcessor::isMidiEffect() const { return false; }
double RedBassAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int RedBassAudioProcessor::getNumPrograms() { return 20; }
int RedBassAudioProcessor::getCurrentProgram() { return currentpreset; }
void RedBassAudioProcessor::setCurrentProgram (int index) {
	if(currentpreset == index) return;

	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
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
		undoManager.beginNewTransaction();
		stopTimer();
		return;
	}
	for(int i = 0; i < paramcount; i++) if(!lerpchanged[i]) {
		lerptable[i] = (params.pots[i].normalize(presets[currentpreset].values[i])-lerptable[i])*.36f+lerptable[i];
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(lerptable[i]);
	}
}
const String RedBassAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void RedBassAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
}

void RedBassAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
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

bool RedBassAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void RedBassAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float prmsadd = rmsadd.get();
	int prmscount = rmscount.get();

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);

	float* const* channelData = buffer.getArrayOfWritePointers();

	double sidechain = 0;
	double osc = 0;
	for (int sample = 0; sample < numsamples; ++sample) {
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
	return pow(value,2)*.7f;
}

bool RedBassAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* RedBassAudioProcessor::createEditor() {
	return new RedBassAudioProcessorEditor(*this,paramcount,state,params);
}

void RedBassAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;

	data << version << linebreak
		<< currentpreset << linebreak
		<< params.monitorsmooth.getTargetValue() << linebreak;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << linebreak;
		for(int v = 0; v < paramcount; v++)
			data << presets[i].values[v] << linebreak;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void RedBassAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, '\n');
		int saveversion = std::stoi(token);

		if(saveversion >= 2) {
			std::getline(ss, token, '\n');
			currentpreset = std::stoi(token);

			std::getline(ss, token, '\n');
			params.monitor = std::stof(token) > .5 ? 1 : 0;

			for(int i = 0; i < getNumPrograms(); i++) {
				std::getline(ss, token, '\n');
				presets[i].name = token;
				for(int v = 0; v < paramcount; v++) {
					std::getline(ss, token, '\n');
					presets[i].values[v] = std::stof(token);
				}
			}
		} else {
			for(int i = 0; i < 8; i++) {
				std::getline(ss, token, '\n');
				int ii = i<5?i:(i-1);
				if(i == 5) params.monitor = std::stof(token) > .5 ? 1 : 0;
				else {
					float val = std::stof(token);
					if(saveversion <= 0 && params.pots[ii].id == "threshold") val = pow(val/.7,.5);
					presets[currentpreset].values[ii] = val;
				}
			}
		}
	} catch (const char* e) {
		logger.debug((String)"Error loading saved data: "+(String)e);
	} catch(String e) {
		logger.debug((String)"Error loading saved data: "+e);
	} catch(std::exception &e) {
		logger.debug((String)"Error loading saved data: "+(String)e.what());
	} catch(...) {
		logger.debug((String)"Error loading saved data");
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

AudioProcessorValueTreeState::ParameterLayout RedBassAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>("freq"		,"Frequency"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.58f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("threshold"	,"Threshold"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.02f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("attack"		,"Attack"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.17f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("release"		,"Release"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.18f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("lowpass"		,"Sidechain lowpass",juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("monitor"		,"Monitor sidechain"												 ,false	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("dry"			,"Dry"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("wet"			,"Wet"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.18f	));
	return { parameters.begin(), parameters.end() };
}