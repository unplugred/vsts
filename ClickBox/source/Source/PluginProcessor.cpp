#include "PluginProcessor.h"
#include "PluginEditor.h"

ClickBoxAudioProcessor::ClickBoxAudioProcessor() :
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	for(int i = 0; i < getNumPrograms(); i++)
		presets[i].name = "Program " + (String)(i+1);

	params.pots[0] = potentiometer("Intensity"			,"intensity",.001f	,.5f	);
	params.pots[1] = potentiometer("Amount"				,"amount"	,0		,.5f	);
	params.pots[2] = potentiometer("Stereo"				,"stereo"	,.001f	,.28f	);
	params.pots[3] = potentiometer("Side-chain to dry"	,"sidechain",0		,0		,0	,1	,potentiometer::ptype::booltype);
	params.pots[4] = potentiometer("Dry out"			,"dry"		,.002f	,1		,0	,1	,potentiometer::ptype::booltype);
	params.pots[5] = potentiometer("Auto"				,"auto"		,0		,0		);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(params.pots[i].id, this);
	}
	params.x = apvts.getParameter("x")->getValue();
	params.xsmooth.setCurrentAndTargetValue(params.x);
	apvts.addParameterListener("x", this);
	params.y = apvts.getParameter("y")->getValue();
	params.ysmooth.setCurrentAndTargetValue(params.y);
	apvts.addParameterListener("y", this);
	params.overridee = apvts.getParameter("override")->getValue() > .5;
	apvts.addParameterListener("override", this);

	prlin.init();
}
ClickBoxAudioProcessor::~ClickBoxAudioProcessor() {
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(params.pots[i].id, this);
	apvts.removeParameterListener("x", this);
	apvts.removeParameterListener("y", this);
	apvts.removeParameterListener("override", this);
}

const String ClickBoxAudioProcessor::getName() const { return "ClickBox"; }
bool ClickBoxAudioProcessor::acceptsMidi() const { return false; }
bool ClickBoxAudioProcessor::producesMidi() const { return false; }
bool ClickBoxAudioProcessor::isMidiEffect() const { return false; }
double ClickBoxAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ClickBoxAudioProcessor::getNumPrograms() { return 20; }
int ClickBoxAudioProcessor::getCurrentProgram() { return currentpreset; }
void ClickBoxAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	for(int i = 0; i < paramcount; i++) {
		if(lerpstage < .001 || lerpchanged[i])
			lerptable[i] = params.pots[i].normalize(presets[currentpreset].values[i]);
		lerpchanged[i] = false;
	}
	currentpreset = index;
	for(int i = 0; i < paramcount; i++) {
		if(params.pots[i].ttype == potentiometer::ptype::booltype) {
			lerpchanged[i] = true;
			apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		}
	}

	if(lerpstage <= 0) {
		lerpstage = 1;
		startTimerHz(30);
	} else lerpstage = 1;
}
void ClickBoxAudioProcessor::timerCallback() {
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
const String ClickBoxAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void ClickBoxAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
}

void ClickBoxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
		params.pots[i].smooth.reset(sampleRate, params.pots[i].smoothtime);
	params.xsmooth.reset(sampleRate, .05f);
	params.ysmooth.reset(sampleRate, .05f);
}
void ClickBoxAudioProcessor::releaseResources() { }

bool ClickBoxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void ClickBoxAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float** channelData = buffer.getArrayOfWritePointers();
	int channelnum = buffer.getNumChannels();

	float ii = i.get();
	for(int sample = 0; sample < buffer.getNumSamples(); ++sample) {
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			state.values[i] = params.pots[i].smooth.getNextValue();
		params.x = params.xsmooth.getNextValue();
		params.y = params.ysmooth.getNextValue();

		if(state.values[3] > .5) {
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
			float newx = params.x, newy = params.y;
			if(params.overridee < .5) {
				newx = prlin.noise(time,0)*.5f+.5f;
				newy = prlin.noise(0,time)*.5f+.5f;
				compensation = fabs(oldautomod-state.values[5])*sqrt((newx-params.x)*(newx-params.x)+(newy-params.y)*(newy-params.y));
				newx = newx*state.values[5]+params.x*(1-state.values[5]);
				newy = newy*state.values[5]+params.y*(1-state.values[5]);
			}
			oldautomod = state.values[5];

			//coordinate differentials
			if(params.overridee == oldoverride) {
				float xdiff = fabs(x.get()-newx);
				float ydiff = fabs(y.get()-newy);
				ii = (((sqrt(xdiff*xdiff+ydiff*ydiff)-compensation)*120)/samplessincelastperlin)*state.values[1]*state.values[1];
			}
			oldoverride = params.overridee;
			x = newx;
			y = newy;

			//sidechain to dry
			if(state.values[3] > .5) {
				ii *= sqrt(rms)*4;
				rms = 0;
			}
			i = ii;

			samplessincelastperlin = 0;
		}

		//dry out
		for(int i = 0; i < channelnum; i++) channelData[i][sample] *= state.values[4];

		//generate click
		float mult = samplessincelastperlin*.001953125f;
		if(random.nextFloat() < (oldi*(1-mult)+ii*mult)) {
			float ampp = random.nextFloat()*2-1;
			if(state.values[0] < 1) ampp *= state.values[0];
			else ampp = ampp*(2-state.values[0])+(ampp<0?-1:1)*(state.values[0]-1);

			int clickchannel = (int)floorf(random.nextFloat()*channelnum);
			for(int channel = 0; channel < channelnum; channel++) if(channel == clickchannel || random.nextFloat() >= state.values[2]) {
				if(state.values[0] < 1 && state.values[4] > 0)
					channelData[channel][sample] = fmax(fmin(ampp+state.values[4]*channelData[channel][sample]*(1-state.values[0]),1),-1);
				else
					channelData[channel][sample] = fmax(fmin(ampp,1),-1);
			}
		}
	}
}

bool ClickBoxAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* ClickBoxAudioProcessor::createEditor() {
	return new ClickBoxAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void ClickBoxAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version << linebreak
		<< currentpreset << linebreak
		<< params.x << linebreak
		<< params.y << linebreak;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << linebreak;
		for(int v = 0; v < paramcount; v++)
			data << presets[i].values[v] << linebreak;
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

		if(saveversion >= 3) {
			std::getline(ss,token,'\n');
			currentpreset = stoi(token);

			std::getline(ss,token,'\n');
			params.x = stof(token);

			std::getline(ss,token,'\n');
			params.y = stof(token);

			for(int i = 0; i < getNumPrograms(); i++) {
				std::getline(ss, token, '\n');
				presets[i].name = token;
				for(int v = 0; v < paramcount; v++) {
					std::getline(ss, token, '\n');
					presets[i].values[v] = std::stof(token);
				}
			}
		} else {
			std::getline(ss,token,'\n');
			params.x = stof(token);

			std::getline(ss,token,'\n');
			params.y = stof(token);

			for(int i = 0; i < 6; i++) {
				std::getline(ss, token, '\n');
				presets[currentpreset].values[i] = std::stof(token);
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

	for(int i = 0; i < paramcount; i++) {
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		if(params.pots[i].smoothtime > 0) {
			params.pots[i].smooth.setCurrentAndTargetValue(presets[currentpreset].values[i]);
			state.values[i] = presets[currentpreset].values[i];
		}
	}
	apvts.getParameter("x")->setValueNotifyingHost(params.x);
	params.xsmooth.setCurrentAndTargetValue(params.x);
	apvts.getParameter("y")->setValueNotifyingHost(params.y);
	params.ysmooth.setCurrentAndTargetValue(params.y);
}
void ClickBoxAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "override") {
		params.overridee = newValue > .5;
		if(params.overridee) {
			params.xsmooth.setCurrentAndTargetValue(params.xsmooth.getTargetValue());
			params.ysmooth.setCurrentAndTargetValue(params.ysmooth.getTargetValue());
			params.x = params.xsmooth.getTargetValue();
			params.y = params.ysmooth.getTargetValue();
			x = params.x;
			y = params.y;
		}
		return;
	}
	if(parameterID == "x") {
		params.xsmooth.setTargetValue(newValue);
		return;
	}
	if(parameterID == "y") {
		params.ysmooth.setTargetValue(newValue);
		return;
	}
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		if(lerpstage < .001 || lerpchanged[i]) presets[currentpreset].values[i] = newValue;
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ClickBoxAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout ClickBoxAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>("x"			,"X"				,juce::NormalisableRange( 0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("y"			,"Y"				,juce::NormalisableRange( 0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("intensity"	,"Intensity"		,juce::NormalisableRange( 0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("amount"		,"Amount"			,juce::NormalisableRange( 0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("stereo"		,"Stereo"			,juce::NormalisableRange( 0.0f	,1.0f	),0.28f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("sidechain"	,"Side-chain to dry"										 ,false	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("dry"			,"Dry out"													 ,true	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("auto"		,"Auto"				,juce::NormalisableRange( 0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("override"	,"Override"													 ,false	));
	return { parameters.begin(), parameters.end() };
}
