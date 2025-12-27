#include "processor.h"
#include "editor.h"

FMPlusAudioProcessor::FMPlusAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	for(int i = 0; i < 128; ++i)
		pitches[i] = (440.f/32.f)*pow(2.f,((i-9.f)/12.f));

	presets[0] = pluginpreset("Init"); // TODO stringify default preset
	for(int i = 0; i < generalcount; ++i)
		presets[0].general[i] = 0;
	for(int i = 0; i < paramcount; ++i) for(int o = 0; o < MC; ++o)
		presets[0].values[o][i] = 0;
	for(int i = 0; i < (MC+1); ++i) {
		presets[0].oppos[i*2  ] = 56;
		presets[0].oppos[i*2+1] = 180-i*20;
	}

	for(int i = 1; i < getNumPrograms(); ++i) {
		presets[i] = presets[0];
		presets[i].name = "Program " + (String)i;
	}

	params.general[ 0] = potentiometer("Voices"						,"voices"			,0		,presets[0].general  [ 0]	,1	,24	,potentiometer::inttype		);
	params.general[ 1] = potentiometer("Portamento"					,"portamento"		,.001f	,presets[0].general  [ 1]	);
	params.general[ 2] = potentiometer("Legato"						,"legato"			,0		,presets[0].general  [ 2]	,0	,1	,potentiometer::booltype	);
	params.general[ 3] = potentiometer("LFO Sync"					,"lfosync"			,0		,presets[0].general  [ 4]	,0	,3	,potentiometer::inttype		);
	params.general[ 4] = potentiometer("Pitch Bend"					,"pitchbend"		,.001f	,presets[0].general  [ 3]	,0	,24	,potentiometer::inttype		);
	params.general[ 5] = potentiometer("Arp On"						,"arpon"			,0		,presets[0].general  [ 5]	,0	,1	,potentiometer::booltype	);
	params.general[ 6] = potentiometer("Arp Direction"				,"arpdirection"		,0		,presets[0].general  [ 6]	,0	,5	,potentiometer::inttype		);
	params.general[ 7] = potentiometer("Arp Length"					,"arplength"		,0		,presets[0].general  [ 7]	);
	params.general[ 8] = potentiometer("Arp Speed"					,"arpspeed"			,0		,presets[0].general  [ 8]	);
	params.general[ 9] = potentiometer("Arp Speed (BPM sync)"		,"arpbpm"			,0		,presets[0].general  [ 9]	,0	,10	,potentiometer::inttype		);
	params.general[10] = potentiometer("Vibrato On"					,"vibratoon"		,.001f	,presets[0].general  [10]	,0	,1	,potentiometer::booltype	);
	params.general[11] = potentiometer("Vibrato Rate"				,"vibratorate"		,.001f	,presets[0].general  [11]	);
	params.general[12] = potentiometer("Vibrato Rate (BPM sync)"	,"vibratobpm"		,.001f	,presets[0].general  [12]	,0	,10	,potentiometer::inttype		);
	params.general[13] = potentiometer("Vibrato Amount"				,"vibratoamount"	,.001f	,presets[0].general  [13]	);
	params.general[14] = potentiometer("Vibrato Attack"				,"vibratoattack"	,0		,presets[0].general  [14]	);
	// TODO per op defaults
	params.values [ 0] = potentiometer("On"							,"on"		 		,0		,presets[0].values[0][ 0]	,0	,1	,potentiometer::booltype	);
	params.values [ 1] = potentiometer("Pan"						,"pan"				,0		,presets[0].values[0][ 1]	);
	params.values [ 2] = potentiometer("Amplitude"					,"amp"				,0		,presets[0].values[0][ 2]	);
	params.values [ 3] = potentiometer("Tone"						,"tone"				,.001f	,presets[0].values[0][ 3]	);
	params.values [ 4] = potentiometer("Velocity"					,"velocity"			,0		,presets[0].values[0][ 4]	);
	params.values [ 5] = potentiometer("Mod W/Aft.T"				,"modat"			,.001f	,presets[0].values[0][ 5]	);
	params.values [ 6] = potentiometer("Frequency Mode"				,"freqmode"			,0		,presets[0].values[0][ 6]	,0	,1	,potentiometer::booltype	);
	params.values [ 7] = potentiometer("Frequency Multiplier"		,"freqmult"			,.001f	,presets[0].values[0][ 7]	,0	,24	);
	params.values [ 8] = potentiometer("Frequency Offset"			,"freqadd"			,.001f	,presets[0].values[0][ 8]	);
	params.values [ 9] = potentiometer("Attack"						,"attack"			,.001f	,presets[0].values[0][ 9]	);
	params.values [10] = potentiometer("Decay"						,"decay"			,.001f	,presets[0].values[0][10]	);
	params.values [11] = potentiometer("Sustain"					,"sustain"			,.001f	,presets[0].values[0][11]	);
	params.values [12] = potentiometer("Release"					,"release"			,.001f	,presets[0].values[0][12]	);
	params.values [13] = potentiometer("LFO On"						,"lfoon"			,.001f	,presets[0].values[0][13]	,0	,1	,potentiometer::booltype	);
	params.values [14] = potentiometer("LFO Target"					,"lfotarget"		,0		,presets[0].values[0][14]	,0	,3	,potentiometer::inttype		);
	params.values [15] = potentiometer("LFO Rate"					,"lforate"			,.001f	,presets[0].values[0][15]	);
	params.values [16] = potentiometer("LFO Rate (BPM sync)"		,"lfobpm"			,.001f	,presets[0].values[0][16]	,0	,12	,potentiometer::inttype		);
	params.values [17] = potentiometer("LFO Amount"					,"lfoamount"		,.001f	,presets[0].values[0][17]	);
	params.values [18] = potentiometer("LFO Attack"					,"lfoattack"		,.001f	,presets[0].values[0][18]	);
	// TODO FX

	for(int i = 0; i < generalcount; ++i) {
		state.general[i] = params.general[i].inflate(apvts.getParameter(params.general[i].id)->getValue());
		presets[currentpreset].general[i] = state.general[i];
		if(params.general[i].smoothtime > 0) params.general[i].smooth[0].setCurrentAndTargetValue(state.general[i]);
		add_listener(params.general[i].id);
	}
	for(int i = 0; i < paramcount; ++i) {
		for(int o = 0; o < MC; ++o) {
			state.values[o][i] = params.values[i].inflate(apvts.getParameter("o"+(String)o+params.values[i].id)->getValue());
			presets[currentpreset].values[o][i] = state.values[o][i];
			if(params.values[i].smoothtime > 0) params.values[i].smooth[o].setCurrentAndTargetValue(state.values[o][i]);
			add_listener("o"+(String)o+params.values[i].id);
		}
	}
	params.antialiasing = apvts.getParameter("antialias")->getValue();
	add_listener("antialias");

	updatedcurve = 1+2+4+8+16+32+64+128;
	updatevis = true;
}

FMPlusAudioProcessor::~FMPlusAudioProcessor(){
	close();
}

const String FMPlusAudioProcessor::getName() const { return "FM+"; }
bool FMPlusAudioProcessor::acceptsMidi() const { return false; }
bool FMPlusAudioProcessor::producesMidi() const { return false; }
bool FMPlusAudioProcessor::isMidiEffect() const { return false; }
double FMPlusAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int FMPlusAudioProcessor::getNumPrograms() { return 20; }
int FMPlusAudioProcessor::getCurrentProgram() { return currentpreset; }
void FMPlusAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;

	for(int i = 0; i < generalcount; ++i)
		apvts.getParameter(params.general[i].id)->setValueNotifyingHost(params.general[i].normalize(presets[currentpreset].general[i]));
	for(int i = 0; i < paramcount; ++i) for(int o = 0; o < MC; ++o)
		apvts.getParameter("o"+(String)o+params.values[i].id)->setValueNotifyingHost(params.values[i].normalize(presets[currentpreset].values[o][i]));

	updatedcurve = 1+2+4+8+16+32+64+128;
	updatevis = true;
}
const String FMPlusAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void FMPlusAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void FMPlusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	if(!saved && sampleRate > 60000) {
		params.antialiasing = .5f;
		apvts.getParameter("antialias")->setValueNotifyingHost(.5f);
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

	ospointerarray.resize(channelnum);
	for(int i = 0; i < 3; ++i) {
		os[i].reset(new dsp::Oversampling<float>(channelnum,i+1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
		os[i]->initProcessing(samplesperblock);
		os[i]->setUsingIntegerLatency(true);
	}
	preparedtoplay = true;
	setoversampling();

	for(int o = 0; o < MC; ++o)
		state.curves[o].resizechannels(channelnum);

	updatedcurve = 1+2+4+8+16+32+64+128;
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

	if(updatedcurve.get() > 0) {
		int uc = updatedcurve.get();
		updatedcurve = 0;
		for(int o = 0; o < MC; ++o) if((uc&(1<<o)) > 0)
			state.curves[o].points = presets[currentpreset].curves[o].points;
	}

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);
	if(osindex > 0) {
		dsp::AudioBlock<float> osblock = os[osindex-1]->processSamplesUp(block);
		for(int i = 0; i < channelnum; ++i)
			ospointerarray[i] = osblock.getChannelPointer(i);
		osbuffer = AudioBuffer<float>(ospointerarray.data(), channelnum, static_cast<int>(osblock.getNumSamples()));
		numsamples = osbuffer.getNumSamples();
	}

	float* const* channelData;
	if(osindex > 0) channelData = osbuffer.getArrayOfWritePointers();
	else channelData = buffer.getArrayOfWritePointers();

	for(int sample = 0; sample < numsamples; ++sample) {
		for(int i = 0; i < generalcount; ++i) if(params.general[i].smoothtime > 0)
			state.general[i] = params.general[i].smooth[0].getNextValue();
		for(int i = 0; i < paramcount; ++i) if(params.values[i].smoothtime > 0) for(int o = 0; o < MC; ++o)
			state.values[o][i] = params.values[i].smooth[o].getNextValue();

		for(int channel = 0; channel < channelnum; ++channel) {
			//channelData[channel][sample] = ???; TODO processing
			if(prmscount < samplerate*2) {
				prmsadd += channelData[channel][sample]*channelData[channel][sample];
				prmscount++;
			}
		}
	}

	if(osindex > 0) os[osindex-1]->processSamplesDown(block);

	rmsadd = prmsadd;
	rmscount = prmscount;
}

void FMPlusAudioProcessor::setoversampling() {
	if(!preparedtoplay) return;
	osindex = fmin(3,fmax(0,floor(params.antialiasing*8-4)));
	if(osindex > 0) {
		if(channelnum <= 0) return;
		os[osindex-1]->reset();
		setLatencySamples(os[osindex-1]->getLatencyInSamples());
	} else {
		setLatencySamples(0);
	}
	int oversampleamount = pow(2,osindex);
	for(int i = 0; i < paramcount; ++i) if(params.general[i].smoothtime > 0)
		params.general[i].smooth[0].reset(samplerate*oversampleamount,params.general[i].smoothtime);
	for(int i = 0; i < paramcount; ++i) if(params.values[i].smoothtime > 0) for(int o = 0; o < MC; ++o)
		params.values[i].smooth[o].reset(samplerate*oversampleamount,params.values[i].smoothtime);
}

bool FMPlusAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* FMPlusAudioProcessor::createEditor() {
	return new FMPlusAudioProcessorEditor(*this,generalcount,paramcount,presets[currentpreset],params);
}

void FMPlusAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter
		<< (params.presetunsaved?1:0) << delimiter
		<< params.antialiasing << delimiter
		<< params.selectedtab.get() << delimiter
		<< params.tuningfile << delimiter;

	for(int i = 0; i < 128; ++i)
		data << pitches[i] << delimiter;

	for(int i = 0; i < getNumPrograms(); ++i)
		data << get_preset(i) << delimiter;

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void FMPlusAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	//*/
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
		params.presetunsaved = std::stoi(token) > .5;

		std::getline(ss, token, delimiter);
		params.antialiasing = std::stof(token);

		std::getline(ss, token, delimiter);
		params.selectedtab = std::stoi(token);

		std::getline(ss, token, delimiter);
		params.tuningfile = (String)token;

		for(int i = 0; i < 128; ++i) {
			std::getline(ss, token, delimiter);
			pitches[i] = std::stof(token);
		}

		for(int i = 0; i < getNumPrograms(); ++i) {
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

	apvts.getParameter("antialias")->setValueNotifyingHost(params.antialiasing);
	//*/
}
const String FMPlusAudioProcessor::get_preset(int preset_id, const char delimiter) {

	std::ostringstream data;

	data << version << delimiter
		<< presets[preset_id].name.replace(",","ñ") << delimiter;

	for(int i = 0; i < generalcount; i++)
		data << presets[preset_id].general[i] << delimiter;
	for(int i = 0; i < paramcount; i++) for(int o = 0; o < MC; ++o)
		data << presets[preset_id].values[o][i] << delimiter;
	for(int o = 0; o < MC; ++o) {
		data << presets[preset_id].curves[o].points.size() << delimiter;
		for(int p = 0; p < presets[preset_id].curves[o].points.size(); ++p)
			data << presets[preset_id].curves[o].points[p].x << delimiter << presets[preset_id].curves[o].points[p].y << delimiter << presets[preset_id].curves[o].points[p].tension << delimiter;
	}
	for(int i = 0; i < (MC+1)*2; ++i)
		data << presets[preset_id].oppos[i] << delimiter;
	for(int o = 0; o < (MC+1); ++o) {
		data << presets[preset_id].opconnections[o].size() << delimiter;
		for(int i = 0; i < presets[preset_id].opconnections[o].size(); ++i) {
			data << presets[preset_id].opconnections[o][i].input << delimiter
				<< presets[preset_id].opconnections[o][i].influence << delimiter;
		}
	}

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

		std::getline(ss, token, delimiter);
		presets[preset_id].name = ((String)token).replace("ñ",",");

		for(int i = 0; i < generalcount; i++) {
			std::getline(ss, token, delimiter);
			presets[preset_id].general[i] = std::stof(token);
		}
		for(int i = 0; i < paramcount; i++) for(int o = 0; o < MC; ++o) {
			std::getline(ss, token, delimiter);
			presets[preset_id].values[o][i] = std::stof(token);
		}

		for(int o = 0; o < MC; ++o) {
			std::getline(ss, token, delimiter);
			int size = std::stof(token);
			if(size < 2) throw std::invalid_argument("Invalid point data");
			presets[preset_id].curves[o].points.clear();
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
				presets[preset_id].curves[o].points.push_back(point(x,y,tension));
			}
		}

		for(int i = 0; i < (MC+1)*2; ++i) {
			std::getline(ss, token, delimiter);
			presets[preset_id].oppos[i] = std::stoi(token);
		}
		for(int o = 0; o < (MC+1); ++o) {
			std::getline(ss, token, delimiter);
			int size = std::stoi(token);
			for(int i = 0; i < size; ++i) {
				std::getline(ss, token, delimiter);
				int input = std::stoi(token);
				std::getline(ss, token, delimiter);
				float influence = std::stof(token);
				presets[preset_id].opconnections[o].push_back(connection(input,o,influence));
			}
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

	for(int i = 0; i < generalcount; ++i)
		apvts.getParameter(params.general[i].id)->setValueNotifyingHost(params.general[i].normalize(presets[currentpreset].general[i]));
	for(int i = 0; i < paramcount; ++i) for(int o = 0; o < MC; ++o)
		apvts.getParameter("o"+(String)o+params.values[i].id)->setValueNotifyingHost(params.values[i].normalize(presets[currentpreset].values[o][i]));
	updatedcurve = 1+2+4+8+16+32+64+128;
	updatevis = true;
}

void FMPlusAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "antialias") {
		params.antialiasing = newValue;
		setoversampling();
		return;
	}
	for(int i = 0; i < generalcount; ++i) if(parameterID == params.general[i].id) {
		if(params.general[i].smoothtime > 0) params.general[i].smooth[0].setTargetValue(newValue);
		else state.general[i] = newValue;
		presets[currentpreset].general[i] = newValue;

		params.presetunsaved = true;
		return;
	}
	for(int i = 0; i < paramcount; ++i) for(int o = 0; o < MC; ++o) if(parameterID == ("o"+(String)o+params.values[i].id)) {
		if(params.values[i].smoothtime > 0) params.values[i].smooth[o].setTargetValue(newValue);
		else state.values[o][i] = newValue;
		presets[currentpreset].values[o][i] = newValue;

		params.presetunsaved = true;
		return;
	}
}

void FMPlusAudioProcessor::movepoint(int index, float x, float y) {
	int i = params.selectedtab.get();
	presets[currentpreset].curves[i].points[index].x = x;
	presets[currentpreset].curves[i].points[index].y = y;
	updatedcurve = updatedcurve.get()|(1<<i);
}
void FMPlusAudioProcessor::movetension(int index, float tension) {
	int i = params.selectedtab.get();
	presets[currentpreset].curves[i].points[index].tension = tension;
	updatedcurve = updatedcurve.get()|(1<<i);
}
void FMPlusAudioProcessor::addpoint(int index, float x, float y) {
	int i = params.selectedtab.get();
	presets[currentpreset].curves[i].points.insert(presets[currentpreset].curves[i].points.begin()+index,point(x,y,presets[currentpreset].curves[i].points[index-1].tension));
	updatedcurve = updatedcurve.get()|(1<<i);
}
void FMPlusAudioProcessor::deletepoint(int index) {
	int i = params.selectedtab.get();
	presets[currentpreset].curves[i].points.erase(presets[currentpreset].curves[i].points.begin()+index);
	updatedcurve = updatedcurve.get()|(1<<i);
}
const String FMPlusAudioProcessor::curvetostring(const char delimiter) {
	int i = params.selectedtab.get();
	return presets[currentpreset].curves[i].tostring(delimiter);
}
void FMPlusAudioProcessor::curvefromstring(String str, const char delimiter) {
	int i = params.selectedtab.get();
	String revert = presets[currentpreset].curves[i].tostring();
	try {
		presets[currentpreset].curves[i] = curve(str,delimiter);
	} catch(...) {
		presets[currentpreset].curves[i] = curve(revert);
	}
	updatevis = true;
	updatedcurve = updatedcurve.get()|(1<<i);
}
void FMPlusAudioProcessor::resetcurve() {
	int i = params.selectedtab.get();
	presets[currentpreset].curves[i] = curve("3,0,0,0.5,0.5,1,0.5,1,0,0.5");
	updatevis = true;
	updatedcurve = updatedcurve.get()|(1<<i);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FMPlusAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout FMPlusAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"voices"					,1},"Voices"																	 ,1		,24		 ,12	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"portamento"				,1},"Portamento"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(toportamento		).withValueFromStringFunction(fromportamento	)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"legato"					,1},"Legato"																	 				 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"lfosync"					,1},"LFO Sync"																	 ,0		,3		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(tolfosync			).withValueFromStringFunction(fromlfosync		))); // NOTE, RANDOM, FREE, TRIGGER
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"pitchbend"				,1},"Pitch Bend"																 ,0		,24		 ,2		,AudioParameterIntAttributes()	.withStringFromValueFunction(topitchbend		).withValueFromStringFunction(frompitchbend		)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"arpon"					,1},"Arp On"																	 				 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"arpdirection"			,1},"Arp Direction"																 ,0		,5		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(toarpdirection		).withValueFromStringFunction(fromarpdirection	))); // SEQUENCE, UP, DOWN, U&D, D&U, RANDOM
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"arpspeed"				,1},"Arp Speed"									,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(toarpspeed			).withValueFromStringFunction(fromarpspeed		)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"arpbpm"					,1},"Arp Speed (BPM sync)"														 ,0		,10		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(toarpbpm			).withValueFromStringFunction(fromarpbpm		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"arplength"				,1},"Arp Length"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),1.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(toarplength		).withValueFromStringFunction(fromarplength		)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"vibratoon"				,1},"Vibrato On"																 				 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"vibratorate"				,1},"Vibrato Rate"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tovibratorate		).withValueFromStringFunction(fromvibratorate	)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"vibratobpm"				,1},"Vibrato Rate (BPM sync)"													 ,0		,10		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(tovibratobpm		).withValueFromStringFunction(fromvibratobpm	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"vibratoamount"			,1},"Vibrato Amount"							,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tovibratoamount	).withValueFromStringFunction(fromvibratoamount	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"vibratoattack"			,1},"Vibrato Attack"							,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tovibratoattack	).withValueFromStringFunction(fromvibratoattack	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"antialias"				,1},"Anti-Alias"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.7f	,AudioParameterFloatAttributes().withStringFromValueFunction(toantialias		).withValueFromStringFunction(fromantialias		)));
	for(int o = 0; o < MC; ++o) {
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"o"+(String)o+"on"		,1},"OP"+(String)(o+1)+" On"													 				 ,o<=1	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"pan"		,1},"OP"+(String)(o+1)+" Pan"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(topan				).withValueFromStringFunction(frompan			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"amp"		,1},"OP"+(String)(o+1)+" Amplitude"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(toamp				).withValueFromStringFunction(fromamp			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"tone"		,1},"OP"+(String)(o+1)+" Tone"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized		).withValueFromStringFunction(fromnormalized	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"velocity"	,1},"OP"+(String)(o+1)+" Velocity"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tovelocity			).withValueFromStringFunction(fromvelocity		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"modat",1},"OP"+(String)(o+1)+" Mod Wheel / After-Touch"	,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tomodat			).withValueFromStringFunction(frommodat			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"o"+(String)o+"freqmode"	,1},"OP"+(String)(o+1)+" Frequency Mode"														 ,true	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tofreqmode			).withValueFromStringFunction(fromfreqmode		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"freqmult"	,1},"OP"+(String)(o+1)+" Frequency Multiplier"	,juce::NormalisableRange<float	>(0.0f	,24.0f	),1.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tofreqmult			).withValueFromStringFunction(fromfreqmult		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"freqadd"	,1},"OP"+(String)(o+1)+" Frequency Offset"		,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tofreqadd			).withValueFromStringFunction(fromfreqadd		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"attack"	,1},"OP"+(String)(o+1)+" Attack"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(toattack			).withValueFromStringFunction(fromattack		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"decay"		,1},"OP"+(String)(o+1)+" Decay"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(todecay			).withValueFromStringFunction(fromdecay			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"sustain"	,1},"OP"+(String)(o+1)+" Sustain"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),1.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized		).withValueFromStringFunction(fromnormalized	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"release"	,1},"OP"+(String)(o+1)+" Release"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(torelease			).withValueFromStringFunction(fromrelease		)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"o"+(String)o+"lfoon"		,1},"OP"+(String)(o+1)+" LFO On"												 				 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"o"+(String)o+"lfotarget"	,1},"OP"+(String)(o+1)+" LFO Target"											 ,0		,3		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(tolfotarget		).withValueFromStringFunction(fromlfotarget		))); // AMPLITUDE, PITCH, PAN, TONE
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"lforate"	,1},"OP"+(String)(o+1)+" LFO Rate"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tolforate			).withValueFromStringFunction(fromlforate		)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"o"+(String)o+"lfobpm"	,1},"OP"+(String)(o+1)+" LFO Rate (BPM sync)"									 ,0		,12		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(tolfobpm			).withValueFromStringFunction(fromlfobpm		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"lfoamount"	,1},"OP"+(String)(o+1)+" LFO Amount"			,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tolfoamount		).withValueFromStringFunction(fromlfoamount		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"lfoattack"	,1},"OP"+(String)(o+1)+" LFO Attack"			,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tolfoattack		).withValueFromStringFunction(fromlfoattack		)));
	}
	// TODO FX
	return { parameters.begin(), parameters.end() };
}
