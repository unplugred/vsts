#include "processor.h"
#include "editor.h"

PrismaAudioProcessor::PrismaAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts),
	forwardFFT(fftOrder), window(fftSize, dsp::WindowingFunction<float>::hann) {

	init();

	/*
	if(BAND_COUNT > 1) {
		presets[0].name = "Default";
		set_preset("2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.25,0.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.5,0.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.75,0.5,1,4,",0);

		presets[1].name = "Grit";
		set_preset("2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.25,0.5,0.235,10,0.53,13,0.0899999,15,0,0,0,0,0,0,0,0,0,0,0.63,0.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.695,0.5,1,4,",1);

		presets[2].name = "Tight kick";
		set_preset("2,0,12,0.615,13,0.775,8,0.23,13,0,0,0,0,0,0,0,0,0.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.295,0.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.5,0.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.75,0.5,1,4,",2);

	} else {
		presets[0].name = "Default";
		set_preset("2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.5,1,4,",0);
	}
	*/

	for(int i = 0; i < getNumPrograms(); ++i)
		presets[i].name = "Program " + (String)(i+1);

	for(int b = 0; b < BAND_COUNT; ++b) {
		filtercount += fmin(BAND_COUNT-1,b+1);
		removedc[b] = false;
		declickprogress[b] = 1;

		for(int m = 0; m < MAX_MOD; ++m) {
			state[0].values[b][m] = apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->getValue();
			pots.bands[b].modules[m].value.setCurrentAndTargetValue(state[0].values[b][m]);
			add_listener("b"+(String)b+"m"+(String)m+"val");

			state[0].id[b][m] = apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->getValue();
			add_listener("b"+(String)b+"m"+(String)m+"id");
		}

		if(b >= 1) {
			state[0].crossover[b-1] = apvts.getParameter("b"+(String)b+"cross")->getValue();
			crossovertruevalue[b-1] = state[0].crossover[b-1];
			add_listener("b"+(String)b+"cross");
		}

		state[0].gain[b] = apvts.getParameter("b"+(String)b+"gain")->getValue();
		pots.bands[b].gain.setCurrentAndTargetValue(state[0].gain[b]);
		add_listener("b"+(String)b+"gain");

		if(BAND_COUNT > 1) {
			pots.bands[b].mute = apvts.getParameter("b"+(String)b+"mute")->getValue();
			add_listener("b"+(String)b+"mute");

			pots.bands[b].solo = apvts.getParameter("b"+(String)b+"solo")->getValue();
			add_listener("b"+(String)b+"solo");

			pots.bands[b].bypass = apvts.getParameter("b"+(String)b+"bypass")->getValue();
			pots.bands[b].bypasssmooth = pots.bands[b].bypass?1.f:0.f;
			pots.bands[b].bandbypass.setCurrentAndTargetValue(pots.bands[b].bypasssmooth);
			add_listener("b"+(String)b+"bypass");
		} else {
			pots.bands[b].mute = false;
			pots.bands[b].solo = false;
			pots.bands[b].bypass = false;
			pots.bands[b].bypasssmooth = 0;
			pots.bands[b].bandactive.setCurrentAndTargetValue(0);
			pots.bands[b].bandbypass.setCurrentAndTargetValue(0);
		}
	}
	state[0].wet = apvts.getParameter("wet")->getValue();
	pots.wet.setCurrentAndTargetValue(state[0].wet);
	add_listener("wet");

	state[0].modulecount = apvts.getParameter("modulecount")->getValue()*(MAX_MOD-MIN_MOD)+MIN_MOD;
	add_listener("modulecount");

	pots.oversampling = apvts.getParameter("oversampling")->getValue();
	add_listener("oversampling");

	pots.isb = apvts.getParameter("ab")->getValue();
	add_listener("ab");

	crossover.resize(filtercount);

	recalcactivebands();
	for(int b = 0; b < BAND_COUNT; ++b) {
		pots.bands[b].activesmooth = pots.bands[b].bandactive.getTargetValue();
		pots.bands[b].bandactive.setCurrentAndTargetValue(pots.bands[b].activesmooth);
	}

	state[1] = state[0];
	String presetname = presets[currentpreset].name;
	presets[currentpreset] = state[0];
	presets[currentpreset].name = presetname;

	if(BAND_COUNT > 1) {
		for(int i = 0; i < scopeSize; ++i) scopeData[i] = 0;
		for(int i = 0; i < fftSize; ++i) fifo[i] = 0;
		for(int i = 0; i < fftSize*2; ++i) fftData[i] = 0;
	}
}

PrismaAudioProcessor::~PrismaAudioProcessor() {
	close();
}

const String PrismaAudioProcessor::getName() const {
#ifdef PRISMON
	return "Prismon";
#else
	return "Prisma";
#endif
}
bool PrismaAudioProcessor::acceptsMidi() const { return false; }
bool PrismaAudioProcessor::producesMidi() const { return false; }
bool PrismaAudioProcessor::isMidiEffect() const { return false; }
double PrismaAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PrismaAudioProcessor::getNumPrograms() { return 20; }
int PrismaAudioProcessor::getCurrentProgram() { return currentpreset; }
void PrismaAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	transition = true;
	currentpreset = index;

	for(int b = 0; b < (BAND_COUNT-1); ++b)
		crossovertruevalue[b] = presets[currentpreset].crossover[b];
	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int m = 0; m < MAX_MOD; ++m) {
			apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->setValueNotifyingHost(presets[currentpreset].values[b][m]);
			valuesy_gui[b][m] = presets[currentpreset].valuesy[b][m];
			apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->setValueNotifyingHost(((float)presets[currentpreset].id[b][m])/MODULE_COUNT);
		}
		if(b >= 1) apvts.getParameter("b"+(String)b+"cross")->setValueNotifyingHost(presets[currentpreset].crossover[b-1]);
		apvts.getParameter("b"+(String)b+"gain")->setValueNotifyingHost(presets[currentpreset].gain[b]);
	}
	apvts.getParameter("wet")->setValueNotifyingHost(presets[currentpreset].wet);
	apvts.getParameter("modulecount")->setValueNotifyingHost((((float)presets[currentpreset].modulecount)-MIN_MOD)/(MAX_MOD-MIN_MOD));
}
const String PrismaAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void PrismaAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
}

void PrismaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
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

	dsp::ProcessSpec spec;
	spec.sampleRate = samplerate*(pots.oversampling?2:1);
	spec.maximumBlockSize = samplesperblock;
	spec.numChannels = channelnum;

	dsp::ProcessSpec monospec;
	monospec.sampleRate = samplerate*(pots.oversampling?2:1);
	monospec.maximumBlockSize = samplesperblock;
	monospec.numChannels = 1;

	if(BAND_COUNT > 1) {
		//fft
		nextFFTBlockReady = false;
		for(int i = 0; i < fftSize; ++i) fifo[i] = 0;
		for(int i = 0; i < fftSize*2; ++i) fftData[i] = 0;
		fifoIndex = 0;

		//crossover filters
		for(int i = 0; i < filtercount; ++i) {
			crossover[i].reset();
			crossover[i].prepare(spec);
		}
		int startindex = 0;
		int currentfilter = 0;
		for(int b = 0; b < BAND_COUNT; ++b) {
			int index = startindex;
			if(b >= 1) {
				//STAGE 1 - HIGHPASS
				//debug("HP BAND "+((String)b)+" CROSS "+((String)index));
				crossover[currentfilter].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[index]));
				crossover[currentfilter].setType(dsp::LinkwitzRileyFilterType::highpass);
				++index;
				++currentfilter;

				//STAGE 2 - COPY
				//if(index < (BAND_COUNT-1)) debug("COPY BAND "+((String)b)+" TO "+((String)(b+1)));
				startindex = index;
			}
			if(index < (BAND_COUNT-1)) {
				//STAGE 3 - LOWPASS
				//debug("LP BAND "+((String)b)+" CROSS "+((String)index));
				crossover[currentfilter].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[index]));
				crossover[currentfilter].setType(dsp::LinkwitzRileyFilterType::lowpass);
				++index;
				++currentfilter;
			}
			while(index < (BAND_COUNT-1)) {
				//STAGE 4 - ALLPASS
				//debug("AP BAND "+((String)b)+" CROSS "+((String)index));
				crossover[currentfilter].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[index]));
				crossover[currentfilter].setType(dsp::LinkwitzRileyFilterType::allpass);
				++index;
				++currentfilter;
			}
		}
	}

	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int m = 0; m < MAX_MOD; ++m) {
			//ring mod
			ringmod[b*MAX_MOD+m] = 0;

			//sample divide
			holdtime[b*MAX_MOD+m] = 0;

			//parameter smoothing
			pots.bands[b].modules[m].value.reset(samplerate*(pots.oversampling?2:1),.001f);
			pots.bands[b].modules[m].valuey.reset(samplerate*(pots.oversampling?2:1),.001f);
		}
	}

	//sample divide
	sampleandhold.resize(channelnum*BAND_COUNT*MAX_MOD);

	//peak
	modulepeaks.resize(channelnum*BAND_COUNT*MAX_MOD);

	//dc filter
	dcfilter.init(samplerate*(pots.oversampling?2:1),BAND_COUNT*channelnum);

	//declick
	declick.resize(channelnum*BAND_COUNT);

	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int c = 0; c < channelnum; ++c) {
			//sample divide
			for(int m = 0; m < MAX_MOD; ++m)
				sampleandhold[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m] = 0;

			//dc filter
			dcfilter.reset(b*channelnum+c);

			//declick
			declick[c*BAND_COUNT+b] = 0;
		}
		declickprogress[b] = 0;

		for(int m = 0; m < MAX_MOD; ++m) {
			//lowpass+highpass+peak modules
			if(state[pots.isb?1:0].id[b][m] == 15 || state[pots.isb?1:0].id[b][m] == 16) {
				modulefilters[b*MAX_MOD+m].prepare(spec);
				modulefilters[b*MAX_MOD+m].setType(state[pots.isb?1:0].id[b][m] == 15 ? dsp::StateVariableTPTFilterType::lowpass : dsp::StateVariableTPTFilterType::highpass);
				modulefilters[b*MAX_MOD+m].setResonance(state[pots.isb?1:0].id[b][m] == 15 ? 1.2 : 1.1);
				modulefilters[b*MAX_MOD+m].reset();
			} else if(state[pots.isb?1:0].id[b][m] == 17) {
				dsp::IIR::Coefficients<float>::Ptr coefficients = dsp::IIR::Coefficients<float>::makePeakFilter(samplerate*(pots.oversampling?2:1),calcfilter(state[pots.isb?1:0].values[b][m]),1.5f,Decibels::decibelsToGain(state[pots.isb?1:0].valuesy[b][m]*30.f));
				for(int c = 0; c < channelnum; ++c) {
					modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].prepare(monospec);
					*modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].coefficients = *coefficients;
					modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].reset();
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

bool PrismaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void PrismaAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	saved = true;

	for(auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	bool isoversampling = pots.oversampling;

	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);
	if(isoversampling) {
		dsp::AudioBlock<float> osblock = os->processSamplesUp(block);
		for(int i = 0; i < channelnum; ++i)
			ospointerarray[i] = osblock.getChannelPointer(i);
		osbuffer = AudioBuffer<float>(ospointerarray.data(), channelnum, static_cast<int>(osblock.getNumSamples()));
		numsamples = osbuffer.getNumSamples();
	}

	for(int b = 0; b < BAND_COUNT; ++b) {
		if(isoversampling) filterbuffers[b] = osbuffer;
		else filterbuffers[b] = buffer;
	}
	if(BAND_COUNT > 1) {
		int startindex = 0;
		int currentfilter = 0;
		for(int b = 0; b < BAND_COUNT; ++b) {
			dsp::AudioBlock<float> filterblock(filterbuffers[b]);
			dsp::ProcessContextReplacing<float> filtercontext(filterblock);
			int index = startindex;
			if(b >= 1) {
				//STAGE 1 - HIGHPASS
				crossover[currentfilter].process(filtercontext);
				++index;
				++currentfilter;

				//STAGE 2 - COPY
				if(index < (BAND_COUNT-1)) {
					filterbuffers[b+1] = filterbuffers[b];
					startindex = index;
				}
			}
			if(index < (BAND_COUNT-1)) {
				//STAGE 3 - LOWPASS
				crossover[currentfilter].process(filtercontext);
				++index;
				++currentfilter;
			}
			while(index < (BAND_COUNT-1)) {
				//STAGE 4 - ALLPASS
				crossover[currentfilter].process(filtercontext);
				++index;
				++currentfilter;
			}
		}
	}

	float* const* wetChannelData[BAND_COUNT];
	float* const* dryChannelData[BAND_COUNT];
	for(int b = 0; b < BAND_COUNT; ++b) {
		wetbuffers[b] = filterbuffers[b];
		wetChannelData[b] = wetbuffers[b].getArrayOfWritePointers();
		dryChannelData[b] = filterbuffers[b].getArrayOfWritePointers();
	}


	for(int b = 0; b < BAND_COUNT; ++b) {
		bool newremovedc = false;
		for(int m = 0; m < state[pots.isb?1:0].modulecount; ++m) {
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
							double mix = fmin(fmax(holdtime[b*MAX_MOD+m],0),1);
							bool isreplacing = ++holdtime[b*MAX_MOD+m] >= time;
							for(int c = 0; c < channelnum; ++c) {
								double dry = wetChannelData[b][c][s];
								wetChannelData[b][c][s] = dry*(1-mix)+sampleandhold[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m]*mix;
								if(isreplacing) sampleandhold[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m] = dry;
							}
							holdtime[b*MAX_MOD+m] = fmod(holdtime[b*MAX_MOD+m],time);
						}
					}

				//DC
				} else if(state[pots.isb?1:0].id[b][m] == 11) {
					newremovedc = true;
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] != .5f) {
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
							if(state[pots.isb?1:0].values[b][m] != .5f) {
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
						if(state[pots.isb?1:0].values[b][m] != .35f) {
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
							if(state[pots.isb?1:0].values[b][m] != .5f) {
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
						if(state[pots.isb?1:0].id[b][m] == 15) //low pass
							modulefilters[b*MAX_MOD+m].setCutoffFrequency(calcfilter(1-state[pots.isb?1:0].values[b][m]));
						else //high pass
							modulefilters[b*MAX_MOD+m].setCutoffFrequency(calcfilter(state[pots.isb?1:0].values[b][m]));
						for(int c = 0; c < channelnum; ++c) {
							if(std::isinf(wetChannelData[b][c][s]) || std::isnan(wetChannelData[b][c][s]))
								wetChannelData[b][c][s] = 0;
							wetChannelData[b][c][s] = modulefilters[b*MAX_MOD+m].processSample(c,wetChannelData[b][c][s]);
						}
					}

				//PEAK
				} else if(state[pots.isb?1:0].id[b][m] == 17) {
					if(valuesy_gui[b][m].get() != pots.bands[b].modules[m].valuey.getTargetValue()) {
						presets[currentpreset].valuesy[b][m] = valuesy_gui[b][m].get();
						pots.bands[b].modules[m].valuey.setTargetValue(presets[currentpreset].valuesy[b][m]);
					}
					for(int s = 0; s < numsamples; ++s) {
						float prevvalx = state[pots.isb?1:0].values[b][m];
						float prevvaly = state[pots.isb?1:0].valuesy[b][m];
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						state[pots.isb?1:0].valuesy[b][m] = pots.bands[b].modules[m].valuey.getNextValue();
						if(state[pots.isb?1:0].values[b][m] != prevvalx || state[pots.isb?1:0].valuesy[b][m] != prevvaly) {
							dsp::IIR::Coefficients<float>::Ptr coefficients = dsp::IIR::Coefficients<float>::makePeakFilter(samplerate*(pots.oversampling?2:1),calcfilter(state[pots.isb?1:0].values[b][m]),1.5f,Decibels::decibelsToGain(state[pots.isb?1:0].valuesy[b][m]*30.f));
							for(int c = 0; c < channelnum; ++c)
								*modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].coefficients = *coefficients;
						}
						for(int c = 0; c < channelnum; ++c) {
							if(std::isinf(wetChannelData[b][c][s]) || std::isnan(wetChannelData[b][c][s]))
								wetChannelData[b][c][s] = 0;
							wetChannelData[b][c][s] = modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].processSample(wetChannelData[b][c][s]);
						}
					}

				//DRY WET
				} else if(state[pots.isb?1:0].id[b][m] == 18) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						if(state[pots.isb?1:0].values[b][m] > 0) {
							for(int c = 0; c < channelnum; ++c) {
								wetChannelData[b][c][s] = wetChannelData[b][c][s]*(1-state[pots.isb?1:0].values[b][m])+dryChannelData[b][c][s]*state[pots.isb?1:0].values[b][m];
							}
						}
					}

				//STEREO RECTIFY
				} else if(state[pots.isb?1:0].id[b][m] == 19) {
					if(channelnum > 1) {
						newremovedc = true;
						double channeloffset = 0;
						for(int s = 0; s < numsamples; ++s) {
							state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
							if(state[pots.isb?1:0].values[b][m] > 0) {
								for(int c = 0; c < channelnum; ++c) {
									channeloffset = (((double)c)/(channelnum-1))-.5;
									wetChannelData[b][c][s] = wetChannelData[b][c][s]*(state[pots.isb?1:0].values[b][m]*channeloffset*2*(wetChannelData[b][c][s]>0?1:-1)+1);
								}
							}
						}
					}

				//STEREO DC
				} else if(state[pots.isb?1:0].id[b][m] == 20) {
					if(channelnum > 1) {
						newremovedc = true;
						double channeloffset = 0;
						for(int s = 0; s < numsamples; ++s) {
							state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
							for(int c = 0; c < channelnum; ++c) {
								channeloffset = (((double)c)/(channelnum-1))-.5;
								wetChannelData[b][c][s] += pow((double)state[pots.isb?1:0].values[b][m],5)*channeloffset;
							}
						}
					}

				//RING MOD
				} else if(state[pots.isb?1:0].id[b][m] == 21) {
					for(int s = 0; s < numsamples; ++s) {
						state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getNextValue();
						ringmod[b*MAX_MOD+m] = fmod(ringmod[b*MAX_MOD+m]+calcfilter(state[pots.isb?1:0].values[b][m])/(samplerate*(pots.oversampling?2:1)),1.f);
						double mult = cos(ringmod[b*MAX_MOD+m]*MathConstants<double>::twoPi);
						for(int c = 0; c < channelnum; ++c) {
							wetChannelData[b][c][s] *= mult;
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
		for(int b = 0; b < BAND_COUNT; ++b) {
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
					wetChannelData[b][c][s] = declick[c*BAND_COUNT+b]*(1-declickease)+wetChannelData[b][c][s]*declickease;

				dryChannelData[b][c][s] = (
					((double)dryChannelData[b][c][s])*(1-state[pots.isb?1:0].wet*(1-pots.bands[b].bypasssmooth))+
					((double)wetChannelData[b][c][s])*state[pots.isb?1:0].wet*(1-pots.bands[b].bypasssmooth)*pow((double)state[pots.isb?1:0].gain[b]*2,2)
					)*pots.bands[b].activesmooth;
			}
		}
	}

	for(int b = 0; b < BAND_COUNT; ++b) {
		if(declickprogress[b] >= .999f) {
			for(int c = 0; c < channelnum; ++c) {
				declick[c*BAND_COUNT+b] = wetChannelData[b][c][numsamples-1];
			}
		}
	}

	if(isoversampling) {
		osbuffer.clear();
		for(int b = 0; b < BAND_COUNT; ++b) for(int c = 0; c < channelnum; ++c)
			osbuffer.addFrom(c,0,filterbuffers[b],c,0,numsamples);
		os->processSamplesDown(block);
		numsamples = buffer.getNumSamples();
	} else {
		buffer.clear();
		for(int b = 0; b < BAND_COUNT; ++b) for(int c = 0; c < channelnum; ++c)
			buffer.addFrom(c,0,filterbuffers[b],c,0,numsamples);
	}

	if(BAND_COUNT > 1 && dofft.get()) {
		float* const* channelData = buffer.getArrayOfWritePointers();
		for(int s = 0; s < numsamples; ++s) {
			float avg = 0;
			for(int c = 0; c < channelnum; ++c) avg += channelData[c][s];
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

	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int m = 0; m < MAX_MOD; ++m) {
			pots.bands[b].modules[m].value.reset(s,.001f);
			pots.bands[b].modules[m].valuey.reset(s,.001f);
		}
		pots.bands[b].gain.reset(s,.001f);
	}
	pots.wet.reset(s,.001f);

	dsp::ProcessSpec spec;
	spec.sampleRate = s;
	spec.maximumBlockSize = samplesperblock;
	spec.numChannels = channelnum;

	dsp::ProcessSpec monospec;
	monospec.sampleRate = s;
	monospec.maximumBlockSize = samplesperblock;
	monospec.numChannels = 1;

	for(int i = 0; i < filtercount; ++i) crossover[i].prepare(spec);

	for(int i = 0; i < filtercount; ++i) crossover[i].prepare(spec);

	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int m = 0; m < MAX_MOD; ++m) {
			if(state[pots.isb?1:0].id[b][m] == 15 || state[pots.isb?1:0].id[b][m] == 16) {
				modulefilters[b*MAX_MOD+m].prepare(spec);
				modulefilters[b*MAX_MOD+m].setType(state[pots.isb?1:0].id[b][m] == 15 ? dsp::StateVariableTPTFilterType::lowpass : dsp::StateVariableTPTFilterType::highpass);
				modulefilters[b*MAX_MOD+m].setResonance(state[pots.isb?1:0].id[b][m] == 15 ? 1.2 : 1.1);
			} else if(state[pots.isb?1:0].id[b][m] == 17) {
				dsp::IIR::Coefficients<float>::Ptr coefficients = dsp::IIR::Coefficients<float>::makePeakFilter(samplerate*(pots.oversampling?2:1),calcfilter(state[pots.isb?1:0].values[b][m]),1.5f,Decibels::decibelsToGain(state[pots.isb?1:0].valuesy[b][m]*30.f));
				for(int c = 0; c < channelnum; ++c) {
					modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].prepare(monospec);
					*modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].coefficients = *coefficients;
				}
			}
		}
	}

	dcfilter.init(s,BAND_COUNT*channelnum);
}

void PrismaAudioProcessor::pushNextSampleIntoFifo(float sample) noexcept {
	if(BAND_COUNT <= 1) return;
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
	if(BAND_COUNT <= 1) return;
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

void PrismaAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter;
	data << (pots.isb?1:0) << delimiter;
	data << currentpreset << delimiter;

	for(int i = 0; i < 2; ++i) {
		pluginpreset newstate = state[i];
		for(int b = 0; b < BAND_COUNT; ++b) {
			for(int m = 0; m < MAX_MOD; ++m) {
				if(pots.isb == (i>.5))
					data << pots.bands[b].modules[m].value.getTargetValue() << delimiter;
				else
					data << newstate.values[b][m] << delimiter;
				data << newstate.id[b][m] << delimiter;
				if(newstate.id[b][m] == 17)
					data << newstate.valuesy[b][m] << delimiter;
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
		data << newstate.modulecount << delimiter;
	}
	if(BAND_COUNT > 1) {
		for(int b = 0; b < BAND_COUNT; ++b) {
			data << (pots.bands[b].mute?1:0) << delimiter;
			data << (pots.bands[b].solo?1:0) << delimiter;
			data << (pots.bands[b].bypass?1:0) << delimiter;
		}
	}
	data << (pots.oversampling?1:0) << delimiter;

	for(int i = 0; i < getNumPrograms(); ++i) {
		for(int b = 0; b < BAND_COUNT; ++b) {
			for(int m = 0; m < MAX_MOD; ++m) {
				data << presets[i].values[b][m] << delimiter;
				data << presets[i].id[b][m] << delimiter;
				if(presets[i].id[b][m] == 17)
					data << presets[i].valuesy[b][m] << delimiter;
			}
			if(b >= 1) data << presets[i].crossover[b-1] << delimiter;
			data << presets[i].gain[b] << delimiter;
		}
		data << presets[i].wet << delimiter;
		data << presets[i].modulecount << delimiter;
	}

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void PrismaAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	//*/
	const char delimiter = '\n';
	saved = true;
	int saveversion = version;
	try {
		std::stringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		apvts.getParameter("ab")->setValueNotifyingHost(std::stoi(token));

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		for(int i = 0; i < 2; ++i) {
			for(int b = 0; b < BAND_COUNT; ++b) {
				for(int m = 0; m < (saveversion>=2?MAX_MOD:4); ++m) {
					std::getline(ss, token, delimiter);
					state[i].values[b][m] = std::stof(token);
					std::getline(ss, token, delimiter);
					state[i].id[b][m] = std::stoi(token);
					if(state[i].id[b][m] == 17) {
						std::getline(ss, token, delimiter);
						state[i].valuesy[b][m] = std::stof(token);
					}
				}
				if(b >= 1) {
					std::getline(ss, token, delimiter);
					state[i].crossover[b-1] = std::stof(token);
				}
				std::getline(ss, token, delimiter);
				state[i].gain[b] = std::stof(token);
			}
			std::getline(ss, token, delimiter);
			state[i].wet = std::stof(token);

			if(saveversion >= 2) {
				std::getline(ss, token, delimiter);
				state[i].modulecount = std::stof(token);
			}
		}

		if(BAND_COUNT > 1) {
			for(int b = 0; b < BAND_COUNT; ++b) {
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
			for(int b = 0; b < BAND_COUNT; ++b) {
				pots.bands[b].activesmooth = pots.bands[b].bandactive.getTargetValue();
				pots.bands[b].bandactive.setCurrentAndTargetValue(pots.bands[b].activesmooth);
			}
		}

		std::getline(ss, token, delimiter);
		apvts.getParameter("oversampling")->setValueNotifyingHost(std::stof(token));

		for(int i = 0; i < (saveversion>=1?getNumPrograms():8); ++i) {
			for(int b = 0; b < BAND_COUNT; ++b) {
				for(int m = 0; m < (saveversion>=2?MAX_MOD:4); ++m) {
					std::getline(ss, token, delimiter);
					if(i != currentpreset) presets[i].values[b][m] = std::stof(token);
					std::getline(ss, token, delimiter);
					if(i != currentpreset) presets[i].id[b][m] = std::stoi(token);
					if(std::stoi(token) == 17) {
						std::getline(ss, token, delimiter);
						if(i != currentpreset) presets[i].valuesy[b][m] = std::stof(token);
					}
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

			if(saveversion >= 2) {
				std::getline(ss, token, delimiter);
				if(i != currentpreset) presets[i].modulecount = std::stof(token);
			}
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

	for(int b = 0; b < (BAND_COUNT-1); ++b)
		crossovertruevalue[b] = state[pots.isb?1:0].crossover[b];
	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int m = 0; m < (saveversion>=2?MAX_MOD:4); ++m) {
			apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->setValueNotifyingHost(state[pots.isb?1:0].values[b][m]);
			pots.bands[b].modules[m].value.setCurrentAndTargetValue(state[pots.isb?1:0].values[b][m]);
			if(state[pots.isb?1:0].id[b][m] == 17) {
				valuesy_gui[b][m] = state[pots.isb?1:0].valuesy[b][m];
				pots.bands[b].modules[m].valuey.setCurrentAndTargetValue(state[pots.isb?1:0].valuesy[b][m]);
				presets[currentpreset].valuesy[b][m] = state[pots.isb?1:0].valuesy[b][m];
			}
			apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->setValueNotifyingHost(((float)state[pots.isb?1:0].id[b][m])/MODULE_COUNT);
		}
		if(b >= 1) {
			apvts.getParameter("b"+(String)b+"cross")->setValueNotifyingHost(state[pots.isb?1:0].crossover[b-1]);
		}
		apvts.getParameter("b"+(String)b+"gain")->setValueNotifyingHost(state[pots.isb?1:0].gain[b]);
		pots.bands[b].gain.setCurrentAndTargetValue(state[pots.isb?1:0].gain[b]);
	}
	apvts.getParameter("wet")->setValueNotifyingHost(state[pots.isb?1:0].wet);
	pots.wet.setCurrentAndTargetValue(state[pots.isb?1:0].wet);
	apvts.getParameter("modulecount")->setValueNotifyingHost((((float)state[pots.isb?1:0].modulecount)-MIN_MOD)/(MAX_MOD-MIN_MOD));
	//*/
}
const String PrismaAudioProcessor::get_preset(int preset_id, const char delimiter) {
	std::ostringstream data;

	data << version << delimiter;

	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int m = 0; m < MAX_MOD; ++m) {
			data << presets[preset_id].values[b][m] << delimiter;
			data << presets[preset_id].id[b][m] << delimiter;
			if(presets[preset_id].id[b][m] == 17)
				data << presets[preset_id].valuesy[b][m] << delimiter;
		}
		if(b >= 1) data << presets[preset_id].crossover[b-1] << delimiter;
		data << presets[preset_id].gain[b] << delimiter;
	}
	data << presets[preset_id].wet << delimiter;
	data << presets[preset_id].modulecount << delimiter;

	return data.str();
}
void PrismaAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(preset_id);
	int saveversion = version;
	try {
		std::stringstream ss(preset.trim().toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		saveversion = std::stoi(token);

		for(int b = 0; b < BAND_COUNT; ++b) {
			for(int m = 0; m < (saveversion>=2?MAX_MOD:4); ++m) {
				std::getline(ss, token, delimiter);
				presets[preset_id].values[b][m] = std::stof(token);
				std::getline(ss, token, delimiter);
				presets[preset_id].id[b][m] = std::stof(token);
				if(presets[preset_id].id[b][m] == 17) {
					std::getline(ss, token, delimiter);
					presets[preset_id].valuesy[b][m] = std::stof(token);
				}
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

		if(saveversion >= 2) {
			std::getline(ss, token, delimiter);
			presets[preset_id].modulecount = std::stof(token);
		}

	} catch(const char* e) {
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
			debug(error);
		set_preset(revert, preset_id);
		return;
	}

	if(currentpreset != preset_id) return;

	for(int b = 0; b < (BAND_COUNT-1); ++b)
		crossovertruevalue[b] = presets[preset_id].crossover[b];
	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int m = 0; m < (saveversion>=2?MAX_MOD:4); ++m) {
			apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->setValueNotifyingHost(presets[preset_id].values[b][m]);
			if(presets[preset_id].id[b][m] == 17)
				valuesy_gui[b][m] = presets[preset_id].valuesy[b][m];
			apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->setValueNotifyingHost(((float)presets[preset_id].id[b][m])/MODULE_COUNT);
		}
		if(b >= 1) {
			apvts.getParameter("b"+(String)b+"cross")->setValueNotifyingHost(presets[preset_id].crossover[b-1]);
		}
		apvts.getParameter("b"+(String)b+"gain")->setValueNotifyingHost(presets[preset_id].gain[b]);
	}
	apvts.getParameter("wet")->setValueNotifyingHost(presets[preset_id].wet);
	apvts.getParameter("modulecount")->setValueNotifyingHost((((float)state[pots.isb?1:0].modulecount)-MIN_MOD)/(MAX_MOD-MIN_MOD));
}
void PrismaAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "modulecount") {
		for(int b = 0; b < BAND_COUNT; ++b) {
			for(int m = fmin(newValue,state[pots.isb?1:0].modulecount); m < fmax(newValue,state[pots.isb?1:0].modulecount); ++m) {
				if(state[pots.isb?1:0].id[b][m] != 0 && declickprogress[b] >= .999f)
					declickprogress[b] = 0;
				if(newValue > state[pots.isb?1:0].modulecount) {
					if(state[pots.isb?1:0].id[b][m] == 10) {
						holdtime[b*MAX_MOD+m] = 0;
						for(int c = 0; c < channelnum; ++c) sampleandhold[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m] = 0;
					} else if((state[pots.isb?1:0].id[b][m] == 15 || state[pots.isb?1:0].id[b][m] == 16) && channelnum > 0 && samplesperblock > 0) {
						dsp::ProcessSpec spec;
						spec.sampleRate = samplerate*(pots.oversampling?2:1);
						spec.maximumBlockSize = samplesperblock;
						spec.numChannels = channelnum;
						modulefilters[b*MAX_MOD+m].prepare(spec);
						modulefilters[b*MAX_MOD+m].setType(state[pots.isb?1:0].id[b][m] == 15 ? dsp::StateVariableTPTFilterType::lowpass : dsp::StateVariableTPTFilterType::highpass);
						modulefilters[b*MAX_MOD+m].setResonance(state[pots.isb?1:0].id[b][m] == 15 ? 1.2 : 1.1);
					} else if(state[pots.isb?1:0].id[b][m] == 17) {
						dsp::ProcessSpec monospec;
						monospec.sampleRate = samplerate*(pots.oversampling?2:1);
						monospec.maximumBlockSize = samplesperblock;
						monospec.numChannels = 1;
						dsp::IIR::Coefficients<float>::Ptr coefficients = dsp::IIR::Coefficients<float>::makePeakFilter(samplerate*(pots.oversampling?2:1),calcfilter(state[pots.isb?1:0].values[b][m]),1.5f,Decibels::decibelsToGain(state[pots.isb?1:0].valuesy[b][m]*30.f));
						for(int c = 0; c < channelnum; ++c) {
							modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].prepare(monospec);
							*modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].coefficients = *coefficients;
						}
					} else if(state[pots.isb?1:0].id[b][m] == 21) {
						ringmod[b*MAX_MOD+m] = 0;
					}
				}
			}
		}
		state[pots.isb?1:0].modulecount = newValue;
		presets[currentpreset].modulecount = newValue;
		return;
	}
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

		for(int b = 0; b < (BAND_COUNT-1); ++b)
			presets[currentpreset].crossover[b] = state[pots.isb?1:0].crossover[b];

		int startindex = 0;
		int currentfilter = 0;
		for(int b = 0; b < BAND_COUNT; ++b) {
			int index = startindex;
			if(b >= 1) {
				//STAGE 1 - HIGHPASS
				crossover[currentfilter].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[index]));
				++index;
				++currentfilter;

				//STAGE 2 - COPY
				startindex = index;
			}
			if(index < (BAND_COUNT-1)) {
				//STAGE 3 - LOWPASS
				crossover[currentfilter].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[index]));
				++index;
				++currentfilter;
			}
			while(index < (BAND_COUNT-1)) {
				//STAGE 4 - ALLPASS
				crossover[currentfilter].setCutoffFrequency(calcfilter(state[pots.isb?1:0].crossover[index]));
				++index;
				++currentfilter;
			}
		}

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
		state[pots.isb?1:0].values[b][m] = pots.bands[b].modules[m].value.getTargetValue();
		pots.bands[b].modules[m].value.setCurrentAndTargetValue(state[pots.isb?1:0].values[b][m]);

		if(m < state[pots.isb?1:0].modulecount) {
			if(state[pots.isb?1:0].id[b][m] != 0 && declickprogress[b] >= .999f)
				declickprogress[b] = 0;
			if(newValue == 10) {
				holdtime[b*MAX_MOD+m] = 0;
				for(int c = 0; c < channelnum; ++c) sampleandhold[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m] = 0;
			} else if((newValue == 15 || newValue == 16) && channelnum > 0 && samplesperblock > 0) {
				dsp::ProcessSpec spec;
				spec.sampleRate = samplerate*(pots.oversampling?2:1);
				spec.maximumBlockSize = samplesperblock;
				spec.numChannels = channelnum;
				modulefilters[b*MAX_MOD+m].prepare(spec);
				modulefilters[b*MAX_MOD+m].setType(newValue == 15 ? dsp::StateVariableTPTFilterType::lowpass : dsp::StateVariableTPTFilterType::highpass);
				modulefilters[b*MAX_MOD+m].setResonance(state[pots.isb?1:0].id[b][m] == 15 ? 1.2 : 1.1);
			} else if(newValue == 17) {
				state[pots.isb?1:0].valuesy[b][m] = valuesy_gui[b][m].get();
				presets[currentpreset].valuesy[b][m] = state[pots.isb?1:0].valuesy[b][m];
				pots.bands[b].modules[m].valuey.setCurrentAndTargetValue(state[pots.isb?1:0].valuesy[b][m]);
				dsp::ProcessSpec monospec;
				monospec.sampleRate = samplerate*(pots.oversampling?2:1);
				monospec.maximumBlockSize = samplesperblock;
				monospec.numChannels = 1;
				dsp::IIR::Coefficients<float>::Ptr coefficients = dsp::IIR::Coefficients<float>::makePeakFilter(samplerate*(pots.oversampling?2:1),calcfilter(state[pots.isb?1:0].values[b][m]),1.5f,Decibels::decibelsToGain(state[pots.isb?1:0].valuesy[b][m]*30.f));
				for(int c = 0; c < channelnum; ++c) {
					modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].prepare(monospec);
					*modulepeaks[c*BAND_COUNT*MAX_MOD+b*MAX_MOD+m].coefficients = *coefficients;
				}
			} else if(newValue == 21) {
				ringmod[b*MAX_MOD+m] = 0;
			}
		}

		state[pots.isb?1:0].id[b][m] = newValue;
		presets[currentpreset].id[b][m] = newValue;
		return;
	}
}
void PrismaAudioProcessor::calccross(float* input, float* output) {
	float prevout = 0;
	for(int b = 0; b < (BAND_COUNT-1); ++b) {
		output[b] = fmin(fmax(fmax(input[b],prevout+.02f),.02f*(b+1)),1.f-.02f*(BAND_COUNT-1-b));
		prevout = output[b];
	}
}
double PrismaAudioProcessor::calcfilter(float val) {
	return mapToLog10(val,20.f,20000.f);
}

void PrismaAudioProcessor::switchpreset(bool isbb) {
	if(isbb == pots.isb) return;

	pluginpreset prevstate = state[!isbb];
	pluginpreset newstate = state[isbb];

	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int m = 0; m < MAX_MOD; ++m) {
			prevstate.values[b][m] = pots.bands[b].modules[m].value.getTargetValue();
			prevstate.valuesy[b][m] = pots.bands[b].modules[m].valuey.getTargetValue();
		}
		prevstate.gain[b] = pots.bands[b].gain.getTargetValue();
	}
	prevstate.wet = pots.wet.getTargetValue();

	transition = true;
	pots.isb = isbb;

	for(int b = 0; b < (BAND_COUNT-1); ++b)
		crossovertruevalue[b] = newstate.crossover[b];
	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int m = 0; m < MAX_MOD; ++m) {
			apvts.getParameter("b"+(String)b+"m"+(String)m+"val")->setValueNotifyingHost(newstate.values[b][m]);
			valuesy_gui[b][m] = newstate.valuesy[b][m];
			presets[currentpreset].valuesy[b][m] = newstate.valuesy[b][m];
			apvts.getParameter("b"+(String)b+"m"+(String)m+"id")->setValueNotifyingHost(((float)newstate.id[b][m])/MODULE_COUNT);
		}
		if(b >= 1) apvts.getParameter("b"+(String)b+"cross")->setValueNotifyingHost(newstate.crossover[b-1]);
		apvts.getParameter("b"+(String)b+"gain")->setValueNotifyingHost(newstate.gain[b]);
	}
	apvts.getParameter("wet")->setValueNotifyingHost(newstate.wet);
	apvts.getParameter("modulecount")->setValueNotifyingHost((((float)newstate.modulecount)-MIN_MOD)/(MAX_MOD-MIN_MOD));

	state[!isbb] = prevstate;
}
void PrismaAudioProcessor::copypreset(bool isbb) {
	state[isbb?0:1] = state[isbb?1:0];
}

void PrismaAudioProcessor::recalcactivebands() {
	bool soloed = false;
	for(int b = 0; b < BAND_COUNT; ++b) {
		if(pots.bands[b].solo) {
			soloed = true;
			break;
		}
	}
	if(soloed) {
		for(int b = 0; b < BAND_COUNT; ++b) pots.bands[b].bandactive.setTargetValue(pots.bands[b].solo?1.f:0.f);
	} else {
		for(int b = 0; b < BAND_COUNT; ++b) pots.bands[b].bandactive.setTargetValue(pots.bands[b].mute?0.f:1.f);
	}
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PrismaAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout PrismaAudioProcessor::create_parameters() {//72  41
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	for(int b = 0; b < BAND_COUNT; ++b) {
		String name = "";
		if(b == 0) {
			if(BAND_COUNT > 1)
				name = "Low ";
		} else if(b == (BAND_COUNT-1)) {
			name = "High ";
		} else if(b == 1) {
			if(BAND_COUNT == 3)
				name = "Mid ";
			else
				name = "Low Mid ";
		} else if(b == 2) {
			if(BAND_COUNT == 5)
				name = "Mid ";
			else
				name = "High Mid ";
		} else {
			name = "High Mid ";
		}
		for(int m = 0; m < MAX_MOD; ++m) {
			parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b"+((String)b)+"m"+((String)m)+"val"	,1},name+"Module "+((String)(m+1))+" Value"	,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
			parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"b"+((String)b)+"m"+((String)m)+"id"	,1},name+"Module "+((String)(m+1))+" Type"									,0		,MODULE_COUNT	 ,0		,""												,toid			,fromid			));
		}
		if(b >= 1) {
			float def = ((float)b)/BAND_COUNT;
			parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"b"+((String)b)+"cross"				,1},name+"Crossover"						,juce::NormalisableRange<float>( 0.0f	,1.0f			),def	,"",AudioProcessorParameter::genericParameter	,tocross		,fromcross		));
		}
		parameters.push_back(	std::make_unique<AudioParameterFloat	>(ParameterID{"b"+((String)b)+"gain"				,1},name+"Gain"								,juce::NormalisableRange<float>( 0.0f	,1.0f			),0.5f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
		if(BAND_COUNT > 1) {
			parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b"+((String)b)+"mute"				,1},name+"Mute"																						 ,false	,""												,tobool			,frombool		));
			parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b"+((String)b)+"solo"				,1},name+"Solo"																						 ,false	,""												,tobool			,frombool		));
			parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"b"+((String)b)+"bypass"				,1},name+"Bypass"																					 ,false	,""												,tobool			,frombool		));
		}
	}
	parameters.push_back(		std::make_unique<AudioParameterFloat	>(ParameterID{"wet"									,1},"Wet"									,juce::NormalisableRange<float>( 0.0f	,1.0f			),1.0f	,"",AudioProcessorParameter::genericParameter	,tonormalized	,fromnormalized	));
	parameters.push_back(		std::make_unique<AudioParameterBool		>(ParameterID{"oversampling"						,1},"Quality"																						 ,true	,""												,toquality		,fromquality	));
	parameters.push_back(		std::make_unique<AudioParameterBool		>(ParameterID{"ab"									,1},"A/B"																							 ,false	,""												,toab			,fromab			));
	parameters.push_back(		std::make_unique<AudioParameterInt		>(ParameterID{"modulecount"							,1},"Module Count"															,MIN_MOD,MAX_MOD		 ,DEF_MOD																				));
	return { parameters.begin(), parameters.end() };
}
