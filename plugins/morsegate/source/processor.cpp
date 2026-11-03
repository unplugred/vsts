#include "processor.h"
#include "editor.h"

MorseGateAudioProcessor::MorseGateAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	presets[0] = pluginpreset(""	,0.0f	,0.0f	,0.45f	,2.0f	);
	// TODO presets
	for(int i = 1; i < getNumPrograms(); i++)
		presets[i] = presets[0];

	params.pots[0] = potentiometer("Retrigger"			,"retrigger"	,presets[0].values[0]	,0	,1	,potentiometer::booltype	);
	params.pots[1] = potentiometer("Release"			,"release"		,presets[0].values[1]	);
	params.pots[2] = potentiometer("Length"				,"length"		,presets[0].values[2]	);
	params.pots[3] = potentiometer("Length (BPM sync)"	,"lengthbpm"	,presets[0].values[3]	,0	,4	,potentiometer::inttype		);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		add_listener(params.pots[i].id);
	}

	set_text("");
}

MorseGateAudioProcessor::~MorseGateAudioProcessor(){
	close();
}

const String MorseGateAudioProcessor::getName() const { return "Morse Gate"; }
bool MorseGateAudioProcessor::acceptsMidi() const { return true; }
bool MorseGateAudioProcessor::producesMidi() const { return true; }
bool MorseGateAudioProcessor::isMidiEffect() const { return false; }
double MorseGateAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MorseGateAudioProcessor::getNumPrograms() { return 20; }
int MorseGateAudioProcessor::getCurrentProgram() { return currentpreset; }
void MorseGateAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += getProgramName(index));
	currentpreset = index;

	for(int i = 0; i < paramcount; i++)
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
	set_text(presets[currentpreset].text);
}
const String MorseGateAudioProcessor::getProgramName(int index) {
	if(presets[index].text != "")
		return { presets[index].text };
	if(index == 0)
		return "Default";
	return "Program "+(String)index;
}
void MorseGateAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].text = newName;
	if(index == currentpreset)
		set_text(presets[currentpreset].text);
}

void MorseGateAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	if(processedtext.size() == 0) {
		letterindex = 9999;
	} else {
		letterindex = 0;
		if(processedtext[letterindex] == 32)
			currentletter = 0;
		else if(processedtext[letterindex] >= 48 && processedtext[letterindex] <  58)
			currentletter = processedtext[letterindex]-48+1;
		else if(processedtext[letterindex] >= 97 && processedtext[letterindex] < 123)
			currentletter = processedtext[letterindex]-97+11;
	}

	elementindex = 0;
	currentelement = letters[currentletter][elementindex+1];

	unitindex = 0;
	currentunit = elements[currentelement][unitindex+1];

	unitprogress = 0;
	gainlerp = 0;

	reseteverything();
}
void MorseGateAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void MorseGateAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return; // TODO
}
void MorseGateAudioProcessor::releaseResources() { }

bool MorseGateAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	return (layouts.getMainOutputChannelSet()==layouts.getMainInputChannelSet());
}

void MorseGateAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	int numsamples = buffer.getNumSamples();
	float* const* channelData = buffer.getArrayOfWritePointers();

	bool reset = false;
	if(prevtext != presets[currentpreset].text) {
		prevtext = presets[currentpreset].text;
		if(prevtext.length() == 0) {
			String txt = "an end to genocide ";
			processedtext.resize(txt.length());
			for(int i = 0; i < txt.length(); ++i)
				processedtext[i] = txt[i];
			return;
		}
		//if((prevtext[prevtext.length()-1] >= 48 && prevtext[prevtext.length()-1] <  58) || (prevtext[prevtext.length()-1] >= 97 && prevtext[prevtext.length()-1] < 123)) {
		//	processedtext.resize(prevtext.length()+1);
		//	processedtext[processedtext.size()-1] = " "; TODO
		//} else
		processedtext.resize(prevtext.length());
		bool paused = false;
		bool hasspace = false;
		for(int i = 0; i < prevtext.length(); ++i) {
			if(prevtext[i] == 32 || prevtext[i] == '\n') {
				if(paused && !hasspace)
					processedtext[i] = '-';
				paused = true;
				hasspace = true;
				continue;
			}
			if((prevtext[i] >= 48 && prevtext[i] <  58) || (prevtext[i] >= 97 && prevtext[i] < 123)) {
				processedtext[i] = prevtext[i];
				paused = false;
				hasspace = false;
				continue;
			}
			if(!paused) {
				processedtext[i] = ' ';
				paused = true;
				hasspace = false;
				continue;
			}
			processedtext[i] = '-';
		}
		if(letterindex >= processedtext.size()) reset = true;
	}

	if((state.values[0]>.5f) != lastretrigger) {
		lastretrigger = state.values[0]>.5f;
		if(lastretrigger) reset = true;
	}

	if(reset) {
		if(unitprogress <= .25f) {
			letterindex = 0;
			if(processedtext[letterindex] == 32)
				currentletter = 0;
			else if(processedtext[letterindex] >= 48 && processedtext[letterindex] <  58)
				currentletter = processedtext[letterindex]-48+1;
			else if(processedtext[letterindex] >= 97 && processedtext[letterindex] < 123)
				currentletter = processedtext[letterindex]-97+11;
		} else {
			letterindex = -1;
			currentletter = 0;
		}

		elementindex = 0;
		currentelement = letters[currentletter][elementindex+1];

		unitindex = 0;
		currentunit = elements[currentelement][unitindex+1];
	}

	float speed = 0;
	if(state.values[3] > .1f) {
		AudioPlayHead* playhead = getPlayHead();
		if(playHead != nullptr) if(auto positioninfo = playhead->getPosition()) {
			if(auto bpm = positioninfo->getBpm        ()) if((*bpm) != lastbpm) {
				lastbpm = *bpm;
			}
			if(auto ppq = positioninfo->getPpqPosition()) if((*ppq) != lastppq) {
				lastppq = *ppq;
				float offset = fmod(lastppq*powf(2,4-state.values[3]),1.f);
				unitprogress = round(unitprogress-offset)+offset;
			}
		}
		speed = (lastbpm*powf(2,4-state.values[3]))/(60*samplerate);
	} else {
		speed = 1.f/((state.values[2]*state.values[2]*(.5f-.05f)+.05f)*samplerate);
	}

	// testing sine
	int s;
	//*/
	for(s = 0; s < numsamples; ++s) {
		crntsmpl = fmod(crntsmpl+1000.f/samplerate,1);
		channelData[0][s] = sin(crntsmpl*MathConstants<double>::twoPi)*.25f;
		for(int c = 0; c < channelnum; ++c) channelData[c][s] = channelData[0][s];
	}
	//*/

	s = 0;
	while(s < numsamples) {
		// progress
		if(unitprogress >= 1) { // next unit
			unitprogress = fmod(unitprogress,1.f);
			++unitindex;
			if(unitindex >= elements[currentelement][0]) { // next element
				unitindex = 0;
				++elementindex;
				if(elementindex >= letters[currentletter][0]) { // next letter
					elementindex = 0;
					++letterindex;
					if(letterindex >= processedtext.size())
						letterindex = 0;
					uichar = letterindex;
					if(processedtext[letterindex] == 32)
						currentletter = 0;
					else if(processedtext[letterindex] >= 48 && processedtext[letterindex] <  58)
						currentletter = processedtext[letterindex]-48+1;
					else if(processedtext[letterindex] >= 97 && processedtext[letterindex] < 123)
						currentletter = processedtext[letterindex]-97+11;
				}
				currentelement = letters[currentletter][elementindex+1];
			}
			currentunit = elements[currentelement][unitindex+1];
			uiison = currentunit==2||(currentunit==1&&unitprogress<=.5f);
		}

		// gate
		int numproc = fmin(numsamples-s,ceil((1-unitprogress)*(1.f/speed)));
		if(currentunit == 0) { // off
			for(int c = 0; c < channelnum; ++c) {
				for(int ss = s; ss < (s+numproc); ++ss) {
					channelData[c][ss] = 0;
				}
			}
			gainlerp = 0;
			unitprogress += speed*numproc;
		} else if(currentunit == 2) { // on
			float gainlerpinit = gainlerp;
			for(int c = 0; c < channelnum; ++c) {
				gainlerp = gainlerpinit;
				for(int ss = s; ss < (s+numproc); ++ss) {
					gainlerp = SMOOTHING*gainlerp+(1-SMOOTHING);
					channelData[c][ss] *= gainlerp;
				}
			}
			unitprogress += speed*numproc;
		} else if(state.values[1] < 0.001f) { // transition
			float unitprogressinit = unitprogress;
			float gainlerpinit = gainlerp;
			for(int c = 0; c < channelnum; ++c) {
				unitprogress = unitprogressinit;
				gainlerp = gainlerpinit;
				for(int ss = s; ss < (s+numproc); ++ss) {
					gainlerp = SMOOTHING*gainlerp+(1-SMOOTHING)*(1-round(unitprogress));
					channelData[c][ss] *= gainlerp;
					unitprogress += speed;
				}
			}
			uiison = unitprogress<=.5f;
		} else {
			float unitprogressinit = unitprogress;
			float gainlerpinit = gainlerp;
			for(int c = 0; c < channelnum; ++c) {
				unitprogress = unitprogressinit;
				gainlerp = gainlerpinit;
				for(int ss = s; ss < (s+numproc); ++ss) {
					gainlerp = SMOOTHING*gainlerp+(1-SMOOTHING)*fmin(1,fmax(0,(.5f-unitprogress)/state.values[1]+.5f));
					channelData[c][ss] *= gainlerp;
					unitprogress += speed;
				}
			}
			uiison = unitprogress<=.5f;
		}
		s += numproc;
	}
}

bool MorseGateAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* MorseGateAudioProcessor::createEditor() {
	return new MorseGateAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void MorseGateAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter;

	for(int i = 0; i < getNumPrograms(); i++)
		data << get_preset(i) << delimiter;

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void MorseGateAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
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
const String MorseGateAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int v = 0; v < paramcount; ++v)
		data << presets[preset_id].values[v] << delimiter;
	data << presets[preset_id].text.replaceCharacter(delimiter,'`').replace("\n","§");

	return data.str();
}
void MorseGateAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(preset_id);
	try {
		std::istringstream ss(preset.trim().toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int save_version = std::stoi(token);

		for(int v = 0; v < paramcount; ++v) {
			std::getline(ss, token, delimiter);
			presets[preset_id].values[v] = std::stof(token);
		}

		std::getline(ss, token, delimiter);
		presets[preset_id].text = ((String)token).replaceCharacter('`',delimiter).replace("§","\n");

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

	for(int i = 0; i < paramcount; i++)
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
	set_text(presets[currentpreset].text);
}
void MorseGateAudioProcessor::set_text(const String& text) {
	debug(text);
	presets[currentpreset].text = text.toLowerCase().retainCharacters(" !\"#$%&'()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz[\\]^_{|}~\n");
	// TODO limit length
}

void MorseGateAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		state.values[i] = newValue;
		presets[currentpreset].values[i] = newValue;
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MorseGateAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout MorseGateAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"retrigger"	,1},"Retrigger"													 ,false	,AudioParameterBoolAttributes	().withStringFromValueFunction(toretrigger	).withValueFromStringFunction(fromretrigger	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"release"		,1},"Release"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes	().withStringFromValueFunction(torelease	).withValueFromStringFunction(fromrelease	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"length"		,1},"Length"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.45f	,AudioParameterFloatAttributes	().withStringFromValueFunction(tolength		).withValueFromStringFunction(fromlength	)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"lengthbpm"	,1},"Length (BPM sync)"							 ,0		,4		 ,2		,AudioParameterIntAttributes	().withStringFromValueFunction(tobpm		).withValueFromStringFunction(frombpm		)));
	return { parameters.begin(), parameters.end() };
}
