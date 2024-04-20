#include "processor.h"
#include "editor.h"

PisstortionAudioProcessor::PisstortionAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	presets[0] = pluginpreset("Default"				,0.17f	,1.0f	,0.35f	,0.31f	,0.1f	,1.0f	);
	presets[1] = pluginpreset("Sand in Your Ear"	,0.3f	,1.0f	,1.0f	,0.18f	,0.0f	,1.0f	);
	presets[2] = pluginpreset("Mega Drive"			,0.02f	,0.42f	,0.0f	,1.0f	,0.0f	,1.0f	);
	presets[3] = pluginpreset("Easy to Destroy"		,0.32f	,1.0f	,0.13f	,1.0f	,0.0f	,1.0f	);
	presets[4] = pluginpreset("Screamy"				,0.58f	,1.0f	,0.0f	,0.51f	,0.52f	,1.0f	);
	presets[5] = pluginpreset("I Love Piss"			,1.0f	,1.0f	,0.0f	,1.0f	,0.25f	,1.0f	);
	presets[6] = pluginpreset("You Broke It"		,0.09f	,1.0f	,0.0f	,0.02f	,0.0f	,1.0f	);
	presets[7] = pluginpreset("Splashing"			,1.0f	,1.0f	,0.39f	,0.07f	,0.22f	,1.0f	);
	presets[8] = pluginpreset("PISS LASERS"			,0.39f	,0.95f	,0.36f	,0.59f	,0.34f	,1.0f	);
	for(int i = 9; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "Program " + (String)(i-8);
	}

	params.pots[0] = potentiometer("Frequency"			,"freq"		,.001f	,presets[0].values[0]	);
	params.pots[1] = potentiometer("Piss"				,"piss"		,.002f	,presets[0].values[1]	);
	params.pots[2] = potentiometer("Noise Reduction"	,"noise"	,.002f	,presets[0].values[2]	);
	params.pots[3] = potentiometer("Harmonics"			,"harm"		,.001f	,presets[0].values[3]	);
	params.pots[4] = potentiometer("Stereo"				,"stereo"	,.001f	,presets[0].values[4]	);
	params.pots[5] = potentiometer("Out Gain"			,"gain"		,.002f	,presets[0].values[5]	);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		add_listener(params.pots[i].id);
	}
	params.oversampling = apvts.getParameter("oversampling")->getValue() > .5;
	add_listener("oversampling");

	updatevis = true;
}

PisstortionAudioProcessor::~PisstortionAudioProcessor(){
	close();
}

const String PisstortionAudioProcessor::getName() const { return "Pisstortion"; }
bool PisstortionAudioProcessor::acceptsMidi() const { return false; }
bool PisstortionAudioProcessor::producesMidi() const { return false; }
bool PisstortionAudioProcessor::isMidiEffect() const { return false; }
double PisstortionAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PisstortionAudioProcessor::getNumPrograms() { return 20; }
int PisstortionAudioProcessor::getCurrentProgram() { return currentpreset; }
void PisstortionAudioProcessor::setCurrentProgram(int index) {
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
void PisstortionAudioProcessor::timerCallback() {
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
const String PisstortionAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void PisstortionAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void PisstortionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	if(!saved && sampleRate > 60000) {
		params.oversampling = false;
		apvts.getParameter("oversampling")->setValueNotifyingHost(0);
	}
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void PisstortionAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void PisstortionAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	dcfilter.init(samplerate*(state.values[6]>.5?2:1),channelnum);
	for(int i = 0; i < channelnum; i++) dcfilter.reset(i);

	for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
		params.pots[i].smooth.reset(samplerate*(params.oversampling?2:1), params.pots[i].smoothtime);

	preparedtoplay = true;
	ospointerarray.resize(channelnum);
	os.reset(new dsp::Oversampling<float>(channelnum,1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
	os->initProcessing(samplesperblock);
	os->setUsingIntegerLatency(true);
	setoversampling(params.oversampling);
}
void PisstortionAudioProcessor::releaseResources() { }

bool PisstortionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void PisstortionAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	saved = true;

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	int prmsadd = rmsadd.get();
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

	for(int sample = 0; sample < numsamples; ++sample) {
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			state.values[i] = params.pots[i].smooth.getNextValue();

		for(int channel = 0; channel < channelnum; ++channel) {
			channelData[channel][sample] = pisstortion(channelData[channel][sample],channel,channelnum,state,true);
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

float PisstortionAudioProcessor::pisstortion(float source, int channel, int channelcount, pluginpreset stt, bool removedc) {
	if(source == 0) {
		if(removedc) return (float)fmax(fmin((dcfilter.process(0,channel)*stt.values[1]+((double)source)*(1-stt.values[1]))*stt.values[5],1),-1);
		else return 0;
	}

	double channeloffset = 0;
	if(channelcount > 1) channeloffset = ((double)channel/(channelcount-1))-.5;
	double ampamount = 50*(stt.values[0]+stt.values[4]*stt.values[0]*channeloffset*(source>0?1:-1));
	double f = sin(((double)source)*ampamount)+source*fmax(1-ampamount,0);

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

		if(f > 0)
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
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			params.pots[i].smooth.reset(samplerate*2, params.pots[i].smoothtime);
		dcfilter.init(samplerate*2,channelnum);
	} else {
		setLatencySamples(0);
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			params.pots[i].smooth.reset(samplerate, params.pots[i].smoothtime);
		dcfilter.init(samplerate,channelnum);
	}
}

bool PisstortionAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* PisstortionAudioProcessor::createEditor() {
	return new PisstortionAudioProcessorEditor(*this,paramcount,state,params);
}

void PisstortionAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter
		<< (params.oversampling?1:0) << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << delimiter;
		for(int v = 0; v < paramcount; v++)
			data << presets[i].values[v] << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PisstortionAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	saved = true;
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		if(saveversion >= 3) {
			std::getline(ss, token, delimiter);
			params.oversampling = std::stof(token) > .5;

			for(int i = 0; i < getNumPrograms(); i++) {
				std::getline(ss, token, delimiter);
				presets[i].name = token;
				for(int v = 0; v < paramcount; v++) {
					std::getline(ss, token, delimiter);
					presets[i].values[v] = std::stof(token);
				}
			}
		} else {
			for(int i = 0; i < 6; i++) {
				std::getline(ss, token, delimiter);
				presets[currentpreset].values[i] = std::stof(token);
			}

			std::getline(ss, token, delimiter);
			float val = std::stof(token);
			if(saveversion <= 1) val = val > 1.5 ? 1 : 0;
			params.oversampling = val>.5;

			for(int i = 0; i < (saveversion == 0 ? 8 : getNumPrograms()); i++) {
				std::getline(ss, token, delimiter);
				presets[i].name = token;
				for(int v = 0; v < 5; v++) {
					std::getline(ss, token, delimiter);
					if(currentpreset != i) presets[i].values[v] = std::stof(token);
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
	apvts.getParameter("oversampling")->setValueNotifyingHost(params.oversampling);
	updatevis = true;
}
const String PisstortionAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int v = 0; v < paramcount; v++)
		data << presets[preset_id].values[v] << delimiter;

	return data.str();
}
void PisstortionAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
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

void PisstortionAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
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

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PisstortionAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout PisstortionAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"freq"		,1},"Frequency"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.17f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"piss"		,1},"Piss"				,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"noise"		,1},"Noise Reduction"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.35f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"harm"		,1},"Harmonics"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.31f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"stereo"		,1},"Stereo"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.1f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"gain"		,1},"Out Gain"			,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"oversampling",1},"Over-Sampling"														 ,true	));
	return { parameters.begin(), parameters.end() };
}
