#include "processor.h"
#include "editor.h"

MPaintAudioProcessor::MPaintAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withOutput("Output", AudioChannelSet::stereo(), true), &apvts) {

	init();

	sound = (apvts.getParameter("sound")->getValue())*14;
	limit = (apvts.getParameter("limit")->getValue())>.5;
	add_listener("sound");
	add_listener("limit");

	formatmanager.registerBasicFormats();
	error = false;
	for(int i = 0; i < 15; i++) {
		synth[i].setNoteStealingEnabled(true);

		for(int v = 0; v < numvoices; v++)
			synth[i].addVoice(new SamplerVoice());
	}

	futurevoid = std::async(std::launch::async, [this] {
		File dir = File::getSpecialLocation(File::currentApplicationFile);
		File file = dir;
		String name = "";
		for(int i = 0; i < 15; ++i) {
			for(int n = 59; n <= 79; ++n) {
				if(loaded.get()) return;
				name = ((String)i).paddedLeft('0',2)+"_"+((String)n).paddedLeft('0',2)+".wav";
				file = file.getSiblingFile(name);
				if(!file.existsAsFile()) {
					dir = File::getSpecialLocation(File::currentApplicationFile);
					if(!dir.isDirectory()) dir = dir.getParentDirectory();
					for(int f = 0; f < 5; f++) {
						file = dir.getChildFile("MPaint samples/"+name);
						if(file.existsAsFile()) break;
						file = dir.getChildFile("MPaint/"+name);
						if(file.existsAsFile()) break;
						file = dir.getChildFile(name);
						if(file.existsAsFile()) break;
						if(dir.isRoot()) break;
						dir = dir.getParentDirectory();
					}
				}
				if(file.existsAsFile()) {

					std::unique_ptr<AudioFormatReader> formatreader(formatmanager.createReaderFor(file));
					if(formatreader != nullptr){
						BigInteger range;
						range.setRange(n,1,true);
						synth[i].addSound(new SamplerSound((String)n, *formatreader, range, n, 0, 0.008f, 1));

					} else error = true;
				} else error = true;
			}
		}
		loaded = true;
	});
}

MPaintAudioProcessor::~MPaintAudioProcessor() {
	loaded = true;
	close();
}

const String MPaintAudioProcessor::getName() const { return "MPaint"; }
bool MPaintAudioProcessor::acceptsMidi() const { return true; }
bool MPaintAudioProcessor::producesMidi() const { return false; }
bool MPaintAudioProcessor::isMidiEffect() const { return false; }
double MPaintAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MPaintAudioProcessor::getNumPrograms() { return 15; }
int MPaintAudioProcessor::getCurrentProgram() { return sound.get(); }
void MPaintAudioProcessor::setCurrentProgram(int index) {
	apvts.getParameter("sound")->setValueNotifyingHost(index/14.f);
}
const String MPaintAudioProcessor::getProgramName(int index) {
	const char* g[15] = {"Mario","Mushroom","Yoshi","Star","Flower","Gameboy","Dog","Cat","Pig","Swan","Face","Plane","Boat","Car","Heart"};
	return g[index];
}
void MPaintAudioProcessor::changeProgramName(int index, const String& newName) { }

void MPaintAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	samplerate = sampleRate;

	for(int i = 0; i < 15; i++)
		synth[i].setCurrentPlaybackSampleRate(samplerate);
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
	buffer.clear();

	MidiBuffer coolerbuffer;
	if(timetillnoteplay >= 0 && timetillnoteplay < buffer.getNumSamples())
		for(int v = 0; v < 3; v++)
			if(voicecycle[v] > 10 && voiceheld[v])
				coolerbuffer.addEvent(MidiMessage::noteOn(1,voicecycle[v], 1.f), timetillnoteplay);

	MidiBuffer prevsoundbuffer;
	for(const MidiMessageMetadata metadata : midiMessages) {
		MidiMessage message = metadata.getMessage();
		int time = metadata.samplePosition;

		if(message.isNoteOn() && message.getNoteNumber() >= 59 && message.getNoteNumber() <= 79) {
			currentvoice = (currentvoice+1)%3;

			int timeoffset = 0;
			if(limit.get()) {
				int timesincelastevent = time-prevtime;
				if(timesincelastevent > (samplerate*.1f)) { //monophonic
					timeoffset = time-(int)round(samplerate*.019);
					coolerbuffer.addEvent(MidiMessage::allNotesOff(1),fmax(timeoffset,0));
					prevsoundbuffer.addEvent(MidiMessage::allNotesOff(1),fmax(timeoffset, 0));
					for(int v = 0; v < 3; v++) {
						voicecycle[v] = 0;
						voiceheld[v] = false;
					}
					//if(timesincelastevent > samplerate) timeoffset = 0;
					timeoffset = fmin(timeoffset,0);

				} else if(voicecycle[currentvoice] > 10) { //3 voice poly
					timeoffset = fmax(time-(int)round(samplerate*.019),0);
					if(voiceheld[currentvoice] && timetillnoteplay > timeoffset && timetillnoteplay < time) timeoffset = timetillnoteplay+1;
					if(!voiceheld[currentvoice] || timetillnoteplay < timeoffset) {
						MidiMessage hi = MidiMessage::noteOff(1,voicecycle[currentvoice]);
						coolerbuffer.addEvent(hi,timeoffset);
						prevsoundbuffer.addEvent(hi,timeoffset);
						voiceheld[currentvoice] = false;
					}
					timeoffset = 0;
				}

			}

			int notetime = fmax(timetillnoteplay, time-timeoffset);
			if(notetime >= buffer.getNumSamples()) {
				if(timetillnoteplay < notetime) {
					for(int v = 0; v < 3; v++) voiceheld[v] = false;
					timetillnoteplay = notetime;
				}
				voiceheld[currentvoice] = true;
			} else {
				coolerbuffer.addEvent(MidiMessage::noteOn(1,message.getNoteNumber(),1.f),notetime);
				voiceheld[currentvoice] = false;
			}

			voicecycle[currentvoice] = message.getNoteNumber();
			prevtime = time;

		} else if(message.isNoteOff() && !limit.get()) {
			message.setVelocity(1.f);
			message.setChannel(1);
			coolerbuffer.addEvent(message,time);
			prevsoundbuffer.addEvent(message,time);
		}
	}
	prevtime -= buffer.getNumSamples();
	timetillnoteplay -= buffer.getNumSamples();

	if(timesincesoundswitch > 0) {
		synth[prevsound].renderNextBlock(buffer, prevsoundbuffer, 0, fmin(buffer.getNumSamples(),timesincesoundswitch));
		timesincesoundswitch -= buffer.getNumSamples();
	}
	synth[sound.get()].renderNextBlock(buffer, coolerbuffer, 0, buffer.getNumSamples());
}

bool MPaintAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* MPaintAudioProcessor::createEditor() {
	return new MPaintAudioProcessorEditor(*this, sound.get());
}

void MPaintAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;
	data << version
		<< delimiter << (int)(sound.get())
		<< delimiter << (limit.get()?1:0) << delimiter;
	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void MPaintAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		apvts.getParameter("sound")->setValueNotifyingHost(std::stoi(token)/14.f);

		std::getline(ss, token, delimiter);
		apvts.getParameter("limit")->setValueNotifyingHost(std::stoi(token));

	} catch(const char* e) {
		debug((String)"Error loading saved data: "+(String)e);
	} catch(String e) {
		debug((String)"Error loading saved data: "+e);
	} catch(std::exception &e) {
		debug((String)"Error loading saved data: "+(String)e.what());
	} catch(...) {
		debug((String)"Error loading saved data");
	}
}
void MPaintAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "sound") {
		prevsound = sound.get();
		sound = newValue;
		timesincesoundswitch = samplerate;
	} else if(parameterID == "limit") limit = newValue > .5;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MPaintAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout MPaintAudioProcessor::create_parameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterInt	>(ParameterID{"sound",1},"Sound"		,0	,14	,0		,""	,tosound	,fromsound	));
	parameters.push_back(std::make_unique<AudioParameterBool>(ParameterID{"limit",1},"Limit Voices"			,true	,""	,tobool		,frombool	));
	return { parameters.begin(), parameters.end() };
}
