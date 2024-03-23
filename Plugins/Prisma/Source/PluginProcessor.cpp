#include "PluginProcessor.h"
#include "PluginEditor.h"

PrismaAudioProcessor::PrismaAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts),
	forwardFFT(fftOrder), window(fftSize, dsp::WindowingFunction<float>::hann) {

	init();

	for(int i = 0; i < getNumPrograms(); i++)
		presets[i].name = "Program " + (String)(i+1);

	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			state[0].values[b][m] = apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->getValue();
			pots.bands[b].modules[m].value.setCurrentAndTargetValue(state[0].values[b][m]);
			add_listener("b"+(String)b+"m"+(String)m+"val");

			state[0].id[b][m] = apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->getValue();
			add_listener("b"+(String)b+"m"+(String)m+"id");
		}

		if(b >= 1) {
			state[0].crossover[b-1] = apvts.getParameter("b" + (String)b + "cross")->getValue();
			add_listener("b"+(String)b+"cross");
		}

		state[0].gain[b] = apvts.getParameter("b" + (String)b + "gain")->getValue();
		pots.bands[b].gain.setCurrentAndTargetValue(state[0].gain[b]);
		add_listener("b"+(String)b+"gain");

		pots.bands[b].mute = apvts.getParameter("b" + (String)b + "mute")->getValue();
		add_listener("b"+(String)b+"mute");

		pots.bands[b].solo = apvts.getParameter("b" + (String)b + "solo")->getValue();
		add_listener("b"+(String)b+"solo");

		pots.bands[b].bypass = apvts.getParameter("b" + (String)b + "bypass")->getValue();
		pots.bands[b].bypasssmooth = pots.bands[b].bypass>.5?1.f:0.f;
		pots.bands[b].bandbypass.setCurrentAndTargetValue(pots.bands[b].bypasssmooth);
		add_listener("b"+(String)b+"bypass");
	}
	state[0].wet = apvts.getParameter("wet")->getValue();
	pots.wet.setCurrentAndTargetValue(state[0].wet);
	add_listener("wet");

	pots.oversampling = apvts.getParameter("oversampling")->getValue();
	add_listener("oversampling");

	pots.isb = apvts.getParameter("ab")->getValue();
	add_listener("ab");

	recalcactivebands();
	for(int b = 0; b < 4; b++) {
		pots.bands[b].activesmooth = pots.bands[b].bandactive.getTargetValue();
		pots.bands[b].bandactive.setCurrentAndTargetValue(pots.bands[b].activesmooth);
	}

	state[1] = state[0];
	String presetname = presets[currentpreset].name;
	presets[currentpreset] = state[0];
	presets[currentpreset].name = presetname;

	for(int i = 0; i < scopeSize; i++) scopeData[i] = 0;
	for(int i = 0; i < fftSize; i++) fifo[i] = 0;
	for(int i = 0; i < fftSize*2; i++) fftData[i] = 0;
}

PrismaAudioProcessor::~PrismaAudioProcessor() {
	close();
}

const String PrismaAudioProcessor::getName() const { return "Prisma"; }
bool PrismaAudioProcessor::acceptsMidi() const { return false; }
bool PrismaAudioProcessor::producesMidi() const { return false; }
bool PrismaAudioProcessor::isMidiEffect() const { return false; }
double PrismaAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PrismaAudioProcessor::getNumPrograms() { return 20; }
int PrismaAudioProcessor::getCurrentProgram() { return currentpreset; }
void PrismaAudioProcessor::setCurrentProgram (int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	transition = true;
	currentpreset = index;

	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->setValueNotifyingHost(presets[currentpreset].values[b][m]);
			apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->setValueNotifyingHost(((float)presets[currentpreset].id[b][m])/MODULE_COUNT);
		}
		if(b >= 1) apvts.getParameter("b"+(String)b+"cross")->setValueNotifyingHost(presets[currentpreset].crossover[b-1]);
		apvts.getParameter("b"+(String)b+"gain")->setValueNotifyingHost(presets[currentpreset].gain[b]);
	}
	apvts.getParameter("wet")->setValueNotifyingHost(presets[currentpreset].wet);
}
const String PrismaAudioProcessor::getProgramName (int index) {
	return { presets[index].name };
}
void PrismaAudioProcessor::changeProgramName (int index, const String& newName) {
	presets[index].name = newName;
}

void PrismaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
	if(!saved && sampleRate > 60000) {
		pots.oversampling = 0;
		apvts.getParameter("oversampling")->setValueNotifyingHost(0);
	}
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void PrismaAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void PrismaAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	//fft
	nextFFTBlockReady = false;
	for(int i = 0; i < fftSize; i++) fifo[i] = 0;
	for(int i = 0; i < fftSize*2; i++) fftData[i] = 0;
	fifoIndex = 0;

	//crossover filters
	dsp::ProcessSpec spec;
	spec.sampleRate = samplerate*(pots.oversampling?2:1);
	spec.maximumBlockSize = samplesperblock;
	spec.numChannels = channelnum;
	for(int i = 0; i < 9; i++) {
		crossover[i].reset();
		crossover[i].prepare(spec);
	}
	crossover[0].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[0]));
	crossover[0].setType(dsp::LinkwitzRileyFilterType::lowpass);
	crossover[1].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[1]));
	crossover[1].setType(dsp::LinkwitzRileyFilterType::allpass);
	crossover[2].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[2]));
	crossover[2].setType(dsp::LinkwitzRileyFilterType::allpass);
	crossover[3].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[0]));
	crossover[3].setType(dsp::LinkwitzRileyFilterType::highpass);
	crossover[4].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[1]));
	crossover[4].setType(dsp::LinkwitzRileyFilterType::lowpass);
	crossover[5].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[2]));
	crossover[5].setType(dsp::LinkwitzRileyFilterType::allpass);
	crossover[6].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[1]));
	crossover[6].setType(dsp::LinkwitzRileyFilterType::highpass);
	crossover[7].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[2]));
	crossover[7].setType(dsp::LinkwitzRileyFilterType::lowpass);
	crossover[8].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[2]));
	crossover[8].setType(dsp::LinkwitzRileyFilterType::highpass);

	//dc filter
	dcfilter.init(samplerate*(pots.oversampling?2:1),channelnum*4);

	//declick
	declick.resize(channelnum*4);

	//sample divide
	sampleandhold.resize(channelnum*16);
	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			holdtime[b*4+m] = 0;

			//parameter smoothing
			pots.bands[b].modules[m].value.reset(samplerate*(pots.oversampling?2:1),.001f);
		}
	}

	//lowpass+highpass modules
	modulefilters.resize(channelnum*16);
	dsp::ProcessSpec monospec;
	monospec.sampleRate = samplerate*(pots.oversampling?2:1);
	monospec.maximumBlockSize = samplesperblock;
	monospec.numChannels = 1;
	for(int c = 0; c < channelnum; c++) {
		for(int b = 0; b < 4; b++) {

			//dc filter
			dcfilter.reset(b*channelnum+c);

			//declick
			declick[c*4+b] = 0;
			declickprogress[b] = 0;

			//sample divide
			for(int m = 0; m < 4; m++) {
				sampleandhold[c*16+b*4+m] = 0;

				//lowpass+highpass modules
				if(state[pots.isb?1:0].id[b][m] == 15 || state[pots.isb?1:0].id[b][m] == 16) {
					modulefilters[c*16+b*4+m].reset();
					(*modulefilters[c*16+b*4+m].parameters.get()).type = state[pots.isb?1:0].id[b][m] == 15 ? dsp::StateVariableFilter::Parameters<float>::Type::lowPass : dsp::StateVariableFilter::Parameters<float>::Type::highPass;
					modulefilters[c*16+b*4+m].prepare(monospec);
				}
			}
		}
	}

	//oversampling
	preparedtoplay = true;
	ospointerarray.resize(channelnum);
	os.reset(new dsp::Oversampling<float>(channelnum,1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
	os->initProcessing(samplesperblock);
	os->setUsingIntegerLatency(true);
	setoversampling(pots.oversampling);
}
void PrismaAudioProcessor::releaseResources() { }

bool PrismaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void PrismaAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	saved = true;

	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	bool isoversampling = pots.oversampling;

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);
	if(isoversampling) {
		dsp::AudioBlock<float> osblock = os->processSamplesUp(block);
		for(int i = 0; i < channelnum; i++)
			ospointerarray[i] = osblock.getChannelPointer(i);
		osbuffer = AudioBuffer<float>(ospointerarray.data(), channelnum, static_cast<int>(osblock.getNumSamples()));
		numsamples = osbuffer.getNumSamples();
	}

	for(int b = 0; b < 4; b++) {
		if(isoversampling) filterbuffers[b] = osbuffer;
		else filterbuffers[b] = buffer;
	}
	dsp::AudioBlock<float> lowblock		(filterbuffers[0]);
	dsp::AudioBlock<float> lowmidblock	(filterbuffers[1]);
	dsp::AudioBlock<float> highmidblock	(filterbuffers[2]);
	dsp::AudioBlock<float> highblock	(filterbuffers[3]);
	dsp::ProcessContextReplacing<float> lowcontext		(lowblock);
	dsp::ProcessContextReplacing<float> lowmidcontext	(lowmidblock);
	dsp::ProcessContextReplacing<float> highmidcontext	(highmidblock);
	dsp::ProcessContextReplacing<float> highcontext		(highblock);
	crossover[0].process(lowcontext);		//LP L
	crossover[1].process(lowcontext);		//AP L
	crossover[2].process(lowcontext);		//AP L
	crossover[3].process(lowmidcontext);	//HP LM
	filterbuffers[2] = filterbuffers[1];	//HM=LM
	crossover[4].process(lowmidcontext);	//LP LM
	crossover[5].process(lowmidcontext);	//AP LM
	crossover[6].process(highmidcontext);	//HP HM
	filterbuffers[3] = filterbuffers[2];	//H =LM
	crossover[7].process(highmidcontext);	//LP HM
	crossover[8].process(highcontext);		//HP H

	float* const* wetChannelData[4];
	float* const* dryChannelData[4];
	for(int b = 0; b < 4; b++) {
		wetbuffers[b] = filterbuffers[b];
		wetChannelData[b] = wetbuffers[b].getArrayOfWritePointers();
		dryChannelData[b] = filterbuffers[b].getArrayOfWritePointers();
	}


	for(int b = 0; b < 4; b++) {
		bool newremovedc = false;
		for(int m = 0; m < 4; m++) {
			if(state[pots.isb?1:0].id[b][m] != 0) { //NONE

				//SOFT CLIP
				if(state[pots.isb?1:0].id[b][m] == 1) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							double val = 1-(1-(pow(1-state[pots.isb?1:0].values[b][m],10)+(1-pow(state[pots.isb?1:0].values[b][m],.2)))*.5)*.99;
							for(int c = 0; c < channelnum; ++c) {
								wetChannelData[b][c][s] = (1-pow(1-fmin(fabs((double)wetChannelData[b][c][s]),1),1/val))*(wetChannelData[b][c][s]>0?1:-1)*(1-(1-val)*.92);
							}
						}
					}

				//HARD CLIP
				} else if(state[pots.isb?1:0].id[b][m] == 2) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							double val = 1-(1-(pow(1-state[pots.isb?1:0].values[b][m],10)+(1-pow(state[pots.isb?1:0].values[b][m],.2)))*.5)*.99;
							for(int c = 0; c < channelnum; ++c) {
								wetChannelData[b][c][s] = fmax(fmin(wetChannelData[b][c][s],val),-val)*(.08/val+.92);
							}
						}
					}

				//HEAVY
				} else if(state[pots.isb?1:0].id[b][m] == 3) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							double val = 1-state[pots.isb?1:0].values[b][m]*.99;
							for(int c = 0; c < channelnum; ++c) {
								wetChannelData[b][c][s] = pow(fabs((double)wetChannelData[b][c][s]),val)*(wetChannelData[b][c][s]>0?1:-1)*(1-(1-pow(val,2))*.92);
							}
						}
					}

				//ASYM
				} else if(state[pots.isb?1:0].id[b][m] == 4) {
					newremovedc = true;
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							double val = 1-state[pots.isb?1:0].values[b][m]*.99;
							for(int c = 0; c < channelnum; ++c) {
								if(wetChannelData[b][c][s] >= 0)
									wetChannelData[b][c][s] = pow((double)wetChannelData[b][c][s],val);
								else
									wetChannelData[b][c][s] = abs(pow((double)wetChannelData[b][c][s]+1,val))*(wetChannelData[b][c][s]>-1?1:-1)-1;
								wetChannelData[b][c][s] *= 1-(1-pow(val,1.5))*.86;
							}
						}
					}

				//RECTIFY
				} else if(state[pots.isb?1:0].id[b][m] == 5) {
					newremovedc = true;
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							for(int c = 0; c < channelnum; ++c) {
								wetChannelData[b][c][s] = (wetChannelData[b][c][s]*(1-state[pots.isb?1:0].values[b][m])+fabs(wetChannelData[b][c][s])*state[pots.isb?1:0].values[b][m])*(state[pots.isb?1:0].values[b][m]+1);
							}
						}
					}

				//FOLD
				} else if(state[pots.isb?1:0].id[b][m] == 6) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							double val = pow(state[pots.isb?1:0].values[b][m],2);
							for(int c = 0; c < channelnum; ++c) {
								double amped = fmod(fabs(wetChannelData[b][c][s]*(val*70+1)),4);
								wetChannelData[b][c][s] = (amped>3?(amped-4):(amped>1?(2-amped):amped))*(wetChannelData[b][c][s]>0?1:-1)*(1-(1-(pow(1-val,40)+(1-pow(val,.2)))*.5)*.92);
							}
						}
					}

				//SINE FOLD
				} else if(state[pots.isb?1:0].id[b][m] == 7) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							double val = pow(state[pots.isb?1:0].values[b][m],2);
							for(int c = 0; c < channelnum; ++c) {
								wetChannelData[b][c][s] = (sin(((double)wetChannelData[b][c][s])*(val*35+1)*3.14159234129)*fmin(val*5,1)+((double)wetChannelData[b][c][s])*fmax(1-val*5,0))*(1-(1-(pow(1-val,40)+(1-pow(val,.2)))*.5)*.92);
							}
						}
					}

				//ZERO CROSS
				} else if(state[pots.isb?1:0].id[b][m] == 8) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							if(state[pots.isb?1:0].values[b][m] == 1) {
								for(int c = 0; c < channelnum; ++c) wetChannelData[b][c][s] = 0;
							} else {
								double cropmount = pow((double)state[pots.isb?1:0].values[b][m],10);
								for(int c = 0; c < channelnum; ++c) {
									wetChannelData[b][c][s] = ((double)wetChannelData[b][c][s]-fmin(fmax((double)wetChannelData[b][c][s],-cropmount),cropmount))/(1-cropmount);
									if(wetChannelData[b][c][s] > -1 && wetChannelData[b][c][s] < 1)
										wetChannelData[b][c][s] = wetChannelData[b][c][s]*pow(1-pow(1-abs(wetChannelData[b][c][s]),12.5),(3/(1-state[pots.isb?1:0].values[b][m]*.98))-3);
								}
							}
						}
					}

				//BIT CRUSH
				} else if(state[pots.isb?1:0].id[b][m] == 9) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							double val = pow((double)state[pots.isb?1:0].values[b][m],5)*.99+.01;
							for(int c = 0; c < channelnum; ++c) {
								wetChannelData[b][c][s] = round(wetChannelData[b][c][s]/val)*val;
							}
						}
					}

				//SAMPLE DIVIDE
				} else if(state[pots.isb?1:0].id[b][m] == 10) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							double time = (samplerate*(pots.oversampling?2:1))/mapToLog10(1-(double)state[pots.isb?1:0].values[b][m],40.0,40000.0);
							double mix = fmin(fmax(holdtime[b*4+m],0),1);
							bool isreplacing = ++holdtime[b*4+m] >= time;
							for(int c = 0; c < channelnum; ++c) {
								double dry = wetChannelData[b][c][s];
								wetChannelData[b][c][s] = dry*(1-mix)+sampleandhold[c*16+b*4+m]*mix;
								if(isreplacing) sampleandhold[c*16+b*4+m] = dry;
							}
							holdtime[b*4+m] = fmod(holdtime[b*4+m],time);
						}
					}

				//DC
				} else if(state[pots.isb?1:0].id[b][m] == 11) {
					newremovedc = true;
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] != .5) {
							for(int c = 0; c < channelnum; ++c) {
								wetChannelData[b][c][s] += pow((double)state[pots.isb?1:0].values[b][m]*2-1,5)*.5;
							}
						}
					}

				//WIDTH
				} else if(state[pots.isb?1:0].id[b][m] == 12) {
					if(channelnum > 1) {
						for(int s = 0; s < numsamples; ++s) {
							state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
							if(state[pots.isb?1:0].values[b][m] != .5) {
								double sum = 0;
								for(int c = 0; c < channelnum; ++c) sum += wetChannelData[b][c][s];
								sum /= channelnum;
								for(int c = 0; c < channelnum; ++c)
									wetChannelData[b][c][s] = sum+(wetChannelData[b][c][s]-sum)*state[pots.isb?1:0].values[b][m]*2;
							}
						}
					}

				//GAIN
				} else if(state[pots.isb?1:0].id[b][m] == 13) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] != .35) {
							for(int c = 0; c < channelnum; ++c) {
								wetChannelData[b][c][s] *= pow(state[pots.isb?1:0].values[b][m]/.35,2);
							}
						}
					}

				//PAN
				} else if(state[pots.isb?1:0].id[b][m] == 14) {
					if(channelnum > 1) {
						for(int s = 0; s < numsamples; ++s) {
							state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
							if(state[pots.isb?1:0].values[b][m] != .5) {
								for(int c = 0; c < channelnum; ++c) {
									double linearpan = (((double)c)/(channelnum-1))*(state[pots.isb?1:0].values[b][m]*2-1)+(1-state[pots.isb?1:0].values[b][m]);
									wetChannelData[b][c][s] *= sin(linearpan*1.5707963268)*.7071067812+linearpan;
								}
							}
						}
					}

				//LOW PASS + HIGH PASS
				} else if(state[pots.isb?1:0].id[b][m] == 15 || state[pots.isb?1:0].id[b][m] == 16) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						for(int c = 0; c < channelnum; ++c) {
							if(std::isinf(wetChannelData[b][c][s]) || std::isnan(wetChannelData[b][c][s]))
								wetChannelData[b][c][s] = 0;
							if(state[pots.isb?1:0].id[b][m] == 15) //low pass
								(*modulefilters[c*16+b*4+m].parameters.get()).setCutOffFrequency(samplerate*(pots.oversampling?2:1),calcfilter(1-state[pots.isb?1:0].values[b][m]),1.2);
							else //high pass
								(*modulefilters[c*16+b*4+m].parameters.get()).setCutOffFrequency(samplerate*(pots.oversampling?2:1),calcfilter(state[pots.isb?1:0].values[b][m]),1.1);
							wetChannelData[b][c][s] = modulefilters[c*16+b*4+m].processSample(wetChannelData[b][c][s]);
						}
					}
				}
			}
		}
		for(int s = 0; s < numsamples; ++s) {
			for(int c = 0; c < channelnum; ++c) {
				if(std::isinf(wetChannelData[b][c][s]) || std::isnan(wetChannelData[b][c][s]))
					wetChannelData[b][c][s] = 0;
				wetChannelData[b][c][s] = fmax(fmin(wetChannelData[b][c][s],100),-100);
			}
		}
		if(newremovedc != removedc[b]) {
			removedc[b] = newremovedc;
			for(int c = 0; c < channelnum; ++c) dcfilter.reset(b*channelnum+c);
		}
		if(removedc[b]) {
			for(int s = 0; s < numsamples; ++s) {
				for(int c = 0; c < channelnum; ++c) {
					wetChannelData[b][c][s] = dcfilter.process(wetChannelData[b][c][s],b*channelnum+c);
				}
			}
		}
	}

	for(int s = 0; s < numsamples; ++s) {
		state[pots.isb?1:0].wet = pots.wet.getNextValue();
		for(int b = 0; b < 4; b++) {
			state[pots.isb?1:0].gain[b] = pots.bands[b].gain.getNextValue();
			pots.bands[b].activesmooth = pots.bands[b].bandactive.getNextValue();
			pots.bands[b].bypasssmooth = pots.bands[b].bandbypass.getNextValue();
			float declickease = 1;
			if(declickprogress[b] < 1) {
				declickprogress[b] = fmin(declickprogress[b]+.015625f,1);
				declickease = (cos(declickprogress[b]*3.14159234129f)-1)*-.5f;
			}
			for(int c = 0; c < channelnum; ++c) {
				if(declickease < .999f)
					wetChannelData[b][c][s] = declick[c*4+b]*(1-declickease)+wetChannelData[b][c][s]*declickease;

				dryChannelData[b][c][s] = (
					((double)dryChannelData[b][c][s])*(1-state[pots.isb?1:0].wet*(1-pots.bands[b].bypasssmooth))+
					((double)wetChannelData[b][c][s])*state[pots.isb?1:0].wet*(1-pots.bands[b].bypasssmooth)*pow((double)state[pots.isb?1:0].gain[b]*2,2)
					)*pots.bands[b].activesmooth;
			}
		}
	}

	for(int b = 0; b < 4; b++) {
		if(declickprogress[b] >= .999f) {
			for(int c = 0; c < channelnum; ++c) {
				declick[c*4+b] = wetChannelData[b][c][numsamples-1];
			}
		}
	}

	if(isoversampling) {
		osbuffer.clear();
		for(int b = 0; b < 4; b++) for (int c = 0; c < channelnum; ++c)
			osbuffer.addFrom(c,0,filterbuffers[b],c,0,numsamples);
		os->processSamplesDown(block);
		numsamples = buffer.getNumSamples();
	} else {
		buffer.clear();
		for(int b = 0; b < 4; b++) for (int c = 0; c < channelnum; ++c)
			buffer.addFrom(c,0,filterbuffers[b],c,0,numsamples);
	}

	if(dofft.get()) {
		float* const* channelData = buffer.getArrayOfWritePointers();
		for (int s = 0; s < numsamples; ++s) {
			float avg = 0;
			for (int c = 0; c < channelnum; ++c) avg += channelData[c][s];
			if(channelnum <= 1) pushNextSampleIntoFifo(avg);
			else pushNextSampleIntoFifo(avg/channelnum);
		}
	}
}
void PrismaAudioProcessor::setoversampling(bool toggle) {
	if(!preparedtoplay) return;

	int s = samplerate;
	if(toggle) {
		if(channelnum <= 0) return;
		os->reset();
		setLatencySamples(os->getLatencyInSamples());
		s *= 2;
	} else setLatencySamples(0);

	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++)
			pots.bands[b].modules[m].value.reset(s,.001f);
		pots.bands[b].gain.reset(s,.001f);
	}
	pots.wet.reset(s,.001f);

	dsp::ProcessSpec spec;
	spec.sampleRate = s;
	spec.maximumBlockSize = samplesperblock;
	spec.numChannels = channelnum;
	for(int i = 0; i < 9; i++) crossover[i].prepare(spec);

	dsp::ProcessSpec monospec;
	monospec.sampleRate = s;
	monospec.maximumBlockSize = samplesperblock;
	monospec.numChannels = 1;
	for(int c = 0; c < channelnum; c++) {
		for(int b = 0; b < 4; b++) {
			for(int m = 0; m < 4; m++) {
				if(state[pots.isb?1:0].id[b][m] == 15 || state[pots.isb?1:0].id[b][m] == 16) {
					modulefilters[c*16+b*4+m].prepare(monospec);
				}
			}
		}
	}

	dcfilter.init(s,channelnum*4);
}

void PrismaAudioProcessor::pushNextSampleIntoFifo(float sample) noexcept {
	if(fifoIndex >= fftSize) {
		if(!nextFFTBlockReady.get()) {
			zeromem(fftData,sizeof(fftData));
			memcpy(fftData,fifo,sizeof(fifo));
			nextFFTBlockReady = true;
		}

		fifoIndex = 0;
	}
	fifo[fifoIndex++] = sample;
}
void PrismaAudioProcessor::drawNextFrameOfSpectrum(int fallfactor) {
	window.multiplyWithWindowingTable(fftData,fftSize);
	forwardFFT.performFrequencyOnlyForwardTransform(fftData);

	float mindB = -48.f;
	float maxdB = 0.f;
	float fundstrength = 0.f;

	for(int i = 0; i < scopeSize; ++i) {
		float fftDataIndex = fmin(fmax((mapToLog10(((float)i)/scopeSize,20.f,20000.f)*fftSize)/samplerate,0.f),fftSize*.5f);
		float lerp = (cos(3.14159234129f*fmod(fftDataIndex,1))-1)*-.5f;
		float slope = Decibels::decibelsToGain((((float)i)/scopeSize-.5)*4.5f*9.96578428466209f);
		scopeData[i] = fmax(scopeData[i]*powf(.8f,fallfactor)-.04f*fallfactor,jmap(jlimit(mindB,maxdB,(float)(
			Decibels::gainToDecibels((fftData[(int)floor(fftDataIndex)]*slope)/(fftSize*.5f))*(1-lerp)+
			Decibels::gainToDecibels((fftData[(int)ceil(fftDataIndex)]*slope)/(fftSize*.5f))*lerp)),mindB,maxdB,.0f,1.f));
		if(std::isinf(scopeData[i]) || std::isnan(scopeData[i])) scopeData[i] = 0;
	}
}

bool PrismaAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* PrismaAudioProcessor::createEditor() {
	return new PrismaAudioProcessorEditor(*this,presets[currentpreset],pots);
}

void PrismaAudioProcessor::getStateInformation (MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter << (pots.isb?1:0) << delimiter << currentpreset << delimiter;

	for(int i = 0; i < 2; i++) {
		pluginpreset newstate = state[i];
		for(int b = 0; b < 4; b++) {
			for(int m = 0; m < 4; m++) {
				if(pots.isb == (i>.5))
					data << pots.bands[b].modules[m].value.getTargetValue() << delimiter;
				else
					data << newstate.values[b][m] << delimiter;
				data << newstate.id[b][m] << delimiter;
			}
			if(b >= 1) data << newstate.crossover[b-1] << delimiter;
			if(pots.isb == (i>.5))
				data << pots.bands[b].gain.getTargetValue() << delimiter;
			else
				data << newstate.gain[b] << delimiter;
		}
		if(pots.isb == (i>.5))
			data << pots.wet.getTargetValue() << delimiter;
		else
			data << newstate.wet << delimiter;
	}
	for(int b = 0; b < 4; b++) {
		data << (pots.bands[b].mute?1:0) << delimiter;
		data << (pots.bands[b].solo?1:0) << delimiter;
		data << (pots.bands[b].bypass?1:0) << delimiter;
	}
	data << (pots.oversampling?1:0) << delimiter;

	for(int i = 0; i < getNumPrograms(); i++) {
		for(int b = 0; b < 4; b++) {
			for(int m = 0; m < 4; m++) {
				data << presets[i].values[b][m] << delimiter;
				data << presets[i].id[b][m] << delimiter;
			}
			if(b >= 1) data << presets[i].crossover[b-1] << delimiter;
			data << presets[i].gain[b] << delimiter;
		}
		data << presets[i].wet << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PrismaAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
	const char delimiter = '\n';
	saved = true;
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		apvts.getParameter("ab")->setValueNotifyingHost(std::stoi(token));

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		for(int i = 0; i < 2; i++) {
			for(int b = 0; b < 4; b++) {
				for(int m = 0; m < 4; m++) {
					std::getline(ss, token, delimiter);
					state[i].values[b][m] = std::stof(token);
					std::getline(ss, token, delimiter);
					state[i].id[b][m] = std::stoi(token); // TODO: weird else here
				}
				if(b >= 1) {
					std::getline(ss, token, delimiter);
					state[i].crossover[b-1] = std::stof(token); // ELSE HERE TOO
				}
				std::getline(ss, token, delimiter);
				state[i].gain[b] = std::stof(token);
			}
			std::getline(ss, token, delimiter);
			state[i].wet = std::stof(token);
		}

		for(int b = 0; b < 4; b++) {
			std::getline(ss, token, delimiter);
			pots.bands[b].mute = std::stof(token) > .5;
			apvts.getParameter("b"+(String)b+"mute")->setValueNotifyingHost(std::stof(token));

			std::getline(ss, token, delimiter);
			pots.bands[b].solo = std::stof(token) > .5;
			apvts.getParameter("b"+(String)b+"solo")->setValueNotifyingHost(std::stof(token));

			std::getline(ss, token, delimiter);
			float val = std::stof(token);
			pots.bands[b].bandbypass.setCurrentAndTargetValue(val);
			pots.bands[b].bypasssmooth = val;
			pots.bands[b].bypass = val > .5;
			apvts.getParameter("b"+(String)b+"bypass")->setValueNotifyingHost(val);
		}
		recalcactivebands();
		for(int b = 0; b < 4; b++) {
			pots.bands[b].activesmooth = pots.bands[b].bandactive.getTargetValue();
			pots.bands[b].bandactive.setCurrentAndTargetValue(pots.bands[b].activesmooth);
		}

		std::getline(ss, token, delimiter);
		apvts.getParameter("oversampling")->setValueNotifyingHost(std::stof(token));

		for(int i = 0; i < (saveversion >= 1 ? getNumPrograms() : 8); i++) {
			for(int b = 0; b < 4; b++) {
				for(int m = 0; m < 4; m++) {
					std::getline(ss, token, delimiter);
					if(i != currentpreset) presets[i].values[b][m] = std::stof(token);
					std::getline(ss, token, delimiter);
					if(i != currentpreset) presets[i].id[b][m] = std::stof(token);
				}
				if(b >= 1) {
					std::getline(ss, token, delimiter);
					if(i != currentpreset) presets[i].crossover[b-1] = std::stof(token);
				}
				std::getline(ss, token, delimiter);
				if(i != currentpreset) presets[i].gain[b] = std::stof(token);
			}
			std::getline(ss, token, delimiter);
			if(i != currentpreset) presets[i].wet = std::stof(token);
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

	for(int b = 0; b < 3; b++)
		crossovertruevalue[b] = state[pots.isb].crossover[b];
	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->setValueNotifyingHost(state[pots.isb].values[b][m]);
			pots.bands[b].modules[m].value.setCurrentAndTargetValue(state[pots.isb].values[b][m]);
			apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->setValueNotifyingHost(((float)state[pots.isb].id[b][m])/MODULE_COUNT);
		}
		if(b >= 1) {
			apvts.getParameter("b"+(String)b+"cross")->setValueNotifyingHost(state[pots.isb].crossover[b-1]);
		}
		apvts.getParameter("b"+(String)b+"gain")->setValueNotifyingHost(state[pots.isb].gain[b]);
		pots.bands[b].gain.setCurrentAndTargetValue(state[pots.isb].gain[b]);
	}
	apvts.getParameter("wet")->setValueNotifyingHost(state[pots.isb].wet);
	pots.wet.setCurrentAndTargetValue(state[pots.isb].wet);
}
const String PrismaAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;

	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			data << presets[preset_id].values[b][m] << delimiter;
			data << presets[preset_id].id[b][m] << delimiter;
		}
		if(b >= 1) data << presets[preset_id].crossover[b-1] << delimiter;
		data << presets[preset_id].gain[b] << delimiter;
	}
	data << presets[preset_id].wet << delimiter;

	return data.str();
}
void PrismaAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(preset_id);
	try {
		std::stringstream ss(preset.trim().toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int save_version = std::stoi(token);

		for(int b = 0; b < 4; b++) {
			for(int m = 0; m < 4; m++) {
				std::getline(ss, token, delimiter);
				presets[preset_id].values[b][m] = std::stof(token);
				std::getline(ss, token, delimiter);
				presets[preset_id].id[b][m] = std::stof(token);
			}
			if(b >= 1) {
				std::getline(ss, token, delimiter);
				presets[preset_id].crossover[b-1] = std::stof(token);
			}
			std::getline(ss, token, delimiter);
			presets[preset_id].gain[b] = std::stof(token);
		}
		std::getline(ss, token, delimiter);
		presets[preset_id].wet = std::stof(token);

	} catch (const char* e) {
		error = "Error loading saved data: "+(String)e;
	} catch(String e) {
		error = "Error loading saved data: "+e;
	} catch(std::exception &e) {
		error = "Error loading saved data: "+(String)e.what();
	} catch(...) {
		error = "Error loading saved data";
	}
	if(error != "") {
		if(print_errors)
			logger.debug(error);
		set_preset(revert, preset_id);
		return;
	}

	if(currentpreset != preset_id) return;

	for(int b = 0; b < 3; b++)
		crossovertruevalue[b] = presets[preset_id].crossover[b];
	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->setValueNotifyingHost(presets[preset_id].values[b][m]);
			apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->setValueNotifyingHost(((float)presets[preset_id].id[b][m])/MODULE_COUNT);
		}
		if(b >= 1) {
			apvts.getParameter("b"+(String)b+"cross")->setValueNotifyingHost(presets[preset_id].crossover[b-1]);
		}
		apvts.getParameter("b"+(String)b+"gain")->setValueNotifyingHost(presets[preset_id].gain[b]);
	}
	apvts.getParameter("wet")->setValueNotifyingHost(presets[preset_id].wet);
}
void PrismaAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "ab") {
		switchpreset(newValue > .5);
		return;
	}
	if(parameterID == "oversampling") {
		pots.oversampling = newValue > .5 ? 1 : 0;
		setoversampling(newValue > .5);
		return;
	}
	if(parameterID == "wet") {
		pots.wet.setTargetValue(newValue);
		presets[currentpreset].wet = newValue;
		return;
	}
	int b = std::stoi(std::string(1,parameterID.toStdString()[1]));
	if(parameterID.endsWith("mute")) {
		pots.bands[b].mute = newValue > .5;
		if(newValue < .5 && pots.bands[b].gain.getTargetValue() <= .01f)
			apvts.getParameter("b"+(String)b+"gain")->setValueNotifyingHost(.5f);
		recalcactivebands();
		return;
	}
	if(parameterID.endsWith("solo")) {
		pots.bands[b].solo = newValue > .5;
		recalcactivebands();
		return;
	}
	if(parameterID.endsWith("bypass")) {
		pots.bands[b].bypass = newValue > .5;
		pots.bands[b].bandbypass.setTargetValue(newValue>.5?1.f:0.f);
		return;
	}
	if(parameterID.endsWith("cross")) {
		crossovertruevalue[b-1] = newValue;
		calccross(crossovertruevalue,state[pots.isb?1:0].crossover);

		presets[currentpreset].crossover[0] = state[pots.isb?1:0].crossover[0];
		presets[currentpreset].crossover[1] = state[pots.isb?1:0].crossover[1];
		presets[currentpreset].crossover[2] = state[pots.isb?1:0].crossover[2];

		crossover[0].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[0]));
		crossover[1].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[1]));
		crossover[2].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[2]));
		crossover[3].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[0]));
		crossover[4].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[1]));
		crossover[5].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[2]));
		crossover[6].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[1]));
		crossover[7].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[2]));
		crossover[8].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[2]));

		return;
	}
	if(parameterID.endsWith("gain")) {
		pots.bands[b].gain.setTargetValue(newValue);
		presets[currentpreset].gain[b] = newValue;
		return;
	}
	int m = std::stoi(std::string(1,parameterID.toStdString()[3]));
	if(parameterID.endsWith("val")) {
		pots.bands[b].modules[m].value.setTargetValue(newValue);
		presets[currentpreset].values[b][m] = newValue;
		return;
	}
	if(parameterID.endsWith("id")) {
		if(state[pots.isb?1:0].id[b][m] != 0 && declickprogress[b] >= .999f) declickprogress[b] = 0;
		state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getTargetValue();
		pots.bands[b].modules[m].value.setCurrentAndTargetValue(state[pots.isb?1:0].values[b][m]);

		if(newValue == 10) {
			holdtime[b*4+m] = 0;
			for(int c = 0; c < channelnum; c++) sampleandhold[c*16+b*4+m] = 0;
		} else if(newValue == 15 || newValue == 16) {
			dsp::ProcessSpec monospec;
			monospec.sampleRate = samplerate*(pots.oversampling?2:1);
			monospec.maximumBlockSize = samplesperblock;
			monospec.numChannels = 1;
			for(int c = 0; c < channelnum; c++) {
				modulefilters[c*16+b*4+m].reset();
				(*modulefilters[c*16+b*4+m].parameters.get()).type = newValue == 15 ? dsp::StateVariableFilter::Parameters<float>::Type::lowPass : dsp::StateVariableFilter::Parameters<float>::Type::highPass;
				modulefilters[c*16+b*4+m].prepare(monospec);
			}
		}

		state[pots.isb?1:0].id[b][m] = newValue;
		presets[currentpreset].id[b][m] = newValue;
		return;
	}
}
void PrismaAudioProcessor::calccross(float* input, float* output) {
	output[0] = fmin(fmax(input[0],.02f),.94f);
	output[1] = fmin(fmax(fmax(input[1],input[0]+.02f),.04f),.96f);
	output[2] = fmin(fmax(fmax(fmax(input[2],input[1]+.02f),input[0]+.04f),.06f),.98f);
}
double PrismaAudioProcessor::calcfilter(float val) {
	return mapToLog10(val,20.f,20000.f);
}

void PrismaAudioProcessor::switchpreset(bool isbb) {
	if(isbb == pots.isb) return;

	pluginpreset prevstate = state[!isbb];
	pluginpreset newstate = state[isbb];

	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			prevstate.values[b][m] = pots.bands[b].modules[m].value.getTargetValue();
		}
		prevstate.gain[b] = pots.bands[b].gain.getTargetValue();
	}
	prevstate.wet = pots.wet.getTargetValue();

	transition = true;
	pots.isb = isbb;

	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->setValueNotifyingHost(newstate.values[b][m]);
			apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->setValueNotifyingHost(((float)newstate.id[b][m])/MODULE_COUNT);
		}
		if(b >= 1) apvts.getParameter("b"+(String)b+"cross")->setValueNotifyingHost(newstate.crossover[b-1]);
		apvts.getParameter("b"+(String)b+"gain")->setValueNotifyingHost(newstate.gain[b]);
	}
	apvts.getParameter("wet")->setValueNotifyingHost(newstate.wet);

	state[!isbb] = prevstate;
}
void PrismaAudioProcessor::copypreset(bool isbb) {
	state[isbb?0:1] = state[isbb?1:0];
}

void PrismaAudioProcessor::recalcactivebands() {
	bool soloed = false;
	for(int b = 0; b < 4; b++) {
		if(pots.bands[b].solo) {
			soloed = true;
			break;
		}
	}
	if(soloed) {
		for(int b = 0; b < 4; b++) pots.bands[b].bandactive.setTargetValue(pots.bands[b].solo?1.f:0.f);
	} else {
		for(int b = 0; b < 4; b++) pots.bands[b].bandactive.setTargetValue(pots.bands[b].mute?0.f:1.f);
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PrismaAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout PrismaAudioProcessor::create_parameters() {//72  41
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b0m0val"		,1},"Low Module 1 Value"		,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b0m0id"		,1},"Low Module 1 Type"											,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b0m1val"		,1},"Low Module 2 Value"		,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b0m1id"		,1},"Low Module 2 Type"											,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b0m2val"		,1},"Low Module 3 Value"		,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b0m2id"		,1},"Low Module 3 Type"											,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b0m3val"		,1},"Low Module 4 Value"		,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b0m3id"		,1},"Low Module 4 Type"											,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b0gain"		,1},"Low Gain"					,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.5f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b0mute"		,1},"Low Mute"																			 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b0solo"		,1},"Low Solo"																			 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b0bypass"	,1},"Low Bypass"																		 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b1m0val"		,1},"Low Mid Module 1 Value"	,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b1m0id"		,1},"Low Mid Module 1 Type"										,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b1m1val"		,1},"Low Mid Module 2 Value"	,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b1m1id"		,1},"Low Mid Module 2 Type"										,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b1m2val"		,1},"Low Mid Module 3 Value"	,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b1m2id"		,1},"Low Mid Module 3 Type"										,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b1m3val"		,1},"Low Mid Module 4 Value"	,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b1m3id"		,1},"Low Mid Module 4 Type"										,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b1cross"		,1},"Low Mid Crossover"			,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.25f	,"",AudioProcessorParameter::genericParameter	,tocross		,fromcross		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b1gain"		,1},"Low Mid Gain"				,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.5f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b1mute"		,1},"Low Mid Mute"																		 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b1solo"		,1},"Low Mid Solo"																		 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b1bypass"	,1},"Low Mid Bypass"																	 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b2m0val"		,1},"High Mid Module 1 Value"	,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b2m0id"		,1},"High Mid Module 1 Type"									,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b2m1val"		,1},"High Mid Module 2 Value"	,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b2m1id"		,1},"High Mid Module 2 Type"									,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b2m2val"		,1},"High Mid Module 3 Value"	,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b2m2id"		,1},"High Mid Module 3 Type"									,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b2m3val"		,1},"High Mid Module 4 Value"	,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b2m3id"		,1},"High Mid Module 4 Type"									,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b2cross"		,1},"High Mid Crossover"		,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.5f	,"",AudioProcessorParameter::genericParameter	,tocross		,fromcross		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b2gain"		,1},"High Mid Gain"				,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.5f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b2mute"		,1},"High Mid Mute"																		 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b2solo"		,1},"High Mid Solo"																		 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b2bypass"	,1},"High Mid Bypass"																	 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b3m0val"		,1},"High Module 1 Value"		,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b3m0id"		,1},"High Module 1 Type"										,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b3m1val"		,1},"High Module 2 Value"		,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b3m1id"		,1},"High Module 2 Type"										,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b3m2val"		,1},"High Module 3 Value"		,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b3m2id"		,1},"High Module 3 Type"										,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b3m3val"		,1},"High Module 4 Value"		,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b3m3id"		,1},"High Module 4 Type"										,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b3cross"		,1},"High Crossover"			,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.75f	,"",AudioProcessorParameter::genericParameter	,tocross		,fromcross		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b3gain"		,1},"High Gain"					,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.5f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b3mute"		,1},"High Mute"																			 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b3solo"		,1},"High Solo"																			 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b3bypass"	,1},"High Bypass"																		 ,false	,""												,tobool			,frombool		));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"wet"			,1},"Wet"						,juce::NormalisableRange<float>( 0.0f	,1.0f			),1.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"oversampling",1},"Quality"																			 ,true	,""												,toquality		,fromquality	));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"ab"			,1},"A/B"																				 ,false	,""												,toab			,fromab			));
	return { parameters.begin(), parameters.end() };
}
