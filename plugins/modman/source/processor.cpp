#include "processor.h"
#include "editor.h"

ModManAudioProcessor::ModManAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	currentpreset = -1;
	presets[0].name = "default";
	set_preset("0,0,1,0.5,0.3,2,0,0,0.5,1,1,0.5,0,0,0.3,0.5,0.3,2,0,0,0.5,1,1,0.5,0,0,0.3,0.5,0.3,2,0,0,0.5,1,1,0.5,0,0,0.3,0.5,0.3,2,0,0,0.5,1,1,0.5,0,0,0.3,0.5,0.3,2,0,0,0.5,1,1,0.5,0.5,",0); //TODO better defaults
	currentpreset = 0;
	for(int i = 1; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "program "+((String)i);
		for(int m = 0; m < MC; ++m)
			presets[i].curves[m] = curve("2,0,0,0.5,1,1,0.5"); //TODO
	}

	params.pots[0] = potentiometer("on"				,"on"			,0	,0.f	,1.f	,potentiometer::booltype);
	params.pots[1] = potentiometer("min range"		,"min"			,0	);
	params.pots[2] = potentiometer("max range"		,"max"			,0	);
	params.pots[3] = potentiometer("speed"			,"speed"		,0	);
	params.pots[4] = potentiometer("stereo"			,"stereo"		,0	);
	params.pots[5] = potentiometer("master speed"	,"masterspeed"	,0	);

	params.modulators[0].name = M1;
	params.modulators[1].name = M2;
	params.modulators[2].name = M3;
	params.modulators[3].name = M4;
	params.modulators[4].name = M5;

	for(int m = 0; m < MC; ++m) {
		time[m] = 0;
		params.modulators[m].id = m;
		for(int i = 0; i < (paramcount-1); i++) {
			if(i == 1 && m == 0) {
				params.modulators[m].defaults[i] = 0;
				state.values[m][i] = 0;
				presets[currentpreset].values[m][i] = 0;
				continue;
			}
			params.modulators[m].defaults[i] = presets[0].values[m][i];
			state.values[m][i] = params.pots[i].inflate(apvts.getParameter("m"+((String)m)+params.pots[i].id)->getValue());
			presets[currentpreset].values[m][i] = state.values[m][i];
			add_listener("m"+((String)m)+params.pots[i].id);
		}
	}
	state.masterspeed = params.pots[paramcount-1].inflate(apvts.getParameter(params.pots[paramcount-1].id)->getValue());
	presets[currentpreset].masterspeed = state.masterspeed;
	add_listener(params.pots[paramcount-1].id);

	for(int c = 0 ; c < 2; ++c)
		cuber_rot[c] = -.1f;

	prlin.init();

	updatedcurve = 1+2+4+8+16;
	updatevis = true;
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
			if(i == 1 && m == 0) continue;
			apvts.getParameter("m"+((String)m)+params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[m][i]));
		}
	}
	apvts.getParameter(params.pots[paramcount-1].id)->setValueNotifyingHost(params.pots[paramcount-1].normalize(presets[currentpreset].masterspeed));

	updatedcurve = 1+2+4+8+16;
	updatevis = true;
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
		state.curves[m].resizechannels(channelnum);

	modulator_data.resize(MC*channelnum*samplesperblock);
	for(int i = 0; i < (MC*channelnum*samplesperblock); ++i)
		modulator_data[i] = 0;

	drift_data.resize(channelnum*MAX_DRIFT*samplerate);
	for(int i = 0; i < (channelnum*MAX_DRIFT*samplerate); ++i)
		drift_data[i] = 0;

	smooth.resize(MC*channelnum);
	for(int i = 0; i < (MC*channelnum); ++i)
		smooth[i].reset(samplerate,i<channelnum?.01f:.001f);
	resetsmooth = true;

	dsp::ProcessSpec spec;
	spec.sampleRate = samplerate;
	spec.maximumBlockSize = samplesperblock;
	spec.numChannels = channelnum;
	lowpass.prepare(spec);
	lowpass.setType(dsp::StateVariableTPTFilterType::lowpass);
	lowpass.setCutoffFrequency(20000);
	lowpass.setResonance(1./MathConstants<double>::sqrt2);
	lowpass.reset();

	updatedcurve = 1+2+4+8+16;
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

	bool previson = ison[params.selectedmodulator.get()];

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);

	float* const* channel_data = buffer.getArrayOfWritePointers();

	if(updatedcurve.get() > 0) {
		int uc = updatedcurve.get();
		updatedcurve = 0;
		for(int m = 0; m < MC; ++m) if((uc&(1<<m)) > 0)
			state.curves[m].points = presets[currentpreset].curves[m].points;
	}

	for(int m = 0; m < MC; ++m) {
		float center = 0;
		float min = state.values[m][1];
		float max = state.values[m][2];
		float speed = state.values[m][3];
		float stereo = pow(state.values[m][4],2);
		switch(m) {
			case 0: // DRIFT
				min = 0;
				driftmult = 1.f/fmax(max,0.0001f);
				max = pow(max,2);
				break;
			case 1: // LOW PASS
				center = 1;
				break;
			case 2: // LOW PASS RESONANCE
				center = max;
				break;
			case 3: // SATURATION
				break;
			case 4: // AMPLITUDE
				center = .62996052493f;
				break;
		}

		ison[m] = state.values[m][0] > .5;
		if(!ison[m])
			for(int c = 0; c < channelnum; ++c)
				if(smooth[m*channelnum+c].getCurrentValue() != center)
					ison[m] = true;
		if(!ison[m]) continue;

		float v = 0;
		for(int s = 0; s < numsamples; ++s) {
			time[m] += pow(speed*2,4)*pow(state.masterspeed*2,4)*.00003f;
			for(int c = 0; c < channelnum; ++c) {
				v = (state.curves[m].process(prlin.noise(time[m],((((float)c)/(channelnum-1))-.5)*stereo+m*10)*.5f+.5f,c)*(max-min)+min)*state.values[m][0]+center*(1-state.values[m][0]);
				if(s == 0 && resetsmooth)
					smooth[m*channelnum+c].setCurrentAndTargetValue(v);
				else
					smooth[m*channelnum+c].setTargetValue(v);
				modulator_data[(m*channelnum+c)*samplesperblock+s] = smooth[m*channelnum+c].getNextValue();
			}
		}

		float mono = 0;
		for(int c = 0; c < channelnum; ++c)
			mono += modulator_data[(m*channelnum+c)*samplesperblock];
		if(m == 0) flower_rot[0] = (mono/channelnum)*driftmult;
		else flower_rot[m] = mono/channelnum;
	}
	resetsmooth = false;

	if(previson) for(int c = 0 ; c < fmin(channelnum,2); ++c) {
		float r = modulator_data[(params.selectedmodulator.get()*channelnum+c*(channelnum-1))*samplesperblock];
		if(params.selectedmodulator.get() == 0) r *= driftmult;
		cuber_rot[c] = r;
	}

	//DRIFT
	for(int s = 0; s < numsamples; ++s) {
		driftindex = fmod(driftindex+1,MAX_DRIFT*samplerate);
		for(int c = 0; c < channelnum; ++c) {
			drift_data[c*MAX_DRIFT*samplerate+driftindex] = channel_data[c][s];
			if(ison[0])
				channel_data[c][s] = interpolatesamples(&drift_data[c*MAX_DRIFT*samplerate],driftindex+1+MAX_DRIFT*samplerate*(1-2.f/(samplerate*MAX_DRIFT))*modulator_data[c*samplesperblock+s],MAX_DRIFT*samplerate);
		}
	}

	//LOWPASS
	if(ison[1]) for(int s = 0; s < numsamples; ++s) for(int c = 0; c < channelnum; ++c) {
		lowpass.setCutoffFrequency(calccutoff(modulator_data[(channelnum+c)*samplesperblock+s]));
		lowpass.setResonance(calcresonance(modulator_data[(2*channelnum+c)*samplesperblock+s]));
		channel_data[c][s] = lowpass.processSample(c,channel_data[c][s]);
	}

	//SATURATION
	if(ison[3]) for(int s = 0; s < numsamples; ++s) for(int c = 0; c < channelnum; ++c) {
		float satval = 1-(1-(pow(1-modulator_data[(3*channelnum+c)*samplesperblock+s],10)+(1-pow(modulator_data[(3*channelnum+c)*samplesperblock+s],.2)))*.5)*.99;
		channel_data[c][s] = (1-pow(1-fmin(fabs(channel_data[c][s]),1),1/satval))*(channel_data[c][s]>0?1:-1)*(1-(1-satval)*.92);
	}

	//AMPLITUDE
	if(ison[4]) for(int s = 0; s < numsamples; ++s) for(int c = 0; c < channelnum; ++c) {
		channel_data[c][s] *= 2*pow(modulator_data[(4*channelnum+c)*samplesperblock+s],1.5f);
	}
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
		<< params.selectedmodulator.get() << delimiter;

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
			if(i == 1 && m == 0) continue;
			data << presets[preset_id].values[m][i] << delimiter;
		}
		data << presets[preset_id].curves[m].points.size() << delimiter;
		for(int p = 0; p < presets[preset_id].curves[m].points.size(); ++p)
			data << presets[preset_id].curves[m].points[p].x << delimiter << presets[preset_id].curves[m].points[p].y << delimiter << presets[preset_id].curves[m].points[p].tension << delimiter;
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
				if(i == 1 && m == 0) continue;
				std::getline(ss, token, delimiter);
				presets[preset_id].values[m][i] = std::stof(token);
			}
			presets[preset_id].curves[m].points.clear();
			std::getline(ss, token, delimiter);
			int size = std::stof(token);
			if(size < 2) throw std::invalid_argument("Invalid point data");
			float prevx = 0;
			for(int p = 0; p < size; ++p) {
				std::getline(ss, token, delimiter);
				float x = std::stof(token);
				std::getline(ss, token, delimiter);
				float y = std::stof(token);
				std::getline(ss, token, delimiter);
				float tension = std::stof(token);
				if(x > 1 || x < prevx || y > 1 || y < 0 || tension > 1 || tension < 0 || (p == 0 && x != 0) || (p == (size-1) && x != 1))
					throw std::invalid_argument("Invalid point data");
				prevx = x;
				presets[preset_id].curves[m].points.push_back(point(x,y,tension));
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
			if(i == 1 && m == 0) continue;
			apvts.getParameter("m"+((String)m)+params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[m][i]));
		}
	}
	apvts.getParameter(params.pots[paramcount-1].id)->setValueNotifyingHost(params.pots[paramcount-1].normalize(presets[currentpreset].masterspeed));
	updatedcurve = 1+2+4+8+16;
	updatevis = true;
}

void ModManAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == params.pots[paramcount-1].id) {
		state.masterspeed = newValue;
		presets[currentpreset].masterspeed = newValue;
	}
	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (paramcount-1); i++) {
			if(parameterID == ("m"+((String)m)+params.pots[i].id)) {
				state.values[m][i] = newValue;
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

void ModManAudioProcessor::movepoint(int index, float x, float y) {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i].points[index].x = x;
	presets[currentpreset].curves[i].points[index].y = y;
	updatedcurve = updatedcurve.get()|(1<<i);
}
void ModManAudioProcessor::movetension(int index, float tension) {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i].points[index].tension = tension;
	updatedcurve = updatedcurve.get()|(1<<i);
}
void ModManAudioProcessor::addpoint(int index, float x, float y) {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i].points.insert(presets[currentpreset].curves[i].points.begin()+index,point(x,y,presets[currentpreset].curves[i].points[index-1].tension));
	updatedcurve = updatedcurve.get()|(1<<i);
}
void ModManAudioProcessor::deletepoint(int index) {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i].points.erase(presets[currentpreset].curves[i].points.begin()+index);
	updatedcurve = updatedcurve.get()|(1<<i);
}
const String ModManAudioProcessor::curvetostring(const char delimiter) {
	int i = params.selectedmodulator.get();
	return presets[currentpreset].curves[i].tostring(delimiter);
}
void ModManAudioProcessor::curvefromstring(String str, const char delimiter) {
	int i = params.selectedmodulator.get();
	String revert = presets[currentpreset].curves[i].tostring();
	try {
		presets[currentpreset].curves[i] = curve(str,delimiter);
	} catch(...) {
		presets[currentpreset].curves[i] = curve(revert);
	}
	updatevis = true;
	updatedcurve = updatedcurve.get()|(1<<i);
}
void ModManAudioProcessor::resetcurve() {
	int i = params.selectedmodulator.get();
	presets[currentpreset].curves[i] = curve("2,0,0,0.5,1,1,0.5"); //TODO
	updatevis = true;
	updatedcurve = updatedcurve.get()|(1<<i);
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
		} //TODO defaults
		parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"m"+((String)m)+"on"		,1},name+" on"															 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool			).withValueFromStringFunction(frombool			)));
		if(m == 0) {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" range"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),1.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(toms			).withValueFromStringFunction(fromms			)));
		} else if(m == 1) {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"min"		,1},name+" min range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tocutoff		).withValueFromStringFunction(fromcutoff		)));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" max range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	,AudioParameterFloatAttributes().withStringFromValueFunction(tocutoff		).withValueFromStringFunction(fromcutoff		)));
		} else if(m == 2) {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"min"		,1},name+" min range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(toresonance	).withValueFromStringFunction(fromresonance		)));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" max range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	,AudioParameterFloatAttributes().withStringFromValueFunction(toresonance	).withValueFromStringFunction(fromresonance		)));
		} else {
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"min"		,1},name+" min range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"max"		,1},name+" max range"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
		}
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"speed"	,1},name+" speed"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"m"+((String)m)+"stereo"	,1},name+" stereo"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.3f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
	}
		parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"masterspeed"				,1},"master speed"		,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized	)));
	return { parameters.begin(), parameters.end() };
}
