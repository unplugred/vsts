/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

ClickBoxAudioProcessor::ClickBoxAudioProcessor() : apvts(*this, &undoManager, "Parameters", createParameters()) {
	pots[0] = potentiometer("X"					,"x"		,.05f	,.5f	,0	,1	,false);
	pots[1] = potentiometer("Y"					,"y"		,.05f	,.5f	,0	,1	,false);
	pots[2] = potentiometer("Intensity"			,"intensity",.001f	,.5f	);
	pots[3] = potentiometer("Amount"			,"amount"	,0		,.5f	);
	pots[4] = potentiometer("Stereo"			,"stereo"	,.001f	,.28f	);
	pots[5] = potentiometer("Side-chain to dry"	,"sidechain",0		,0		,0	,1	,true	,potentiometer::ptype::booltype);
	pots[6] = potentiometer("Dry out"			,"dry"		,.002f	,1		,0	,1	,true	,potentiometer::ptype::booltype);
	pots[7] = potentiometer("Auto"				,"auto"		,0		,0		);
	pots[8] = potentiometer("Override"			,"override"	,0		,0		,0	,1	,false	,potentiometer::ptype::booltype);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = pots[i].inflate(apvts.getParameter(pots[i].id)->getValue());
		if(pots[i].smoothtime > 0) pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(pots[i].id, this);
	}

	prlin.init();
}
ClickBoxAudioProcessor::~ClickBoxAudioProcessor() {
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(pots[i].id, this);
}

const String ClickBoxAudioProcessor::getName() const { return "ClickBox"; }
bool ClickBoxAudioProcessor::acceptsMidi() const { return false; }
bool ClickBoxAudioProcessor::producesMidi() const { return false; }
bool ClickBoxAudioProcessor::isMidiEffect() const { return false; }
double ClickBoxAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ClickBoxAudioProcessor::getNumPrograms() { return 1; }
int ClickBoxAudioProcessor::getCurrentProgram() { return 0; }
void ClickBoxAudioProcessor::setCurrentProgram(int index) { }
const String ClickBoxAudioProcessor::getProgramName(int index) { return ":)"; }
void ClickBoxAudioProcessor::changeProgramName(int index, const String& newName) { }

void ClickBoxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0)
		pots[i].smooth.reset(sampleRate, pots[i].smoothtime);
}
void ClickBoxAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	channelData.clear();
	for (int i = 0; i < getNumInputChannels(); i++)
		channelData.push_back(nullptr);
}
void ClickBoxAudioProcessor::releaseResources() { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool ClickBoxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void ClickBoxAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	for (int channel = 0; channel < channelnum; ++channel)
		channelData[channel] = buffer.getWritePointer(channel);

	float ii = i.get();
	for(int sample = 0; sample < buffer.getNumSamples(); ++sample) {
		for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0)
			state.values[i] = pots[i].smooth.getNextValue();

		if(state.values[5] > .5) {
			float monoamp = 0;
			for(int i = 0; i < channelnum; i++) monoamp += channelData[i][sample];
			rms += (monoamp*monoamp)/channelnum;
		}

		//calculate click chance
		samplessincelastperlin++;
		if(samplessincelastperlin >= 512) {
			oldi = ii;

			//calculate coordinates
			time += samplessincelastperlin*.00001f;
			float compensation = 0;
			float newx = state.values[0], newy = state.values[1];
			if(state.values[8] < .5) {
				newx = prlin.noise(time,0)*.5f+.5f;
				newy = prlin.noise(0,time)*.5f+.5f;
				compensation = fabs(oldautomod-state.values[7])*sqrt((newx-state.values[0])*(newx-state.values[0])+(newy - state.values[1])*(newy-state.values[1]));
				newx = newx*state.values[7]+state.values[0]*(1-state.values[7]);
				newy = newy*state.values[7]+state.values[1]*(1-state.values[7]);
			}
			oldautomod = state.values[7];

			//coordinate differentials
			if(state.values[8] == oldoverride) {
				float xdiff = fabs(x.get()-newx);
				float ydiff = fabs(y.get()-newy);
				ii = (((sqrt(xdiff*xdiff+ydiff*ydiff)-compensation)*120)/samplessincelastperlin)*state.values[3]*state.values[3];
			}
			oldoverride = state.values[8];
			x = newx;
			y = newy;

			//sidechain to dry
			if(state.values[5] > .5) {
				ii *= sqrt(rms)*4;
				rms = 0;
			}
			i = ii;

			samplessincelastperlin = 0;
		}

		//dry out
		for(int i = 0; i < channelnum; i++) channelData[i][sample] *= state.values[6];

		//generate click
		float mult = samplessincelastperlin*.001953125f;
		if(random.nextFloat() < (oldi*(1-mult)+ii*mult)) {
			float ampp = random.nextFloat()*2-1;
			if(state.values[2] < 1) ampp *= state.values[2];
			else ampp = ampp*(2-state.values[2])+(ampp<0?-1:1)*(state.values[2]-1);

			int clickchannel = (int)floorf(random.nextFloat()*channelnum);
			for(int channel = 0; channel < channelnum; channel++) if(channel == clickchannel || random.nextFloat() >= state.values[4]) {
				if(state.values[2] < 1 && state.values[6] > 0)
					channelData[channel][sample] = fmax(fmin(ampp+state.values[6]*channelData[channel][sample]*(1-state.values[2]),1),-1);
				else
					channelData[channel][sample] = fmax(fmin(ampp,1),-1);
			}
		}
	}
}

bool ClickBoxAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* ClickBoxAudioProcessor::createEditor() {
	return new ClickBoxAudioProcessorEditor(*this,paramcount,state,pots);
}

void ClickBoxAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version << linebreak;

	pluginpreset newstate = state;
	for(int i = 0; i < paramcount; i++) if(pots[i].id != "override") {
		if(pots[i].smoothtime > 0)
			data << pots[i].smooth.getTargetValue() << linebreak;
		else
			data << newstate.values[i] << linebreak;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void ClickBoxAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	try {
		std::stringstream ss(String::createStringFromData(data,sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss,token,'\n');
		int saveversion = stoi(token);

		for(int i = 0; i < paramcount; i++) if(pots[i].id != "override") {
			std::getline(ss, token, '\n');
			float val = std::stof(token);
			apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(val));
			if(pots[i].smoothtime > 0) {
				pots[i].smooth.setCurrentAndTargetValue(val);
				state.values[i] = val;
			}
		}
	} catch(const char* e) {
		logger.debug((String)"Error loading saved data: "+(String)e);
	} catch(String e) {
		logger.debug((String)"Error loading saved data: "+e);
	} catch(std::exception &e) {
		logger.debug((String)"Error loading saved data: "+(String)e.what());
	} catch(...) {
		logger.debug((String)"Error loading saved data");
	}
}
void ClickBoxAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; i++) if(parameterID == pots[i].id) {
		if(pots[i].smoothtime > 0) pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		if(parameterID == "override" && newValue > .5) {
			pots[0].smooth.setCurrentAndTargetValue(pots[0].smooth.getTargetValue());
			pots[1].smooth.setCurrentAndTargetValue(pots[1].smooth.getTargetValue());
			state.values[0] = pots[0].smooth.getTargetValue();
			state.values[1] = pots[1].smooth.getTargetValue();
			x = state.values[0];
			y = state.values[1];
		}
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ClickBoxAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout ClickBoxAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat>("x","X",0.0f,1.0f,0.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("y","Y",0.0f,1.0f,0.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("intensity","Intensity",0.0f,1.0f,0.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("amount","Amount",0.0f,1.0f,0.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("stereo","Stereo",0.0f,1.0f,0.28f));
	parameters.push_back(std::make_unique<AudioParameterBool>("sidechain","Side-chain to dry",false));
	parameters.push_back(std::make_unique<AudioParameterBool>("dry","Dry out",true));
	parameters.push_back(std::make_unique<AudioParameterFloat>("auto","Auto",0.0f,1.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterBool>("override","Override",false));
	return { parameters.begin(), parameters.end() };
}

