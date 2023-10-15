#include "PluginProcessor.h"
#include "PluginEditor.h"

PFAudioProcessor::PFAudioProcessor() :
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	presets[0] = pluginpreset("Default"				,0.32f	,0.0f	,0.0f	,0.0f	,0.37f	,.4f	);
	presets[1] = pluginpreset("Digital Driver"		,0.27f	,-11.36f,0.53f	,0.0f	,0.0f	,.4f	);
	presets[2] = pluginpreset("Noisy Bass Pumper"	,0.55f	,20.0f	,0.59f	,0.77f	,0.0f	,.4f	);
	presets[3] = pluginpreset("Broken Earphone"		,0.19f	,-1.92f	,1.0f	,0.0f	,0.88f	,.4f	);
	presets[4] = pluginpreset("Fatass"				,0.62f	,-5.28f	,1.0f	,0.0f	,0.0f	,.4f	);
	presets[5] = pluginpreset("Screaming Alien"		,0.63f	,5.6f	,0.2f	,0.0f	,0.0f	,.4f	);
	presets[6] = pluginpreset("Bad Connection"		,0.25f	,-2.56f	,0.48f	,0.0f	,0.0f	,.4f	);
	presets[7] = pluginpreset("Ouch"				,0.9f	,-20.0f	,0.0f	,0.0f	,0.69f	,.4f	);
	for(int i = 8; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "Program " + (String)(i-7);
	}

	params.pots[0] = potentiometer("Frequency"	,"freq"		,.001f	,presets[0].values[0]	);
	params.pots[1] = potentiometer("Fatness"	,"fat"		,.002f	,presets[0].values[1]	,-20.f	,20.f	);
	params.pots[2] = potentiometer("Drive"		,"drive"	,.001f	,presets[0].values[2]	);
	params.pots[3] = potentiometer("Dry"		,"dry"		,.002f	,presets[0].values[3]	);
	params.pots[4] = potentiometer("Stereo"		,"stereo"	,.001f	,presets[0].values[4]	);
	params.pots[5] = potentiometer("Out Gain"	,"gain"		,.002f	,presets[0].values[5]	);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(params.pots[i].id, this);
	}
	params.oversampling = apvts.getParameter("oversampling")->getValue() > .5;
	apvts.addParameterListener("oversampling", this);
}

PFAudioProcessor::~PFAudioProcessor(){
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(params.pots[i].id, this);
	apvts.removeParameterListener("oversampling", this);
}

const String PFAudioProcessor::getName() const { return "Plastic Funeral"; }
bool PFAudioProcessor::acceptsMidi() const { return false; }
bool PFAudioProcessor::producesMidi() const { return false; }
bool PFAudioProcessor::isMidiEffect() const { return false; }
double PFAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PFAudioProcessor::getNumPrograms() { return 20; }
int PFAudioProcessor::getCurrentProgram() { return currentpreset; }
void PFAudioProcessor::setCurrentProgram (int index) {
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
void PFAudioProcessor::timerCallback() {
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
const String PFAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void PFAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
}

void PFAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	if(!saved && sampleRate > 60000) {
		params.oversampling = false;
		apvts.getParameter("oversampling")->setValueNotifyingHost(0);
	}
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;
	
	reseteverything();
}
void PFAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void PFAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
		params.pots[i].smooth.reset(samplerate*(params.oversampling?2:1), params.pots[i].smoothtime);

	preparedtoplay = true;
	ospointerarray.resize(channelnum);
	os.reset(new dsp::Oversampling<float>(channelnum,1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
	os->initProcessing(samplesperblock);
	os->setUsingIntegerLatency(true);
	setoversampling(params.oversampling);
}
void PFAudioProcessor::releaseResources() { }

bool PFAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void PFAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	saved = true;

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float prmsadd = rmsadd.get();
	int prmscount = rmscount.get();

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);
	if(params.oversampling) {
		dsp::AudioBlock<float> osblock = os->processSamplesUp(block);
		for(int i = 0; i < channelnum; i++)
			ospointerarray[i] = osblock.getChannelPointer(i);
		osbuffer = AudioBuffer<float>(ospointerarray.data(), channelnum, static_cast<int>(osblock.getNumSamples()));
		numsamples = osbuffer.getNumSamples();
	}

	float* const* channelData;
	if(params.oversampling) channelData = osbuffer.getArrayOfWritePointers();
	else channelData = buffer.getArrayOfWritePointers();

	for (int sample = 0; sample < numsamples; ++sample) {
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			state.values[i] = params.pots[i].smooth.getNextValue();

		if(curfat != state.values[1] || curdry != state.values[3]) {
			curfat = state.values[1];
			curdry = state.values[3];
			curnorm = normalizegain(curfat,curdry);
		}

		for (int channel = 0; channel < channelnum; ++channel) {
			channelData[channel][sample] = plasticfuneral(channelData[channel][sample],channel,channelnum,state,curnorm);
			if(prmscount < samplerate*2) {
				prmsadd += channelData[channel][sample]*channelData[channel][sample];
				prmscount++;
			}
		}
	}

	if(params.oversampling) os->processSamplesDown(block);

	rmsadd = prmsadd;
	rmscount = prmscount;
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
	if(!preparedtoplay) return;
	if(toggle) {
		if(channelnum <= 0) return;
		os->reset();
		setLatencySamples(os->getLatencyInSamples());
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			params.pots[i].smooth.reset(samplerate*2, params.pots[i].smoothtime);
	} else {
		setLatencySamples(0);
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			params.pots[i].smooth.reset(samplerate, params.pots[i].smoothtime);
	}
}

bool PFAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* PFAudioProcessor::createEditor() {
	return new PFAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void PFAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;

	data << version << linebreak
		<< currentpreset << linebreak
		<< (params.oversampling?1:0) << linebreak;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << linebreak;
		for(int v = 0; v < paramcount; v++)
			data << presets[i].values[v] << linebreak;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PFAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	saved = true;
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, '\n');
		int saveversion = std::stoi(token);

		std::getline(ss, token, '\n');
		currentpreset = std::stoi(token);

		if(saveversion >= 4) {
			std::getline(ss, token, '\n');
			params.oversampling = std::stof(token) > .5;

			for(int i = 0; i < getNumPrograms(); i++) {
				std::getline(ss, token, '\n');
				presets[i].name = token;
				for(int v = 0; v < paramcount; v++) {
					std::getline(ss, token, '\n');
					presets[i].values[v] = std::stof(token);
				}
			}
		} else {
			for(int i = 0; i < 6; i++) {
				std::getline(ss, token, '\n');
				presets[currentpreset].values[i] = std::stof(token);
			}

			if(saveversion > 1) {
				std::getline(ss, token, '\n');
				float val = std::stof(token);
				if(saveversion <= 2) val = val > 1.5 ? 1 : 0;
				params.oversampling = val>.5;
			}

			for(int i = 0; i < 8; i++) {
				std::getline(ss, token, '\n');
				presets[i].name = token;
				for(int v = 0; v < 5; v++) {
					std::getline(ss, token, '\n');
					if(currentpreset != i) presets[i].values[v] = std::stof(token);
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
	apvts.getParameter("oversampling")->setValueNotifyingHost(params.oversampling);
}
void PFAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		params.oversampling = newValue>.5;
		setoversampling(newValue>.5);
		return;
	}
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		if(lerpstage < .001 || lerpchanged[i]) presets[currentpreset].values[i] = newValue;
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PFAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout PFAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"freq"		,1},"Frequency"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.32f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"fat"			,1},"Fatness"		,juce::NormalisableRange<float>( -20.0f	,20.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"drive"		,1},"Drive"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"dry"			,1},"Dry"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"stereo"		,1},"Stereo"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.37f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"gain"		,1},"Out Gain"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.4f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"oversampling",1},"Over-Sampling"													 ,true	));
	return { parameters.begin(), parameters.end() };
}
