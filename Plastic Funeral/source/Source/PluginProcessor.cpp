/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

PFAudioProcessor::PFAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	presets[0].name = "Default";
	presets[0].freq = 0.32f;
	presets[0].fat = 0.0f;
	presets[0].drive = 0.0f;
	presets[0].dry = 0.0f;
	presets[0].stereo = 0.37f;

	presets[1].name = "Digital Driver";
	presets[1].freq = 0.27f;
	presets[1].fat = -11.36f;
	presets[1].drive = 0.53f;
	presets[1].dry = 0.0f;
	presets[1].stereo = 0.0f;

	presets[2].name = "Noisy Bass Pumper";
	presets[2].freq = 0.55f;
	presets[2].fat = 20.0f;
	presets[2].drive = 0.59f;
	presets[2].dry = 0.77f;
	presets[2].stereo = 0.0f;

	presets[3].name = "Broken Earphone";
	presets[3].freq = 0.19f;
	presets[3].fat = -1.92f;
	presets[3].drive = 1.0f;
	presets[3].dry = 0.0f;
	presets[3].stereo = 0.88f;

	presets[4].name = "Fatass";
	presets[4].freq = 0.62f;
	presets[4].fat = -5.28f;
	presets[4].drive = 1.0f;
	presets[4].dry = 0.0f;
	presets[4].stereo = 0.0f;

	presets[5].name = "Screaming Alien";
	presets[5].freq = 0.63f;
	presets[5].fat = 5.6f;
	presets[5].drive = 0.2f;
	presets[5].dry = 0.0f;
	presets[5].stereo = 0.0f;

	presets[6].name = "Bad Connection";
	presets[6].freq = 0.25f;
	presets[6].fat = -2.56f;
	presets[6].drive = 0.48f;
	presets[6].dry = 0.0f;
	presets[6].stereo = 0.0f;

	presets[7].name = "Ouch";
	presets[7].freq = 0.9f;
	presets[7].fat = -20.0f;
	presets[7].drive = 0.0f;
	presets[7].dry = 0.0f;
	presets[7].stereo = 0.69f;

	normalizegain();
}

PFAudioProcessor::~PFAudioProcessor(){}

const String PFAudioProcessor::getName() const { return "Plastic Funeral"; }
bool PFAudioProcessor::acceptsMidi() const { return false; }
bool PFAudioProcessor::producesMidi() const { return false; }
bool PFAudioProcessor::isMidiEffect() const { return false; }
double PFAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PFAudioProcessor::getNumPrograms() { return 8; }
int PFAudioProcessor::getCurrentProgram() { return currentpreset; }
void PFAudioProcessor::setCurrentProgram (int index) {
	if(!boot) return;

	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;
	lerptable[0] = freq.get();
	lerptable[1] = fat.get()*.025f + .5f;
	lerptable[2] = drive.get();
	lerptable[3] = dry.get();
	lerptable[4] = stereo.get();

	if(lerpstage <= 0) {
		lerpstage = 1;
		startTimerHz(30);
	} else lerpstage = 1;
}
void PFAudioProcessor::timerCallback() {
	lerpstage *= .64f;
	if(lerpstage < .001) {
		apvts.getParameter("freq")->setValueNotifyingHost(presets[currentpreset].freq);
		apvts.getParameter("fat")->setValueNotifyingHost(presets[currentpreset].fat*.025f + .5f);
		apvts.getParameter("drive")->setValueNotifyingHost(presets[currentpreset].drive);
		apvts.getParameter("dry")->setValueNotifyingHost(presets[currentpreset].dry);
		apvts.getParameter("stereo")->setValueNotifyingHost(presets[currentpreset].stereo);
		lerpstage = 0;
		undoManager.beginNewTransaction();
		stopTimer();
		return;
	}
	lerpValue("freq", lerptable[0], presets[currentpreset].freq);
	lerpValue("fat", lerptable[1], presets[currentpreset].fat*.025f + .5f);
	lerpValue("drive", lerptable[2], presets[currentpreset].drive);
	lerpValue("dry", lerptable[3], presets[currentpreset].dry);
	lerpValue("stereo", lerptable[4], presets[currentpreset].stereo);
}
void PFAudioProcessor::lerpValue(StringRef slider, float& oldval, float newval) {
	oldval = (newval-oldval)*.36f+oldval;
	apvts.getParameter(slider)->setValueNotifyingHost(oldval);
}
const String PFAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void PFAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
	presets[index].freq = freq.get();
	presets[index].fat = fat.get();
	presets[index].drive = drive.get();
	presets[index].dry = dry.get();
	presets[index].stereo = stereo.get();
}

void PFAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
}
void PFAudioProcessor::releaseResources() {
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PFAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
		return false;

	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void PFAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;
	auto totalNumInputChannels	= getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float newfreq = freq.get(), newfat = fat.get(), newdrive = drive.get(), newdry = dry.get(), newstereo = stereo.get(), newgain = gain.get(), prmsadd = rmsadd.get();
	int prmscount = rmscount.get();
	if(newfat != oldfat || newdry != olddry) normalizegain();
	float newnorm = norm.get();

	float unit = 0, mult = 0;
	for (int channel = 0; channel < totalNumInputChannels; ++channel)
	{
		auto* channelData = buffer.getWritePointer (channel);
		unit = 1.f / buffer.getNumSamples();
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
			mult = unit * sample;
			channelData[sample] = plasticfuneral(channelData[sample], channel,
				oldfreq  *(1-mult)+newfreq  *mult,
				oldfat   *(1-mult)+newfat   *mult,
				olddrive *(1-mult)+newdrive *mult,
				olddry   *(1-mult)+newdry   *mult,
				oldstereo*(1-mult)+newstereo*mult,
				oldgain  *(1-mult)+newgain  *mult)*(
				oldnorm  *(1-mult)+newnorm  *mult);
			prmsadd += channelData[sample]*channelData[sample];
			prmscount++;
		}
	}

	oldfreq = newfreq;
	oldfat = newfat;
	olddrive = newdrive;
	olddry = newdry;
	oldstereo = newstereo;
	oldgain = newgain;
	oldnorm = newnorm;
	rmsadd = prmsadd;
	rmscount = prmscount;

	boot = true;
}

float PFAudioProcessor::plasticfuneral(float source, int channel, float freq, float fat, float drive, float dry, float stereo, float gain) {
	freq = fmax(fmin(freq + stereo * .2f * ((float)channel * 2 - 1), 1), 0);
	float smpl = source * (100 - cos(freq * 1.5708f) * 99);
	float pfreq = fmod(fabs(smpl), 4);
	pfreq = (pfreq > 3 ? (pfreq - 4) : (pfreq > 1 ? (2 - pfreq) : pfreq)) * (smpl > 0 ? 1 : -1);
	float pdrive = fmax(fmin(smpl, 1), -1);
	smpl = pdrive * drive + pfreq * (1 - drive);
	return ((smpl + (sin(smpl * 1.5708f) - smpl) * fat) * (1 - dry) + source * dry) * gain;
}

void PFAudioProcessor::normalizegain() {
	float newnorm = 0.25f, newfat = fat.get(), newdry = dry.get();
	for (float i = 0; i <= 1; i += .005f)
		newnorm = fmax(newnorm,fabs(i+(sin(i*1.5708f)-i)*newfat)*(1-newdry)+i*newdry);
	norm = 1.f/newnorm;
}

bool PFAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* PFAudioProcessor::createEditor() { return new PFAudioProcessorEditor (*this); }

void PFAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version
		<< linebreak << currentpreset
		<< linebreak << freq.get()
		<< linebreak << fat.get()
		<< linebreak << drive.get()
		<< linebreak << dry.get()
		<< linebreak << stereo.get()
		<< linebreak << gain.get()
		<< linebreak << (oversampling.get()+1) << linebreak;
	for (int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name
			<< linebreak << presets[i].freq
			<< linebreak << presets[i].fat
			<< linebreak << presets[i].drive
			<< linebreak << presets[i].dry
			<< linebreak << presets[i].stereo << linebreak;
	}
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PFAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
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
	fat = std::stof(token);
	apvts.getParameter("fat")->setValueNotifyingHost(std::stof(token)*.025 + .5);

	std::getline(ss, token, '\n');
	drive = std::stof(token);
	apvts.getParameter("drive")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	dry = std::stof(token);
	apvts.getParameter("dry")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	stereo = std::stof(token);
	apvts.getParameter("stereo")->setValueNotifyingHost(std::stof(token));

	std::getline(ss, token, '\n');
	gain = std::stof(token);
	apvts.getParameter("gain")->setValueNotifyingHost(std::stof(token));

	if (saveversion > 1) {
		std::getline(ss, token, '\n');
		setoversampling(std::stoi(token)-1);
		apvts.getParameter("oversampling")->setValueNotifyingHost((std::stoi(token)-1)/3.f);
	}

	normalizegain();

	for(int i = 0; i < getNumPrograms(); i++) {
		std::getline(ss, token, '\n'); presets[i].name = token;
		std::getline(ss, token, '\n'); presets[i].freq = std::stof(token);
		std::getline(ss, token, '\n'); presets[i].fat = std::stof(token);
		std::getline(ss, token, '\n'); presets[i].drive = std::stof(token);
		std::getline(ss, token, '\n'); presets[i].dry = std::stof(token);
		std::getline(ss, token, '\n'); presets[i].stereo = std::stof(token);
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PFAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout
	PFAudioProcessor::createParameters()
{
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat>("freq","Frequency",0.0f,1.0f,0.32f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("fat","Fatness",-20.0f,20.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("drive","Drive",0.0f,1.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("dry","Dry",0.0f,1.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("stereo","Stereo",0.0f,1.0f,0.37f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("gain","Out Gain",0.0f,1.0f,0.4f));
	parameters.push_back(std::make_unique<AudioParameterInt>("oversampling","Over-Sampling",1,4,1));
	return { parameters.begin(), parameters.end() };
}
