/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

VuAudioProcessor::VuAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	//SystemStats::setApplicationCrashHandler(crashhandler);
}
/*
void VuAudioProcessor::crashhandler(void*) {
	File::getCurrentWorkingDirectory().getChildFile("digital_tombstone.txt").replaceWithText(SystemStats::getStackBacktrace());
}
*/
VuAudioProcessor::~VuAudioProcessor() {}

const String VuAudioProcessor::getName() const { return "VU"; }
bool VuAudioProcessor::acceptsMidi() const { return false; }
bool VuAudioProcessor::producesMidi() const { return false; }
bool VuAudioProcessor::isMidiEffect() const { return false; }
double VuAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int VuAudioProcessor::getNumPrograms() { return 1; }
int VuAudioProcessor::getCurrentProgram() { return 0; }
void VuAudioProcessor::setCurrentProgram (int index) {} 
const String VuAudioProcessor::getProgramName (int index) { return ":)"; }
void VuAudioProcessor::changeProgramName (int index, const String& newName) {}

void VuAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {}
void VuAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VuAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
		return false;

	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void VuAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;
	auto totalNumInputChannels	= getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float leftrms = 0, rightrms = 0, newleftpeak = leftpeak.get(), newrightpeak = rightpeak.get();
	for (int channel = 0; channel < fmin(totalNumInputChannels,2); ++channel)
	{
		auto* channelData = buffer.getWritePointer (channel);
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
			float data = channelData[sample];
			data *= data;
			if(channel == 0) {
				leftrms += data;
				newleftpeak = newleftpeak || fabs(channelData[sample]) >= .999;
			} else {
				rightrms += data;
				newrightpeak = newrightpeak || fabs(channelData[sample]) >= .999;
			}
			if(JUCEApplication::isStandaloneApp()) channelData[sample] = 0;
		}
		if(channel == 0)
			leftrms = std::sqrt(leftrms/buffer.getNumSamples());
		else
			rightrms = std::sqrt(rightrms/buffer.getNumSamples());
	}
	buffercount = buffercount.get()+1;
	if(totalNumInputChannels == 1) {
		float output = leftvu.get()+leftrms*leftrms;
		leftvu = output;
		rightvu = output;
		leftpeak = newleftpeak;
		rightpeak = newleftpeak;
	} else if(stereo.get()) {
		leftvu = leftvu.get()+leftrms*leftrms;
		rightvu = rightvu.get()+rightrms*rightrms;
		leftpeak = newleftpeak;
		rightpeak = newrightpeak;
	} else {
		float centervu = std::sqrt((leftrms*leftrms+rightrms*rightrms)*.5f);
		leftvu = leftvu.get()+centervu*centervu;
		leftpeak = newleftpeak || newrightpeak;
	}
}

bool VuAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* VuAudioProcessor::createEditor() { return new VuAudioProcessorEditor (*this); }

void VuAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version
		<< linebreak << nominal.get()
		<< linebreak << damping.get()
		<< linebreak << (stereo.get()?1:0)
		<< linebreak << height.get()
		<< linebreak;
	MemoryOutputStream stream(destData,false);
	stream.writeString(data.str());
}

void VuAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
	std::string token;

	std::getline(ss,token,'\n');
	int saveversion = std::stoi(token);

	std::getline(ss,token,'\n');
	nominal = std::stoi(token);
	apvts.getParameter("nominal")->setValueNotifyingHost((std::stof(token)+24)/18);
	
	std::getline(ss,token,'\n');
	damping = std::stoi(token);
	apvts.getParameter("damping")->setValueNotifyingHost((std::stof(token)-1)/8);

	std::getline(ss,token,'\n');
	stereo = std::stoi(token) == 1;
	apvts.getParameter("stereo")->setValueNotifyingHost(std::stof(token));

	std::getline(ss,token,'\n');
	height = std::stoi(token);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VuAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout
	VuAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterInt>("nominal","Nominal",-24,-6,-18));
	parameters.push_back(std::make_unique<AudioParameterInt>("damping","Damping",1,9,5));
	parameters.push_back(std::make_unique<AudioParameterBool>("stereo","Stereo",false));
	return{parameters.begin(),parameters.end()};
}
