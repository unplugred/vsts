#include "processor.h"
#include "editor.h"

VUAudioProcessor::VUAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	pots[0] = potentiometer("Nominal"		,"nominal"	,presets[0].values[0]	,-24	,-6	,potentiometer::ptype::inttype);
	pots[1] = potentiometer("Rise speed"	,"damping"	,presets[0].values[1]	,1		,10	,potentiometer::ptype::inttype);
	pots[2] = potentiometer("Decay speed"	,"decay"	,presets[0].values[2]	,1		,10	,potentiometer::ptype::inttype);
	pots[3] = potentiometer("Stereo"		,"stereo"	,presets[0].values[3]	,0		,1	,potentiometer::ptype::booltype);

	presets[0] = pluginpreset("Default",-18,3,5,0);
	PropertiesFile* user_settings = props.getUserSettings();
	for(int i = 0; i < paramcount; ++i) {
		if(user_settings->containsKey(pots[i].id)) {
			pots[i].defaultvalue = props.getUserSettings()->getDoubleValue(pots[i].id);
			presets[0].values[i] = pots[i].defaultvalue;
		} else {
			presets[0].values[i] = pots[i].inflate(apvts.getParameter(pots[i].id)->getValue());
		}
		add_listener(pots[i].id);
	}

	for(int i = 1; i < getNumPrograms(); i++) {
		presets[i] = presets[0];
		presets[i].name = "Program "+(String)(i);
	}
}
VUAudioProcessor::~VUAudioProcessor() {
	close();
}

const String VUAudioProcessor::getName() const { return "VU"; }
bool VUAudioProcessor::acceptsMidi() const { return false; }
bool VUAudioProcessor::producesMidi() const { return false; }
bool VUAudioProcessor::isMidiEffect() const { return false; }
double VUAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int VUAudioProcessor::getNumPrograms() { return 20; }
int VUAudioProcessor::getCurrentProgram() { return currentpreset; }
void VUAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;

	for(int i = 0; i < paramcount; i++)
		apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(presets[index].values[i]));
}
const String VUAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void VUAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void VUAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {}
void VUAudioProcessor::releaseResources() {}

bool VUAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	int numChannels = layouts.getMainInputChannels();
	return (numChannels > 0 && numChannels <= 2);
}

void VUAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	int channelnum = getTotalNumInputChannels();

	float leftrms = 0, rightrms = 0, newleftpeak = leftpeak.get(), newrightpeak = rightpeak.get();
	for(int channel = 0; channel < fmin(channelnum,2); ++channel) {
		const float* channelData = buffer.getReadPointer(channel);
		for(int sample = 0; sample < buffer.getNumSamples(); ++sample) {
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
	else for(int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	buffercount = buffercount.get()+1;

	if(channelnum == 1) {
		float output = leftvu.get()+leftrms*leftrms;
		leftvu = output;
		rightvu = output;
		leftpeak = newleftpeak;
		rightpeak = newleftpeak;
	} else if(presets[currentpreset].values[3]>.5) {
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

bool VUAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* VUAudioProcessor::createEditor() {
	return new VUAudioProcessorEditor(*this,paramcount,presets[currentpreset],pots);
}

void VUAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;
	data << version << delimiter
		<< height.get() << delimiter
		<< settingsx.get() << delimiter
		<< settingsy.get() << delimiter
		<< (settingsopen.get()?1:0) << delimiter
		<< currentpreset << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << delimiter;
		for(int v = 0; v < paramcount; v++)
			data << presets[i].values[v] << delimiter;
	}

	MemoryOutputStream stream(destData,false);
	stream.writeString(data.str());
}
void VUAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss,token,delimiter);
		int saveversion = std::stoi(token);

		if(saveversion >= 2) {
			std::getline(ss,token,delimiter);
			height = std::stoi(token);

			if(saveversion >= 3) {
				std::getline(ss, token, delimiter);
				settingsx = std::stoi(token);

				std::getline(ss, token, delimiter);
				settingsy = std::stoi(token);

				std::getline(ss, token, delimiter);
				settingsopen = std::stoi(token)>.5;
			}

			std::getline(ss,token,delimiter);
			currentpreset = std::stoi(token);

			for(int i = 0; i < getNumPrograms(); i++) {
				std::getline(ss, token, delimiter);
				presets[i].name = token;
				for(int v = 0; v < paramcount; v++) {
					if(saveversion < 3 && v == 2) {
						presets[i].values[2] = presets[i].values[1];
						continue;
					}
					std::getline(ss, token, delimiter);
					presets[i].values[v] = std::stof(token);
				}
			}
		} else {
			for(int v = 0; v < paramcount; v++) {
				if(v == 2) {
					presets[currentpreset].values[2] = presets[currentpreset].values[1];
					continue;
				}
				std::getline(ss, token, delimiter);
				presets[currentpreset].values[v] = std::stof(token);
			}

			std::getline(ss,token,delimiter);
			height = std::stoi(token);
		}
	} catch(const char* e) {
		debug((String)"Error loading saved data: "+(String)e);
	} catch(String e) {
		debug((String)"Error loading saved data: "+e);
	} catch(std::exception &e) {
		debug((String)"Error loading saved data: "+(String)e.what());
	} catch(...) {
		debug((String)"Error loading saved data");
	}

	for(int i = 0; i < paramcount; i++) {
		apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(presets[currentpreset].values[i]));
	}
}
void VUAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < paramcount; i++) if(parameterID == pots[i].id) {
		presets[currentpreset].values[i] = newValue;
		return;
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VUAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout VUAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterInt	>(ParameterID{"nominal"	,1},"Nominal"		,-24,-6	,-18	,"",todb		,fromdb		));
	parameters.push_back(std::make_unique<AudioParameterInt	>(ParameterID{"damping"	,1},"Rise speed"	,1	,10	,3									));
	parameters.push_back(std::make_unique<AudioParameterInt	>(ParameterID{"decay"	,1},"Decay speed"	,1	,10	,5									));
	parameters.push_back(std::make_unique<AudioParameterBool>(ParameterID{"stereo"	,1},"Stereo"				,false	,"",tostereo	,fromstereo	));
	return { parameters.begin(), parameters.end() };
}
