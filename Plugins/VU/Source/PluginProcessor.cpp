#include "PluginProcessor.h"
#include "PluginEditor.h"

VuAudioProcessor::VuAudioProcessor() :
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	for(int i = 0; i < getNumPrograms(); i++) {
		presets[i] = pluginpreset("Program "+(String)(i+1),-18,5,0);
	}

	pots[0] = potentiometer("Nominal"	,"nominal"	,presets[0].values[0]	,-24	,-6	,potentiometer::ptype::inttype);
	pots[1] = potentiometer("Damping"	,"damping"	,presets[0].values[1]	,1		,9	,potentiometer::ptype::inttype);
	pots[2] = potentiometer("Stereo"	,"stereo"	,presets[0].values[2]	,0		,1	,potentiometer::ptype::booltype);

	for(int i = 0; i < paramcount; i++) {
		presets[currentpreset].values[i] = pots[i].inflate(apvts.getParameter(pots[i].id)->getValue());
		apvts.addParameterListener(pots[i].id, this);
	}
}
VuAudioProcessor::~VuAudioProcessor() {
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(pots[i].id, this);
}

const String VuAudioProcessor::getName() const { return "VU"; }
bool VuAudioProcessor::acceptsMidi() const { return false; }
bool VuAudioProcessor::producesMidi() const { return false; }
bool VuAudioProcessor::isMidiEffect() const { return false; }
double VuAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int VuAudioProcessor::getNumPrograms() { return 20; }
int VuAudioProcessor::getCurrentProgram() { return currentpreset; }
void VuAudioProcessor::setCurrentProgram (int index) {
	if(currentpreset == index) return;

	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;

	for(int i = 0; i < paramcount; i++)
		apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(presets[index].values[i]));
}
const String VuAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void VuAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
}

void VuAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {}
void VuAudioProcessor::releaseResources() {}

bool VuAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	int numChannels = layouts.getMainInputChannels();
	return (numChannels > 0 && numChannels <= 2);
}

void VuAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	int channelnum = getTotalNumInputChannels();

	float leftrms = 0, rightrms = 0, newleftpeak = leftpeak.get(), newrightpeak = rightpeak.get();
	for (int channel = 0; channel < fmin(channelnum,2); ++channel)
	{
		const float* channelData = buffer.getReadPointer(channel);
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
		}
		if(channel == 0)
			leftrms = std::sqrt(leftrms/buffer.getNumSamples());
		else
			rightrms = std::sqrt(rightrms/buffer.getNumSamples());
	}

	if(JUCEApplication::isStandaloneApp()) buffer.clear();
	else for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	buffercount = buffercount.get()+1;

	if(channelnum == 1) {
		float output = leftvu.get()+leftrms*leftrms;
		leftvu = output;
		rightvu = output;
		leftpeak = newleftpeak;
		rightpeak = newleftpeak;
	} else if(presets[currentpreset].values[2]>.5) {
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
AudioProcessorEditor* VuAudioProcessor::createEditor() {
	return new VuAudioProcessorEditor(*this,paramcount,presets[currentpreset],pots);
}

void VuAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version << linebreak
		<< height.get() << linebreak
		<< currentpreset << linebreak;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << linebreak;
		for(int v = 0; v < paramcount; v++)
			data << presets[i].values[v] << linebreak;
	}

	MemoryOutputStream stream(destData,false);
	stream.writeString(data.str());
}
void VuAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss,token,'\n');
		int saveversion = std::stoi(token);

		if(saveversion >= 2) {
			std::getline(ss,token,'\n');
			height = std::stoi(token);

			std::getline(ss,token,'\n');
			currentpreset = std::stoi(token);

			for(int i = 0; i < getNumPrograms(); i++) {
				std::getline(ss, token, '\n');
				presets[i].name = token;
				for(int v = 0; v < paramcount; v++) {
					std::getline(ss, token, '\n');
					presets[i].values[v] = std::stof(token);
				}
			}
		} else {
			for(int i = 0; i < 3; i++) {
				std::getline(ss, token, '\n');
				presets[currentpreset].values[i] = std::stof(token);
			}

			std::getline(ss,token,'\n');
			height = std::stoi(token);
		}
	} catch (const char* e) {
		logger.debug((String)"Error loading saved data: "+(String)e);
	} catch(String e) {
		logger.debug((String)"Error loading saved data: "+e);
	} catch(std::exception &e) {
		logger.debug((String)"Error loading saved data: "+(String)e.what());
	} catch(...) {
		logger.debug((String)"Error loading saved data");
	}

	for(int i = 0; i < paramcount; i++) {
		apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(presets[currentpreset].values[i]));
	}
}
void VuAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; i++) if(parameterID == pots[i].id) {
		presets[currentpreset].values[i] = newValue;
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VuAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout VuAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterInt>("nominal","Nominal",-24,-6,-18));
	parameters.push_back(std::make_unique<AudioParameterInt>("damping","Damping",1,9,5));
	parameters.push_back(std::make_unique<AudioParameterBool>("stereo","Stereo",false));
	return{parameters.begin(),parameters.end()};
}
