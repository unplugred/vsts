#include "processor.h"
#include "editor.h"

ModManAudioProcessor::ModManAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	currentpreset = -1;
	//set_preset("",0); TODO
	currentpreset = 0;
	for(int i = 1; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "Program "+((String)i);
	}

	params.pots[0] = potentiometer("On"				,"on"			,.001f	,0.f	,1.f	,potentiometer::booltype);
	params.pots[1] = potentiometer("Min Range"		,"min"			,.001f	);
	params.pots[2] = potentiometer("Max Range"		,"max"			,.001f	);
	params.pots[3] = potentiometer("Speed"			,"speed"		,0		);
	params.pots[4] = potentiometer("Stereo"			,"stereo"		,.001f	);
	params.pots[5] = potentiometer("Master Speed"	,"masterspeed"	,0		);

	params.modulators[0].name = M1;
	params.modulators[1].name = M2;
	params.modulators[2].name = M3;
	params.modulators[3].name = M4;
	params.modulators[4].name = M5;
	params.modulators[5].name = M6;
	params.modulators[6].name = M7;

	for(int m = 0; m < MC; ++m) {
		time[m] = 0;
		params.modulators[m].id = m;
		for(int i = 0; i < (paramcount-1); i++) {
			if(i == 1 && (m == 0 || m == 5)) {
				params.modulators[m].defaults[i] = 0;
				state.values[m][i] = 0;
				presets[currentpreset].values[m][i] = 0;
				if(params.pots[i].smoothtime > 0) params.pots[i].smooth[m].setCurrentAndTargetValue(0);
				continue;
			}
			params.modulators[m].defaults[i] = presets[0].values[m][i];
			state.values[m][i] = params.pots[i].inflate(apvts.getParameter("m"+((String)m)+params.pots[i].id)->getValue());
			presets[currentpreset].values[m][i] = state.values[m][i];
			if(params.pots[i].smoothtime > 0) params.pots[i].smooth[m].setCurrentAndTargetValue(state.values[m][i]);
			add_listener("m"+((String)m)+params.pots[i].id);
		}
	}
	state.masterspeed = params.pots[paramcount-1].inflate(apvts.getParameter(params.pots[paramcount-1].id)->getValue());
	presets[currentpreset].masterspeed = state.masterspeed;
	if(params.pots[paramcount-1].smoothtime > 0) params.pots[paramcount-1].smooth[0].setCurrentAndTargetValue(state.masterspeed);
	add_listener(params.pots[paramcount-1].id);

	prlin.init();
}

ModManAudioProcessor::~ModManAudioProcessor(){
	close();
}

const String ModManAudioProcessor::getName() const { return "ModMan"; }
bool ModManAudioProcessor::acceptsMidi() const { return false; }
bool ModManAudioProcessor::producesMidi() const { return false; }
bool ModManAudioProcessor::isMidiEffect() const { return false; }
double ModManAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int ModManAudioProcessor::getNumPrograms() { return 20; }
int ModManAudioProcessor::getCurrentProgram() { return currentpreset; }
void ModManAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;

	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (paramcount-1); i++) {
			if(i == 1 && (m == 0 || m == 5)) continue;
			apvts.getParameter("m"+((String)m)+params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[m][i]));
		}
	}
	apvts.getParameter(params.pots[paramcount-1].id)->setValueNotifyingHost(params.pots[paramcount-1].normalize(presets[currentpreset].masterspeed));
}
const String ModManAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void ModManAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void ModManAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void ModManAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void ModManAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	for(int m = 0; m < MC; ++m)
		for(int i = 0; i < (paramcount-1); ++i) if(params.pots[i].smoothtime > 0)
			params.pots[i].smooth[m].reset(samplerate,params.pots[i].smoothtime);
	if(params.pots[paramcount-1].smoothtime > 0)
		params.pots[paramcount-1].smooth[0].reset(samplerate,params.pots[paramcount-1].smoothtime);

	modulator_data.resize(MC*channelnum*samplesperblock);
	for(int i = 0; i < (MC*channelnum*samplesperblock); ++i)
		modulator_data[i] = 0;

	drift_data.resize(channelnum*MAX_DRIFT*samplerate);
	for(int i = 0; i < (channelnum*MAX_DRIFT*samplerate); ++i)
		drift_data[i] = 0;

	flange_data.resize(channelnum*MAX_FLANGE*samplerate);
	for(int i = 0; i < (channelnum*MAX_FLANGE*samplerate); ++i)
		flange_data[i] = 0;

	dsp::ProcessSpec spec;
	spec.sampleRate = samplerate;
	spec.maximumBlockSize = samplesperblock;
	spec.numChannels = channelnum;
	lowpass.prepare(spec);
	lowpass.setType(dsp::StateVariableTPTFilterType::lowpass);
	lowpass.setCutoffFrequency(20000);
	lowpass.setResonance(1./MathConstants<double>::sqrt2);
	lowpass.reset();
}
void ModManAudioProcessor::releaseResources() { }

bool ModManAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void ModManAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	float prmsadd = rmsadd.get();
	int prmscount = rmscount.get();

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);

	float* const* channel_data = buffer.getArrayOfWritePointers();

	for(int m = 0; m < MC; ++m) {
		if(params.pots[0].smoothtime > 0)
			ison[m] = params.pots[0].smooth[m].getCurrentValue() > 0 || params.pots[0].smooth[m].getTargetValue() > 0;
		else
			ison[m] = state.values[m][0];
		for(int s = 0; s < numsamples; ++s) {
			for(int m = 0; m < MC; ++m)
				for(int i = 0; i < (paramcount-1); ++i) if(params.pots[i].smoothtime > 0)
					state.values[m][i] = params.pots[i].smooth[m].getNextValue();
			if(params.pots[paramcount-1].smoothtime > 0)
				state.masterspeed = params.pots[paramcount-1].smooth[0].getNextValue();

			float center = 0;
			float on = state.values[m][0];
			float min = state.values[m][1];
			float max = state.values[m][2];
			float speed = state.values[m][3];
			float stereo = pow(state.values[m][4],2);
			switch(m) {
				case 0: // DRIFT
					min = 0;
					max = pow(max,2);
					break;
				case 1: // LOW PASS
					center = 1;
					break;
				case 2: // LOW PASS RESONANCE
					//TODO: center = parameter
					center = .5f;
					break;
				case 3: // SATURATION
					break;
				case 4: // BITCRUSH
					center = 1;
					break;
				case 5: // FLANGE
					min = 0;
					max = 1;
					break;
				case 6: // AMPLITUDE
					center = .5f;
					break;
			}

			time[m] += pow(speed*2,4)*pow(state.masterspeed*2,4)*.00003f;
			for(int c = 0; c < channelnum; ++c) {
				modulator_data[(m*channelnum+c)*samplesperblock+s] = ((prlin.noise(time[m],((((float)c)/(channelnum-1))-.5)*stereo+m*10)*.5f+.5f)*(max-min)+min)*on+center*(1-on);
			}
		}
	}
	for(int s = 0; s < numsamples; ++s) {
		driftindex = fmod(driftindex+1,MAX_DRIFT*samplerate);
		flangeindex = fmod(flangeindex+1,MAX_FLANGE*samplerate);
		for(int c = 0; c < channelnum; ++c) {

			//DRIFT
			if(ison[0]) {
				drift_data[c*MAX_DRIFT*samplerate+driftindex] = channel_data[c][s];
				channel_data[c][s] = interpolatesamples(&drift_data[c*MAX_DRIFT*samplerate],driftindex+1+MAX_DRIFT*samplerate*(1-2.f/(samplerate*MAX_DRIFT))*modulator_data[c*samplesperblock+s],MAX_DRIFT*samplerate);
			}

			//LOWPASS
			if(ison[1]) {
				lowpass.setCutoffFrequency(calccutoff(modulator_data[(channelnum+c)*samplesperblock+s]));
				lowpass.setResonance(calcresonance(modulator_data[(2*channelnum+c)*samplesperblock+s]));
				channel_data[c][s] = lowpass.processSample(c,channel_data[c][s]);
			}

			//SATURATION
			if(ison[3]) {
				float satval = 1-(1-(pow(1-modulator_data[(3*channelnum+c)*samplesperblock+s],10)+(1-pow(modulator_data[(3*channelnum+c)*samplesperblock+s],.2)))*.5)*.99;
				channel_data[c][s] = (1-pow(1-fmin(fabs(channel_data[c][s]),1),1/satval))*(channel_data[c][s]>0?1:-1)*(1-(1-satval)*.92);
			}

			//BITCRUSH
			if(ison[4]) {
				float bitval = pow(1-modulator_data[(4*channelnum+c)*samplesperblock+s],3)*.99+.01;
				channel_data[c][s] = round(channel_data[c][s]/bitval)*bitval;
			}

			//FLANGE
			if(ison[5]) {
				flange_data[c*MAX_FLANGE*samplerate+flangeindex] = channel_data[c][s];
				channel_data[c][s] = interpolatesamples(&flange_data[c*MAX_FLANGE*samplerate],flangeindex+1+MAX_FLANGE*samplerate*(1-2.f/(samplerate*MAX_FLANGE))*modulator_data[(5*channelnum+c)*samplesperblock+s],MAX_FLANGE*samplerate)*state.values[5][2]*.5f+channel_data[c][s]*(1-state.values[5][2]*.5f);
			}

			//AMPLITUDE
			if(ison[6]) {
				channel_data[c][s] *= 2*pow(modulator_data[(6*channelnum+c)*samplesperblock+s],2);
			}

			if(prmscount < samplerate*2) {
				prmsadd += channel_data[c][s]*channel_data[c][s];
				prmscount++;
			}
		}
	}

	rmsadd = prmsadd;
	rmscount = prmscount;
}
float ModManAudioProcessor::interpolatesamples(float* buffer, float position, int buffersize) {
	return buffer[((int)floor(position))%buffersize]*(1-fmod(position,1.f))+buffer[((int)ceil(position))%buffersize]*fmod(position,1.f);
}

bool ModManAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* ModManAudioProcessor::createEditor() {
	return new ModManAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void ModManAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter
		<< params.selectedmodulator << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << delimiter;
		data << get_preset(i) << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void ModManAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		std::getline(ss, token, delimiter);
		params.selectedmodulator = std::stoi(token);

		for(int i = 0; i < getNumPrograms(); i++) {
			std::getline(ss,token,delimiter);
			presets[i].name = token;

			std::getline(ss, token, delimiter);
			set_preset(token,i,',',true);
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
const String ModManAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (paramcount-1); i++) {
			if(i == 1 && (m == 0 || m == 5))
				continue;
			data << presets[preset_id].values[m][i] << delimiter;
		}
	}
	data << presets[preset_id].masterspeed << delimiter;

	return data.str();
}
void ModManAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(preset_id);
	try {
		std::stringstream ss(preset.trim().toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int save_version = std::stoi(token);

		for(int m = 0; m < MC; ++m) {
			for(int i = 0; i < (paramcount-1); i++) {
				if(i == 1 && (m == 0 || m == 5))
					continue;
				std::getline(ss, token, delimiter);
				presets[preset_id].values[m][i] = std::stof(token);
			}
		}
		std::getline(ss, token, delimiter);
		presets[preset_id].masterspeed = std::stof(token);

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

	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (paramcount-1); i++) {
			if(i == 1 && (m == 0 || m == 5)) continue;
			apvts.getParameter("m"+((String)m)+params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[m][i]));
		}
	}
	apvts.getParameter(params.pots[paramcount-1].id)->setValueNotifyingHost(params.pots[paramcount-1].normalize(presets[currentpreset].masterspeed));
}

void ModManAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == params.pots[paramcount-1].id) {
		if(params.pots[paramcount-1].smoothtime > 0) params.pots[paramcount-1].smooth[0].setTargetValue(newValue);
		else state.masterspeed = newValue;
		presets[currentpreset].masterspeed = newValue;
	}
	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (paramcount-1); i++) {
			if(parameterID == ("m"+((String)m)+params.pots[i].id)) {
				if(params.pots[i].smoothtime > 0) params.pots[i].smooth[m].setTargetValue(newValue);
				else state.values[m][i] = newValue;
				presets[currentpreset].values[m][i] = newValue;
				return;
			}
		}
	}
}
float ModManAudioProcessor::calccutoff(float val) {
	return mapToLog10(val,20.f,20000.f);
}
float ModManAudioProcessor::calcresonance(float val) {
	return mapToLog10(val,0.1f,40.f);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ModManAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout ModManAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	for(int m = 0; m < MC; ++m) {
		String name;
		switch(m) {
			case 0: name = M1; break;
			case 1: name = M2; break;
			case 2: name = M3; break;
			case 3: name = M4; break;
			case 4: name = M5; break;
			case 5: name = M6; break;
			case 6: name = M7; break;
		} //TODO defaults, lpres
		parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"m"+((String)m)+"on"		,1},name+" On"															 ,false	));
		if(m == 0 || m == 5) {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" Range"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	));
		} else {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"min"		,1},name+" Min Range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" Max Range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	));
		}
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"speed"	,1},name+" Speed"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.5f	));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"stereo"	,1},name+" Stereo"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	));
		if(m == 1) {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"res"		,1},name+" Resonance"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	));
		}
	}
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"masterspeed"				,1},"Master Speed"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.5f	));
	return { parameters.begin(), parameters.end() };
}
