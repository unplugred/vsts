#include "PluginProcessor.h"
#include "PluginEditor.h"

CRMBLAudioProcessor::CRMBLAudioProcessor() :
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	//												,time	,sync	,modamp	,modfreq,pingpon,pingpos,feedbac,reverse,chew	,pitch	,lowpass,wet	,
	presets[0] = pluginpreset("Default"				,0.32f	,0.0f	,0.15f	,0.5f	,0.0f	,0.0f	,0.5f	,0.0f	,0.0f	,0.0f	,0.3f	,0.4	);
	presets[1] = pluginpreset("Digital Driver"		,0.27f	,-11.36f,0.53f	,0.0f	,0.0f	);
	presets[2] = pluginpreset("Noisy Bass Pumper"	,0.55f	,20.0f	,0.59f	,0.77f	,0.0f	);
	presets[3] = pluginpreset("Broken Earphone"		,0.19f	,-1.92f	,1.0f	,0.0f	,0.88f	);
	presets[4] = pluginpreset("Fatass"				,0.62f	,-5.28f	,1.0f	,0.0f	,0.0f	);
	presets[5] = pluginpreset("Screaming Alien"		,0.63f	,5.6f	,0.2f	,0.0f	,0.0f	); 
	presets[6] = pluginpreset("Bad Connection"		,0.25f	,-2.56f	,0.48f	,0.0f	,0.0f	);
	presets[7] = pluginpreset("Ouch"				,0.9f	,-20.0f	,0.0f	,0.0f	,0.69f	);

	pots[0] = potentiometer("Time (MS)"				,"time"				,.001f	,presets[0].values[0]	);
	pots[1] = potentiometer("Time (Eighth note)"	,"sync"				,0		,presets[0].values[1]	,0		,16		,true	,potentiometer::inttype);
	pots[2] = potentiometer("Mod Amount"			,"modamp"			,.001f	,presets[0].values[2]	);
	pots[3] = potentiometer("Mod Frequency"			,"modfreq"			,0		,presets[0].values[3]	);
	pots[4] = potentiometer("Ping Pong"				,"pingpong"			,.001f	,presets[0].values[4]	,-1.f	,1.f	);
	pots[5] = potentiometer("Ping Post Feedback"	,"pingpostfeedback"	,.001f	,presets[0].values[5]	,0		,1		,true	,potentiometer::booltype);
	pots[6] = potentiometer("Feedback"				,"feedback"			,.002f	,presets[0].values[6]	);
	pots[7] = potentiometer("Reverse"				,"reverse"			,.002f	,presets[0].values[7]	);
	pots[8] = potentiometer("Chew"					,"chew"			,.001f	,presets[0].values[8]	);
	pots[9] = potentiometer("Pitch"					,"pitch"			,0		,presets[0].values[9]	,-24.f	,24.f	);
	pots[10] = potentiometer("Lowpass"				,"lowpass"			,.001f	,presets[0].values[10]	);
	pots[11] = potentiometer("Dry/Wet"				,"wet"				,.002f	,presets[0].values[11]	);
	pots[12] = potentiometer("Hold"					,"hold"				,.002f	,0						,0		,1		,false	,potentiometer::booltype);
	//pots[13] = potentiometer("Randomize"			,"randomize"		,0		,0						,0		,1		,false	,potentiometer::booltype);
	pots[13] = potentiometer("Over-Sampling"		,"oversampling"		,0		,1						,0		,1		,false	,potentiometer::booltype);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = pots[i].inflate(apvts.getParameter(pots[i].id)->getValue());
		if(pots[i].smoothtime > 0) pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(pots[i].id, this);
	}
}

CRMBLAudioProcessor::~CRMBLAudioProcessor(){
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(pots[i].id, this);
}

const String CRMBLAudioProcessor::getName() const { return "CRMBL"; }
bool CRMBLAudioProcessor::acceptsMidi() const { return false; }
bool CRMBLAudioProcessor::producesMidi() const { return false; }
bool CRMBLAudioProcessor::isMidiEffect() const { return false; }
double CRMBLAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int CRMBLAudioProcessor::getNumPrograms() { return 8; }
int CRMBLAudioProcessor::getCurrentProgram() { return currentpreset; }
void CRMBLAudioProcessor::setCurrentProgram (int index) {
	if(!boot) return;

	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;
	for(int i = 0; i < paramcount; i++) if(pots[i].savedinpreset)
		lerptable[i] = pots[i].normalize(state.values[i]);

	if(lerpstage <= 0) {
		lerpstage = 1;
		startTimerHz(30);
	} else lerpstage = 1;
}
void CRMBLAudioProcessor::timerCallback() {
	lerpstage *= .64f;
	if(lerpstage < .001) {
		for(int i = 0; i < paramcount; i++) if(pots[i].savedinpreset)
			apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(presets[currentpreset].values[i]));
		lerpstage = 0;
		undoManager.beginNewTransaction();
		stopTimer();
		return;
	}
	for(int i = 0; i < paramcount; i++) if(pots[i].savedinpreset)
		lerpValue(pots[i].id, lerptable[i], pots[i].normalize(presets[currentpreset].values[i]));
}
void CRMBLAudioProcessor::lerpValue(StringRef slider, float& oldval, float newval) {
	oldval = (newval-oldval)*.36f+oldval;
	apvts.getParameter(slider)->setValueNotifyingHost(oldval);
}
const String CRMBLAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void CRMBLAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
	
	for(int i = 0; i < paramcount; i++) if(pots[i].savedinpreset)
		presets[index].values[i] = state.values[i];
}

void CRMBLAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	if(!saved && sampleRate > 60000) {
		state.values[13] = 0;
		apvts.getParameter("oversampling")->setValueNotifyingHost(0);
	}
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}

void CRMBLAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void CRMBLAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	for(int i = 0; i < paramcount; i++) if(pots[i].smoothtime > 0 && i != 8)
		pots[i].smooth.reset(samplerate,pots[i].smoothtime);
	pots[8].smooth.reset(samplerate*(state.values[13]>.5?2:1),pots[8].smoothtime);

	blocksizething = samplesperblock>=(MIN_DLY*samplerate)?512:samplesperblock;
	delaytimelerp.setSize(channelnum,blocksizething,false,false,false);
	dampdelaytime.reset(0,1.,-1,samplerate,channelnum);
	dampchanneloffset.reset(0,.3,-1,samplerate,channelnum);
	dampamp.reset(0,.5,-1,samplerate,1);
	damplimiter.reset(1,.1,-1,samplerate*(state.values[13]>.5?2:1),channelnum);
	damppitchlatency.reset(0,.01,-1,samplerate,2);
	prevclear.resize(channelnum);
	prevfilter.resize(channelnum);
	reversecounter.resize(channelnum);
	for(int i = 0; i < channelnum; i++) {
		prevclear[i] = 0;
		prevfilter[i] = 0;
		reversecounter[i] = 0;
	}
	resetdampenings = true;

	//soundtouch
	pitchshift.setChannels(channelnum);
	pitchshift.setSampleRate(samplerate*(state.values[13]>.5?2:1));
	pitchshift.setPitchSemiTones(state.values[9]);
	pitchprocessbuffer.resize(samplerate*blocksizething*(state.values[13]>.5?2:1));

	//delay buffer
	delaybuffer.setSize(channelnum,samplerate*MAX_DLY+blocksizething+257,true,true,false);
	delayprocessbuffer.setSize(channelnum,blocksizething,true,true,false);
	delaypointerarray.resize(channelnum);

	//dc filter
	dcfilter.init(samplerate*(state.values[13]>.5?2:1),channelnum);
	for(int i = 0; i < channelnum; i++) dcfilter.reset(i);

	//oversampling
	preparedtoplay = true;
	os.reset(new dsp::Oversampling<float>(channelnum,1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
	os->initProcessing(blocksizething);
	os->setUsingIntegerLatency(true);
	setoversampling(state.values[13]>.5);

}
void CRMBLAudioProcessor::releaseResources() { }

bool CRMBLAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void CRMBLAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	saved = true;

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	float prmsadd = rmsadd.get();
	int prmscount = rmscount.get();
	bool isoversampling = (state.values[13]>.5);

	int delaybuffernumsamples = delaybuffer.getNumSamples();
	float** channelData = buffer.getArrayOfWritePointers();
	float** delayData = delaybuffer.getArrayOfWritePointers();
	float** delayProcessData = delayprocessbuffer.getArrayOfWritePointers();
	float** delaytimelerpData = delaytimelerp.getArrayOfWritePointers();


	double bpm = 120;
	if(getPlayHead() != nullptr) {
		AudioPlayHead::CurrentPositionInfo cpi;
		getPlayHead()->getCurrentPosition(cpi);
		bpm = cpi.bpm;
	}

	int pitchlatency = 0;
	bool ispitchbypassed = state.values[9] >= -.005 && state.values[9] <= .005f && prevpitch >= -.005 && prevpitch <= .005f;
	if(!ispitchbypassed) pitchlatency = pitchshift.getSetting(SETTING_NOMINAL_INPUT_SEQUENCE);
	if(prevpitchbypass != ispitchbypassed) {
		damppitchlatency.v_current[0] = pitchlatency;
		damppitchlatency.v_current[1] = pitchlatency;
		prevpitchbypass = ispitchbypassed;
	}

	int startread = 0;
	double osc = 0;
	double dampmodamp = 0;
	while(true) {
		int numsamples = fmin(buffer.getNumSamples()-startread,blocksizething);
		if(numsamples <= 0) break;

		if(prevpitch != state.values[9]) {
			prevpitch = state.values[9];
			pitchshift.setPitchSemiTones(state.values[9]);
		}

		//----delay time calculous----
		for(int sample = 0; sample < numsamples; ++sample) {
			state.values[0] = pots[0].smooth.getNextValue();
			state.values[2] = pots[2].smooth.getNextValue();
			state.values[4] = pots[4].smooth.getNextValue();
			state.values[5] = pots[5].smooth.getNextValue();

			double time = pow(state.values[0],2);
			if(state.values[1] > 0) {
				time = ((30.*state.values[1])/bpm-MIN_DLY)/(MAX_DLY-MIN_DLY);
				if(time > 1) {
					time = 1;
					outofrange = true;
				} else outofrange = false;
			}

			if(resetdampenings && sample == 0) dampamp.v_current[0] = state.values[2];
			double dampmodamp = pow(dampamp.nextvalue(state.values[2]),4)*.36;
			double frqpow = state.values[3]*1.4+.15;
			crntsmpl = fmod(crntsmpl+((frqpow*frqpow)/samplerate),1);
			if(dampmodamp > 0) osc = (sin(crntsmpl*MathConstants<double>::twoPi)*.5+.5)*dampmodamp;
			else osc = 0;

			for (int channel = 0; channel < channelnum; ++channel) {
				double channelval = 1-time;
				if(state.values[5] < 1 && state.values[4] != 0 && channelnum > 1) {
					channelval = (double)channel/(channelnum-1);
					if(state.values[4] < 0) channelval--;
					channelval = 1-time*(1-(state.values[4]*(1-state.values[5])*channelval));
				}
				double setdelaytime = (round((1-channelval)*((MAX_DLY-MIN_DLY)*samplerate)+MIN_DLY*samplerate)-MIN_DLY*samplerate)/((MAX_DLY-MIN_DLY)*samplerate);
				if(resetdampenings && sample == 0) dampdelaytime.v_current[channel] = setdelaytime;
				delaytimelerpData[channel][sample] = dampdelaytime.nextvalue(setdelaytime,channel)*(1-dampmodamp)+osc;
			}
		}

		//----feedback----
		for(int sample = 0; sample < numsamples; ++sample) {
			state.values[6] = pots[6].smooth.getNextValue();
			state.values[12] = pots[12].smooth.getNextValue();
			if(resetdampenings && sample == 0) {
				damppitchlatency.v_current[0] = pitchlatency;
				damppitchlatency.v_current[1] = pitchlatency;
			}
			damppitchlatency.nextvalue(pitchlatency,0);

			for (int channel = 0; channel < channelnum; ++channel) {
				delayProcessData[channel][sample] = channelData[channel][sample+startread]*(1-state.values[12])+interpolatesamples(delayData[channel],
					sample+delaybufferindex-fmax((delaytimelerpData[channel][sample]*(MAX_DLY-MIN_DLY)+MIN_DLY)*samplerate-damppitchlatency.v_current[0],512)+delaybuffernumsamples
					,delaybuffernumsamples)*(1-(1-state.values[6])*(1-state.values[12]));
			}
		}

		//----prepare delay buffer----
		dsp::AudioBlock<float> delayprocessblock;
		AudioBuffer<float>* delayprocessbufferpointer;
		int upsamplednumsamples = numsamples;
		if(isoversampling) {
			for(int i = 0; i < channelnum; i++) delaypointerarray[i] = delayprocessbuffer.getWritePointer(i);
			delayprocessblock = os->processSamplesUp(dsp::AudioBlock<float>(delaypointerarray.data(), channelnum, static_cast<int>(numsamples)));
			upsamplednumsamples = delayprocessblock.getNumSamples();
			for(int i = 0; i < channelnum; i++) delaypointerarray[i] = delayprocessblock.getChannelPointer(i);
			delayprocessbufferpointer = &AudioBuffer<float>(delaypointerarray.data(), channelnum, static_cast<int>(upsamplednumsamples));
		} else {
			for(int i = 0; i < channelnum; i++) delaypointerarray[i] = delayprocessbuffer.getWritePointer(i);
			delayprocessbufferpointer = &AudioBuffer<float>(delaypointerarray.data(), channelnum, static_cast<int>(upsamplednumsamples));
			delayprocessblock = dsp::AudioBlock<float>(*delayprocessbufferpointer);
		}
		delayProcessData = delayprocessbufferpointer->getArrayOfWritePointers();

		//----process delay buffer----
		for(int sample = 0; sample < upsamplednumsamples; ++sample) {
			state.values[8] = pots[8].smooth.getNextValue();
			for(int channel = 0; channel < channelnum; ++channel) {
				//remove dc
				delayProcessData[channel][sample] = dcfilter.process(delayProcessData[channel][sample],channel);

				//limit
				double thresh = fmin(Decibels::decibelsToGain(-1)/fmax(fabs(delayProcessData[channel][sample]),.000001),1);
				if(thresh < damplimiter.v_current[channel])
					damplimiter.v_smoothtime = .01;
				else 
					damplimiter.v_smoothtime = .1;
				delayProcessData[channel][sample] = fmax(fmin(delayProcessData[channel][sample]*damplimiter.nextvalue(thresh,channel),1.f),-1.f);

				//chew
				if(state.values[8] > 0)
					delayProcessData[channel][sample] = fmax(fmin(pnch(delayProcessData[channel][sample],state.values[8]),1.f),-1.f);
			}
		}
		//pitch
		if(!ispitchbypassed) {
			AudioDataConverters::interleaveSamples(delayprocessbufferpointer->getArrayOfReadPointers(),pitchprocessbuffer.data(),upsamplednumsamples,channelnum);
			pitchshift.putSamples(pitchprocessbuffer.data(), upsamplednumsamples);
			pitchshift.receiveSamples(pitchprocessbuffer.data(), upsamplednumsamples);
			AudioDataConverters::deinterleaveSamples(pitchprocessbuffer.data(),delayProcessData,upsamplednumsamples,channelnum);
		}

		//----down sample----
		if(isoversampling) {
			for(int i = 0; i < channelnum; i++) delaypointerarray[i] = delayprocessbuffer.getWritePointer(i);
				delayprocessbufferpointer = &AudioBuffer<float>(delaypointerarray.data(), channelnum, static_cast<int>(numsamples));
			os->processSamplesDown(dsp::AudioBlock<float>(*delayprocessbufferpointer));
			delayProcessData = delayprocessbufferpointer->getArrayOfWritePointers();
		}

		//lowpass
		for(int sample = 0; sample < numsamples; ++sample) {
			state.values[10] = pots[10].smooth.getNextValue();
			if(state.values[10] > 0) {
				double val = std::exp(-6.2831853072*mapToLog10(1-(double)state.values[10],250.0,20000.0)/samplerate);
				for(int channel = 0; channel < channelnum; ++channel) {
					delayProcessData[channel][sample] = delayProcessData[channel][sample]*(1-val)+prevfilter[channel]*val;
					prevfilter[channel] = delayProcessData[channel][sample];
				}
			}
		}

		//----process main buffer----
		for(int sample = 0; sample < numsamples; ++sample) {
			state.values[7] = pots[7].smooth.getNextValue();
			state.values[11] = pots[11].smooth.getNextValue();
			damppitchlatency.nextvalue(pitchlatency,1);

			for (int channel = 0; channel < channelnum; ++channel) {
				//vis
				if(prmscount < samplerate*2) {
					prmsadd += channelData[channel][sample+startread]*channelData[channel][sample+startread];
					prmscount++;
				}
				//channel offset
				double channeloffset = 1;
				if(state.values[5] > 0 && state.values[4] != 0 && channelnum > 1) {
					channeloffset = ((double)channel)/(channelnum-1);
					if(state.values[4] < 0) channeloffset--;
					channeloffset = 1-state.values[4]*state.values[5]*channeloffset;
				}
				//reverse
				double reverse = 0;
				double forward = 0;
				if(resetdampenings && sample == 0) dampchanneloffset.v_current[channel] = channeloffset;
				double dampchanneloffsetval = dampchanneloffset.nextvalue(channeloffset,channel);
				if(state.values[7] > 0) {
					double dlytime = (delaytimelerpData[channel][sample]*(MAX_DLY-MIN_DLY)+MIN_DLY)*samplerate;
					reversecounter[channel] += 2;
					if(reversecounter[channel] >= dlytime) reversecounter[channel] = 0;
					double calccounter = fmod(reversecounter[channel]-dampchanneloffsetval*dlytime+dlytime,dlytime);
					reverse = interpolatesamples(delayData[channel],delaybufferindex-calccounter+delaybuffernumsamples,delaybuffernumsamples);
					if(calccounter <= 256) {
						double lerpamount = calccounter*.00390625;
						calccounter += dlytime;
						reverse = reverse*sqrt(lerpamount)+interpolatesamples(delayData[channel],delaybufferindex-calccounter+delaybuffernumsamples,delaybuffernumsamples)*sqrt(1-lerpamount);
					}
				}
				if(state.values[7] < 1) {
					double dlytime = fmax((delaytimelerpData[channel][sample]*dampchanneloffsetval*(MAX_DLY-MIN_DLY)+MIN_DLY)*samplerate-damppitchlatency.v_current[1], 512);
					forward = interpolatesamples(delayData[channel],delaybufferindex-dlytime+delaybuffernumsamples,delaybuffernumsamples);
				}
				//dry wet
				float mixval = state.values[11]<.5?2*state.values[11]*state.values[11]:1-pow(-2*state.values[11]+2,2)*.5;
				channelData[channel][sample+startread] = channelData[channel][sample+startread]*sqrt(1-mixval)+(forward*sqrt(1-state.values[7])+reverse*sqrt(state.values[7]))*sqrt(mixval);
				//update delay buffer
				delayData[channel][delaybufferindex] = delayProcessData[channel][sample];
			}
			delaybufferindex = (delaybufferindex+1)%delaybuffernumsamples;
		}
		delayProcessData = delayprocessbuffer.getArrayOfWritePointers();

		startread += numsamples;
		resetdampenings = false;
	}

	rmsadd = prmsadd;
	rmscount = prmscount;
	lastosc = osc;
	lastmodamp = dampmodamp;

	boot = true;
}
double CRMBLAudioProcessor::pnch(double source, float amount) {
	if(source == 0) return 0;
	if(amount <= 0) return source;
	double cropmount = pow(amount*.75,10);
	double sinkedsource = (source-fmin(fmax(source,-cropmount),cropmount))/(1-cropmount);
	return sinkedsource*pow(1-pow(1-abs(sinkedsource),12.5),(3/(1-amount*.73))-3);
}
float CRMBLAudioProcessor::interpolatesamples(float* buffer, float position, int buffersize) {
	return buffer[((int)floor(position))%buffersize]*(1-fmod(position,1.f))+buffer[((int)ceil(position))%buffersize]*fmod(position,1.f);
}

void CRMBLAudioProcessor::setoversampling(bool toggle) {
	if(!preparedtoplay) return;
	if(toggle) {
		if(channelnum <= 0) return;
		os->reset();
		//setLatencySamples(os->getLatencyInSamples());
	}
	//else setLatencySamples(0);

	pots[8].smooth.reset(samplerate*(toggle?2:1),pots[8].smoothtime);
	damplimiter.v_samplerate = samplerate*(toggle?2:1);
	pitchshift.setSampleRate(samplerate*(toggle?2:1));
	pitchprocessbuffer.resize(samplerate*blocksizething*(toggle?2:1));
	dcfilter.init(samplerate*(toggle?2:1),channelnum);
}

bool CRMBLAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* CRMBLAudioProcessor::createEditor() {
	return new CRMBLAudioProcessorEditor(*this,paramcount,state,pots);
}

void CRMBLAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;

	data << version
		<< linebreak << currentpreset << linebreak;

	pluginpreset newstate = state;
	for(int i = 0; i < paramcount; i++) {
		if(pots[i].smoothtime > 0)
			data << pots[i].smooth.getTargetValue() << linebreak;
		else
			data << newstate.values[i] << linebreak;
	}

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << linebreak;
		for(int v = 0; v < paramcount; v++) if(pots[v].savedinpreset)
			data << presets[i].values[v] << linebreak;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void CRMBLAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	saved = true;
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, '\n');
		int saveversion = std::stoi(token);

		std::getline(ss, token, '\n');
		currentpreset = std::stoi(token);

		for(int i = 0; i < paramcount; i++) {
			std::getline(ss, token, '\n');
			float val = std::stof(token);
			apvts.getParameter(pots[i].id)->setValueNotifyingHost(pots[i].normalize(val));
			if(pots[i].smoothtime > 0) {
				pots[i].smooth.setCurrentAndTargetValue(val);
				state.values[i] = val;
			}
		}

		for(int i = 0; i < getNumPrograms(); i++) {
			std::getline(ss, token, '\n'); presets[i].name = token;
			for(int v = 0; v < paramcount; v++) if(pots[v].savedinpreset) {
				std::getline(ss, token, '\n');
				presets[i].values[v] = std::stof(token);
			}
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
}
void CRMBLAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		state.values[13] = newValue > .5 ? 1 : 0;
		setoversampling(newValue > .5);
		return;
	} else if(parameterID == "pingpostfeedback" && channelnum > 1) {
		for(int i = 0; i < channelnum; i++) reversecounter[i] = state.values[4] > 0 ? reversecounter[0] : reversecounter[channelnum-1];
	//} else if (parameterID == "randomize") {
		//return randomize();
	}
	for(int i = 0; i < paramcount; i++) if(parameterID == pots[i].id) {
		if(pots[i].smoothtime > 0) pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		return;
	}
}
/*
void CRMBLAudioProcessor::randomize() {
	float val = 0;
	apvts.getParameter("time")->setValueNotifyingHost(random.nextFloat());

	if(random.nextFloat() < .7) val = 0;
	else val = floor(random.nextFloat()*16+1);
	apvts.getParameter("sync")->setValueNotifyingHost(val/16);

	apvts.getParameter("modamp")->setValueNotifyingHost(pow(random.nextFloat(),3));

	apvts.getParameter("modfreq")->setValueNotifyingHost(pow(random.nextFloat(),3));

	val = random.nextFloat();
	if(val < .6) val = .5;
	else if(val < .8) val = random.nextBool()?.25:.75;
	else val = random.nextFloat();
	apvts.getParameter("pingpong")->setValueNotifyingHost(val);

	apvts.getParameter("pingpostfeedback")->setValueNotifyingHost(random.nextFloat()<.7?1:0);

	apvts.getParameter("feedback")->setValueNotifyingHost(random.nextFloat());

	val = random.nextFloat();
	if(val < .7) val = 0;
	else if(val < .9) val = 1;
	else val = random.nextFloat();
	apvts.getParameter("reverse")->setValueNotifyingHost(val);

	if(random.nextFloat() < .3) val = 0;
	else val = pow(random.nextFloat(),2);
	apvts.getParameter("chew")->setValueNotifyingHost(val);

	if(random.nextFloat() < .7) val = 0;
	else {
		val = random.nextFloat();
		if(val < .3) val = 7;
		else if(val < .6) val = 12;
		else if(val < .9) val = floor(random.nextFloat()*12+1);
		else val = random.nextFloat()*12;
		if(random.nextFloat() < .1) val += 12;
		if(random.nextFloat() < .3) val *= -1;
	}
	apvts.getParameter("pitch")->setValueNotifyingHost(val/48+.5);

	if(random.nextFloat() < .7) val = random.nextFloat();
	else val = 0;
	apvts.getParameter("lowpass")->setValueNotifyingHost(val);

	apvts.getParameter("hold")->setValueNotifyingHost(0);
}
*/

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CRMBLAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout CRMBLAudioProcessor::createParameters() {
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>("time"			,"Time (MS)"			,0.0f	,1.0f	,0.32f	));
	parameters.push_back(std::make_unique<AudioParameterInt		>("sync"			,"Time (Eighth note)"	,0.0f	,16.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("modamp"			,"Mod Amount"			,0.0f	,1.0f	,0.15f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("modfreq"			,"Mod Frequency"		,0.0f	,1.0f	,0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("pingpong"		,"Ping Pong"			,-1.0f	,1.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("pingpostfeedback","Ping Post Feedback"	,true	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("feedback"		,"Feedback"				,0.0f	,1.0f	,0.5f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("reverse"			,"Reverse"				,0.0f	,1.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("chew"			,"Chew"				,0.0f	,1.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("pitch"			,"Pitch"				,-24.0f	,24.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("lowpass"			,"Lowpass"				,0.0f	,1.0f	,0.3f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("wet"				,"Dry/Wet"				,0.0f	,1.0f	,0.4f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("hold"			,"Hold"					,false	));
	//parameters.push_back(std::make_unique<AudioParameterBool	>("randomize"		,"Randomize"			,false	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("oversampling"	,"Over-Sampling"		,true	));
	return { parameters.begin(), parameters.end() };
}
