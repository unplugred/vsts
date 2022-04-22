/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

RedBassAudioProcessor::RedBassAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	pots[0] = potentiometer("Frequency"			,"freq"		,.001f	,0.48f	);
	pots[1] = potentiometer("Threshold"			,"threshold",0		,0.02f	);
	pots[2] = potentiometer("Attack"			,"attack"	,0		,0.17f	);
	pots[3] = potentiometer("Release"			,"release"	,0		,0.18f	);
	pots[4] = potentiometer("Sidechain lowpass"	,"lowpass"	,0		,0		);
	pots[5] = potentiometer("Monitor sidechain"	,"monitor"	,.01f	,0		,0.f	,1.f	,false	,potentiometer::ptype::booltype);
	pots[6] = potentiometer("Dry"				,"dry"		,.001f	,1		);
	pots[7] = potentiometer("Wet"				,"wet"		,.001f	,0.18f	);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = pots[i].inflate(apvts.getParameter(pots[i].id)->getValue());
		if(pots[i].smoothtime > 0) pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(pots[i].id, this);
	}
}

RedBassAudioProcessor::~RedBassAudioProcessor(){
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(pots[i].id, this);
}

const String RedBassAudioProcessor::getName() const { return "Red Bass"; }
bool RedBassAudioProcessor::acceptsMidi() const { return false; }
bool RedBassAudioProcessor::producesMidi() const { return false; }
bool RedBassAudioProcessor::isMidiEffect() const { return false; }
double RedBassAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int RedBassAudioProcessor::getNumPrograms() { return 1; }
int RedBassAudioProcessor::getCurrentProgram() { return 0; }
void RedBassAudioProcessor::setCurrentProgram (int index) { }
const String RedBassAudioProcessor::getProgramName (int index) { return { "hello" }; }
void RedBassAudioProcessor::changeProgramName (int index, const String& newName) { }

void RedBassAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0)
		pots[i].smooth.reset(sampleRate*(state.values[6]+1), pots[i].smoothtime);
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	envelopefollower.setattack(calculateattack(state.values[2]), sampleRate);
	envelopefollower.setrelease(calculaterelease(state.values[3]), sampleRate);
	envelopefollower.setthreshold(calculatethreshold(state.values[1]), sampleRate);
}
void RedBassAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	channelData.clear();
	for (int i = 0; i < channelnum; i++)
		channelData.push_back(nullptr);
}
void RedBassAudioProcessor::resetfilter() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	dsp::ProcessSpec spec;
	spec.sampleRate = samplerate;
	spec.maximumBlockSize = samplesperblock;
	spec.numChannels = getNumInputChannels();
	filter.reset();
	(*filter.parameters.get()).setCutOffFrequency(samplerate,calculatelowpass(state.values[4]));
	(*filter.parameters.get()).type = dsp::StateVariableFilter::Parameters<float>::Type::lowPass;
	filter.prepare(spec);
}
void RedBassAudioProcessor::releaseResources() { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool RedBassAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

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

	for (int channel = 0; channel < channelnum; ++channel)
		channelData[channel] = buffer.getWritePointer(channel);

	double sidechain = 0;
	double osc = 0;
	for (int sample = 0; sample < numsamples; ++sample) {
		for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0)
			state.values[i] = pots[i].smooth.getNextValue();

		sidechain = 0;

		double lvl = fmin(20*envelopefollower.envelope*state.values[7],1);

		if(prmscount < samplerate*2) {
			prmsadd += lvl;
			prmscount++;
		}

		crntsmpl = fmod(crntsmpl+(calculatefrequency(state.values[0]) / getSampleRate()), 1);
		if(state.values[5] >= 1) osc = 0;
		else osc = sin(crntsmpl*MathConstants<double>::twoPi)*lvl;

		for(int channel = 0; channel < channelnum; ++channel) {
			sidechain += channelData[channel][sample];
			if(state.values[5] < 1)
				channelData[channel][sample] = (float)fmin(fmax((osc+((double)channelData[channel][sample])*state.values[6])*(1-state.values[5]),-1),1);
			else channelData[channel][sample] = 0;
		}

		sidechain = sidechain/channelnum;

		if(state.values[4] < 1) sidechain = filter.processSample(sidechain);
		else filter.processSample(sidechain);
		if(state.values[5] > 0) for(int channel = 0; channel < channelnum; ++channel)
			channelData[channel][sample] = (float)fmin(fmax(channelData[channel][sample]+sidechain*state.values[5],-1),1);

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
	double timestwo = value*131.4213562373+10;
	return timestwo*timestwo;
}
double RedBassAudioProcessor::calculatefrequency(double value) {
	double timestwo = value*5.527864045+4.472135955;
	return timestwo*timestwo;
}
double RedBassAudioProcessor::calculatethreshold(double value) {
	return pow(value,2)*.7f;
}

bool RedBassAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* RedBassAudioProcessor::createEditor() {
	return new RedBassAudioProcessorEditor(*this,paramcount,state,pots);
}

void RedBassAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;

	data << version << linebreak;

	pluginpreset newstate = state;
	for(int i = 0; i < paramcount; i++) {
		if(pots[i].smoothtime > 0)
			data << pots[i].smooth.getTargetValue() << linebreak;
		else
			data << newstate.values[i] << linebreak;
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

		for(int i = 0; i < paramcount; i++) {
			std::getline(ss, token, '\n');
			float val = std::stof(token);
			if(saveversion <= 0 && pots[i].id == "threshold") val = pow(val/.7,.5);
			apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(val));
			if(pots[i].smoothtime > 0) {
				pots[i].smooth.setCurrentAndTargetValue(val);
				state.values[i] = val;
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
}
void RedBassAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; i++) if(parameterID == pots[i].id) {
		if(pots[i].smoothtime > 0) pots[i].smooth.setTargetValue(newValue);
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
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RedBassAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout RedBassAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>("freq"		,"Frequency"		,0.0f	,1.0f	,0.48f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("threshold"	,"Threshold"		,0.0f	,1.0f	,0.02f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("attack"		,"Attack"			,0.0f	,1.0f	,0.17f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("release"		,"Release"			,0.0f	,1.0f	,0.18f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("lowpass"		,"Sidechain lowpass",0.0f	,1.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("monitor"		,"Monitor sidechain",false	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("dry"			,"Dry"				,0.0f	,1.0f	,1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("wet"			,"Wet"				,0.0f	,1.0f	,0.18f	));
	return { parameters.begin(), parameters.end() };
}
