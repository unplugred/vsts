/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

PisstortionAudioProcessor::PisstortionAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	apvts.addParameterListener("freq",this);
	apvts.addParameterListener("piss",this);
	apvts.addParameterListener("noise",this);
	apvts.addParameterListener("harm",this);
	apvts.addParameterListener("stereo",this);
	apvts.addParameterListener("gain",this);
	apvts.addParameterListener("oversampling",this);

	presets[0].name = "Default";
	presets[0].freq = 0.17f;
	presets[0].piss = 1.0f;
	presets[0].noise = 0.35f;
	presets[0].harm = 0.31f;
	presets[0].stereo = 0.1f;

	presets[1].name = "Sand in Your Ear";
	presets[1].freq = 0.3f;
	presets[1].piss = 1.0f;
	presets[1].noise = 1.0f;
	presets[1].harm = 0.18f;
	presets[1].stereo = 0.0f;

	presets[2].name = "Mega Drive";
	presets[2].freq = 0.02f;
	presets[2].piss = 0.42f;
	presets[2].noise = 0.0f;
	presets[2].harm = 1.0f;
	presets[2].stereo = 0.0f;

	presets[3].name = "Easy to Destroy";
	presets[3].freq = 0.32f;
	presets[3].piss = 1.0f;
	presets[3].noise = 0.13f;
	presets[3].harm = 1.0f;
	presets[3].stereo = 0.0f;

	presets[4].name = "Screamy";
	presets[4].freq = 0.58f;
	presets[4].piss = 1.0f;
	presets[4].noise = 0.0f;
	presets[4].harm = 0.51f;
	presets[4].stereo = 0.52f;

	presets[5].name = "I Love Piss";
	presets[5].freq = 1.0f;
	presets[5].piss = 1.0f;
	presets[5].noise = 0.0f;
	presets[5].harm = 1.0f;
	presets[5].stereo = 0.25f;

	presets[6].name = "You Broke It";
	presets[6].freq = 0.09f;
	presets[6].piss = 1.0f;
	presets[6].noise = 0.0f;
	presets[6].harm = 0.02f;
	presets[6].stereo = 0.0f;

	presets[7].name = "Splashing";
	presets[7].freq = 1.0f;
	presets[7].piss = 1.0f;
	presets[7].noise = 0.39f;
	presets[7].harm = 0.07f;
	presets[7].stereo = 0.22f;

	presets[8].name = "PISS LASERS";
	presets[8].freq = 0.39f;
	presets[8].piss = 0.95f;
	presets[8].noise = 0.36f;
	presets[8].harm = 0.59f;
	presets[8].stereo = 0.34f;

	updatevis = true;
}

PisstortionAudioProcessor::~PisstortionAudioProcessor(){
	apvts.removeParameterListener("freq",this);
	apvts.removeParameterListener("piss",this);
	apvts.removeParameterListener("noise",this);
	apvts.removeParameterListener("harm",this);
	apvts.removeParameterListener("stereo",this);
	apvts.removeParameterListener("gain",this);
	apvts.removeParameterListener("oversampling",this);

}

const String PisstortionAudioProcessor::getName() const { return "Pisstortion"; }
bool PisstortionAudioProcessor::acceptsMidi() const { return false; }
bool PisstortionAudioProcessor::producesMidi() const { return false; }
bool PisstortionAudioProcessor::isMidiEffect() const { return false; }
double PisstortionAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PisstortionAudioProcessor::getNumPrograms() { return 9; }
int PisstortionAudioProcessor::getCurrentProgram() { return currentpreset; }
void PisstortionAudioProcessor::setCurrentProgram (int index) {
	if(!boot) return;

	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;
	lerptable[0] = freq.get();
	lerptable[1] = piss.get();
	lerptable[2] = noise.get();
	lerptable[3] = harm.get();
	lerptable[4] = stereo.get();

	if(lerpstage <= 0) {
		lerpstage = 1;
		startTimerHz(30);
	} else lerpstage = 1;
}
void PisstortionAudioProcessor::timerCallback() {
	lerpstage *= .64f;
	if(lerpstage < .001) {
		apvts.getParameter("freq")->setValueNotifyingHost(presets[currentpreset].freq);
		apvts.getParameter("piss")->setValueNotifyingHost(presets[currentpreset].piss);
		apvts.getParameter("noise")->setValueNotifyingHost(presets[currentpreset].noise);
		apvts.getParameter("harm")->setValueNotifyingHost(presets[currentpreset].harm);
		apvts.getParameter("stereo")->setValueNotifyingHost(presets[currentpreset].stereo);
		lerpstage = 0;
		undoManager.beginNewTransaction();
		stopTimer();
		return;
	}
	lerpValue("freq", lerptable[0], presets[currentpreset].freq);
	lerpValue("piss", lerptable[1], presets[currentpreset].piss);
	lerpValue("noise", lerptable[2], presets[currentpreset].noise);
	lerpValue("harm", lerptable[3], presets[currentpreset].harm);
	lerpValue("stereo", lerptable[4], presets[currentpreset].stereo);
}
void PisstortionAudioProcessor::lerpValue(StringRef slider, float& oldval, float newval) {
	oldval = (newval-oldval)*.36f+oldval;
	apvts.getParameter(slider)->setValueNotifyingHost(oldval);
}
const String PisstortionAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void PisstortionAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
	presets[index].freq = freq.get();
	presets[index].piss = piss.get();
	presets[index].noise = noise.get();
	presets[index].harm = harm.get();
	presets[index].stereo = stereo.get();
}

void PisstortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	for(int i = 0; i < 3; i++) {
		os[i].reset(new dsp::Oversampling<float>(getTotalNumInputChannels(),i+1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
		os[i]->initProcessing(samplesPerBlock);
		os[i]->setUsingIntegerLatency(true);
	}
	if(oversampling.get() == 0) setLatencySamples(0);
	else setLatencySamples(os[oversampling.get()-1]->getLatencyInSamples());
	preparedtoplay = true;
}
void PisstortionAudioProcessor::releaseResources() {
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PisstortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
		return false;

	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void PisstortionAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float newfreq = freq.get(), newpiss = piss.get(), newnoise = noise.get(), newharm = harm.get(), newstereo = stereo.get(), newgain = gain.get(), prmsadd = rmsadd.get();
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
			channelData[sample] = pisstortion(channelData[sample], channel,
				oldfreq  *(1-mult)+newfreq  *mult,
				oldpiss*(1-mult)+newpiss*mult,
				oldnoise *(1-mult)+newnoise *mult,
				oldharm  *(1-mult)+newharm  *mult,
				oldstereo*(1-mult)+newstereo*mult,
				oldgain  *(1-mult)+newgain  *mult);
			prmsadd += channelData[sample]*channelData[sample];
			prmscount++;
		}
	}

	if(poversampling > 0) os[poversampling-1]->processSamplesDown(block);

	oldfreq = newfreq;
	oldpiss = newpiss;
	oldnoise = newnoise;
	oldharm = newharm;
	oldstereo = newstereo;
	oldgain = newgain;
	rmsadd = prmsadd;
	rmscount = prmscount;

	boot = true;
}

float PisstortionAudioProcessor::pisstortion(float source, int channel, float freq, float piss, float noise, float harm, float stereo, float gain) {
	if(source == 0) return 0;

	float f = sin(source*50*(freq+stereo*freq*((float)channel-.5f)));

	if (harm == 1) {
		if(f > 0) f = 1;
		else if(f < 0) f = -1;
	} else if(harm == 0) {
		f = 0;
	} else if(harm != .5) {
		float h = harm;
		if (h < .5)
			h = h*2;
		else
			h = .5/(1-h);

		if (f > 0)
			f = 1-powf(1-f,h);
		else
			f = powf(f+1,h)-1;
	}
	
	if (noise > 0) {
		f *= 1-powf(1-fabs(source),1./noise);
	}

	return ((f*piss)+(source*(1-piss)))*gain;
}

void PisstortionAudioProcessor::setoversampling(int factor) {
	oversampling = factor;
	if(factor == 0) setLatencySamples(0);
	else if(preparedtoplay) {
		os[factor-1]->reset();
		setLatencySamples(os[factor-1]->getLatencyInSamples());
	}
}

bool PisstortionAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* PisstortionAudioProcessor::createEditor() { return new PisstortionAudioProcessorEditor (*this); }

void PisstortionAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version
		<< linebreak << currentpreset
		<< linebreak << freq.get()
		<< linebreak << piss.get()
		<< linebreak << noise.get()
		<< linebreak << harm.get()
		<< linebreak << stereo.get()
		<< linebreak << gain.get()
		<< linebreak << (oversampling.get()+1) << linebreak;
	for (int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name
			<< linebreak << presets[i].freq
			<< linebreak << presets[i].piss
			<< linebreak << presets[i].noise
			<< linebreak << presets[i].harm
			<< linebreak << presets[i].stereo << linebreak;
	}
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PisstortionAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
	std::string token;

	std::getline(ss, token, '\n');
	int saveversion = std::stoi(token);

	std::getline(ss, token, '\n');
	currentpreset = std::stoi(token);

	std::getline(ss, token, '\n');
	freq = std::stof(token);
	apvts.getParameter("freq")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	piss = std::stof(token);
	apvts.getParameter("piss")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	noise = std::stof(token);
	apvts.getParameter("noise")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	harm = std::stof(token);
	apvts.getParameter("harm")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	stereo = std::stof(token);
	apvts.getParameter("stereo")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	gain = std::stof(token);
	apvts.getParameter("gain")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	setoversampling(std::stoi(token)-1);
	apvts.getParameter("oversampling")->setValueNotifyingHost((std::stoi(token)-1)/3.f);

	updatevis = true;

	for(int i = 0; i < (saveversion == 0 ? 8 : getNumPrograms()); i++) {
		std::getline(ss, token, '\n'); presets[i].name = token;
		std::getline(ss, token, '\n'); presets[i].freq = std::stof(token);
		std::getline(ss, token, '\n'); presets[i].piss = std::stof(token);
		std::getline(ss, token, '\n'); presets[i].noise = std::stof(token);
		std::getline(ss, token, '\n'); presets[i].harm = std::stof(token);
		std::getline(ss, token, '\n'); presets[i].stereo = std::stof(token);
	}
}
void PisstortionAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "freq") {
		freq = newValue;
	} else if(parameterID == "piss") {
		piss = newValue;
	} else if(parameterID == "noise") {
		noise = newValue;
	} else if(parameterID == "harm") {
		harm = newValue;
	} else if(parameterID == "stereo") {
		stereo = newValue;
	} else if(parameterID == "gain") {
		gain = newValue;
	} else if(parameterID == "oversampling") {
		setoversampling(newValue-1);
		oversampling = newValue-1;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PisstortionAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout
	PisstortionAudioProcessor::createParameters()
{
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat>("freq","Frequency",0.0f,1.0f,0.17f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("piss","Piss",0.0f,1.0f,1.0f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("noise","Noise Reduction",0.0f,1.0f,0.35f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("harm","Harmonics",0.0f,1.0f,0.31f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("stereo","Stereo",0.0f,1.0f,0.1f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("gain","Out Gain",0.0f,1.0f,1.0f));
	parameters.push_back(std::make_unique<AudioParameterInt>("oversampling","Over-Sampling",1,4,2));
	return { parameters.begin(), parameters.end() };
}
