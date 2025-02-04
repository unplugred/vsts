#include "processor.h"
#include "editor.h"

TripleDAudioProcessor::TripleDAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	presets[0] = pluginpreset("Default",0.6f,0.66f);
	presets[0].values[2] = 0.187f;
	presets[0].values[3] = 0.556f;
	presets[0].values[4] = 0.447f;
	for(int i = 8; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "Program " + (String)(i-7);
	}

	params.pots[0] = potentiometer("Dry/Wet"	,"wet"		,.002f	,presets[0].values[0]	);
	params.pots[1] = potentiometer("Feedback"	,"feedback"	,.002f	,presets[0].values[1]	);
	for(int d = 0; d < DLINES; ++d)
		params.pots[2+d] = potentiometer("Delay "+((String)(d+1)),"delay"+((String)(d+1)),.001f,presets[0].values[2+d]);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		presets[currentpreset].values[i] = state.values[i];
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		add_listener(params.pots[i].id);
	}
	add_listener("randomize");
}

TripleDAudioProcessor::~TripleDAudioProcessor(){
	close();
}

const String TripleDAudioProcessor::getName() const { return "Triple D"; }
bool TripleDAudioProcessor::acceptsMidi() const { return false; }
bool TripleDAudioProcessor::producesMidi() const { return false; }
bool TripleDAudioProcessor::isMidiEffect() const { return false; }
double TripleDAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int TripleDAudioProcessor::getNumPrograms() { return 20; }
int TripleDAudioProcessor::getCurrentProgram() { return currentpreset; }
void TripleDAudioProcessor::setCurrentProgram(int index) {
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
void TripleDAudioProcessor::timerCallback() {
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
const String TripleDAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void TripleDAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void TripleDAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void TripleDAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void TripleDAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
		params.pots[i].smooth.reset(samplerate,params.pots[i].smoothtime);

	delaybuffersize = samplerate*MAX_DLY;
	delaybuffer.setSize(channelnum,delaybuffersize,false,false,false);
	delaybuffer.clear();
	readpos = readpos%delaybuffersize;
	delayamt.reset(0,1.,-1,samplerate,DLINES);
	resetdampenings = true;
	dcfilter.init(samplerate,channelnum);
	for(int i = 0; i < channelnum; i++) dcfilter.reset(i);

	preparedtoplay = true;
}
void TripleDAudioProcessor::releaseResources() { }

bool TripleDAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void TripleDAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	float prmsadd = rmsadd.get();
	int prmscount = rmscount.get();

	int numsamples = buffer.getNumSamples();

	float* const* channeldata = buffer.getArrayOfWritePointers();
	float* const* delaydata = delaybuffer.getArrayOfWritePointers();
	for(int sample = 0; sample < numsamples; ++sample) {
		for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0)
			state.values[i] = params.pots[i].smooth.getNextValue();
		float feedbackval = 1-powf(1-state.values[1],.3f);

		for(size_t d = 0; d < DLINES; ++d) {
			if(resetdampenings && sample == 0) delayamt.v_current[d] = state.values[2+d];
			dindex[d] = ((readpos-1)-samplerate*(delayamt.nextvalue(state.values[2+d],d)*(MAX_DLY-MIN_DLY)+MIN_DLY))+delaybuffersize*2;
		}

		for(int channel = 0; channel < channelnum; ++channel) {
			float feedbackout = 0;
			float pannedout = 0;
			for(size_t d = 0; d < DLINES; ++d) {
				float pan = ((float)d)/(DLINES-1);
				pan = (((float)channel)/(channelnum-1))*(pan*2-1)+(1-pan);
				float delout = interpolatesamples(delaydata[channel],dindex[d],delaybuffersize);
				feedbackout += delout;
				pannedout += delout*pan;
			}

			delaydata[channel][readpos] = fmax(-1.f,fmin(1.f,dcfilter.process((feedbackval*feedbackout)+channeldata[channel][sample],channel)));
			channeldata[channel][sample] = state.values[0]*pannedout+(1-state.values[0])*channeldata[channel][sample];

			if(prmscount < samplerate*2) {
				prmsadd += channeldata[channel][sample]*channeldata[channel][sample];
				prmscount++;
			}
		}
		readpos = fmod(readpos+1,delaybuffersize);
	}
	resetdampenings = false;

	rmsadd = prmsadd;
	rmscount = prmscount;
}
float TripleDAudioProcessor::interpolatesamples(float* buffer, float position, int buffersize) {
	return buffer[((int)floor(position))%buffersize]*(1-fmod(position,1.f))+buffer[((int)ceil(position))%buffersize]*fmod(position,1.f);
}

bool TripleDAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* TripleDAudioProcessor::createEditor() {
	return new TripleDAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void TripleDAudioProcessor::getStateInformation(MemoryBlock& destData) {
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
void TripleDAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
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
const String TripleDAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;
	for(int v = 0; v < paramcount; v++)
		data << presets[preset_id].values[v] << delimiter;

	return data.str();
}
void TripleDAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
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

void TripleDAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "randomize" && newValue>.5) {
		randomize();
		return;
	}
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		if(lerpstage < .001 || lerpchanged[i]) presets[currentpreset].values[i] = newValue;
		return;
	}
}
void TripleDAudioProcessor::randomize() {
	if(!preparedtoplay) return;
	for(int d = 0; d < DLINES; ++d)
		apvts.getParameter("delay"+((String)(d+1)))->setValueNotifyingHost(random.nextFloat());
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TripleDAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout TripleDAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"wet"			,1},"Dry/Wet"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.6f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"feedback"	,1},"Feedback"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.66f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized	).withValueFromStringFunction(fromnormalized)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"randomize"	,1},"Randomize"													 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool			).withValueFromStringFunction(frombool		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"delay1"		,1},"Delay 1"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.187f,AudioParameterFloatAttributes().withStringFromValueFunction(tolength		).withValueFromStringFunction(fromlength	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"delay2"		,1},"Delay 2"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.556f,AudioParameterFloatAttributes().withStringFromValueFunction(tolength		).withValueFromStringFunction(fromlength	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"delay3"		,1},"Delay 3"	,juce::NormalisableRange<float>( 0.0f	,1.0f	),0.447f,AudioParameterFloatAttributes().withStringFromValueFunction(tolength		).withValueFromStringFunction(fromlength	)));
	return { parameters.begin(), parameters.end() };
}
