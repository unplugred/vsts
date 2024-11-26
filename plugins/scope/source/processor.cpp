#include "processor.h"
#include "editor.h"

ScopeAudioProcessor::ScopeAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	presets[0] = pluginpreset("Default"	,0.0f	,0.4f	,0.4f	,0.0f	,0.0f	,1.0f	,1.0f	,0.54f	,0.9f	,0.65f);
	for(int i = 0; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "Program "+(String)(i);
	}

	params.pots[0] = potentiometer("XY"			,"xy"			,0	,presets[0].values[0]	,0	,1	,potentiometer::ptype::booltype);
	params.pots[1] = potentiometer("Time"		,"time"			,0	,presets[0].values[1]	,0	,1	,potentiometer::ptype::floattype);
	params.pots[2] = potentiometer("Time"		,"timexy"		,0	,presets[0].values[2]	,0	,1	,potentiometer::ptype::floattype);
	params.pots[3] = potentiometer("Scale"		,"scale"		,0	,presets[0].values[3]	,0	,1	,potentiometer::ptype::floattype);
	params.pots[4] = potentiometer("Hold"		,"hold"			,0	,presets[0].values[4]	,0	,1	,potentiometer::ptype::booltype);
	params.pots[5] = potentiometer("Sync"		,"sync"			,0	,presets[0].values[5]	,0	,1	,potentiometer::ptype::booltype);
	params.pots[6] = potentiometer("Grid"		,"grid"			,0	,presets[0].values[6]	,0	,1	,potentiometer::ptype::booltype);
	params.pots[7] = potentiometer("Hue"		,"hue"			,0	,presets[0].values[7]	,0	,1	,potentiometer::ptype::floattype);
	params.pots[8] = potentiometer("Saturation"	,"saturation"	,0	,presets[0].values[8]	,0	,1	,potentiometer::ptype::floattype);
	params.pots[9] = potentiometer("Value"		,"value"		,0	,presets[0].values[9]	,0	,1	,potentiometer::ptype::floattype);

	for(int i = 0; i < paramcount; ++i) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		add_listener(params.pots[i].id);
	}
}

ScopeAudioProcessor::~ScopeAudioProcessor() {
	close();
}

const String ScopeAudioProcessor::getName() const { return "Scope"; }
bool ScopeAudioProcessor::acceptsMidi() const { return false; }
bool ScopeAudioProcessor::producesMidi() const { return false; }
bool ScopeAudioProcessor::isMidiEffect() const { return false; }
double ScopeAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ScopeAudioProcessor::getNumPrograms() { return 20; }
int ScopeAudioProcessor::getCurrentProgram() { return currentpreset; }
void ScopeAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;
	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;

	for(int i = 0; i < paramcount; ++i)
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));

	undo_manager.beginNewTransaction();
}
const String ScopeAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void ScopeAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void ScopeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void ScopeAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void ScopeAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	oscisize = samplerate/30+800;
	osci.resize(channelnum*oscisize);
	oscimode.resize(oscisize);
	osciscore.resize(oscisize);
	line.resize(channelnum*800);
	for(int i = 0; i < (channelnum*oscisize); ++i) osci[i] = 0;
	for(int i = 0; i < oscisize; ++i) oscimode[i] = 0;
	for(int i = 0; i < oscisize; ++i) osciscore[i] = -1;
	for(int i = 0; i < (channelnum*800); ++i) line[i] = 0;

	for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
		params.pots[i].smooth.reset(samplerate*(params.oversampling?2:1), params.pots[i].smoothtime);
}
void ScopeAudioProcessor::releaseResources() {}

bool ScopeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	return (layouts.getMainInputChannels() > 0);
}

void ScopeAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	if(state.values[4] > .5 || sleep.get() > samplerate) return;
	++sleep;

	float time = 0;
	if(state.values[0] < .5)
		time = pow(state.values[1],5)*240+.05f;
	else
		time = pow(state.values[2],4)*16+.05f;

	int numsamples = buffer.getNumSamples();
	const float* const* channelData = buffer.getArrayOfReadPointers();
	for(int sample = 0; sample < numsamples; ++sample) {

		//capture frame
		if(oscskip == 0)
			for(int channel = 0; channel < channelnum; ++channel)
				osci[channel*oscisize+oscindex] = 0;

		//zoom out mode
		if(state.values[0] < .5 && time > 50)
			for(int channel = 0; channel < channelnum; ++channel)
				osci[channel*oscisize+oscindex] += pow(channelData[channel][sample],2);
		//zoom in mode
		else
			for(int channel = 0; channel < channelnum; ++channel)
				osci[channel*oscisize+oscindex] += channelData[channel][sample];

		//advance frame
		++oscskip;
		if(oscskip >= floor(time)) {
			oscskip = 0;
			if(state.values[0] < .5 && time > 50) {
				oscimode[oscindex] = true;
				for(int channel = 0; channel < channelnum; ++channel)
					osci[channel*oscisize+oscindex] = sqrt((1.f/fmax(1,floor(time)))*osci[channel*oscisize+oscindex]);
			} else {
				oscimode[oscindex] = false;
				for(int channel = 0; channel < channelnum; ++channel)
					osci[channel*oscisize+oscindex] *= (1.f/fmax(1,floor(time)));
			}

			oscindex++;
			if(oscindex >= oscisize) oscindex = 0;

			//sync part 2
			if(state.values[0] > .5 || state.values[5] < .5) {
				// no sync
				osciscore[oscindex] = -1;
			} else {
				// sync algorithm
				osciscore[fmod(oscindex-((samplerate/30)/fmax(1,floor(time)))+oscisize,oscisize)] = -1;
				osciscore[oscindex] = 0;
				for(int sample = 0; sample < 800; ++sample) {
					int index = fmod(oscindex-801+sample+oscisize,oscisize);
					for(int channel = 0; channel < channelnum; ++channel)
						if((osci[channel*oscisize+index] > 0) == (line[channel*800+sample] > 0))
							++osciscore[oscindex];
				}
			}
		}
	}

	// update the next frame
	if(!frameready.get()) {
		int syncindex = 0;
		if(state.values[0] > .5 || state.values[5] < .5) {
			syncindex = fmod(oscindex-801+oscisize,oscisize);
		} else {
			int maxscore = -100;
			int maxindex = -100;
			for(int i = 0; i < oscisize; ++i) {
				if(osciscore[i] >= maxscore) {
					maxscore = osciscore[i];
					maxindex = i;
				}
			}
			if(maxscore >= 0)
				syncindex = fmod(maxindex-801+oscisize,oscisize);
			else
				syncindex = fmod(oscindex-801+oscisize,oscisize);
		}
		for(int sample = 0; sample < 800; ++sample) {
			int index = fmod(syncindex+sample,oscisize);
			for(int channel = 0; channel < channelnum; ++channel)
				line[channel*800+sample] = osci[channel*oscisize+index];
			linemode[sample] = oscimode[index];
		}
		frameready = true;
	}

	// mute output audio on standalone mode
	if(JUCEApplication::isStandaloneApp()) buffer.clear();
	else for(int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());
}

bool ScopeAudioProcessor::hasEditor() const { return true; }

AudioProcessorEditor* ScopeAudioProcessor::createEditor() {
	return new ScopeAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void ScopeAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< height.get() << delimiter
		<< settingsx.get() << delimiter
		<< settingsy.get() << delimiter
		<< (settingsopen.get()?1:0) << delimiter
		<< currentpreset << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << delimiter;
		data << get_preset(i) << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void ScopeAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	saved = true;
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		height = std::stoi(token);

		std::getline(ss, token, delimiter);
		settingsx = std::stoi(token);

		std::getline(ss, token, delimiter);
		settingsy = std::stoi(token);

		std::getline(ss, token, delimiter);
		settingsopen = std::stoi(token)>.5;

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		for(int i = 0; i < getNumPrograms(); ++i) {
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

	for(int i = 0; i < paramcount; i++) {
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		if(params.pots[i].smoothtime > 0) {
			params.pots[i].smooth.setCurrentAndTargetValue(presets[currentpreset].values[i]);
			state.values[i] = presets[currentpreset].values[i];
		}
	}
}
const String ScopeAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int v = 0; v < paramcount; v++)
		data << presets[preset_id].values[v] << delimiter;

	return data.str();
}
void ScopeAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
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

void ScopeAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		presets[currentpreset].values[i] = newValue;
		if((parameterID == "xy" && newValue < .5) || (parameterID == "sync" && newValue > .5))
			for(int i = 0; i < oscisize; ++i) osciscore[i] = -1;
		else if(parameterID == "time" && state.values[0] < .5 && state.values[5] > .5) {
			int startsync = oscindex-oscisize;
			int endsync = (oscindex-((samplerate/30)/fmax(1,floor(pow(state.values[1],5)*240+.05f))));
			for(int i = startsync; i <= endsync; ++i)
				osciscore[fmod(i+oscisize,oscisize)] = -1;
		}
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ScopeAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout ScopeAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"xy"			,1},"XY"												 ,false	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"time"		,1},"Time"		,NormalisableRange<float>( 0.0f	,1.0f	),0.4f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"timexy"		,1},"Time XY"	,NormalisableRange<float>( 0.0f	,1.0f	),0.4f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"scale"		,1},"Scale"		,NormalisableRange<float>( 0.f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"hold"		,1},"Hold"												 ,false	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"sync"		,1},"Sync"												 ,true	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"grid"		,1},"Grid"												 ,true	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"hue"			,1},"Hue"		,NormalisableRange<float>( 0.0f	,1.0f	),0.54f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"saturation"	,1},"Saturation",NormalisableRange<float>( 0.0f	,1.0f	),0.9f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"value"		,1},"Value"		,NormalisableRange<float>( 0.0f	,1.0f	),0.65f	));
	return { parameters.begin(), parameters.end() };
}