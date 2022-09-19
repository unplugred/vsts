#include "PluginProcessor.h"
#include "PluginEditor.h"

CRMBLAudioProcessor::CRMBLAudioProcessor() :
	AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
	apvts(*this, &undoManager, "Parameters", createParameters())
{
	//										,time	,sync	,modamp	,modfreq,pingpon,pingpos,feedbac,reverse,chew	,pitch	,lowpass,wet	,
	//presets[0] = pluginpreset("Default"	,0.32f	,0.0f	,0.15f	,0.5f	,0.0f	,1.0f	,0.5f	,0.0f	,0.0f	,0.0f	,0.3f	,0.4	);
	for(int i = 0; i < getNumPrograms(); i++) {
		//presets[i] = presets[0];
		presets[i].name = "Program " + (String)(i+1);
	}

	params.pots[0] = potentiometer("Time (MS)"			,"time"				,.001f	,presets[0].values[0]	);
	params.pots[1] = potentiometer("Time (Eighth note)"	,"sync"				,0		,presets[0].values[1]	,0		,16		,potentiometer::inttype);
	params.pots[2] = potentiometer("Mod Amount"			,"modamp"			,.001f	,presets[0].values[2]	);
	params.pots[3] = potentiometer("Mod Frequency"		,"modfreq"			,0		,presets[0].values[3]	);
	params.pots[4] = potentiometer("Ping Pong"			,"pingpong"			,.001f	,presets[0].values[4]	,-1.f	,1.f	);
	params.pots[5] = potentiometer("Ping Post Feedback"	,"pingpostfeedback"	,.001f	,presets[0].values[5]	,0		,1		,potentiometer::booltype);
	params.pots[6] = potentiometer("Feedback"			,"feedback"			,.002f	,presets[0].values[6]	);
	params.pots[7] = potentiometer("Reverse"			,"reverse"			,.002f	,presets[0].values[7]	);
	params.pots[8] = potentiometer("Chew"				,"chew"				,.001f	,presets[0].values[8]	);
	params.pots[9] = potentiometer("Pitch"				,"pitch"			,0		,presets[0].values[9]	,-24.f	,24.f	);
	params.pots[10] = potentiometer("Lowpass"			,"lowpass"			,.001f	,presets[0].values[10]	);
	params.pots[11] = potentiometer("Dry/Wet"			,"wet"				,.002f	,presets[0].values[11]	);

	for(int i = 0; i < paramcount; i++) {
		state.values[i] = params.pots[i].inflate(apvts.getParameter(params.pots[i].id)->getValue());
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setCurrentAndTargetValue(state.values[i]);
		apvts.addParameterListener(params.pots[i].id, this);
	}
	params.hold = apvts.getParameter("hold")->getValue() > .5 ? 1 : 0;
	params.holdsmooth.setCurrentAndTargetValue(params.hold);
	apvts.addParameterListener("hold", this);
	params.oversampling = apvts.getParameter("oversampling")->getValue() > .5;
	apvts.addParameterListener("oversampling", this);
}

CRMBLAudioProcessor::~CRMBLAudioProcessor(){
	for(int i = 0; i < paramcount; i++) apvts.removeParameterListener(params.pots[i].id, this);
	apvts.removeParameterListener("hold", this);
	apvts.removeParameterListener("oversampling", this);
}

const String CRMBLAudioProcessor::getName() const { return "CRMBL"; }
bool CRMBLAudioProcessor::acceptsMidi() const { return false; }
bool CRMBLAudioProcessor::producesMidi() const { return false; }
bool CRMBLAudioProcessor::isMidiEffect() const { return false; }
double CRMBLAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int CRMBLAudioProcessor::getNumPrograms() { return 20; }
int CRMBLAudioProcessor::getCurrentProgram() { return currentpreset; }
void CRMBLAudioProcessor::setCurrentProgram (int index) {
	if(currentpreset == index) return;

	undoManager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	for(int i = 0; i < paramcount; i++) {
		if(lerpstage < .001 || lerpchanged[i])
			lerptable[i] = params.pots[i].normalize(presets[currentpreset].values[i]);
		lerpchanged[i] = false;
	}
	currentpreset = index;
	for(int i = 0; i < paramcount; i++) {
		if(params.pots[i].ttype == potentiometer::ptype::booltype) {
			lerpchanged[i] = true;
			apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		}
	}

	if(lerpstage <= 0) {
		lerpstage = 1;
		startTimerHz(30);
	} else lerpstage = 1;
}
void CRMBLAudioProcessor::timerCallback() {
	lerpstage *= .64f;
	if(lerpstage < .001) {
		for(int i = 0; i < paramcount; i++) if(!lerpchanged[i])
			apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		lerpstage = 0;
		undoManager.beginNewTransaction();
		stopTimer();
		return;
	}
	for(int i = 0; i < paramcount; i++) if(!lerpchanged[i]) {
		lerptable[i] = (params.pots[i].normalize(presets[currentpreset].values[i])-lerptable[i])*.36f+lerptable[i];
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(lerptable[i]);
	}
}
const String CRMBLAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void CRMBLAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
}

void CRMBLAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	if(!saved && sampleRate > 60000) {
		params.oversampling = false;
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

	for(int i = 0; i < paramcount; i++) if(params.pots[i].smoothtime > 0 && i != 8)
		params.pots[i].smooth.reset(samplerate,params.pots[i].smoothtime);
	params.pots[8].smooth.reset(samplerate*(params.oversampling?2:1),params.pots[8].smoothtime);
	params.holdsmooth.reset(samplerate,.002f);

	blocksizething = samplesperblock>=(MIN_DLY*samplerate)?512:samplesperblock;
	delaytimelerp.setSize(channelnum,blocksizething,false,false,false);
	dampdelaytime.reset(0,1.,-1,samplerate,channelnum);
	dampchanneloffset.reset(0,.3,-1,samplerate,channelnum);
	dampamp.reset(0,.5,-1,samplerate,1);
	damplimiter.reset(1,.1,-1,samplerate*(params.oversampling?2:1),channelnum);
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
	pitchshift.setSampleRate(samplerate*(params.oversampling?2:1));
	pitchshift.setPitchSemiTones(state.values[9]);
	pitchprocessbuffer.resize(samplerate*blocksizething*(params.oversampling?2:1));

	//delay buffer
	delaybuffer.setSize(channelnum,samplerate*MAX_DLY+blocksizething+257,true,true,false);
	delayprocessbuffer.setSize(channelnum,blocksizething,true,true,false);
	delaypointerarray.resize(channelnum);

	//dc filter
	dcfilter.init(samplerate*(params.oversampling?2:1),channelnum);
	for(int i = 0; i < channelnum; i++) dcfilter.reset(i);

	//oversampling
	preparedtoplay = true;
	os.reset(new dsp::Oversampling<float>(channelnum,1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
	os->initProcessing(blocksizething);
	os->setUsingIntegerLatency(true);
	setoversampling(params.oversampling);

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
			state.values[0] = params.pots[0].smooth.getNextValue();
			state.values[2] = params.pots[2].smooth.getNextValue();
			state.values[4] = params.pots[4].smooth.getNextValue();
			state.values[5] = params.pots[5].smooth.getNextValue();

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
			state.values[6] = params.pots[6].smooth.getNextValue();
			params.hold = params.holdsmooth.getNextValue();
			if(resetdampenings && sample == 0) {
				damppitchlatency.v_current[0] = pitchlatency;
				damppitchlatency.v_current[1] = pitchlatency;
			}
			damppitchlatency.nextvalue(pitchlatency,0);

			for (int channel = 0; channel < channelnum; ++channel) {
				delayProcessData[channel][sample] = channelData[channel][sample+startread]*(1-params.hold)+interpolatesamples(delayData[channel],
					sample+delaybufferindex-fmax((delaytimelerpData[channel][sample]*(MAX_DLY-MIN_DLY)+MIN_DLY)*samplerate-damppitchlatency.v_current[0],512)+delaybuffernumsamples
					,delaybuffernumsamples)*(1-(1-state.values[6])*(1-params.hold));
			}
		}

		//----prepare delay buffer----
		dsp::AudioBlock<float> delayprocessblock;
		AudioBuffer<float>* delayprocessbufferpointer;
		int upsamplednumsamples = numsamples;
		if(params.oversampling) {
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
			state.values[8] = params.pots[8].smooth.getNextValue();
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
		if(params.oversampling) {
			for(int i = 0; i < channelnum; i++) delaypointerarray[i] = delayprocessbuffer.getWritePointer(i);
				delayprocessbufferpointer = &AudioBuffer<float>(delaypointerarray.data(), channelnum, static_cast<int>(numsamples));
			os->processSamplesDown(dsp::AudioBlock<float>(*delayprocessbufferpointer));
			delayProcessData = delayprocessbufferpointer->getArrayOfWritePointers();
		}

		//lowpass
		for(int sample = 0; sample < numsamples; ++sample) {
			state.values[10] = params.pots[10].smooth.getNextValue();
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
			state.values[7] = params.pots[7].smooth.getNextValue();
			state.values[11] = params.pots[11].smooth.getNextValue();
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
						double lerpamount = calccounter*.00390625*1.5707963268;
						calccounter += dlytime;
						reverse = reverse*sin(lerpamount)+interpolatesamples(delayData[channel],delaybufferindex-calccounter+delaybuffernumsamples,delaybuffernumsamples)*cos(lerpamount);
					}
				}
				if(state.values[7] < 1) {
					double dlytime = fmax((delaytimelerpData[channel][sample]*dampchanneloffsetval*(MAX_DLY-MIN_DLY)+MIN_DLY)*samplerate-damppitchlatency.v_current[1], 512);
					forward = interpolatesamples(delayData[channel],delaybufferindex-dlytime+delaybuffernumsamples,delaybuffernumsamples);
				}
				//dry wet
				double mixval = state.values[11]*1.5707963268;
				double reversemixval = state.values[7]*1.5707963268;
				channelData[channel][sample+startread] = channelData[channel][sample+startread]*cos(mixval)+(forward*cos(reversemixval)+reverse*sin(reversemixval))*sin(mixval);
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

	params.pots[8].smooth.reset(samplerate*(toggle?2:1),params.pots[8].smoothtime);
	damplimiter.v_samplerate = samplerate*(toggle?2:1);
	pitchshift.setSampleRate(samplerate*(toggle?2:1));
	pitchprocessbuffer.resize(samplerate*blocksizething*(toggle?2:1));
	dcfilter.init(samplerate*(toggle?2:1),channelnum);
}

bool CRMBLAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* CRMBLAudioProcessor::createEditor() {
	return new CRMBLAudioProcessorEditor(*this,paramcount,presets[currentpreset],params);
}

void CRMBLAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char linebreak = '\n';
	std::ostringstream data;

	data << version << linebreak
		<< currentpreset << linebreak
		<< params.holdsmooth.getTargetValue() << linebreak
		<< (params.oversampling?1:0) << linebreak;

	for(int i = 0; i < getNumPrograms(); i++) {
		data << presets[i].name << linebreak;
		for(int v = 0; v < paramcount; v++)
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

		if(saveversion >= 1) {
			std::getline(ss, token, '\n');
			params.hold = std::stof(token) > .5 ? 1 : 0;

			std::getline(ss, token, '\n');
			params.oversampling = std::stof(token) > .5;

			for(int i = 0; i < getNumPrograms(); i++) {
				std::getline(ss, token, '\n');
				presets[i].name = token;
				for(int v = 0; v < paramcount; v++) {
					std::getline(ss, token, '\n');
					presets[i].values[v] = std::stof(token);
				}
			}
		} else {
			for(int i = 0; i < 12; i++) {
				std::getline(ss, token, '\n');
				presets[currentpreset].values[i] = std::stof(token);
			}

			std::getline(ss, token, '\n');
			params.hold = std::stof(token) > .5 ? 1 : 0;

			std::getline(ss, token, '\n');
			params.oversampling = std::stof(token)>.5;

			for(int i = 0; i < 8; i++) {
				std::getline(ss, token, '\n');
				presets[i].name = token;
				for(int v = 0; v < 12; v++) {
					std::getline(ss, token, '\n');
					if(currentpreset != i) presets[i].values[v] = std::stof(token);
				}
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

	for(int i = 0; i < paramcount; i++) {
		apvts.getParameter(params.pots[i].id)->setValueNotifyingHost(params.pots[i].normalize(presets[currentpreset].values[i]));
		if(params.pots[i].smoothtime > 0) {
			params.pots[i].smooth.setCurrentAndTargetValue(presets[currentpreset].values[i]);
			state.values[i] = presets[currentpreset].values[i];
		}
	}
	apvts.getParameter("hold")->setValueNotifyingHost(params.hold);
	params.holdsmooth.setCurrentAndTargetValue(params.hold);
	apvts.getParameter("oversampling")->setValueNotifyingHost(params.oversampling);
}
void CRMBLAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "hold") {
		params.holdsmooth.setTargetValue(newValue>.5?1:0);
		return;
	}
	if(parameterID == "oversampling") {
		params.oversampling = newValue>.5;
		setoversampling(newValue>.5);
		return;
	}
	if(parameterID == "pingpostfeedback" && channelnum > 1) {
		for(int i = 0; i < channelnum; i++) reversecounter[i] = state.values[4] > 0 ? reversecounter[0] : reversecounter[channelnum-1];
	}
	for(int i = 0; i < paramcount; i++) if(parameterID == params.pots[i].id) {
		if(params.pots[i].smoothtime > 0) params.pots[i].smooth.setTargetValue(newValue);
		else state.values[i] = newValue;
		if(lerpstage < .001 || lerpchanged[i]) presets[currentpreset].values[i] = newValue;
		return;
	}
}

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
	parameters.push_back(std::make_unique<AudioParameterFloat	>("chew"			,"Chew"					,0.0f	,1.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("pitch"			,"Pitch"				,-24.0f	,24.0f	,0.0f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("lowpass"			,"Lowpass"				,0.0f	,1.0f	,0.3f	));
	parameters.push_back(std::make_unique<AudioParameterFloat	>("wet"				,"Dry/Wet"				,0.0f	,1.0f	,0.4f	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("hold"			,"Hold"					,false	));
	parameters.push_back(std::make_unique<AudioParameterBool	>("oversampling"	,"Over-Sampling"		,true	));
	return { parameters.begin(), parameters.end() };
}
