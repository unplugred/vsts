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
	int num = ((int)floor(amount.get()*31));
	for(int i = 0; i < num; i++) presetname << "U";
	presetname << "NCH";
	if(num == 0) presetname << ".";
	return { presetname.str() };
}
void PNCHAudioProcessor::changeProgramName (int index, const String& newName) { }

void PNCHAudioProcessor::setoversampling(int factor) {
	oversampling = factor;
	if(factor == 0) setLatencySamples(0);
	else if(preparedtoplay) {
		os[factor-1]->reset();
		setLatencySamples(os[factor-1]->getLatencyInSamples());
	}
}
void PNCHAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	//if(!saved && sampleRate > 60000) setoversampling(0);
	for(int i = 0; i < 3; i++) {
		os[i].reset(new dsp::Oversampling<float>(getTotalNumInputChannels(),i+1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
		os[i]->initProcessing(samplesPerBlock);
		os[i]->setUsingIntegerLatency(true);
	}
	if(oversampling.get() == 0) setLatencySamples(0);
	else setLatencySamples(os[oversampling.get()-1]->getLatencyInSamples());
	saved = true;
	preparedtoplay = true;
}
void PNCHAudioProcessor::releaseResources() { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool PNCHAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
		return false;

	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void PNCHAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float newamount = amount.get(), prmsadd = rmsadd.get();
	int prmscount = rmscount.get(), poversampling = oversampling.get();

	dsp::AudioBlock<float> block(buffer);
	dsp::AudioBlock<float> osblock(buffer);
	AudioBuffer<float> osbuffer;
	int numsamples = 0;
	if(poversampling > 0) {
		osblock = os[poversampling-1]->processSamplesUp(block);
		if (getTotalNumInputChannels() == 1) {
			float* ptrArray[] = {osblock.getChannelPointer(0)};
			osbuffer = AudioBuffer<float>(ptrArray,1,static_cast<int>(osblock.getNumSamples()));
		} else if(getTotalNumInputChannels() >= 2){
			float* ptrArray[] = {osblock.getChannelPointer(0),osblock.getChannelPointer(1)};
			osbuffer = AudioBuffer<float>(ptrArray,2,static_cast<int>(osblock.getNumSamples()));
		}
		numsamples = osbuffer.getNumSamples();
	} else numsamples = buffer.getNumSamples();

	float unit = 0, mult = 0;
	for (int channel = 0; channel < getTotalNumInputChannels(); ++channel)
	{
		float* channelData;
		if(poversampling > 0) channelData = osbuffer.getWritePointer (channel);
		else channelData = buffer.getWritePointer (channel);

		unit = 1.f/numsamples;
		for (int sample = 0; sample < numsamples; ++sample) {
			mult = unit * sample;
			channelData[sample] = pnch(channelData[sample], oldamount  *(1-mult)+newamount  *mult);
			prmsadd += channelData[sample]*channelData[sample];
			prmscount++;
		}
	}

	if(poversampling > 0) os[poversampling-1]->processSamplesDown(block);

	oldamount = newamount;
	rmsadd = prmsadd;
	rmscount = prmscount;
}

float PNCHAudioProcessor::pnch(float source, float amount) {
	if(source == 0) return 0;

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

	return fmax(fmin(source*powf(1-powf(1-fabs(source),12.5f),amount*5),1),-1);
}

bool PNCHAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* PNCHAudioProcessor::createEditor() { return new PNCHAudioProcessorEditor (*this); }

void PNCHAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version
		<< linebreak << amount.get()
		<< linebreak << (oversampling.get()+1) << linebreak;
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PNCHAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	saved = true;

	std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
	std::string token;

	std::getline(ss, token, '\n');
	int saveversion = std::stoi(token);

	std::getline(ss, token, '\n');
	amount = std::stof(token);
	apvts.getParameter("amount")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	setoversampling(std::stoi(token)-1);
	apvts.getParameter("oversampling")->setValueNotifyingHost((std::stoi(token)-1)/3.f);
}
void PNCHAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "amount") {
		amount = newValue;
	} else if(parameterID == "oversampling") {
		setoversampling(newValue-1);
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PNCHAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout
	PNCHAudioProcessor::createParameters()
{
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat>("amount","Amount",0.0f,1.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterInt>("oversampling","Over-Sampling",1,4,2));
	return { parameters.begin(), parameters.end() };
}
