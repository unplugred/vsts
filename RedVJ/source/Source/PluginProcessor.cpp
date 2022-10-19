#include "PluginProcessor.h"
#include "PluginEditor.h"

RedVJAudioProcessor::RedVJAudioProcessor() :
	 AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output", AudioChannelSet::stereo(),true)),
	apvts(*this,&undoManager,"Parameters",createParameters()),
	forwardFFT(fftOrder), window(fftSize, dsp::WindowingFunction<float>::hann) {
	for(int i = 1; i <= 16; i++) apvts.addParameterListener("p"+i,this);
}
RedVJAudioProcessor::~RedVJAudioProcessor() {
	for(int i = 1; i <= 16; i++) apvts.removeParameterListener("p"+i,this);
}

const String RedVJAudioProcessor::getName() const { return "RedVJ"; }
bool RedVJAudioProcessor::acceptsMidi() const { return true; }
bool RedVJAudioProcessor::producesMidi() const { return false; }
bool RedVJAudioProcessor::isMidiEffect() const { return false; }
double RedVJAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int RedVJAudioProcessor::getNumPrograms() { return 1; }
int RedVJAudioProcessor::getCurrentProgram() { return 0; }
void RedVJAudioProcessor::setCurrentProgram (int index) {}
const String RedVJAudioProcessor::getProgramName (int index) { return ":)"; }
void RedVJAudioProcessor::changeProgramName (int index, const String& newName) {}

void RedVJAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) { }
void RedVJAudioProcessor::releaseResources() {}

bool RedVJAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if(layouts.getMainInputChannelSet() != layouts.getMainInputChannelSet())
		return false;
	return (layouts.getMainInputChannels() > 0);
}

void RedVJAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;
	auto totalNumInputChannels	= getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	if(dofft.get()) {
		const float* channelDataL = buffer.getReadPointer (0);
		const float* channelDataR;
		if(getTotalNumInputChannels() >= 2)
			channelDataR = buffer.getReadPointer (1);
		
		for(int i = 0; i < buffer.getNumSamples(); i++) {
			if(getTotalNumInputChannels() >= 2)
				pushNextSampleIntoFifo((channelDataL[i]+channelDataR[i])*.5f);
			else
				pushNextSampleIntoFifo(channelDataL[i]);
		}
	}
}
void RedVJAudioProcessor::pushNextSampleIntoFifo(float sample) noexcept {
	if(fifoIndex == fftSize) {
		if(!nextFFTBlockReady.get()) {
			zeromem(fftData,sizeof(fftData));
			memcpy(fftData,fifo,sizeof(fifo));
			nextFFTBlockReady = true;
		}

		fifoIndex = 0;
	}
	fifo[fifoIndex++] = sample;
}
void RedVJAudioProcessor::drawNextFrameOfSpectrum() {
	window.multiplyWithWindowingTable(fftData,fftSize);
	forwardFFT.performFrequencyOnlyForwardTransform(fftData);

	float mindB = -100.f;
	float maxdB = 0.f;
	float fundstrength = 0.f;

	for(int i = 0; i < scopeSize; ++i) {
		auto skewedProportionX = 1.f-std::exp(std::log(1.f-(float)i/(float)scopeSize)*.2f);
		auto fftDataIndex = jlimit(0,fftSize/2,(int)(skewedProportionX*(float)fftSize*.5f));
		scopeData[i] = jmap(jlimit(mindB,maxdB,Decibels::gainToDecibels(fftData[fftDataIndex])-Decibels::gainToDecibels((float)fftSize)),mindB,maxdB,.0f,1.f);
	}
}

bool RedVJAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* RedVJAudioProcessor::createEditor() { return new RedVJAudioProcessorEditor (*this); }

void RedVJAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version
		<< linebreak << presetpath;

	for(int i = 0; i < 16; i++) {
		data
			<< linebreak << inparameters[i].name
			<< linebreak << inparameters[i].value
			<< linebreak << inparameters[i].midimapping;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void RedVJAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
	std::string token;

	std::getline(ss, token, '\n');
	int saveversion = std::stoi(token);

	std::getline(ss, token, '\n');
	presetpath = token;

	for(int i = 0; i < 16; i++) {
		std::getline(ss, token, '\n');
		inparameters[i].name = token;
		std::getline(ss, token, '\n');
		apvts.getParameter("p"+(String)(i+1))->setValueNotifyingHost(stof(token));
		std::getline(ss, token, '\n');
		inparameters[i].midimapping = token;
	}
}
void RedVJAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < 16; i++) {
		if (parameterID == ("p"+(i+1))) {
			inparameters[i].value = newValue;
			return;
		}
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RedVJAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout RedVJAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat>("p1"	,"Parameter01",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p2"	,"Parameter02",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p3"	,"Parameter03",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p4"	,"Parameter04",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p5"	,"Parameter05",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p6"	,"Parameter06",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p7"	,"Parameter07",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p8"	,"Parameter08",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p9"	,"Parameter09",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p10","Parameter10",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p11","Parameter11",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p12","Parameter12",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p13","Parameter13",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p14","Parameter14",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p15","Parameter15",juce::NormalisableRange<float>(0.f,1.f),.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("p16","Parameter16",juce::NormalisableRange<float>(0.f,1.f),.5f));
	return { parameters.begin(), parameters.end() };
}
