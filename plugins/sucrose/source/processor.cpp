#include "processor.h"
#include "editor.h"

SucroseAudioProcessor::SucroseAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	presets[0] = pluginpreset("Default"				,0.f	,.5f	,0.f	,0.f	,0.f	,0.f	,1		);
	for(int i = 1; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "Program "+(String)i;
	}

	params.pots[0] = potentiometer("subharmonix"	,"sub"		,.001f	,presets[0].values[0]	);
	params.pots[1] = potentiometer("fundemental"	,"dry"		,.001f	,presets[0].values[1]	);
	params.pots[2] = potentiometer("2nd harmonix"	,"second"	,.001f	,presets[0].values[2]	);
	params.pots[3] = potentiometer("3rd harmonix"	,"third"	,.001f	,presets[0].values[3]	);
	params.pots[4] = potentiometer("low cut"		,"lc"		,0.f	,presets[0].values[4]	);
	params.pots[5] = potentiometer("high cut"		,"hc"		,0.f	,presets[0].values[5]	);
	params.pots[6] = potentiometer("algorithm"		,"algo"		,0.f	,presets[0].values[6]	,0,2,potentiometer::inttype);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		add_listener(params.pots[i].id);
	}
}

SucroseAudioProcessor::~SucroseAudioProcessor(){
	close();
}

const String SucroseAudioProcessor::getName() const { return "Sucrose"; }
bool SucroseAudioProcessor::acceptsMidi() const { return false; }
bool SucroseAudioProcessor::producesMidi() const { return false; }
bool SucroseAudioProcessor::isMidiEffect() const { return false; }
double SucroseAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int SucroseAudioProcessor::getNumPrograms() { return 20; }
int SucroseAudioProcessor::getCurrentProgram() { return currentpreset; }
void SucroseAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	for(int i = 0; i < paramcount; i++) {
		if(params.pots[i].ttype != potentiometer::floattype) continue;
		if(lerpstage >= .001 && !lerpchanged[i]) continue;
		lerptable[i] = params.pots[i].normalize(presets[currentpreset].values[i]);
		lerpchanged[i] = false;
	}
	currentpreset = index;
	for(int i = 0; i < paramcount; i++) {
		if(params.pots[i].ttype == potentiometer::floattype) continue;
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[index].values[i]));
	}

	if(lerpstage <= 0) {
		lerpstage = 1;
		startTimerHz(30);
	} else lerpstage = 1;
}
void SucroseAudioProcessor::timerCallback() {
	lerpstage *= .64f;
	if(lerpstage < .001) {
		for(int i = 0; i < paramcount; i++) {
			if(params.pots[i].ttype != potentiometer::floattype) continue;
			if(lerpchanged[i]) continue;
			apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		}
		lerpstage = 0;
		undo_manager.beginNewTransaction();
		stopTimer();
		return;
	}
	for(int i = 0; i < paramcount; i++) {
		if(params.pots[i].ttype != potentiometer::floattype) continue;
		if(lerpchanged[i]) continue;
		lerptable[i] = (params.pots[i].normalize(presets[currentpreset].values[i])-lerptable[i])*.36f+lerptable[i];
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(lerptable[i]);
	}
}
const String SucroseAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void SucroseAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void SucroseAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void SucroseAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void SucroseAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
		params.pots[i].smooth.reset(samplerate,params.pots[i].smoothtime);
}
void SucroseAudioProcessor::releaseResources() { }

bool SucroseAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void SucroseAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	int numsamples = buffer.getNumSamples();

	float* const* channelData = buffer.getArrayOfWritePointers();

	for(int sample = 0; sample < numsamples; ++sample) {
		// parameter smoothing
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			state.values[i] = params.pots[i].smooth.getNextValue();

		float sub    = togain(state.values[0]);
		float dry    = togain(state.values[1]);
		float second = togain(state.values[2]);
		float third  = togain(state.values[3]);
		float lc     = tolc  (state.values[4]);
		float hc     = tohc  (state.values[5]);
		int algo     =        state.values[6] ;
		for(int channel = 0; channel < channelnum; ++channel) {
			channelData[channel][sample] = 0; // TODO
		}
	}
}

bool SucroseAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* SucroseAudioProcessor::createEditor() {
	return new SucroseAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void SucroseAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << delimiter;
		data << get_preset(i) << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void SucroseAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	try {
		std::istringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		for(int i = 0; i < getNumPrograms(); i++) {
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
}
const String SucroseAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int v = 0; v < paramcount; v++)
		data << presets[preset_id].values[v] << delimiter;

	return data.str();
}
void SucroseAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(preset_id);
	try {
		std::istringstream ss(preset.trim().toRawUTF8());
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

void SucroseAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0)
			params.pots[i].smooth.setTargetValue(newValue);
		else
			state.values[i] = newValue;
		if(params.pots[i].ttype != potentiometer::floattype || lerpstage < .001 || lerpchanged[i])
			presets[currentpreset].values[i] = newValue;
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SucroseAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout SucroseAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"sub"		,1},"subharmonix"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(stodb	).withValueFromStringFunction(sfromdb	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"dry"		,1},"fundemental"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(stodb	).withValueFromStringFunction(sfromdb	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"second"	,1},"2nd harmonix"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(stodb	).withValueFromStringFunction(sfromdb	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"third"	,1},"3rd harmonix"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(stodb	).withValueFromStringFunction(sfromdb	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"lc"		,1},"low cut"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(stolc	).withValueFromStringFunction(sfromlc	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"hc"		,1},"high cut"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(stohc	).withValueFromStringFunction(sfromhc	)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"algo"	,1},"algorithm"										,0		,2		 ,1		,AudioParameterIntAttributes()	.withStringFromValueFunction(toalgo	).withValueFromStringFunction(fromalgo	)));
	return { parameters.begin(), parameters.end() };
}
