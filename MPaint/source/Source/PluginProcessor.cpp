#include "PluginProcessor.h"
#include "PluginEditor.h"

MPaintAudioProcessor::MPaintAudioProcessor() :
	AudioProcessor(BusesProperties().withOutput("Output", AudioChannelSet::stereo(), true)),
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	sound = (apvts.getParameter("sound")->getValue())*14;
	limit = (apvts.getParameter("limit")->getValue())>.5;
	apvts.addParameterListener("sound", this);
	apvts.addParameterListener("limit", this);

	formatmanager.registerBasicFormats();
	for(int i = 0; i < 15; i++) {
		for(int v = 0; v < numvoices; v++)
			synth[i].addVoice(new SamplerVoice());

		for(int n = 59; n <= 79; n++) {
			formatreader = formatmanager.createReaderFor(File::getCurrentWorkingDirectory().getChildFile("MPaint/" + ((String)i).paddedLeft('0',2) + "_" + ((String)n).paddedLeft('0',2) + ".wav"));
			BigInteger range;
			range.setRange(n,1,true);
			synth[i].addSound(new SamplerSound((String)n, *formatreader, range, n, 0, 0, 10));
		}
	}
}

MPaintAudioProcessor::~MPaintAudioProcessor() {
	apvts.removeParameterListener("sound", this);
	apvts.removeParameterListener("limit", this);

	formatreader = nullptr;
}

const String MPaintAudioProcessor::getName() const { return "MPaint"; }
bool MPaintAudioProcessor::acceptsMidi() const { return true; }
bool MPaintAudioProcessor::producesMidi() const { return false; }
bool MPaintAudioProcessor::isMidiEffect() const { return false; }
double MPaintAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MPaintAudioProcessor::getNumPrograms() { return 1; }
int MPaintAudioProcessor::getCurrentProgram() { return 0; }
void MPaintAudioProcessor::setCurrentProgram(int index) { }
const String MPaintAudioProcessor::getProgramName(int index) { return "Hi :)"; }
void MPaintAudioProcessor::changeProgramName(int index, const String& newName) { }

void MPaintAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	for(int i = 0; i < 15; i++)
		synth[i].setCurrentPlaybackSampleRate(samplerate);
}
void MPaintAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	channelData.clear();
	for (int i = 0; i < channelnum; i++)
		channelData.push_back(nullptr);
}
void MPaintAudioProcessor::releaseResources() { }

bool MPaintAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	int numChannels = layouts.getMainInputChannels();
	return(numChannels > 0 && numChannels <= 2);
}

void MPaintAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());

	buffer.clear();

	int numsamples = buffer.getNumSamples();
	for(int channel = 0; channel < channelnum; ++channel)
		channelData[channel] = buffer.getWritePointer(channel);

	synth[sound.get()].renderNextBlock(buffer, midiMessages, 0, numsamples);
	/*
	for(int sample = 0; sample < numsamples; ++sample) {
		for(int channel = 0; channel < channelnum; ++channel) {
		}
	}
	*/
}

bool MPaintAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* MPaintAudioProcessor::createEditor() {
	return new MPaintAudioProcessorEditor(*this, sound.get());
}

void MPaintAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version
		<< linebreak << (int)(sound.get())
		<< linebreak << (limit.get()?1:0) << linebreak;
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void MPaintAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, '\n');
		int saveversion = std::stoi(token);

		std::getline(ss, token, '\n');
		apvts.getParameter("sound")->setValueNotifyingHost(std::stoi(token)/14.f);

		std::getline(ss, token, '\n');
		apvts.getParameter("limit")->setValueNotifyingHost(std::stoi(token));

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
void MPaintAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "sound") sound = newValue;
	else if(parameterID == "limit") limit = newValue > .5;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MPaintAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout MPaintAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterInt	>("sound","Sound"		, 0, 14, 0));
	parameters.push_back(std::make_unique<AudioParameterBool>("limit","Limit Voices", true));
	return { parameters.begin(), parameters.end() };
}
