/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

PisstortionAudioProcessor::PisstortionAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	presets[0] = pluginpreset("Default"				,0.17f	,1.0f	,0.35f	,0.31f	,0.1f	);
	presets[1] = pluginpreset("Sand in Your Ear"	,0.3f	,1.0f	,1.0f	,0.18f	,0.0f	);
	presets[2] = pluginpreset("Mega Drive"			,0.02f	,0.42f	,0.0f	,1.0f	,0.0f	);
	presets[3] = pluginpreset("Easy to Destroy"		,0.32f	,1.0f	,0.13f	,1.0f	,0.0f	);
	presets[4] = pluginpreset("Screamy"				,0.58f	,1.0f	,0.0f	,0.51f	,0.52f	);
	presets[5] = pluginpreset("I Love Piss"			,1.0f	,1.0f	,0.0f	,1.0f	,0.25f	);
	presets[6] = pluginpreset("You Broke It"		,0.09f	,1.0f	,0.0f	,0.02f	,0.0f	);
	presets[7] = pluginpreset("Splashing"			,1.0f	,1.0f	,0.39f	,0.07f	,0.22f	);
	presets[7] = pluginpreset("PISS LASERS"			,0.39f	,0.95f	,0.36f	,0.59f	,0.34f	);

	pots[0] = potentiometer("Frequency"			,"freq"			,0		,presets[0].values[0]	);
	pots[1] = potentiometer("Piss"				,"piss"			,.001f	,presets[0].values[1]	);
	pots[2] = potentiometer("Noise Reduction"	,"noise"		,.001f	,presets[0].values[2]	);
	pots[3] = potentiometer("Harmonics"			,"harm"			,0		,presets[0].values[3]	);
	pots[4] = potentiometer("Stereo"			,"stereo"		,0		,presets[0].values[4]	);
	pots[5] = potentiometer("Out Gain"			,"gain"			,.001f	,1.0f					,0.f	,1.f	,false	);
	pots[6] = potentiometer("Over-Sampling"		,"oversampling"	,0		,1						,0		,1		,false	,potentiometer::ptype::booltype);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = pots[i].inflate(apvts.getParameter(pots[i].id)->getValue());
		if(pots[i].smoothtime > 0) pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(pots[i].id, this);
	}

	updatevis = true;
}

PisstortionAudioProcessor::~PisstortionAudioProcessor(){
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(pots[i].id, this);
}

const String PisstortionAudioProcessor::getName() const { return "Pisstortion"; }
bool PisstortionAudioProcessor::acceptsMidi() const { return false; }
bool PisstortionAudioProcessor::producesMidi() const { return false; }
bool PisstortionAudioProcessor::isMidiEffect() const { return false; }
double PisstortionAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PisstortionAudioProcessor::getNumPrograms() { return 9; }
int PisstortionAudioProcessor::getCurrentProgram() { return currentpreset; }
void PisstortionAudioProcessor::setCurrentProgram (int index) {
	if(!boot) return;

	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;
	for(int i = 0; i < paramcount; i++) if(pots[i].savedinpreset)
		lerptable[i] = pots[i].normalize(state.values[i]);

	if(lerpstage <= 0) {
		lerpstage = 1;
		startTimerHz(30);
	} else lerpstage = 1;
}
void PisstortionAudioProcessor::timerCallback() {
	lerpstage *= .64f;
	if(lerpstage < .001) {
		for(int i = 0; i < paramcount; i++) if(pots[i].savedinpreset)
			apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(presets[currentpreset].values[i]));
		lerpstage = 0;
		undoManager.beginNewTransaction();
		stopTimer();
		return;
	}
	for(int i = 0; i < paramcount; i++) if(pots[i].savedinpreset)
		lerpValue(pots[i].id, lerptable[i], pots[i].normalize(presets[currentpreset].values[i]));
}
void PisstortionAudioProcessor::lerpValue(StringRef slider, float& oldval, float newval) {
	oldval = (newval-oldval)*.36f+oldval;
	apvts.getParameter(slider)->setValueNotifyingHost(oldval);
}
const String PisstortionAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void PisstortionAudioProcessor::changeProgramName (int index, const String& newName) {
	for(int i = 0; i < paramcount; i++) if(pots[i].savedinpreset)
		presets[index].values[i] = state.values[i];
}

void PisstortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	dcfilter.init((int)sampleRate,getTotalNumInputChannels());
	for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0)
		pots[i].smooth.reset(sampleRate*(state.values[6]+1), pots[i].smoothtime);
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;
	preparedtoplay = true;
}
void PisstortionAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	channelData.clear();
	for (int i = 0; i < channelnum; i++)
		channelData.push_back(nullptr);

	ospointerarray.resize(newchannelnum);
	os.reset(new dsp::Oversampling<float>(newchannelnum,1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
	os->initProcessing(samplesperblock);
	os->setUsingIntegerLatency(true);
	setoversampling(state.values[6] > .5);
}
void PisstortionAudioProcessor::releaseResources() { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool PisstortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void PisstortionAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	int prmsadd = rmsadd.get();
	int prmscount = rmscount.get();
	bool isoversampling = (state.values[6] >= .5);

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);
	if(isoversampling) {
		dsp::AudioBlock<float> osblock = os->processSamplesUp(block);
		for(int i = 0; i < channelnum; i++)
			ospointerarray[i] = osblock.getChannelPointer(i);
		osbuffer = AudioBuffer<float>(ospointerarray.data(), channelnum, static_cast<int>(osblock.getNumSamples()));
		numsamples = osbuffer.getNumSamples();
	}

	for (int channel = 0; channel < channelnum; ++channel) {
		if(isoversampling) channelData[channel] = osbuffer.getWritePointer(channel);
		else channelData[channel] = buffer.getWritePointer(channel);
	}

	for (int sample = 0; sample < numsamples; ++sample) {
		for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0)
			state.values[i] = pots[i].smooth.getNextValue();

		for (int channel = 0; channel < channelnum; ++channel) {
			channelData[channel][sample] = pisstortion(channelData[channel][sample],channel,channelnum,state,true);
			if(prmscount < samplerate*2) {
				prmsadd += channelData[channel][sample]*channelData[channel][sample];
				prmscount++;
			}
		}
	}

	if(isoversampling) os->processSamplesDown(block);

	rmsadd = prmsadd;
	rmscount = prmscount;

	boot = true;
}

float PisstortionAudioProcessor::pisstortion(float source, int channel, int channelcount, pluginpreset stt, bool removedc) {
	if(source == 0) {
		if(removedc) return (float)fmax(fmin((dcfilter.process(0,channel)*stt.values[1]+((double)source)*(1-stt.values[1]))*stt.values[5],1),-1);
		else return 0;
	}

	double channeloffset = 0;
	if(channelcount > 1) channeloffset = ((double)channel/(channelcount-1))-.5;
	double f = sin(((double)source)*50*(stt.values[0]+stt.values[4]*stt.values[0]*channeloffset*(source>0?1:-1)));

	if(stt.values[3] >= 1) {
		if(f > 0) f = 1;
		else if(f < 0) f = -1;
	} else if(stt.values[3] <= 0) {
		f = 0;
	} else if(stt.values[3] != .5) {
		double h = stt.values[3];
		if(h < .5)
			h = h*2;
		else
			h = .5/(1-h);

		if (f > 0)
			f = 1-pow(1-f,h);
		else
			f = pow(f+1,h)-1;
	}

	if(stt.values[2] >= 1)
		f *= abs(source);
	else if(stt.values[2] > 0)
		f *= 1-pow(1-abs(source),1./stt.values[2]);

	f = fmax(fmin(f,1),-1);
	if(removedc) f = dcfilter.process(f,channel);

	return (float)fmax(fmin((f*stt.values[1]+source*(1-stt.values[1]))*stt.values[5],1),-1);
}

void PisstortionAudioProcessor::setoversampling(bool toggle) {
	if(!preparedtoplay) return;
	if(toggle) {
		if(channelnum <= 0) return;
		os->reset();
		setLatencySamples(os->getLatencyInSamples());
		for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0)
			pots[i].smooth.reset(samplerate*2, pots[i].smoothtime);
	} else {
		setLatencySamples(0);
		for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0)
			pots[i].smooth.reset(samplerate, pots[i].smoothtime);
	}
}

bool PisstortionAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* PisstortionAudioProcessor::createEditor() {
	return new PisstortionAudioProcessorEditor (*this,paramcount,state,pots);
}

void PisstortionAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;

	data << version
		<< linebreak << currentpreset << linebreak;

	pluginpreset newstate = state;
	for(int i = 0; i < paramcount; i++) {
		if(pots[i].smoothtime > 0)
			data << pots[i].smooth.getTargetValue() << linebreak;
		else
			data << newstate.values[i] << linebreak;
	}

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << linebreak;
		for(int v = 0; v < paramcount; v++) if(pots[v].savedinpreset)
			data << presets[i].values[v] << linebreak;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PisstortionAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, '\n');
		int saveversion = std::stoi(token);

		std::getline(ss, token, '\n');
		currentpreset = std::stoi(token);

		for(int i = 0; i < paramcount; i++) {
			std::getline(ss, token, '\n');
			float val = std::stof(token);
			if(saveversion <= 1 && pots[i].id == "oversampling") val = val > 1.5 ? 1 : 0;
			apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(val));
			if(pots[i].smoothtime > 0) {
				pots[i].smooth.setCurrentAndTargetValue(val);
				state.values[i] = val;
			}
		}

		for(int i = 0; i < (saveversion == 0 ? 8 : getNumPrograms()); i++) {
			std::getline(ss, token, '\n'); presets[i].name = token;
			for(int v = 0; v < paramcount; v++) if(pots[v].savedinpreset) {
				std::getline(ss, token, '\n');
				presets[i].values[v] = std::stof(token);
			}
		}

		updatevis = true;

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
void PisstortionAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		state.values[6] = newValue > .5 ? 1 : 0;
		setoversampling(newValue > .5);
		return;
	}
	for(int i = 0; i < paramcount; i++) if(parameterID == pots[i].id) {
		if(pots[i].smoothtime > 0) pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PisstortionAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout PisstortionAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>("freq"		,"Frequency"		,0.0f	,1.0f	,0.17f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("piss"		,"Piss"				,0.0f	,1.0f	,1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("noise"		,"Noise Reduction"	,0.0f	,1.0f	,0.35f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("harm"		,"Harmonics"		,0.0f	,1.0f	,0.31f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("stereo"		,"Stereo"			,0.0f	,1.0f	,0.1f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("gain"		,"Out Gain"			,0.0f	,1.0f	,1.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("oversampling","Over-Sampling"	,true	));
	return { parameters.begin(), parameters.end() };
}
