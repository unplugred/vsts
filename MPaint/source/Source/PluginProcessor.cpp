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
	error = false;
	for(int i = 0; i < 15; i++) {
		synth[i].setNoteStealingEnabled(true);

		for(int v = 0; v < numvoices; v++)
			synth[i].addVoice(new SamplerVoice());

		for(int n = 59; n <= 79; n++) {
			File dir = File::getSpecialLocation(File::currentApplicationFile);
			if(!dir.isDirectory()) dir = dir.getParentDirectory();
			File file = dir.getChildFile("MPaint/" + ((String)i).paddedLeft('0',2) + "_" + ((String)n).paddedLeft('0',2) + ".wav");
			if(!file.existsAsFile()) file = dir.getChildFile(((String)i).paddedLeft('0',2) + "_" + ((String)n).paddedLeft('0',2) + ".wav");
			if(file.existsAsFile()) {

				std::unique_ptr<AudioFormatReader> formatreader(formatmanager.createReaderFor(file));
				if(formatreader == nullptr) error = true;
				else {
					BigInteger range;
					range.setRange(n,1,true);
					synth[i].addSound(new SamplerSound((String)n, *formatreader, range, n, 0, 0.008f, 1));
				}
			} else error = true;
		}
	}
}

MPaintAudioProcessor::~MPaintAudioProcessor() {
	apvts.removeParameterListener("sound", this);
	apvts.removeParameterListener("limit", this);
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
	samplesperblock = samplesPerBlock;
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
	timetillnoteplay -= samplesperblock;
	if(timetillnoteplay >= 0 && timetillnoteplay < samplesperblock)
		for(int v = 0; v < 3; v++)
			if(voicecycle[v] > 10 && voiceheld[v])
				coolerbuffer.addEvent(MidiMessage::noteOn(1,voicecycle[v], 1.f), timetillnoteplay);

	MidiBuffer prevsoundbuffer;
	MidiMessage message(0xf0);
	MidiBuffer::Iterator i(midiMessages);
	int time = 0;
	while(i.getNextEvent(message, time)) {
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
			if(notetime >= samplesperblock) {
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

			/*
			situation 1: monophonic (first note) small buffer size
			time till note play below zero;
			time since last event large;
			enters monophonic mode. all notes off sent;
			time offset set to 0 as no notes played previously;
			note is played immediately;
			 
			situation 2: monophobic (cutoff) small buffer size
			time till note play blow zero;
			time since last event event in about half a second;
			enters monophonic mode;
			all notes off sent;
			time offset is set to about 10 milliseconds;
			note time set to after the buffer size;
			time till note play is set to note time;
			note is played at a 19ms delay;

			sutuation 3: monophonic (first note) large buffer size
			same as 1;

			situation 4: monophonic (cutoff) large buffer size
			time till note play blow zero;
			time since last event event in about half a second;
			enters monophonic mode;
			all notes off sent;
			time offset is set to about 10 milliseconds;
			note time set to before the buffer size;
			note is played at a 19ms delay;


			situation 5: polyphonic (first note) small buffer size
			
			situation 6: polyphobic (more notes) small buffer size

			situation 7: polyphobic (out of voices) small buffer size


			sutuation 8: polyphonic (first note) large buffer size

			situation 9: polyphonic (more notes) large buffer size

			situation 9: polyphonic (out of voices) large buffer size
			*/
		} else if(message.isNoteOff() && !limit.get()) {
			message.setVelocity(1.f);
			message.setChannel(1);
			coolerbuffer.addEvent(message,time);
			prevsoundbuffer.addEvent(message,time);
		}
	}
	prevtime -= samplesperblock;

	if(timesincesoundswitch > 0) {
		synth[prevsound].renderNextBlock(buffer, prevsoundbuffer, 0, fmin(samplesperblock,timesincesoundswitch));
		timesincesoundswitch -= samplesperblock;
	}
	synth[sound.get()].renderNextBlock(buffer, coolerbuffer, 0, samplesperblock);
}

bool MPaintAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* MPaintAudioProcessor::createEditor() {
	return new MPaintAudioProcessorEditor(*this, sound.get(), error);
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
	if(parameterID == "sound") {
		prevsound = sound.get();
		sound = newValue;
		timesincesoundswitch = samplerate;
	} else if(parameterID == "limit") limit = newValue > .5;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MPaintAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout MPaintAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterInt	>("sound","Sound"		, 0, 14, 0));
	parameters.push_back(std::make_unique<AudioParameterBool>("limit","Limit Voices", true));
	return { parameters.begin(), parameters.end() };
}
