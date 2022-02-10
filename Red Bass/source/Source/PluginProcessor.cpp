/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

RedBassAudioProcessor::RedBassAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	apvts.addParameterListener("freq",this);
	apvts.addParameterListener("threshold",this);
	apvts.addParameterListener("attack",this);
	apvts.addParameterListener("release",this);
	apvts.addParameterListener("lowpass",this);
	apvts.addParameterListener("monitor",this);
	apvts.addParameterListener("dry",this);
	apvts.addParameterListener("wet",this);
}

RedBassAudioProcessor::~RedBassAudioProcessor(){
	apvts.removeParameterListener("freq",this);
	apvts.removeParameterListener("threshold",this);
	apvts.removeParameterListener("attack",this);
	apvts.removeParameterListener("release",this);
	apvts.removeParameterListener("lowpass",this);
	apvts.removeParameterListener("monitor",this);
	apvts.removeParameterListener("dry",this);
	apvts.removeParameterListener("wet",this);
}

const String RedBassAudioProcessor::getName() const { return "Red Bass"; }
bool RedBassAudioProcessor::acceptsMidi() const { return false; }
bool RedBassAudioProcessor::producesMidi() const { return false; }
bool RedBassAudioProcessor::isMidiEffect() const { return false; }
double RedBassAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int RedBassAudioProcessor::getNumPrograms() { return 1; }
int RedBassAudioProcessor::getCurrentProgram() { return 0; }
void RedBassAudioProcessor::setCurrentProgram (int index) { }
const String RedBassAudioProcessor::getProgramName (int index) { return { "hello"}; }
void RedBassAudioProcessor::changeProgramName (int index, const String& newName) { }

void RedBassAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	samplerate = sampleRate;
	dsp::ProcessSpec spec;
	spec.sampleRate = sampleRate;
	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = getNumInputChannels();
	filter.reset();
	float timestwo = lowpass.get()*124.1008481616f+17.3205080757f;
	(*filter.parameters.get()).setCutOffFrequency(sampleRate,timestwo*timestwo);
	(*filter.parameters.get()).type = dsp::StateVariableFilter::Parameters<float>::Type::lowPass;
	filter.prepare(spec);

	envelopefollower.setattack(calculateattack(attack.get()),sampleRate);
	envelopefollower.setrelease(calculaterelease(release.get()),sampleRate);
	envelopefollower.setthreshold(threshold.get(), sampleRate);

	if(channelData.size() != getNumInputChannels()) {
		channelData.clear();
		for (int i = 0; i < getNumInputChannels(); i++)
			channelData.push_back(nullptr);
	}
}
void RedBassAudioProcessor::releaseResources() { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool RedBassAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
		return false;

	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void RedBassAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float
		newfreq = freq.get(),
		newthreshold = threshold.get(),
		newattack = attack.get(),
		newrelease = release.get(),
		newlowpass = lowpass.get(),
		newmonitor = monitor.get()?1:0,
		newdry = dry.get(),
		newwet = wet.get(),
		curfreq = 0,
		curthreshold = 0,
		curattack = 0,
		currelease = 0,
		curlowpass = 0,
		curmonitor = 0,
		curdry = 0,
		curwet = 0;

	int numsamples = buffer.getNumSamples();
	int numchannels = buffer.getNumChannels();

	for (int channel = 0; channel < numchannels; ++channel)
		channelData[channel] = buffer.getWritePointer(channel);

	float mult = 0;
	float unit = 1.f/numsamples;
	float sidechain = 0;
	float osc = 0;
	for (int sample = 0; sample < numsamples; ++sample) {
		mult = unit * sample;
		sidechain = 0;

		curfreq      = (oldfreq     *(1-mult)+newfreq     *mult);
		curthreshold = (oldthreshold*(1-mult)+newthreshold*mult);
		curattack    = (oldattack   *(1-mult)+newattack   *mult);
		currelease   = (oldrelease  *(1-mult)+newrelease  *mult);
		curlowpass   = (oldlowpass  *(1-mult)+newlowpass  *mult);
		curmonitor   = (oldmonitor  *(1-mult)+newmonitor  *mult);
		curdry       = (olddry      *(1-mult)+newdry      *mult);
		curwet       = (oldwet      *(1-mult)+newwet      *mult);
		
		float lvl = fmin(20*envelopefollower.envelope*curwet,1);
		float vc = viscount.get();
		if(vc < samplerate*2) {
			viscount = vc+1;
			visamount = visamount.get()+lvl;
		}

		crntsmpl = fmod(crntsmpl+(calculatefrequency(curfreq)/getSampleRate()),1);
		if(curmonitor >= 1) osc = 0;
		else osc = sin(crntsmpl*MathConstants<double>::twoPi)*lvl;

		for(int channel = 0; channel < numchannels; ++channel) {
			sidechain += channelData[channel][sample];
			if(curmonitor < 1)
				channelData[channel][sample] = (osc+channelData[channel][sample]*curdry)*(1-curmonitor);
			else channelData[channel][sample] = 0;
		}

		sidechain = sidechain/numchannels;

		if(oldlowpass != newlowpass) {
			(*filter.parameters.get()).setCutOffFrequency(samplerate,calculatelowpass(curlowpass));
		}
		if(curlowpass < 1) sidechain = filter.processSample(sidechain);
		else filter.processSample(sidechain);
		if(curmonitor > 0) for(int channel = 0; channel < numchannels; ++channel)
			channelData[channel][sample] += sidechain*curmonitor;

		if(oldattack != newattack) {
			envelopefollower.setattack(calculateattack(curattack),samplerate);
		}
		if(oldrelease != newrelease) {
			envelopefollower.setrelease(calculaterelease(currelease),samplerate);
		}
		if(oldthreshold != newthreshold)
			envelopefollower.setthreshold(curthreshold,samplerate);
		envelopefollower.process(sidechain);
	}

	oldfreq = newfreq;
	oldthreshold = newthreshold;
	oldattack = newattack;
	oldrelease = newrelease;
	oldlowpass = newlowpass;
	oldmonitor = newmonitor>.5;
	olddry = newdry;
	oldwet = newwet;
}
float RedBassAudioProcessor::calculateattack(float value) {
	float timestwo = value*15.2896119631f+7.0710678119f;
	return timestwo*timestwo;
}
float RedBassAudioProcessor::calculaterelease(float value) {
	float timestwo = value*26.4823847482f+12.2474487139f;
	return timestwo*timestwo;
}
float RedBassAudioProcessor::calculatelowpass(float value) {
	float timestwo = value*131.4213562373f+10;
	return timestwo*timestwo;
}
float RedBassAudioProcessor::calculatefrequency(float value) {
	float timestwo = value*5.527864045f+4.472135955f;
	return timestwo*timestwo;
}



bool RedBassAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* RedBassAudioProcessor::createEditor() { return new RedBassAudioProcessorEditor (*this); }

void RedBassAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version
		<< linebreak << freq.get()
		<< linebreak << threshold.get()
		<< linebreak << attack.get()
		<< linebreak << release.get()
		<< linebreak << lowpass.get()
		<< linebreak << (monitor.get()?1:0)
		<< linebreak << dry.get()
		<< linebreak << wet.get() << linebreak;
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void RedBassAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
	std::string token;

	std::getline(ss, token, '\n');
	int saveversion = std::stoi(token);

	std::getline(ss, token, '\n');
	freq = std::stof(token);
	apvts.getParameter("freq")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	threshold = std::stof(token);
	apvts.getParameter("threshold")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	attack = std::stof(token);
	apvts.getParameter("attack")->setValueNotifyingHost(std::stof(token));
	envelopefollower.setattack(calculateattack(attack.get()),samplerate);

	std::getline(ss, token, '\n');
	release = std::stof(token);
	apvts.getParameter("release")->setValueNotifyingHost(std::stof(token));
	envelopefollower.setrelease(calculaterelease(release.get()),samplerate);

	envelopefollower.setthreshold(threshold.get(), samplerate);

	std::getline(ss, token, '\n');
	lowpass = std::stof(token);
	apvts.getParameter("lowpass")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	monitor = std::stoi(token) == 1;
	apvts.getParameter("monitor")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	dry = std::stof(token);
	apvts.getParameter("dry")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	wet = std::stof(token);
	apvts.getParameter("wet")->setValueNotifyingHost(std::stof(token));
}
void RedBassAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "freq") {
		freq = newValue;
	} else if(parameterID == "threshold") {
		threshold = newValue;
	} else if(parameterID == "attack") {
		attack = newValue;
	} else if(parameterID == "release") {
		release = newValue;
	} else if(parameterID == "lowpass") {
		lowpass = newValue;
	} else if(parameterID == "monitor") {
		monitor = newValue>.5f;
	} else if(parameterID == "dry") {
		dry = newValue;
	} else if(parameterID == "wet") {
		wet = newValue;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RedBassAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout
	RedBassAudioProcessor::createParameters()
{
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat>("freq","Frequency",0.0f,1.0f,0.48f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("threshold","Threshold",0.0f,1.0f,0.02f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("attack","Attack",0.0f,1.0f,0.17f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("release","Release",0.0f,1.0f,0.18f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("lowpass","Sidechain lowpass",0.0f,1.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterBool>("monitor","Monitor sidechain",false));
	parameters.push_back(std::make_unique<AudioParameterFloat>("dry","Dry",0.0f,1.0f,1.0f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("wet","Wet",0.0f,1.0f,0.18f));
	return { parameters.begin(), parameters.end() };
}
	Atomic<float> freq = 0.48, threshold = 0.02, attack = 0.17, release = 0.18, lowpass = 0, dry = 1, wet = 0.18;
