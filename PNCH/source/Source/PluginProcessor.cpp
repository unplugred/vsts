/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

PNCHAudioProcessor::PNCHAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	apvts.addParameterListener("amount",this);
	apvts.addParameterListener("oversampling",this);
}

PNCHAudioProcessor::~PNCHAudioProcessor(){
	apvts.removeParameterListener("amount",this);
	apvts.removeParameterListener("oversampling",this);

}

const String PNCHAudioProcessor::getName() const { return "PNCH"; }
bool PNCHAudioProcessor::acceptsMidi() const { return false; }
bool PNCHAudioProcessor::producesMidi() const { return false; }
bool PNCHAudioProcessor::isMidiEffect() const { return false; }
double PNCHAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PNCHAudioProcessor::getNumPrograms() { return 1; }
int PNCHAudioProcessor::getCurrentProgram() { return 0; }
void PNCHAudioProcessor::setCurrentProgram (int index) { }
const String PNCHAudioProcessor::getProgramName (int index) {
	std::ostringstream presetname;
	presetname << "P";
	int num = ((int)floor(amount*31));
	for(int i = 0; i < num; i++) presetname << "U";
	presetname << "NCH";
	if(num == 0) presetname << ".";
	return { presetname.str() };
}
void PNCHAudioProcessor::changeProgramName (int index, const String& newName) { }

void PNCHAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	//if(!saved && sampleRate > 60000) setoversampling(0);
	saved = true;
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;
	preparedtoplay = true;
}
void PNCHAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	channelData.clear();
	for (int i = 0; i < channelnum; i++)
		channelData.push_back(nullptr);

	ospointerarray.resize(newchannelnum);
	os.reset(new dsp::Oversampling<float>(newchannelnum,1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
	os->initProcessing(samplesperblock);
	os->setUsingIntegerLatency(true);
	setoversampling(oversampling.get());
}
void PNCHAudioProcessor::releaseResources() { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool PNCHAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void PNCHAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;
	
	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

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

	for (int channel = 0; channel < channelnum; ++channel) {
		if(isoversampling) channelData[channel] = osbuffer.getWritePointer(channel);
		else channelData[channel] = buffer.getWritePointer(channel);
	}

	for (int sample = 0; sample < numsamples; ++sample) {
		for (int channel = 0; channel < channelnum; ++channel) {
			channelData[channel][sample] = pnch(channelData[channel][sample],amount);
			prmsadd += channelData[channel][sample]*channelData[channel][sample];
			prmscount++;
		}
	}

	if(isoversampling > 0) os->processSamplesDown(block);

	rmsadd = prmsadd;
	rmscount = prmscount;
}

float PNCHAudioProcessor::pnch(float source, float amount) {
	if(source == 0 || amount >= 1) return 0;

	/*
	float f = source;

	if(q >= 1) f = 0;
	else if(q > 0) {
		if (f > 0)
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

	return (float)fmax(fmin(((double)source)*pow(1-pow(1-abs((double)source),12.5),(3/(1-amount))-3),1),-1);
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
	return new PNCHAudioProcessorEditor(*this,amount);
}

void PNCHAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version
		<< linebreak << amount
		<< linebreak << (oversampling.get()?1:0) << linebreak;
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PNCHAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	try {
		saved = true;

		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, '\n');
		int saveversion = std::stoi(token);

		std::getline(ss, token, '\n');
		amount = std::stof(token);
		if(saveversion <= 1) amount = 1-(3/(3+(5*amount)));
		apvts.getParameter("amount")->setValueNotifyingHost(amount);

		std::getline(ss, token, '\n');
		setoversampling(std::stoi(token)-1);
		apvts.getParameter("oversampling")->setValueNotifyingHost(std::stof(token));

	} catch(const char* e) {
		logger.debug((String)"Error loading saved data: "+(String)e);
	} catch(String e) {
		logger.debug((String)"Error loading saved data: "+e);
	} catch(std::exception &e) {
		logger.debug((String)"Error loading saved data: "+(String)e.what());
	} catch(...) {
		logger.debug((String)"Error loading saved data");
	}
}
void PNCHAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "amount") amount = newValue;
	else if(parameterID == "oversampling") {
		oversampling = newValue > .5;
		setoversampling(newValue > .5);
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PNCHAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout
	PNCHAudioProcessor::createParameters()
{
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>("amount"		,"Amount"		,0.0f	,1.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("oversampling","Over-Sampling",true	));
	return { parameters.begin(), parameters.end() };
}
