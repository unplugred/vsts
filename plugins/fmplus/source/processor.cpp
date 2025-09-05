#include "processor.h"
#include "editor.h"

FMPlusAudioProcessor::FMPlusAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	presets[0] = pluginpreset("Default");
	for(int i = 8; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "Program " + (String)(i-7);
	}

	params.pots[ 0] = potentiometer("Voices"										,"voices"					,0		,presets[0].values[ 0]	,1	,24		,potentiometer::inttype		);
	params.pots[ 1] = potentiometer("Portamento"									,"portamento"				,.001f	,presets[0].values[ 1]	);
	params.pots[ 2] = potentiometer("Legato"										,"legato"					,0		,presets[0].values[ 2]	,0	,1		,potentiometer::booltype	);
	params.pots[ 3] = potentiometer("Pitch Bend"									,"pitchbend"				,.001f	,presets[0].values[ 3]	,0	,24		,potentiometer::inttype		);
	params.pots[ 4] = potentiometer("LFO Sync"										,"lfosync"					,0		,presets[0].values[ 4]	,0	,2		,potentiometer::inttype		);
	params.pots[ 5] = potentiometer("Arp On"										,"arpon"					,0		,presets[0].values[ 5]	,0	,1		,potentiometer::booltype	);
	//params.pots[ 6] = potentiometer("Arp Speed"										,"arpspeed"					,0		,presets[0].values[ 6]	);
	//params.pots[ 7] = potentiometer("Arp Speed (eighth note)"						,"arpbpm"					,0		,presets[0].values[ 7]	,0	,32		,potentiometer::inttype		);
	//params.pots[ 8] = potentiometer("Arp Direction"									,"arpdirection"				,0		,presets[0].values[ 8]	,0	,4		,potentiometer::inttype		);
	//params.pots[ 9] = potentiometer("Arp Length"									,"arplength"				,0		,presets[0].values[ 9]	);
	//params.pots[10] = potentiometer("Vibrato On"									,"vibratoon"				,.001f	,presets[0].values[10]	,0	,1		,potentiometer::booltype	);
	//params.pots[11] = potentiometer("Vibrato Rate"									,"vibratorate"				,.001f	,presets[0].values[11]	);
	//params.pots[12] = potentiometer("Vibrato Rate (eighth note)"					,"vibratobpm"				,.001f	,presets[0].values[12]	,0	,32		,potentiometer::inttype		);
	//params.pots[13] = potentiometer("Vibrato Amount"								,"vibratoamount"			,.001f	,presets[0].values[13]	);
	//params.pots[14] = potentiometer("Vibrato Attack"								,"vibratoattack"			,0		,presets[0].values[14]	);
	//params.pots[15] = potentiometer("Anti-Alias"									,"antialias"				,0		,presets[0].values[15]	);
	//for(int o = 0; o < MC; ++o) {
	//params.pots[ 0] = potentiometer("OP"+(String)(o+1)+" On"						,"o"+(String)o+"on"		 	,0		,presets[0].values[ 0]	,0	,1		,potentiometer::booltype	);
	//params.pots[ 1] = potentiometer("OP"+(String)(o+1)+" Pan"						,"o"+(String)o+"pan"		,0		,presets[0].values[ 1]	);
	//params.pots[ 2] = potentiometer("OP"+(String)(o+1)+" Amplitude"					,"o"+(String)o+"amp"		,0		,presets[0].values[ 2]	);
	//params.pots[ 3] = potentiometer("OP"+(String)(o+1)+" Tone"						,"o"+(String)o+"tone"		,.001f	,presets[0].values[ 3]	);
	//params.pots[ 4] = potentiometer("OP"+(String)(o+1)+" Velocity"					,"o"+(String)o+"velocity"	,0		,presets[0].values[ 4]	);
	//params.pots[ 5] = potentiometer("OP"+(String)(o+1)+" After-Touch"				,"o"+(String)o+"aftertouch"	,.001f	,presets[0].values[ 5]	);
	//params.pots[ 6] = potentiometer("OP"+(String)(o+1)+" Frequency Mode"			,"o"+(String)o+"freqmode"	,0		,presets[0].values[ 6]	,0	,1		,potentiometer::booltype	);
	//params.pots[ 7] = potentiometer("OP"+(String)(o+1)+" Frequency Multiplier"		,"o"+(String)o+"freqmult"	,.001f	,presets[0].values[ 7]	);
	//params.pots[ 8] = potentiometer("OP"+(String)(o+1)+" Frequency Offset"			,"o"+(String)o+"freqadd"	,.001f	,presets[0].values[ 8]	);
	//params.pots[ 9] = potentiometer("OP"+(String)(o+1)+" Attack"					,"o"+(String)o+"attack"		,.001f	,presets[0].values[ 9]	);
	//params.pots[10] = potentiometer("OP"+(String)(o+1)+" Decay"						,"o"+(String)o+"decay"		,.001f	,presets[0].values[10]	);
	//params.pots[11] = potentiometer("OP"+(String)(o+1)+" Sustain"					,"o"+(String)o+"sustain"	,.001f	,presets[0].values[11]	);
	//params.pots[12] = potentiometer("OP"+(String)(o+1)+" Release"					,"o"+(String)o+"release"	,.001f	,presets[0].values[12]	);
	//params.pots[13] = potentiometer("OP"+(String)(o+1)+" LFO On"					,"o"+(String)o+"lfoon"		,.001f	,presets[0].values[13]	,0	,1		,potentiometer::booltype	);
	//params.pots[14] = potentiometer("OP"+(String)(o+1)+" LFO Target"				,"o"+(String)o+"lfotarget"	,0		,presets[0].values[14]	,0	,3		,potentiometer::inttype		);
	//params.pots[15] = potentiometer("OP"+(String)(o+1)+" LFO Rate"					,"o"+(String)o+"lforate"	,.001f	,presets[0].values[15]	);
	//params.pots[16] = potentiometer("OP"+(String)(o+1)+" LFO Rate (eighth note)"	,"o"+(String)o+"lfobpm"		,.001f	,presets[0].values[16]	,0	,32		,potentiometer::inttype		);
	//params.pots[17] = potentiometer("OP"+(String)(o+1)+" LFO Amount"				,"o"+(String)o+"lfoamount"	,.001f	,presets[0].values[17]	);
	//params.pots[18] = potentiometer("OP"+(String)(o+1)+" LFO Attack"				,"o"+(String)o+"lfoattack"	,.001f	,presets[0].values[18]	);
	//}
	// TODO FX

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		add_listener(params.pots[i].id);
	}
}

FMPlusAudioProcessor::~FMPlusAudioProcessor(){
	close();
}

const String FMPlusAudioProcessor::getName() const { return "Plastic Funeral"; }
bool FMPlusAudioProcessor::acceptsMidi() const { return false; }
bool FMPlusAudioProcessor::producesMidi() const { return false; }
bool FMPlusAudioProcessor::isMidiEffect() const { return false; }
double FMPlusAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int FMPlusAudioProcessor::getNumPrograms() { return 20; }
int FMPlusAudioProcessor::getCurrentProgram() { return currentpreset; }
void FMPlusAudioProcessor::setCurrentProgram(int index) {
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
void FMPlusAudioProcessor::timerCallback() {
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
const String FMPlusAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void FMPlusAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void FMPlusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	if(!saved && sampleRate > 60000) {
		params.oversampling = false;
		//apvts.getParameter("oversampling")->setValueNotifyingHost(0);
	}
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void FMPlusAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void FMPlusAudioProcessor::reseteverything() {
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
void FMPlusAudioProcessor::releaseResources() { }

bool FMPlusAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void FMPlusAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	saved = true;

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

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

	for(int sample = 0; sample < numsamples; ++sample) {
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			state.values[i] = params.pots[i].smooth.getNextValue();

		if(curfat != state.values[1] || curdry != state.values[3]) {
			curfat = state.values[1];
			curdry = state.values[3];
			curnorm = normalizegain(curfat,curdry);
		}

		for(int channel = 0; channel < channelnum; ++channel) {
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

float FMPlusAudioProcessor::plasticfuneral(float source, int channel, int channelcount, pluginpreset stt, float nrm) {
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

float FMPlusAudioProcessor::normalizegain(float fat, float dry) {
	if(fat > 0 || dry == 0) {
		double c = fat*dry-fat;
		double x = (c+1)/(c*1.5708);
		if(x > 0 && x < 1) {
			x = acos(x)/1.5708;
			return 1/fmax(fabs((fat*(sin(x*1.5708)-x)+x)*(1-dry)+x*dry),1);
		}
		return 1;
	}
	if(fat < -7.2f) {
		double newnorm = 1;
		for(double i = .4836f; i <= .71f; i += .005f)
			newnorm = fmax(newnorm,fabs(i+(sin(i*1.5708)-i)*fat)*(1-dry)+i*dry);
		return 1/newnorm;
	}
	return 1;
}

void FMPlusAudioProcessor::setoversampling(bool toggle) {
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

bool FMPlusAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* FMPlusAudioProcessor::createEditor() {
	return new FMPlusAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void FMPlusAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter
		<< (params.oversampling?1:0) << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << delimiter;
		data << get_preset(i) << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void FMPlusAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	saved = true;
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		std::getline(ss, token, delimiter);
		params.oversampling = std::stof(token) > .5;

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
const String FMPlusAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int v = 0; v < paramcount; v++)
		data << presets[preset_id].values[v] << delimiter;

	return data.str();
}
void FMPlusAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
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

void FMPlusAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		if(lerpstage < .001 || lerpchanged[i]) presets[currentpreset].values[i] = newValue;
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FMPlusAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout FMPlusAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"voices"					,1},"Voices"																	 ,1		,24		 ,12	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"portamento"				,1},"Portamento"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"legato"					,1},"Legato"																	 				 ,false	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"pitchbend"				,1},"Pitch Bend"																 ,0		,24		 ,2		));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"lfosync"					,1},"LFO Sync"																	 ,0		,2		 ,0		)); // NOTE, RANDOM, FREE
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"arpon"					,1},"Arp On"																	 				 ,false	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"arpspeed"				,1},"Arp Speed"									,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"arpbpm"					,1},"Arp Speed (eighth note)"													 ,0		,32		 ,0		));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"arpdirection"			,1},"Arp Direction"																 ,0		,4		 ,0		)); // UP, DOWN, U&D, D&U, RANDOM
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"arplength"				,1},"Arp Length"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"vibratoon"				,1},"Vibrato On"																 				 ,false	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"vibratorate"				,1},"Vibrato Rate"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"vibratobpm"				,1},"Vibrato Rate (eighth note)"												 ,0		,32		 ,0		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"vibratoamount"			,1},"Vibrato Amount"							,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"vibratoattack"			,1},"Vibrato Attack"							,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"antialias"				,1},"Anti-Alias"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	for(int o = 0; o < MC; ++o) {
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"o"+(String)o+"on"		,1},"OP"+(String)(o+1)+" On"													 				 ,o<=2	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"pan"		,1},"OP"+(String)(o+1)+" Pan"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"amp"		,1},"OP"+(String)(o+1)+" Amplitude"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"tone"		,1},"OP"+(String)(o+1)+" Tone"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"velocity"	,1},"OP"+(String)(o+1)+" Velocity"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"aftertouch",1},"OP"+(String)(o+1)+" After-Touch"			,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"o"+(String)o+"freqmode"	,1},"OP"+(String)(o+1)+" Frequency Mode"														 ,true	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"freqmult"	,1},"OP"+(String)(o+1)+" Frequency Multiplier"	,juce::NormalisableRange<float	>(0.0f	,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"freqadd"	,1},"OP"+(String)(o+1)+" Frequency Offset"		,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"attack"	,1},"OP"+(String)(o+1)+" Attack"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"decay"		,1},"OP"+(String)(o+1)+" Decay"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"sustain"	,1},"OP"+(String)(o+1)+" Sustain"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),1.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"release"	,1},"OP"+(String)(o+1)+" Release"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"o"+(String)o+"lfoon"		,1},"OP"+(String)(o+1)+" LFO On"												 				 ,false	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"o"+(String)o+"lfotarget"	,1},"OP"+(String)(o+1)+" LFO Target"											 ,0		,3		 ,0		)); // AMPLITUDE, PITCH, PAN, TONE
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"lforate"	,1},"OP"+(String)(o+1)+" LFO Rate"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"o"+(String)o+"lfobpm"	,1},"OP"+(String)(o+1)+" LFO Rate (eighth note)"								 ,0		,32		 ,0		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"lfoamount"	,1},"OP"+(String)(o+1)+" LFO Amount"			,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"lfoattack"	,1},"OP"+(String)(o+1)+" LFO Attack"			,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	));
	}
	// TODO FX
	return { parameters.begin(), parameters.end() };
}
