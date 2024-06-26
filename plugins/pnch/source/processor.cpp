#include "processor.h"
#include "editor.h"

PNCHAudioProcessor::PNCHAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	amount.setCurrentAndTargetValue(apvts.getParameter("amount")->getValue());
	oversampling = apvts.getParameter("oversampling")->getValue();
	add_listener("amount");
	add_listener("oversampling");
}

PNCHAudioProcessor::~PNCHAudioProcessor(){
	close();
}

const String PNCHAudioProcessor::getName() const { return "PNCH"; }
bool PNCHAudioProcessor::acceptsMidi() const { return false; }
bool PNCHAudioProcessor::producesMidi() const { return false; }
bool PNCHAudioProcessor::isMidiEffect() const { return false; }
double PNCHAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PNCHAudioProcessor::getNumPrograms() { return 31; }
int PNCHAudioProcessor::getCurrentProgram() {
	return (int)floor(amount.getTargetValue()*31);
}
void PNCHAudioProcessor::setCurrentProgram(int index) {
	apvts.getParameter("amount")->setValueNotifyingHost(index/30.f);
}
const String PNCHAudioProcessor::getProgramName(int index) {
	std::ostringstream presetname;
	presetname << "P";
	for(int i = 0; i < index; i++) presetname << "U";
	presetname << "NCH";
	if(index == 0) presetname << ".";
	return { presetname.str() };
}
void PNCHAudioProcessor::changeProgramName(int index, const String& newName) { }

void PNCHAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	if(!saved && sampleRate > 60000) {
		oversampling = false;
		apvts.getParameter("oversampling")->setValueNotifyingHost(0);
	}
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void PNCHAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void PNCHAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	amount.reset(samplerate*(oversampling.get()?2:1),.001f);

	preparedtoplay = true;
	ospointerarray.resize(channelnum);
	os.reset(new dsp::Oversampling<float>(channelnum,1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
	os->initProcessing(samplesperblock);
	os->setUsingIntegerLatency(true);
	setoversampling(oversampling.get());
}
void PNCHAudioProcessor::releaseResources() { }

bool PNCHAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void PNCHAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;
	
	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	saved = true;

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	float prmsadd = rmsadd.get();
	int prmscount = rmscount.get();
	bool isoversampling = oversampling.get();

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);
	if(isoversampling) {
		dsp::AudioBlock<float> osblock = os->processSamplesUp(block);
		for(int i = 0; i < channelnum; i++)
			ospointerarray[i] = osblock.getChannelPointer(i);
		osbuffer = AudioBuffer<float>(ospointerarray.data(), channelnum, static_cast<int>(osblock.getNumSamples()));
		numsamples = osbuffer.getNumSamples();
	}

	float* const* channelData;
	if(isoversampling) channelData = osbuffer.getArrayOfWritePointers();
	else channelData = buffer.getArrayOfWritePointers();

	for(int sample = 0; sample < numsamples; ++sample) {
		for(int channel = 0; channel < channelnum; ++channel) {
			channelData[channel][sample] = pnch(channelData[channel][sample],amount.getNextValue());
			if(prmscount < samplerate*2) {
				prmsadd += fmin(channelData[channel][sample]*channelData[channel][sample],10);
				prmscount++;
			}
		}
	}

	if(isoversampling) os->processSamplesDown(block);

	rmsadd = prmsadd;
	rmscount = prmscount;
}

float PNCHAudioProcessor::pnch(float source, float amount) {
	if(source == 0 || amount >= 1) return 0;
	if(amount <= 0) return source;

	/*
	float f = source;

	if(q >= 1) f = 0;
	else if(q > 0) {
		if(f > 0)
			f = fmax(source-q,0);
		else
			f = fmin(source+q,0);
		f /= (1-q);
	}

	float mult = 1;
	if(ease >= 1)
		mult = fabs(f);
	else if(ease > 0)
		mult = 1-powf(1-fabs(f),1./ease);
	f *= powf(mult,iterations*5);

	return fmax(fmin(f*wet+source*(1-wet),1),-1);
	*/

	double cropmount = pow((double)amount,10);
	double sinkedsource = ((double)source-fmin(fmax((double)source,-cropmount),cropmount))/(1-cropmount);
	if(sinkedsource >= 1 || sinkedsource <= -1) return sinkedsource;
	return sinkedsource*pow(1-pow(1-abs(sinkedsource),12.5),(3/(1-amount*.98))-3);
}

void PNCHAudioProcessor::setoversampling(bool toggle) {
	if(!preparedtoplay) return;
	if(toggle) {
		if(channelnum <= 0) return;
		os->reset();
		setLatencySamples(os->getLatencyInSamples());
	} else setLatencySamples(0);
}

bool PNCHAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* PNCHAudioProcessor::createEditor() {
	return new PNCHAudioProcessorEditor(*this,amount.getTargetValue());
}

void PNCHAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;
	data << version
		<< delimiter << amount.getTargetValue()
		<< delimiter << (oversampling.get()?1:0) << delimiter;
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PNCHAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	saved = true;
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		float a = std::stof(token);
		if(saveversion <= 1) a = 1-(3/(3+(5*a)));
		apvts.getParameter("amount")->setValueNotifyingHost(a);

		std::getline(ss, token, delimiter);
		apvts.getParameter("oversampling")->setValueNotifyingHost(std::stof(token));

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
const String PNCHAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version
		<< delimiter << amount.getTargetValue()
		<< delimiter << (oversampling.get()?1:0) << delimiter;

	return data.str();
}
void PNCHAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(0);
	try {
		std::stringstream ss(preset.trim().toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int save_version = std::stoi(token);

		std::getline(ss, token, delimiter);
		apvts.getParameter("amount")->setValueNotifyingHost(std::stof(token));

		std::getline(ss, token, delimiter);
		apvts.getParameter("oversampling")->setValueNotifyingHost(std::stof(token));

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
		set_preset(revert, 0);
		return;
	}
}

void PNCHAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "amount") amount.setTargetValue(newValue);
	else if(parameterID == "oversampling") {
		oversampling = newValue>.5;
		setoversampling(newValue>.5);
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PNCHAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout PNCHAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"amount"		,1},"Amount"		,juce::NormalisableRange<float>(0.0f,1.0f)	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"oversampling",1},"Over-Sampling"												,true	));
	return { parameters.begin(), parameters.end() };
}
