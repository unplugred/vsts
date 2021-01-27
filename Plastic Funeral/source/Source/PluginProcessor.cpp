/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

//==============================================================================
FmerAudioProcessor::FmerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
			#if ! JucePlugin_IsMidiEffect
			 #if ! JucePlugin_IsSynth
			  .withInput("Input", juce::AudioChannelSet::stereo(), true)
			 #endif
			  .withOutput("Output", juce::AudioChannelSet::stereo(), true)
			#endif
			  ),
#else
	:
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	presets[0].name = "Default";
	presets[0].freq = 0.3f;
	presets[0].fat = 0.0f;
	presets[0].drive = 0.0f;
	presets[0].dry = 0.0f;
	presets[0].stereo = 0.3f;
	presets[0].gain = 0.4f;

	presets[1].name = "Preset 1";
	presets[1].freq = 0.3f;
	presets[1].fat = 0.0f;
	presets[1].drive = 0.0f;
	presets[1].dry = 0.0f;
	presets[1].stereo = 0.3f;
	presets[1].gain = 0.4f;

	presets[2].name = "Preset 2";
	presets[2].freq = 0.3f;
	presets[2].fat = 0.0f;
	presets[2].drive = 0.0f;
	presets[2].dry = 0.0f;
	presets[2].stereo = 0.3f;
	presets[2].gain = 0.4f;

	presets[3].name = "Preset 3";
	presets[3].freq = 0.3f;
	presets[3].fat = 0.0f;
	presets[3].drive = 0.0f;
	presets[3].dry = 0.0f;
	presets[3].stereo = 0.3f;
	presets[3].gain = 0.4f;

	presets[4].name = "Preset 4";
	presets[4].freq = 0.3f;
	presets[4].fat = 0.0f;
	presets[4].drive = 0.0f;
	presets[4].dry = 0.0f;
	presets[4].stereo = 0.3f;
	presets[4].gain = 0.4f;

	presets[5].name = "Preset 5";
	presets[5].freq = 0.3f;
	presets[5].fat = 0.0f;
	presets[5].drive = 0.0f;
	presets[5].dry = 0.0f;
	presets[5].stereo = 0.3f;
	presets[5].gain = 0.4f;

	presets[6].name = "Preset 6";
	presets[6].freq = 0.3f;
	presets[6].fat = 0.0f;
	presets[6].drive = 0.0f;
	presets[6].dry = 0.0f;
	presets[6].stereo = 0.3f;
	presets[6].gain = 0.4f;

	presets[7].name = "Preset 7";
	presets[7].freq = 0.3f;
	presets[7].fat = 0.0f;
	presets[7].drive = 0.0f;
	presets[7].dry = 0.0f;
	presets[7].stereo = 0.3f;
	presets[7].gain = 0.4f;

	presets[8].name = "Preset 8";
	presets[8].freq = 0.3f;
	presets[8].fat = 0.0f;
	presets[8].drive = 0.0f;
	presets[8].dry = 0.0f;
	presets[8].stereo = 0.3f;
	presets[8].gain = 0.4f;

	presets[9].name = "Preset 9";
	presets[9].freq = 0.3f;
	presets[9].fat = 0.0f;
	presets[9].drive = 0.0f;
	presets[9].dry = 0.0f;
	presets[9].stereo = 0.3f;
	presets[9].gain = 0.4f;
}

FmerAudioProcessor::~FmerAudioProcessor()
{
}

//==============================================================================
const juce::String FmerAudioProcessor::getName() const
{
	return "Plastic Funeral";
}

bool FmerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
	return true;
   #else
	return false;
   #endif
}

bool FmerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
	return true;
   #else
	return false;
   #endif
}

bool FmerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
	return true;
   #else
	return false;
   #endif
}

double FmerAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int FmerAudioProcessor::getNumPrograms()
{
	return 10;
}

int FmerAudioProcessor::getCurrentProgram()
{
	return currentpreset;
}

void FmerAudioProcessor::setCurrentProgram (int index)
{
	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;
	lerptable[0] = freq;
	lerptable[1] = fat;
	lerptable[2] = drive;
	lerptable[3] = dry;
	lerptable[4] = stereo;
	lerptable[5] = gain;
	lerpPreset(1);
}
void FmerAudioProcessor::lerpPreset(float stage)
{
	stage *= .8;
	if(stage < .001) {
		apvts.getParameter("freq")->setValueNotifyingHost(presets[currentpreset].freq);
		apvts.getParameter("fat")->setValueNotifyingHost(presets[currentpreset].fat*.025 + .5);
		apvts.getParameter("drive")->setValueNotifyingHost(presets[currentpreset].drive);
		apvts.getParameter("dry")->setValueNotifyingHost(presets[currentpreset].dry);
		apvts.getParameter("stereo")->setValueNotifyingHost(presets[currentpreset].stereo);
		apvts.getParameter("gain")->setValueNotifyingHost(presets[currentpreset].gain);
		undoManager.beginNewTransaction();
		return;
	}
	lerpValue("freq", lerptable[0], presets[currentpreset].freq);
	lerpValue("fat", lerptable[1], presets[currentpreset].fat*.025 + .5);
	lerpValue("drive", lerptable[2], presets[currentpreset].drive);
	lerpValue("dry", lerptable[3], presets[currentpreset].dry);
	lerpValue("stereo", lerptable[4], presets[currentpreset].stereo);
	lerpValue("gain", lerptable[5], presets[currentpreset].gain);
	new DelayedOneShotLambda(50.f/3.f, [this,stage]() { lerpPreset(stage); });
}
void FmerAudioProcessor::lerpValue(StringRef slider, float& oldval, float newval)
{
	oldval = (newval-oldval)*.2+oldval;
	apvts.getParameter(slider)->setValueNotifyingHost(oldval);
}

const juce::String FmerAudioProcessor::getProgramName (int index)
{
	return { presets[index].name };
}

void FmerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
	presets[index].name = newName;
	presets[index].freq = freq;
	presets[index].fat = fat;
	presets[index].drive = drive;
	presets[index].dry = dry;
	presets[index].stereo = stereo;
	presets[index].gain = gain;
}

//==============================================================================
void FmerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..
}

void FmerAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FmerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
	juce::ignoreUnused (layouts);
	return true;
  #else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
   #endif

	return true;
  #endif
}
#endif

void FmerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels	= getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float unit, mult;
	for (int channel = 0; channel < totalNumInputChannels; ++channel)
	{
		auto* channelData = buffer.getWritePointer (channel);
		unit = 1.f / buffer.getNumSamples();
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
			mult = unit * sample;
			channelData[sample] = plasticfuneral(channelData[sample], channel,
				oldfreq * (1 - mult) + freq * mult,
				oldfat * (1 - mult) + fat * mult,
				olddrive * (1 - mult) + drive * mult,
				olddry * (1 - mult) + dry * mult,
				oldstereo * (1 - mult) + stereo * mult,
				oldgain * (1 - mult) + gain * mult) * (
				oldnorm * (1 - mult) + norm * mult);
		}
	}

	oldfreq = freq;
	oldfat = fat;
	olddrive = drive;
	olddry = dry;
	oldstereo = stereo;
	oldgain = gain;
	oldnorm = norm;
}

float FmerAudioProcessor::plasticfuneral(float source, int channel, float freq, float fat, float drive, float dry, float stereo, float gain)
{
	freq = fmax(fmin(freq + stereo * .2 * ((float)channel * 2 - 1), 1), 0);
	float smpl = source * (100 - cos(freq * 1.5708f) * 99);
	float pdrive = fmax(fmin(smpl, 1), -1);
	float pfreq = fmod(abs(smpl),4);
	pfreq = (pfreq > 3 ? (pfreq - 4) : (pfreq > 1 ? (2 - pfreq) : pfreq)) * (smpl > 0 ? 1 : -1);
	smpl = pdrive * drive + pfreq * (1 - drive);
	return ((smpl + (sin(smpl * 1.5708f) - smpl) * fat) * (1 - dry) + source * dry) * gain;
}

void FmerAudioProcessor::normalizegain() {
	norm = 0.25;
	for (int i = 0; i < 400; i++) {
		norm = fmax(norm, abs(plasticfuneral((i - 200) / 200.f, 0, freq, fat, drive, dry, stereo, 1)));
		norm = fmax(norm, abs(plasticfuneral((i - 200) / 200.f, 1, freq, fat, drive, dry, stereo, 1)));
	}
	norm = 1/norm;
}

//==============================================================================
bool FmerAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FmerAudioProcessor::createEditor()
{
	return new FmerAudioProcessorEditor (*this);
}

//==============================================================================
void FmerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
	const char linebreak = '\n';
	std::ostringstream data;
	data << currentpreset << linebreak << freq << linebreak << fat << linebreak << drive << linebreak << dry << linebreak << stereo << linebreak << gain << linebreak;
	for (int i = 0; i < getNumPrograms(); i++) {
		data <<
		presets[i].name << linebreak <<
		presets[i].freq << linebreak <<
		presets[i].fat << linebreak <<
		presets[i].drive << linebreak <<
		presets[i].dry << linebreak <<
		presets[i].stereo << linebreak <<
		presets[i].gain << linebreak;
	}
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}

void FmerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	//a complete disaster but at least it works
	std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
	std::string token;
	std::getline(ss, token, '\n');
	currentpreset = std::stoi(token);
	std::getline(ss, token, '\n');
	apvts.getParameter("freq")->setValueNotifyingHost(std::stof(token));
	std::getline(ss, token, '\n');
	apvts.getParameter("fat")->setValueNotifyingHost(std::stof(token)*.025 + .5);
	std::getline(ss, token, '\n');
	apvts.getParameter("drive")->setValueNotifyingHost(std::stof(token));
	std::getline(ss, token, '\n');
	apvts.getParameter("dry")->setValueNotifyingHost(std::stof(token));
	std::getline(ss, token, '\n');
	apvts.getParameter("stereo")->setValueNotifyingHost(std::stof(token));
	std::getline(ss, token, '\n');
	apvts.getParameter("gain")->setValueNotifyingHost(std::stof(token));
	for(int i = 0; i < getNumPrograms(); i++) {
		std::getline(ss, token, '\n');
		presets[i].name = token;
		std::getline(ss, token, '\n');
		presets[i].freq = std::stof(token);
		std::getline(ss, token, '\n');
		presets[i].fat = std::stof(token);
		std::getline(ss, token, '\n');
		presets[i].drive = std::stof(token);
		std::getline(ss, token, '\n');
		presets[i].dry = std::stof(token);
		std::getline(ss, token, '\n');
		presets[i].stereo = std::stof(token);
		std::getline(ss, token, '\n');
		presets[i].gain = std::stof(token);
	}
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new FmerAudioProcessor();
}

AudioProcessorValueTreeState::ParameterLayout
	FmerAudioProcessor::createParameters()
{
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat>("freq","Frequency",0.0f,1.0f,0.3f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("fat","Fatness",-20.0f,20.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("drive","Drive",0.0f,1.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("dry","Dry",0.0f,1.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("stereo","Stereo",0.0f,1.0f,0.3f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("gain","Out Gain",0.0f,1.0f,0.4f));
	return { parameters.begin(), parameters.end() };
}
