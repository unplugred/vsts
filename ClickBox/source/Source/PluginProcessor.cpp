/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

ClickBoxAudioProcessor::ClickBoxAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor (BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
#endif
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	apvts.addParameterListener("x",this);
	apvts.addParameterListener("y",this);
	apvts.addParameterListener("intensity",this);
	apvts.addParameterListener("amount",this);
	apvts.addParameterListener("stereo",this);
	apvts.addParameterListener("sidechain",this);
	apvts.addParameterListener("dry",this);
	apvts.addParameterListener("auto",this);
	apvts.addParameterListener("override",this);
	prlin.init();
}
ClickBoxAudioProcessor::~ClickBoxAudioProcessor() {
	apvts.removeParameterListener("x", this);
	apvts.removeParameterListener("y", this);
	apvts.removeParameterListener("intensity", this);
	apvts.removeParameterListener("amount", this);
	apvts.removeParameterListener("stereo", this);
	apvts.removeParameterListener("sidechain", this);
	apvts.removeParameterListener("dry", this);
	apvts.removeParameterListener("auto", this);
	apvts.removeParameterListener("override", this);
}

const String ClickBoxAudioProcessor::getName() const { return "ClickBox"; }
bool ClickBoxAudioProcessor::acceptsMidi() const { return false; }
bool ClickBoxAudioProcessor::producesMidi() const { return false; }
bool ClickBoxAudioProcessor::isMidiEffect() const { return false; }
double ClickBoxAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ClickBoxAudioProcessor::getNumPrograms() { return 1; }
int ClickBoxAudioProcessor::getCurrentProgram() { return 0; }
void ClickBoxAudioProcessor::setCurrentProgram(int index) { }
const String ClickBoxAudioProcessor::getProgramName(int index) { return ":)"; }
void ClickBoxAudioProcessor::changeProgramName(int index, const String& newName) { }

void ClickBoxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) { }
void ClickBoxAudioProcessor::releaseResources() { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool ClickBoxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
		return false;

	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return true;
}
#endif

void ClickBoxAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	float olddi = oldi.get();

	time += buffer.getNumSamples() * .00001f;
	float newx = xval, newy = yval;
	float compensation = 0;
	if (!overridee) {
		newx = prlin.noise(time,0)*.5f+.5f;
		newy = prlin.noise(0,time)*.5f+.5f;
		compensation = abs(oldautomod-automod)*sqrt((newx-xval)*(newx-xval)+(newy-yval)*(newy-yval));
		newx = newx*automod+xval*(1-automod);
		newy = newx*automod+yval*(1-automod);
	}

	float i = olddi;
	if (overridee == oldoverride) {
		float xdiff = abs(x.get()-newx);
		float ydiff = abs(y.get()-newy);
		i = (((sqrt(xdiff*xdiff+ydiff*ydiff)-compensation)*120)/buffer.getNumSamples())*amount*amount;
	}

	if(sidechain) {
		if (totalNumInputChannels < 2)
			i *= buffer.getRMSLevel(0,0,buffer.getNumSamples());
		else {
			float l = buffer.getRMSLevel(0,0,buffer.getNumSamples());
			float r = buffer.getRMSLevel(1,0,buffer.getNumSamples());
			i *= sqrt(l*l+r*r);
		}
		i *= 2;
	}

	float unit = 1.f/buffer.getNumSamples();
	float* channelDataL = buffer.getWritePointer(0);
	float* channelDataR;
	if (totalNumInputChannels >= 2) channelDataR = buffer.getWritePointer(1);
	for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
		float mult = unit*sample;
		float lerpi = olddi*(1-mult)+i*mult,
			lerpintensity = (oldintensity*(1-mult)+intensity*mult)*2,
			lerpstereo = oldstereo*(1-mult)+stereo*mult,
			lerpdry = olddry*(1-mult)+dry*mult;
		bool left = false;
		bool right = false;

		//click happened
		if(random.nextFloat() < lerpi) {
			//input is mono
			if (totalNumInputChannels < 2) {
				left = true;
			//click is stereo
			} else if(random.nextFloat() < lerpstereo) {
				if(random.nextFloat() > .5f) left = true;
				else right = true;
			//click is mono
			} else {
				left = true;
				right = true;
			}

			//create click
			float ampp = generateclick();
			if(lerpintensity < 1) {
				ampp *= lerpintensity;
				if(lerpdry > 0) {
					if(left) channelDataL[sample] = ampp+lerpdry*channelDataL[sample]*(1-lerpintensity);
					if(right) channelDataR[sample] = ampp+lerpdry*channelDataR[sample]*(1-lerpintensity);
				} else {
					if(left) channelDataL[sample] = ampp;
					if(right) channelDataR[sample] = ampp;
				}
			} else {
				ampp = ampp*(2-lerpintensity)+(ampp<0?-1:1)*(lerpintensity-1);
				if(left) channelDataL[sample] = ampp;
				if(right) channelDataR[sample] = ampp;
			}
		}

		//click did not happen
		if(!left) channelDataL[sample] *= lerpdry;
		if(totalNumInputChannels >= 2 && !right) channelDataR[sample] *= lerpdry;
	}

	oldi = i;
	x = newx;
	y = newy;
	oldintensity = intensity;
	oldstereo = stereo;
	olddry = dry;
	oldoverride = overridee;
	oldautomod = automod;
}
float ClickBoxAudioProcessor::generateclick() {
	return random.nextFloat()*2-1;
}

bool ClickBoxAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* ClickBoxAudioProcessor::createEditor() { return new ClickBoxAudioProcessorEditor (*this); }

void ClickBoxAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;
	data << version
		<< linebreak << x.get()
		<< linebreak << y.get()
		<< linebreak << intensity
		<< linebreak << amount
		<< linebreak << stereo
		<< linebreak << (sidechain?1:0)
		<< linebreak << (dry?1:0)
		<< linebreak << automod;
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void ClickBoxAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	std::stringstream ss(String::createStringFromData(data,sizeInBytes).toRawUTF8());
	std::string token;

	std::getline(ss,token,'\n');
	int saveversion = stoi(token);

	std::getline(ss,token,'\n');
	apvts.getParameter("x")->setValueNotifyingHost(stof(token));

	std::getline(ss,token,'\n');
	apvts.getParameter("y")->setValueNotifyingHost(stof(token));

	std::getline(ss,token,'\n');
	apvts.getParameter("intensity")->setValueNotifyingHost(stof(token));

	std::getline(ss,token,'\n');
	apvts.getParameter("amount")->setValueNotifyingHost(stof(token));

	std::getline(ss,token,'\n');
	apvts.getParameter("stereo")->setValueNotifyingHost(stof(token));

	std::getline(ss,token,'\n');
	apvts.getParameter("sidechain")->setValueNotifyingHost(stof(token));

	std::getline(ss,token,'\n');
	apvts.getParameter("dry")->setValueNotifyingHost(stof(token));

	std::getline(ss,token,'\n');
	apvts.getParameter("auto")->setValueNotifyingHost(stof(token));
}
void ClickBoxAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "x") xval = newValue;
	else if(parameterID == "y") yval = newValue;
	else if(parameterID == "intensity") intensity = newValue;
	else if(parameterID == "amount") amount = newValue;
	else if(parameterID == "stereo") stereo = newValue;
	else if(parameterID == "sidechain") sidechain = newValue>.5;
	else if(parameterID == "dry") dry = newValue>.5;
	else if(parameterID == "auto") automod = newValue;
	else if(parameterID == "override") overridee = newValue>.5;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ClickBoxAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout ClickBoxAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat>("x","X",0.0f,1.0f,0.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("y","Y",0.0f,1.0f,0.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("intensity","Intensity",0.0f,1.0f,0.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("amount","Amount",0.0f,1.0f,0.5f));
	parameters.push_back(std::make_unique<AudioParameterFloat>("stereo","Stereo",0.0f,1.0f,0.28f));
	parameters.push_back(std::make_unique<AudioParameterBool>("sidechain","Side-chain to dry",false));
	parameters.push_back(std::make_unique<AudioParameterBool>("dry","Dry out",true));
	parameters.push_back(std::make_unique<AudioParameterFloat>("auto","Auto",0.0f,1.0f,0.0f));
	parameters.push_back(std::make_unique<AudioParameterBool>("override","Override",false));
	return { parameters.begin(), parameters.end() };
}

