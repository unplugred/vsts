/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

PFAudioProcessor::PFAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	presets[0] = pluginpreset("Default"				,0.32f	,0.0f	,0.0f	,0.0f	,0.37f	);
	presets[1] = pluginpreset("Digital Driver"		,0.27f	,-11.36f,0.53f	,0.0f	,0.0f	);
	presets[2] = pluginpreset("Noisy Bass Pumper"	,0.55f	,20.0f	,0.59f	,0.77f	,0.0f	);
	presets[3] = pluginpreset("Broken Earphone"		,0.19f	,-1.92f	,1.0f	,0.0f	,0.88f	);
	presets[4] = pluginpreset("Fatass"				,0.62f	,-5.28f	,1.0f	,0.0f	,0.0f	);
	presets[5] = pluginpreset("Screaming Alien"		,0.63f	,5.6f	,0.2f	,0.0f	,0.0f	); 
	presets[6] = pluginpreset("Bad Connection"		,0.25f	,-2.56f	,0.48f	,0.0f	,0.0f	);
	presets[7] = pluginpreset("Ouch"				,0.9f	,-20.0f	,0.0f	,0.0f	,0.69f	);

	pots[0] = potentiometer("Frequency"		,"freq"			,0		,presets[0].values[0]	);
	pots[1] = potentiometer("Fatness"		,"fat"			,.001f	,presets[0].values[1]	,-20.f	,20.f	);
	pots[2] = potentiometer("Drive"			,"drive"		,0		,presets[0].values[2]	);
	pots[3] = potentiometer("Dry"			,"dry"			,.001f	,presets[0].values[3]	);
	pots[4] = potentiometer("Stereo"		,"stereo"		,0		,presets[0].values[4]	);
	pots[5] = potentiometer("Out Gain"		,"gain"			,.001f	,.4f					,0.f	,1.f	,false	);
	pots[6] = potentiometer("Over-Sampling"	,"oversampling"	,0		,1						,0		,1		,false	,potentiometer::ptype::booltype);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = pots[i].inflate(apvts.getParameter(pots[i].id)->getValue());
		if(pots[i].smoothtime > 0) pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(pots[i].id, this);
	}
}

PFAudioProcessor::~PFAudioProcessor(){
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(pots[i].id, this);
}

const String PFAudioProcessor::getName() const { return "Plastic Funeral"; }
bool PFAudioProcessor::acceptsMidi() const { return false; }
bool PFAudioProcessor::producesMidi() const { return false; }
bool PFAudioProcessor::isMidiEffect() const { return false; }
double PFAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PFAudioProcessor::getNumPrograms() { return 8; }
int PFAudioProcessor::getCurrentProgram() { return currentpreset; }
void PFAudioProcessor::setCurrentProgram (int index) {
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
void PFAudioProcessor::timerCallback() {
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
void PFAudioProcessor::lerpValue(StringRef slider, float& oldval, float newval) {
	oldval = (newval-oldval)*.36f+oldval;
	apvts.getParameter(slider)->setValueNotifyingHost(oldval);
}
const String PFAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void PFAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
	
	for(int i = 0; i < paramcount; i++) if(pots[i].savedinpreset)
		presets[index].values[i] = state.values[i];
}

void PFAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0)
		pots[i].smooth.reset(sampleRate*(state.values[6]+1), pots[i].smoothtime);
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;
	preparedtoplay = true;
}
void PFAudioProcessor::changechannelnum(int newchannelnum) {
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
void PFAudioProcessor::releaseResources() { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool PFAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void PFAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float prmsadd = rmsadd.get();
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

		if(curfat != state.values[1] || curdry != state.values[3]) {
			curfat = state.values[1];
			curdry = state.values[3];
			curnorm = normalizegain(curfat,curdry);
		}

		for (int channel = 0; channel < channelnum; ++channel) {
			channelData[channel][sample] = plasticfuneral(channelData[channel][sample],channel,channelnum,state,curnorm);
			prmsadd += channelData[channel][sample]*channelData[channel][sample];
			prmscount++;
		}
	}

	if(isoversampling) os->processSamplesDown(block);

	rmsadd = prmsadd;
	rmscount = prmscount;

	boot = true;
}

float PFAudioProcessor::plasticfuneral(float source, int channel, int channelcount, pluginpreset stt, float nrm) {
	double channeloffset = 0;
	if(channelcount > 1) channeloffset = ((double)channel / (channelcount - 1)) * 2 - 1;
	double freq = fmax(fmin(stt.values[0] + stt.values[4] * .2 * channeloffset, 1), 0);
	double smpl = source * (100 - cos(freq * 1.5708) * 99);
	double pfreq = fmod(fabs(smpl), 4);
	pfreq = (pfreq > 3 ? (pfreq - 4) : (pfreq > 1 ? (2 - pfreq) : pfreq)) * (smpl > 0 ? 1 : -1);
	double pdrive = fmax(fmin(smpl, 1), -1);
	smpl = pdrive * stt.values[2] + pfreq * (1 - stt.values[2]);
	return (float)fmin(fmax(((smpl + (sin(smpl * 1.5708) - smpl) * stt.values[1]) * (1 - stt.values[3]) + source * stt.values[3]) * stt.values[5] * nrm,-1),1);
}

float PFAudioProcessor::normalizegain(float fat, float dry) {
	if(fat > 0 || dry == 0) {
		double c = fat*dry-fat;
		double x = (c+1)/(c*1.5708);
		if(x > 0 && x < 1) {
			x = acos(x)/1.5708;
			return 1/fmax(fabs((fat*(sin(x*1.5708)-x)+x)*(1-dry)+x*dry),1);
		}
		return 1;
	}
	if (fat < -7.2f) {
		double newnorm = 1;
		for (double i = .4836f; i <= .71f; i += .005f)
			newnorm = fmax(newnorm,fabs(i+(sin(i*1.5708)-i)*fat)*(1-dry)+i*dry);
		return 1/newnorm;
	}
	return 1;
}

void PFAudioProcessor::setoversampling(bool toggle) {
	if(preparedtoplay) {
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
}

bool PFAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* PFAudioProcessor::createEditor() {
	return new PFAudioProcessorEditor(*this,paramcount,state,pots);
}

void PFAudioProcessor::getStateInformation (MemoryBlock& destData) {
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
void PFAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, '\n');
		int saveversion = std::stoi(token);

		std::getline(ss, token, '\n');
		currentpreset = std::stoi(token);

		for(int i = 0; i < paramcount; i++) {
			if(saveversion > 1 || pots[i].id != "oversampling") {
				std::getline(ss, token, '\n');
				float val = std::stof(token);
				if(saveversion <= 2 && pots[i].id == "oversampling") val = val > 1.5 ? 1 : 0;
				apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(val));
				if(pots[i].smoothtime > 0) {
					pots[i].smooth.setCurrentAndTargetValue(val);
					state.values[i] = val;
				}
			}
		}

		for(int i = 0; i < getNumPrograms(); i++) {
			std::getline(ss, token, '\n'); presets[i].name = token;
			for(int v = 0; v < paramcount; v++) if(pots[v].savedinpreset) {
				std::getline(ss, token, '\n');
				presets[i].values[v] = std::stof(token);
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
void PFAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
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

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PFAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout
	PFAudioProcessor::createParameters()
{
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>("freq"		,"Frequency"	,0.0f	,1.0f	,0.32f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("fat"			,"Fatness"		,-20.0f	,20.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("drive"		,"Drive"		,0.0f	,1.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("dry"			,"Dry"			,0.0f	,1.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("stereo"		,"Stereo"		,0.0f	,1.0f	,0.37f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("gain"		,"Out Gain"		,0.0f	,1.0f	,0.4f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("oversampling","Over-Sampling",true	));
	return { parameters.begin(), parameters.end() };
}
