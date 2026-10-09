#include "processor.h"
#include "editor.h"
#include "curves.h"

FMPlusAudioProcessor::FMPlusAudioProcessor() :
	apvts(*this, &undo_manager, "Parameters", create_parameters()),
	plugmachine_dsp(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true), &apvts) {

	init();

	// MEMORY ALLOCATION
	pages[ 0] = &page_gen;
	pages[ 1] = &page_op;
	pages[ 2] = &page_vis;
	pages[ 3] = &page_filter;
	pages[ 4] = &page_dist;
	pages[ 5] = &page_phaser;
	pages[ 6] = &page_chorus;
	pages[ 7] = &page_eq;
	pages[ 8] = &page_comp;
	pages[ 9] = &page_delay;
	pages[10] = &page_reverb;
	for(int p = 0; p < PC; ++p) {
		pages[p]->smooth.resize(pages[p]->params.size());
		for(int i = 0; i < pages[p]->params.size(); ++i)
			pages[p]->smooth[i].resize(pages[p]->instances);

		pages[p]->state.value.resize(pages[p]->params.size());
		pages[p]->valuenorm.resize(pages[p]->params.size());
		pages[p]->knobbox.resize(pages[p]->params.size());
		pages[p]->adsr.resize(pages[p]->adsrnum);
		for(int i = 0; i < pages[p]->params.size(); ++i) {
			pages[p]->state.value[i].resize(pages[p]->instances);
			pages[p]->valuenorm[i].resize(pages[p]->instances);
			if(pages[p]->params[i].adsr >= 0)
				pages[p]->adsr[pages[p]->params[i].adsr].resize(pages[p]->adsr[pages[p]->params[i].adsr].size()+4);
		}

		pages[p]->presets.resize(getNumPrograms());
		for(int n = 0; n < getNumPrograms(); ++n) {
			pages[p]->presets[n].value.resize(pages[p]->params.size());
			for(int i = 0; i < pages[p]->params.size(); ++i)
				pages[p]->presets[n].value[i].resize(pages[p]->instances);
			pages[p]->presets[n].points.resize(pages[p]->curvesnum);
			for(int i = 0; i < pages[p]->curvesnum; ++i)
				pages[p]->presets[n].points[i].resize(pages[p]->instances);
		}

		for(int i = 0; i < pages[p]->curvesnum; ++i) {
			pages[p]->curves[i].c  .resize(pages[p]->instances);
			pages[p]->curves[i].cui.resize(pages[p]->instances);
			pages[p]->curves[i].updatedcurve.resize(pages[p]->instances);
			for(int o = 0; o < pages[p]->instances; ++o) {
				pages[p]->curves[i].c  [o].resizechannels(pages[p]->curves[i].channels);
				pages[p]->curves[i].cui[o].resizechannels(1);
				curve::pointsfromstring(pages[p]->curves[i].c[o].points,"3,0,0,0.5,0.5,1,0.5,1,0,0.5");
				pages[p]->curves[i].cui[o].points = pages[p]->curves[i].c[o].points;
			}
		}

		pages[p]->knobsmooth.reset(.05f,30,pages[p]->params.size());

		if(pages[p]->adsrnum > 0)
			pages[p]->dampadsr.reset(.05f,30,pages[p]->adsrnum*pages[p]->adsr[0].size()/2);
	}

	resettheme();
	PropertiesFile* user_settings = props.getUserSettings();
	if(user_settings->containsKey("tuningdir"))
		params.tuningdir   = File(user_settings->getValue("tuningdir"));
	if(user_settings->containsKey("themedir" ))
		params.themedir    = File(user_settings->getValue("themedir" ));
	if(user_settings->containsKey("themefile"))
		page_gen.themefile =      user_settings->getValue("themefile") ;
	for(int i = 0; i < 13; ++i)
		if(user_settings->containsKey("theme"+((String)i)))
			params.theme[i] = (unsigned char)user_settings->getIntValue("theme"+((String)i));

	for(int n = 0; n < getNumPrograms(); ++n) { // TODO stringify default preset
		presets[n] = pluginpreset(n==0?"Init":("Program "+(String)n));
		for(int p = 0; p < PC; ++p) {
			for(int i = 0; i < pages[p]->params.size(); ++i) for(int o = 0; o < pages[p]->instances; ++o)
				pages[p]->presets[n].value[i][o] = 0;
			for(int i = 0; i < pages[p]->curvesnum; ++i) for(int o = 0; o < pages[p]->instances; ++o)
				pages[p]->presets[n].points[i][o] = pages[p]->curves[i].c[o].points;
		}
		for(int o = 0; o < (MC+1); ++o) {
			presets[n].oppos[      o *2  ] = 56;
			presets[n].oppos[      o *2+1] = 180-o*22;
			for(int i = 0; i < 3; ++i) {
				presets[n].inputs   [o][i] = -1;
				presets[n].outputs  [o][i] = -1;
				presets[n].influence[o][i] = 50;
			}
		}
		for(int o = 0; o < FXC; ++o) {
			presets[n].oppos[(MC+1+o)*2  ] = round((fmod(o,2)*.7f+.15f)*133.f-pages[o+3]->tabname.length()*3.5f);
			presets[n].oppos[(MC+1+o)*2+1] = round(136.5f+10-8.f-(ceil(FXC/2)*.5f-floor(o/2))*22);
		}

		presets[n].inputs [0][0] = 1;
		presets[n].outputs[1][0] = 0;

		presets[n].inputs [0][1] = 2;
		presets[n].outputs[2][0] = 0;

		presets[n].inputs [2][0] = 2;
		presets[n].outputs[2][1] = 2;

		presets[n].inputs [1][0] = 3;
		presets[n].outputs[3][0] = 1;
	}
	page_gen.presetname = presets[currentpreset].name;

	// PARAMETER INITIALIZATION
	for(int p = 0; p < PC; ++p) {
		for(int i = 0; i < pages[p]->params.size(); ++i) {
			String id = pages[p]->params[i].id;
			String oid = id;
			for(int o = 0; o < pages[p]->instances; ++o) {
				if(p == 1) oid = "o"+(String)o+id;
				pages[p]->state.value[i][o] = pages[p]->params[i].inflate(apvts.getParameter(oid)->getValue());
				pages[p]->presets[currentpreset].value[i][o] = pages[p]->state.value[i][o];
				if(pages[p]->params[i].smoothtime > 0) pages[p]->smooth[i][o].setCurrentAndTargetValue(pages[p]->state.value[i][o]);
				add_listener(oid);
			}
			pages[p]->params[i].defaultvalue = pages[p]->state.value[i][0]; // TODO per op defaults
		}
	}

	midihandle.page_gen = &page_gen;
	resettuning();

	for(int v = 0; v < VC; ++v)
		vibattack[v] = 0;

	for(int o = 0; o < (MC*5); ++o)
		egd[o] = 0;
	for(int o = 0; o < MC; ++o) {
		ega[o*5  ] = 1;
		ega[o*5+3] = 0;
		ega[o*5+4] = 0;
	}
	for(int v = 0; v < (MC*VC); ++v) {
		egstage  [v] = 4;
		egprog   [v] = 0;
		eglast   [v] = 0;
		egsustain[v] = 0;
	}

	for(int o = 0; o < (MC+1); ++o)
		indicators[o] = 0;
}

FMPlusAudioProcessor::~FMPlusAudioProcessor(){
	close();
}

const String FMPlusAudioProcessor::getName() const { return "FM+"; }
bool FMPlusAudioProcessor::acceptsMidi() const { return true; }
bool FMPlusAudioProcessor::producesMidi() const { return false; }
bool FMPlusAudioProcessor::isMidiEffect() const { return false; }
double FMPlusAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int FMPlusAudioProcessor::getNumPrograms() { return 20; }
int FMPlusAudioProcessor::getCurrentProgram() { return currentpreset; }
void FMPlusAudioProcessor::setCurrentProgram(int index) {
	if(currentpreset == index) return;

	undo_manager.beginNewTransaction((String)"Changed preset to " += presets[index].name);
	currentpreset = index;
	page_gen.presetname = presets[currentpreset].name;
	updatepreset = true;

	for(int p = 0; p < PC; ++p) {
		for(int i = 0; i < pages[p]->params.size(); ++i) {
			String id = pages[p]->params[i].id;
			String oid = id;
			for(int o = 0; o < pages[p]->instances; ++o) {
				if(p == 1) oid = "o"+(String)o+id;
				apvts.getParameter(oid)->setValueNotifyingHost(pages[p]->params[i].normalize(pages[p]->presets[currentpreset].value[i][o]));
			}
		}

		for(int i = 0; i < pages[p]->curvesnum; ++i) for(int o = 0; o < pages[p]->instances; ++o)
			pages[p]->curves[i].updatedcurve[o] = true;
	}

	updatevis = true;
}
const String FMPlusAudioProcessor::getProgramName(int index) {
	return { presets[index].name };
}
void FMPlusAudioProcessor::changeProgramName(int index, const String& newName) {
	presets[index].name = newName;
	if(index == currentpreset) {
		page_gen.presetname = presets[currentpreset].name;
		updatepreset = true;
	}
}

void FMPlusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
	if(!saved && sampleRate > 60000)
		apvts.getParameter("antialias")->setValueNotifyingHost(.5f);
	samplesperblock = samplesPerBlock;
	samplerate = sampleRate;

	reseteverything();
}
void FMPlusAudioProcessor::changechannelnum(int newchannelnum) {
	channelnum = newchannelnum;
	if(newchannelnum <= 0) return;

	reseteverything();
}
void FMPlusAudioProcessor::reseteverything() {
	if(channelnum <= 0 || samplesperblock <= 0) return;

	ospointerarray.resize(channelnum);
	for(int i = 0; i < 3; ++i) {
		os[i].reset(new dsp::Oversampling<float>(channelnum,i+1,dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));
		os[i]->initProcessing(samplesperblock);
		os[i]->setUsingIntegerLatency(true);
	}
	preparedtoplay = true;
	setoversampling();

	for(int p = 0; p < PC; ++p)
		for(int i = 0; i < pages[p]->curvesnum; ++i) for(int o = 0; o < pages[p]->instances; ++o) // TODO resize state channels ??
			pages[p]->curves[i].updatedcurve[o] = true;
}
void FMPlusAudioProcessor::releaseResources() { }

bool FMPlusAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
	if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;

	return (layouts.getMainInputChannels() > 0);
}

void oscillator::reset(int channelnum, int samplesperblock) {
	fb.resize(channelnum                );
	p .resize(channelnum                );
	a .resize(channelnum*samplesperblock);
	f .resize(           samplesperblock);
	w .resize(           samplesperblock);
	for(int c = 0; c < channelnum; ++c) {
		fb[c] = 0;
		p [c] = 0;
	}
}

void FMPlusAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
	ScopedNoDenormals noDenormals;

	if(buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;
	if(buffer.getNumChannels() != channelnum)
		changechannelnum(buffer.getNumChannels());
	saved = true;

	// clear buffer
	for(int i = 0; i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	int newindex = fmin(3,fmax(0,floor(page_gen.state.value[15][0]*8-4)));
	if(newindex != osindex) setoversampling();

	if(updateconnections.get()) {
		updateconnections = false;
		// TODO !!!!!!!!!!
	}

	// update fx order
	if(updatefxorder.get()) {
		updatefxorder = false;
		for(int o = 0; o < FXC; ++o)
			fxorder[o] = presets[currentpreset].fxorder[o];
	}

	// update curve
	for(int p = 0; p < PC; ++p) {
		for(int i = 0; i < pages[p]->curvesnum; ++i) for(int o = 0; o < pages[p]->instances; ++o) {
			if(pages[p]->curves[i].updatedcurve[o].get()) {
				pages[p]->curves[i].updatedcurve[o] = false;
				pages[p]->curves[i].c[o].points = pages[p]->presets[currentpreset].points[i][o];
			}
		}
	}

	// upsample ??
	int numsamples = buffer.getNumSamples();
	dsp::AudioBlock<float> block(buffer);
	int oversampleamount = pow(2,osindex);
	int ossamplerate = samplerate*oversampleamount;
	if(osindex > 0) { // TODO only process down
		dsp::AudioBlock<float> osblock = os[osindex-1]->processSamplesUp(block);
		for(int i = 0; i < channelnum; ++i)
			ospointerarray[i] = osblock.getChannelPointer(i);
		osbuffer = AudioBuffer<float>(ospointerarray.data(), channelnum, static_cast<int>(osblock.getNumSamples()));
		numsamples = osbuffer.getNumSamples();
	}

	// get write pointers
	float* const* channelData;
	if(osindex > 0) channelData = osbuffer.getArrayOfWritePointers();
	else channelData = buffer.getArrayOfWritePointers();

	// update voice count
	int voicenum = page_gen.state.value[0][0];
	if(midihandle.voicessize != voicenum) {
		int osblocksize = samplesperblock*pow(2,osindex);
		if(midihandle.voicessize < voicenum) {
			for(int v = midihandle.voicessize; v < voicenum; ++v)
				for(int o = 0; o < MC; ++o)
					osc[o][v].reset(channelnum,osblocksize);
		} else {
			for(int v = voicenum; v < midihandle.voicessize; ++v)
				for(int o = 0; o < MC; ++o)
					osc[o][v].reset(0,0);
		}
		midihandle.setvoices(osblocksize);
	}

	opcount = 0; // TODO factor connections
	for(int o = 0; o < MC; ++o)
		if(page_op.state.value[0][o] > .5f || page_op.presets[currentpreset].value[0][o] > .5f)
			oporder[opcount++] = o;

	// update bpm
	AudioPlayHead* playhead = getPlayHead();
	if(playHead != nullptr)
		if(auto positioninfo = playhead->getPosition())
			if(auto bpmfromhost = positioninfo->getBpm())
				if((*bpmfromhost) != bpm) {
					bpm = *bpmfromhost;
					updatearpspeed();
					updatevibspeed();
					for(int o = 0; o < 8; ++o)
						updatelfospeed(o);
	}

	// prepare variables
	midihandle.newbuffer();

	// ---- MIDI ----
	MidiBufferIterator midiiterator = midiMessages.begin();
	for(int s = 0; s < numsamples; ++s) {
		while(midiiterator != midiMessages.end() && ((*midiiterator).samplePosition*oversampleamount) <= s) {
			midihandle.processmessage((*midiiterator).getMessage());
			++midiiterator;
		}
		midihandle.tick();

		for(int v = 0; v < voicenum; ++v) {
			if(midihandle.voices[v].noteid == -1) {
				osc[0][v].f[s] = 0;
				osc[0][v].w[s] = 0;
				osc[1][v].w[s] = 0;
			} else {
				osc[0][v].f[s] = midihandle.getpitch(v);
				osc[0][v].w[s] = midihandle.notes[midihandle.voices[v].noteid].velocity;
				osc[1][v].w[s] = midihandle.voices[v].aftertouchsmooth;
			}
		}
	}
	float maxrls = 1;
	for(int o = 0; o < opcount; ++o)
		maxrls = fmin(maxrls,egd[oporder[o]*5+3]);
	int voicecount = 0;
	for(int v = 0; v < voicenum; ++v) {
		if(midihandle.voices[v].age > 1) continue;
		voiceorder[voicecount++] = v;
		if(!midihandle.voices[v].ison)
			midihandle.voices[v].age += maxrls*numsamples;
	}
	if(voicecount == 0) {
		if(getframe.get()) {
			for(int o = 0; o < (MC+1); ++o)
				indicators[o] = 0;
			if(tabp[params.selectedtab.get()] == 1) {
				framedata[0] = -1;
				framedata[1] = -1;
			}
		}
	} else {

		// ---- VIBRATO ----
		if((page_gen.state.value[10][0] > .00001 || page_gen.presets[currentpreset].value[10][0] > .00001) && (page_gen.state.value[13][0] > .00001 || page_gen.presets[currentpreset].value[13][0] > .00001)) {
			float attackdelta = 1;
			if(page_gen.state.value[14][0] > .0001f)
				attackdelta = 1.f/(page_gen.state.value[14][0]*page_gen.state.value[14][0]*MAXVIBATT*ossamplerate);
			for(int v = 0; v < voicecount; ++v)
				midihandle.voices[voiceorder[v]].eventindex = 0;
			for(int s = 0; s < numsamples; ++s) {
				page_gen.state.value[10][0] = page_gen.smooth[10][0].getNextValue();
				page_gen.state.value[13][0] = page_gen.smooth[13][0].getNextValue();
				float amt = page_gen.state.value[10][0]*page_gen.state.value[13][0];
				vibphase = fmod(vibphase+vibdelta,1.f);
				float vib = sinapprox(vibphase*2-1)*amt*amt/12;

				for(int v = 0; v < voicecount; ++v) {
					int vi = voiceorder[v];
					vibattack[vi] = fmin(1.f,vibattack[vi]+attackdelta);
					int nextevent = midihandle.voices[vi].events[midihandle.voices[vi].eventindex];
					if(nextevent != 0 && nextevent <= (s+1)) {
						if(nextevent > 0) vibattack[vi] = 0;
						++midihandle.voices[vi].eventindex;
					}
					osc[0][vi].f[s] *= pow(2.f,vib*vibattack[vi]);
				}
			}
		}

		// ---- VELOCITY ----
		float inits[3];
		for(int o = 0; o < opcount; ++o) {
			int op = oporder[o];
			if(page_op.state.value[4][op] <= .00001f && page_op.presets[currentpreset].value[4][op] <= .00001f && page_op.state.value[5][op] <= .00001f && page_op.presets[currentpreset].value[5][op] <= .00001f) {
				for(int v = 0; v < voicecount; ++v) {
					int vi = voiceorder[v];
					for(int s = 0; s < numsamples; ++s)
						osc[op][vi].a[s*channelnum] = 1;
				}
				continue;
			}
			inits[0] = page_op.state.value[4][op];
			inits[1] = page_op.state.value[5][op];
			for(int v = 0; v < voicecount; ++v) {
				int vi = voiceorder[v];
				page_op.smooth[4][op].setCurrentAndTargetValue(inits[0]);
				page_op.smooth[5][op].setCurrentAndTargetValue(inits[1]);
				page_op.smooth[4][op].setTargetValue(page_op.presets[currentpreset].value[4][op]);
				page_op.smooth[5][op].setTargetValue(page_op.presets[currentpreset].value[5][op]);
				for(int s = 0; s < numsamples; ++s) {
					page_op.state.value[4][op] = page_op.smooth[4][op].getNextValue();
					page_op.state.value[5][op] = page_op.smooth[5][op].getNextValue();
					osc[op][vi].a[s*channelnum] =
						(osc[0][vi].w[s]*page_op.state.value[4][op]+(1-page_op.state.value[4][op]))*
						(osc[1][vi].w[s]*page_op.state.value[5][op]+(1-page_op.state.value[5][op]));
				}
			}
		}

		// ---- ADSR ----
		for(int o = 0; o < opcount; ++o) {
			int op = oporder[o];
			for(int v = 0; v < voicecount; ++v)
			inits[0] = page_op.state.value[11][op];
			for(int v = 0; v < voicecount; ++v) {
				int vi = voiceorder[v];
				midihandle.voices[vi].eventindex = 0;
				page_op.smooth[11][op].setCurrentAndTargetValue(inits[0]);
				page_op.smooth[11][op].setTargetValue(page_op.presets[currentpreset].value[11][op]);
				for(int s = 0; s < numsamples; ++s) {
					page_op.state.value[11][op] = page_op.smooth[11][op].getNextValue();
					ega[op*5+1] = page_op.state.value[11][op];
					ega[op*5+2] = page_op.state.value[11][op];
					int nextevent = midihandle.voices[vi].events[midihandle.voices[vi].eventindex];
					if(nextevent != 0 && nextevent <= (s+1)) {
						float exp = 1-egprog[op*VC+vi];
						exp = 1-exp*exp;
						eglast[op*VC+vi] =
							eglast          [op*VC+vi] *(1-exp)+
							ega[op*5+egstage[op*VC+vi]]*   exp;
						egstage[op*VC+vi] = (sgn(nextevent)-1)*-1.5f;
						egprog[op*VC+vi] = 0;
						++midihandle.voices[vi].eventindex;
					}
					egprog[op*VC+vi] += egd[op*5+egstage[op*VC+vi]];
					if(egprog[op*VC+vi] >= 1) {
						eglast[op*VC+vi] = ega[op*5+egstage[op*VC+vi]];
						egprog[op*VC+vi] = 0;
						++egstage[op*VC+vi];
					}
					float exp = 1-egprog[op*VC+vi];
					exp = 1-exp*exp;
					osc[op][vi].a[s*channelnum] *=
						eglast          [op*VC+vi] *(1-exp)+
						ega[op*5+egstage[op*VC+vi]]*   exp;
				}
			}
		}

		// ---- LFO ----
		if(page_gen.state.value[4][0] == 2) {
			for(int o = 0; o < opcount; ++o) {
				int op = oporder[o];
				if(page_op.state.value[13][op] < .5f && page_op.presets[currentpreset].value[13][op] < .5f) continue;
				for(int s = 0; s < numsamples; ++s) {
					osc[op][0].w[s] = .5f;
				}
				page_op.curves[0].c[op].bufferstart(lfophase[op*VC],0);
				for(int s = 0; s < numsamples; ++s) {
					lfophase[op*VC] += lfophasedelta[op];
					if(lfophase[op*VC] >= 1) {
						--lfophase[op*VC];
						page_op.curves[0].c[op].wrap(0);
					}
					osc[op][0].w[s] = page_op.curves[0].c[op].process(lfophase[op*VC],0);
				}
				for(int s = 0; s < numsamples; ++s) {
					osc[op][0].w[s] = lfodamp.nextvalue(osc[op][0].w[s],op*VC);
				}
				for(int s = 0; s < numsamples; ++s) {
					page_op.state.value[13][op] = page_op.smooth[13][op].getNextValue();
					page_op.state.value[17][op] = page_op.smooth[17][op].getNextValue();
					osc[op][0].w[s] *= page_op.state.value[13][op]*(page_op.state.value[17][op]*2-1);
				}
			}
		} else {
			for(int o = 0; o < opcount; ++o) {
				int op = oporder[o];
				if(page_op.state.value[13][op] < .5f && page_op.presets[currentpreset].value[13][op] < .5f) continue;
				inits[0] = page_op.state.value[13][op];
				inits[1] = page_op.state.value[17][op];
				for(int v = 0; v < voicecount; ++v) {
					int vi = voiceorder[v];
					midihandle.voices[vi].eventindex = 0;
					page_op.curves[0].c[op].bufferstart(lfophase[op*VC+vi],vi);
					for(int s = 0; s < numsamples; ++s) {
						int nextevent = midihandle.voices[vi].events[midihandle.voices[vi].eventindex];
						if(nextevent != 0 && nextevent <= (s+1)) {
							if(nextevent > 0) {
								if(page_gen.state.value[4][0] == 0 || page_gen.state.value[4][0] == 3) {
									lfophase[op*VC+vi] = 0;
									page_op.curves[0].c[op].wrap(vi);
									lfodamp.setto(page_op.curves[0].c[op].process(lfophase[op*VC+vi],vi),op*VC+vi);
								} else if(page_gen.state.value[4][0] == 1) {
									lfophase[op*VC+vi] = random.nextFloat();
									page_op.curves[0].c[op].wrap(vi);
									lfodamp.setto(page_op.curves[0].c[op].process(lfophase[op*VC+vi],vi),op*VC+vi);
								}
							}
							++midihandle.voices[vi].eventindex;
						}
						lfophase[op*VC+vi] += lfophasedelta[op];
						if(page_gen.state.value[4][0] == 3) {
							lfophase[op*VC+vi] = fmin(lfophase[op*VC+vi],1);
						} else if(lfophase[op*VC+vi] >= 1) {
							--lfophase[op*VC+vi];
							page_op.curves[0].c[op].wrap(vi);
						}
						osc[op][vi].w[s] = lfodamp.nextvalue(page_op.curves[0].c[op].process(lfophase[op*VC+vi],vi),op*VC+vi);
					}
					page_op.smooth[13][op].setCurrentAndTargetValue(inits[0]);
					page_op.smooth[17][op].setCurrentAndTargetValue(inits[1]);
					page_op.smooth[13][op].setTargetValue(page_op.presets[currentpreset].value[13][op]);
					page_op.smooth[17][op].setTargetValue(page_op.presets[currentpreset].value[17][op]);
					for(int s = 0; s < numsamples; ++s) {
						page_op.state.value[13][op] = page_op.smooth[13][op].getNextValue();
						page_op.state.value[17][op] = page_op.smooth[17][op].getNextValue();
						osc[op][vi].w[s] *= page_op.state.value[13][op]*(page_op.state.value[17][op]*2-1);
					}
				}
			}
		}

		// ---- COPY DATA ----
		for(int o = 0; o < opcount; ++o) {
			int op = oporder[o];
			if((page_op.state.value[13][op] > .5f || page_op.presets[currentpreset].value[13][op] > .5f) && page_gen.state.value[4][0] == 2) {
				for(int v = 0; v < voicecount; ++v) {
					if(voiceorder[v] == 0) continue;
					memcpy(
						&osc[op][voiceorder[v]].w[0],
						&osc[op][0            ].w[0],sizeof(float)*numsamples);
				}
			}
			if(op == 0) continue;
			for(int v = 0; v < voicecount; ++v)
				memcpy(
					&osc[op][voiceorder[v]].f[0],
					&osc[0 ][voiceorder[v]].f[0],sizeof(float)*numsamples);
		}

		// ---- LFO ATTACK ----
		for(int o = 0; o < opcount; ++o) {
			int op = oporder[o];
			if(page_op.state.value[13][op] < .5f && page_op.presets[currentpreset].value[13][op] < .5f) continue;
			if(page_op.state.value[18][op] <= .00001f) continue;
			for(int v = 0; v < voicecount; ++v) {
				int vi = voiceorder[v];
				midihandle.voices[vi].eventindex = 0;
				for(int s = 0; s < numsamples; ++s) {
					lfoattack[op*VC+vi] = fmin(1.f,lfoattack[op*VC+vi]+lfoattackdelta[op]);
					int nextevent = midihandle.voices[vi].events[midihandle.voices[vi].eventindex];
					if(nextevent != 0 && nextevent <= (s+1)) {
						if(nextevent > 0) lfoattack[op*VC+vi] = 0;
						++midihandle.voices[vi].eventindex;
					}
					osc[op][vi].w[s] *= lfoattack[op*VC+vi];
				}
			}
		}

		// ---- PARAMS ----
		float lfovals[4]; // amp, pitch, pan, tone
		for(int o = 0; o < opcount; ++o) {
			int op = oporder[o];

			float freqmult = page_op.state.value[7][op];
			if(page_op.state.value[6][op] < .5f && freqmult > .00005f) freqmult = 1.f/freqmult;
			float freqadd = freqaddinflate(page_op.state.value[8][op]);
			inits[0] = page_op.state.value[0][op];
			inits[1] = page_op.state.value[1][op];
			inits[2] = page_op.state.value[2][op];
			inits[3] = page_op.state.value[3][op];
			float mute = 0;
			for(int v = 0; v < voicecount; ++v) {
				int vi = voiceorder[v];
				page_op.smooth[0][op].setCurrentAndTargetValue(inits[0]);
				page_op.smooth[1][op].setCurrentAndTargetValue(inits[1]);
				page_op.smooth[2][op].setCurrentAndTargetValue(inits[2]);
				page_op.smooth[3][op].setCurrentAndTargetValue(inits[3]);
				page_op.smooth[0][op].setTargetValue(page_op.presets[currentpreset].value[0][op]);
				page_op.smooth[1][op].setTargetValue(page_op.presets[currentpreset].value[1][op]);
				page_op.smooth[2][op].setTargetValue(page_op.presets[currentpreset].value[2][op]);
				page_op.smooth[3][op].setTargetValue(page_op.presets[currentpreset].value[3][op]);
				for(int s = 0; s < numsamples; ++s) {
					page_op.state.value[0][op] = page_op.smooth[0][op].getNextValue();
					page_op.state.value[1][op] = page_op.smooth[1][op].getNextValue();
					page_op.state.value[2][op] = page_op.smooth[2][op].getNextValue();
					page_op.state.value[3][op] = page_op.smooth[3][op].getNextValue();
					lfovals[0] = page_op.state.value[2][op];
					lfovals[1] = 0;
					lfovals[2] = page_op.state.value[1][op];
					lfovals[3] = page_op.state.value[3][op];
					lfovals[(int)page_op.state.value[14][op]] += osc[op][vi].w[s];
					lfovals[0] = fmax(0,       lfovals[0] );
					lfovals[2] = fmax(0,fmin(1,lfovals[2]));
					lfovals[3] = fmax(0,fmin(1,lfovals[3]));

					osc[op][vi].a[s*channelnum] *= page_op.state.value[0][op]*lfovals[0]*lfovals[0]*4;
					osc[op][vi].f[s] = (osc[op][vi].f[s]*freqmult+freqadd)*pow(2.f,lfovals[1]*4.f);
					osc[op][vi].w[s] = lfovals[3];

					for(int c = channelnum-1; c >= 0; --c) {
						float linearpan = (((float)c)/(channelnum-1))*(lfovals[2]*2-1)+(1-lfovals[2]);
						osc[op][vi].a[s*channelnum+c] = osc[op][vi].a[s*channelnum]*(sinapprox(linearpan*.5f)*.7071067812+linearpan);
					}
				}

				for(int s = 0; s < numsamples; ++s) {
					if(osc[op][vi].f[s] >= 20 && osc[op][vi].f[s] <= 20000) {
						mute = mutedamp.nextvalue(1.f,op*VC+vi);
					} else {
						mute = mutedamp.nextvalue(0.f,op*VC+vi);
						osc[op][vi].f[s] = fmin(osc[op][vi].f[s],20000);
					}
					osc[op][vi].a[s*channelnum] *= mute;
				}
			}
		}

		// ---- INDICATOR VIS ----
		for(int o = 0; o < opcount; ++o) {
			int op = oporder[o];
			for(int v = 0; v < voicecount; ++v) {
				int vi = voiceorder[v];
				if(egstage[op*VC+vi] == 2)
					egsustain[op*VC+vi] = fmin(1,egsustain[op*VC+vi]+numsamples/((float)ossamplerate));
				else
					egsustain[op*VC+vi] = 0;
			}
		}
		if(getframe.get()) {
			indicators[0] = 0;
			for(int o = 0; o < opcount; ++o) {
				int op = oporder[o];
				float rms = 0;
				for(int v = 0; v < voicecount; ++v) {
					for(int c = 0; c < channelnum; ++c) {
						float a = osc[op][voiceorder[v]].a[(numsamples-1)*channelnum+c];
						rms += a*a;
					}
				}
				indicators[op+1] = sqrt(rms/channelnum);
				indicators[0] += rms;
			}
			indicators[0] = sqrt(indicators[0]/channelnum);
			if(tabp[params.selectedtab.get()] == 1) {
				int latestvoice = tabo[params.selectedtab.get()]*VC;
				if(midihandle.activevoicessize > 0)
					latestvoice += midihandle.  activevoices[midihandle.  activevoicessize-1];
				else
					latestvoice += midihandle.inactivevoices[midihandle.inactivevoicessize-1];
				framedata[0] = egstage[latestvoice]+egprog[latestvoice];
				if(egstage[latestvoice] == 2)
					framedata[0] = framedata[0].get()+egsustain[latestvoice];
				if(page_op.state.value[13][tabo[params.selectedtab.get()]] < .5f)
					framedata[1] = -1;
				else
					framedata[1] = lfophase[page_gen.state.value[4][0]==2?(tabo[params.selectedtab.get()]*VC):latestvoice];
			}
		}

		// ---- OSC ----
		for(int v = 0; v < voicecount; ++v) {
			int vi = voiceorder[v];
			midihandle.voices[vi].eventindex = 0;
			for(int s = 0; s < numsamples; ++s) {
				int nextevent = midihandle.voices[vi].events[midihandle.voices[vi].eventindex];
				if(nextevent != 0 && nextevent <= (s+2)) {
					if(nextevent > 0) {
						for(int o = 0; o < opcount; ++o) {
							int op = oporder[o];
							float amp = 0;
							for(int c = 0; c < channelnum; ++c)
								amp += osc[op][vi].a[s*channelnum+c];
							// .035 ~= sqrt(.01/MINR)
							if(amp == 0 || page_op.state.value[9][op] <= .035f)
								for(int c = 0; c < channelnum; ++c)
									osc[op][vi].p[c] = 0;
						}
					}
					++midihandle.voices[vi].eventindex;
				}
				for(int o = 0; o < opcount; ++o) {
					int op = oporder[o];
					generator[op].update(osc[op][vi].w[s]);
					for(int c = 0; c < channelnum; ++c) {
						osc[op][vi].p[c] = fmod(osc[op][vi].p[c]+(osc[op][vi].f[s]/ossamplerate),1);
						osc[op][vi].a[s*channelnum+c] *= generator[op].calc(osc[op][vi].p[c]*2-1);
						osc[op][vi].fb[c] = osc[op][vi].a[s*channelnum+c];
						channelData[c][s] += osc[op][vi].a[s*channelnum+c]*.125f;
					}
				}
			}
		}
	}

	// ---- FX ----
	//debug((String)"");
	for(int o = 0; o < FXC; ++o) {
		if(pages[fxorder[o]]->state.value[0][0] > .5f || pages[fxorder[o]]->presets[currentpreset].value[0][0] > .5f) {
			//debug(pages[fxorder[o]]->params[0].nameui);
			// TODO downsampling
			// TODO process
		}
	}

	// ---- TODO VIS ----
	if(getframe.get()) {
		for(int o = 0; o < opcount; ++o) {
			int op = oporder[o];
		}
		getframe = false;
	}

	if(osindex > 0) os[osindex-1]->processSamplesDown(block);
}

void FMPlusAudioProcessor::setoversampling() {
	if(!preparedtoplay) return;
	osindex = fmin(3,fmax(0,floor(page_gen.state.value[15][0]*8-4)));
	if(osindex > 0) {
		if(channelnum <= 0) return;
		os[osindex-1]->reset();
		setLatencySamples(os[osindex-1]->getLatencyInSamples());
	} else {
		setLatencySamples(0);
	}
	int oversampleamount = pow(2,osindex);

	for(int p = 0; p < PC; ++p)
		for(int i = 0; i < pages[p]->params.size(); ++i)
			for(int o = 0; o < pages[p]->instances; ++o)
				if(pages[p]->params[i].smoothtime > 0)
					pages[p]->smooth[i][o].reset(samplerate*oversampleamount,pages[p]->params[i].smoothtime);

	midihandle.reset(samplesperblock*oversampleamount,samplerate*oversampleamount);
	mutedamp.reset(.01f,samplerate*oversampleamount,MC*VC,1);
	lfodamp .reset(.01f,samplerate*oversampleamount,MC*VC,0);
	updatearpspeed();
	updatevibspeed();
	for(int o = 0; o < MC; ++o) {
		updatea(o);
		updated(o);
		updater(o);
		updatelfospeed(o);
		updatelfoattackspeed(o);
		for(int v = 0; v < page_gen.state.value[0][0]; ++v)
			osc[o][v].reset(channelnum,samplesperblock*oversampleamount);
	}
}

bool FMPlusAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* FMPlusAudioProcessor::createEditor() {
	return new FMPlusAudioProcessorEditor(*this);
}

void FMPlusAudioProcessor::getStateInformation(MemoryBlock& destData) {
	const char delimiter = '\n';
	std::ostringstream data;

	data << version << delimiter
		<< currentpreset << delimiter
		<< (page_gen.presetunsaved?1:0) << delimiter
		<< page_gen.state.value[15][0] << delimiter
		<< params.selectedtab.get() << delimiter
		<< params.fxtab.get() << delimiter
		<< page_gen.tuningfile.replace(",","/") << delimiter;

	for(int n = 0; n < 128; ++n)
		data << midihandle.notes[n].pitch << delimiter;

	for(int i = 0; i < getNumPrograms(); ++i)
		data << get_preset(i) << delimiter;

	MemoryOutputStream stream(destData, false);
	stream.writeString(data.str());
}
void FMPlusAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
	/*/
	const char delimiter = '\n';
	saved = true;
	try {
		std::istringstream ss(String::createStringFromData(data, sizeInBytes).toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int saveversion = std::stoi(token);

		std::getline(ss, token, delimiter);
		currentpreset = std::stoi(token);

		std::getline(ss, token, delimiter);
		page_gen.presetunsaved = std::stoi(token) > .5;

		std::getline(ss, token, delimiter);
		page_gen.state.value[15][0] = std::stof(token);

		std::getline(ss, token, delimiter);
		params.selectedtab = std::stoi(token);

		std::getline(ss, token, delimiter);
		params.fxtab = std::stoi(token);

		std::getline(ss, token, delimiter);
		page_gen.tuningfile = ((String)token).replace("/",",");

		for(int n = 0; n < 128; ++n) {
			std::getline(ss, token, delimiter);
			midihandle.notes[n].pitch = std::stof(token);
		}

		for(int i = 0; i < getNumPrograms(); ++i) {
			std::getline(ss, token, delimiter);
			set_preset(token, i, ',', true);
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

	apvts.getParameter("antialias")->setValueNotifyingHost(page_gen.state.value[15][0]); // TODO
	//*/
}
const String FMPlusAudioProcessor::get_preset(int preset_id, const char delimiter) {

	std::ostringstream data;

	data << version << delimiter
		<< presets[preset_id].name.replace(",","*") << delimiter;

	for(int o = 0; o < (MC+FXC+1)*2; ++o)
		data << presets[preset_id].oppos[o] << delimiter;
	for(int o = 0; o < (MC+1); ++o) {
		data << presets[preset_id].opconnections[o].size() << delimiter;
		for(int i = 0; i < presets[preset_id].opconnections[o].size(); ++i) {
			data << presets[preset_id].opconnections[o][i].input << delimiter
				<< presets[preset_id].opconnections[o][i].influence << delimiter;
		}
	}
	for(int p = 0; p < PC; ++p) { // TODO offload to page
		for(int i = 0; i < pages[p]->params.size(); ++i) for(int o = 0; o < pages[p]->instances; ++o)
			data << pages[p]->presets[preset_id].value[i][o] << delimiter;

		for(int i = 0; i < pages[p]->curvesnum; ++i) for(int o = 0; o < pages[p]->instances; ++o) {
			data << pages[p]->presets[preset_id].points[i][o].size() << delimiter;
			for(int n = 0; n < pages[p]->presets[preset_id].points[i][o].size(); ++n)
				data << pages[p]->presets[preset_id].points[i][o][n].x << delimiter << pages[p]->presets[preset_id].points[i][o][n].y << delimiter << pages[p]->presets[preset_id].points[i][o][n].tension << delimiter;
		}
	}

	return data.str();
}
void FMPlusAudioProcessor::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
	String error = "";
	String revert = get_preset(preset_id);
	try {
		std::istringstream ss(preset.trim().toRawUTF8());
		std::string token;

		std::getline(ss, token, delimiter);
		int save_version = std::stoi(token);

		std::getline(ss, token, delimiter);
		presets[preset_id].name = ((String)token).replace("*",",");

		for(int o = 0; o < (MC+FXC+1)*2; ++o) {
			std::getline(ss, token, delimiter);
			presets[preset_id].oppos[o] = std::stoi(token);
		}
		for(int o = 0; o < (MC+1); ++o) {
			std::getline(ss, token, delimiter);
			int size = std::stoi(token);
			for(int i = 0; i < size; ++i) {
				std::getline(ss, token, delimiter);
				int input = std::stoi(token);
				std::getline(ss, token, delimiter);
				float influence = std::stof(token);
				presets[preset_id].opconnections[o].push_back(connection(input,o,influence));
			}
		}

		for(int p = 0; p < PC; ++p) { // TODO offload to page
			for(int i = 0; i < pages[p]->params.size(); ++i) for(int o = 0; o < pages[p]->instances; ++o) {
				std::getline(ss, token, delimiter);
				pages[p]->presets[preset_id].value[i][o] = std::stof(token);
			}

			for(int i = 0; i < pages[p]->curvesnum; ++i) for(int o = 0; o < pages[p]->instances; ++o) {
				std::getline(ss, token, delimiter);
				int size = std::stof(token);
				if(size < 2) throw std::invalid_argument("Invalid point data");
				pages[p]->presets[preset_id].points[i][o].clear();
				float prevx = 0;
				for(int n = 0; n < size; ++n) {
					std::getline(ss, token, delimiter);
					float x = std::stof(token);
					std::getline(ss, token, delimiter);
					float y = std::stof(token);
					std::getline(ss, token, delimiter);
					float tension = std::stof(token);
					if(x > 1 || x < prevx || y > 1 || y < 0 || tension > 1 || tension < 0 || (n == 0 && x != 0) || (n == (size-1) && x != 1))
						throw std::invalid_argument("Invalid point data");
					prevx = x;
					pages[p]->presets[preset_id].points[i][o].push_back(point(x,y,tension));
				}
			}
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

	page_gen.presetname = presets[currentpreset].name;
	updatepreset = true;

	for(int p = 0; p < PC; ++p) { // TODO offload to page
		for(int i = 0; i < pages[p]->params.size(); ++i) for(int o = 0; o < pages[p]->instances; ++o)
			apvts.getParameter(p==1?("o"+(String)o+pages[p]->params[i].id):pages[p]->params[i].id)->setValueNotifyingHost(pages[p]->params[i].normalize(pages[p]->presets[currentpreset].value[i][o]));
		for(int i = 0; i < pages[p]->curvesnum; ++i) for(int o = 0; o < pages[p]->instances; ++o)
			pages[p]->curves[i].updatedcurve[o] = true;
	}
	updatevis = true;
}

void FMPlusAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
	for(int p = 0; p < PC; ++p) {
		for(int o = 0; o < pages[p]->instances; ++o) {
			if(p == 1 && !parameterID.startsWith("o"+(String)o)) continue;
			for(int i = 0; i < pages[p]->params.size(); ++i) {
				if(parameterID != (p==1?("o"+(String)o+pages[p]->params[i].id):pages[p]->params[i].id)) continue;

				if(p == 0) {
					if(parameterID == "glide" && midihandle.glide.current.size() > 0) {
						if(page_gen.state.value[i][0] == 0)
							midihandle.glide.current[0] = -666;
						midihandle.glide.setsmoothtime(newValue*newValue*MAXGLIDE);
					} else if(parameterID == "lfosync") {
						if(page_gen.state.value[4][0] == 2) {
							for(int o = 0; o < opcount; ++o) {
								int op = oporder[o];
								for(int v = 1; v < VC; ++v)
									lfophase[op*VC+v] = lfophase[op*VC];
							}
						} else if(newValue == 2) {
							int latestvoice = 0;
							if(midihandle.activevoicessize > 0)
								latestvoice = midihandle.  activevoices[midihandle.  activevoicessize-1];
							else
								latestvoice = midihandle.inactivevoices[midihandle.inactivevoicessize-1];
							for(int o = 0; o < opcount; ++o) {
								int op = oporder[o];
								lfophase[op*VC] = lfophase[op*VC+latestvoice];
							}
						}
					}
				}

				if(pages[p]->params[i].smoothtime > 0) pages[p]->smooth[i][o].setTargetValue(newValue);
				else pages[p]->state.value[i][o] = newValue;
				pages[p]->presets[currentpreset].value[i][o] = newValue;
				if(pages[p]->params[i].saved) {
					page_gen.presetunsaved = true;
					updatepreset = true;
				}

				if(p == 0) {
						 if(parameterID == "pitchbend")
						midihandle.pitchesupdate();
					else if(parameterID == "arpon")
						midihandle.arpset();
					else if(parameterID == "arpdirection")
						midihandle.arpupdate();
					else if(parameterID == "arpspeed" || parameterID == "arpbpm")
						updatearpspeed();
					else if(parameterID == "vibratorate" || parameterID == "vibratobpm")
						updatevibspeed();

				} else if(p == 1) {
					if(i == 0) {
						if(newValue < .5) {
							indicators[o+1] = 0;
						} else for(int v = 0; v < VC; ++v) {
							egstage  [o*VC+v] = 4;
							egprog   [o*VC+v] = 0;
							eglast   [o*VC+v] = 0;
							egsustain[o*VC+v] = 0;
							lfophase [o*VC+v] = 0;
						}
					} else if(i == 9)
						updatea(o);
					else if(i == 10)
						updated(o);
					else if(i == 12)
						updater(o);
					else if(i == 15 || i == 16)
						updatelfospeed(o);
					else if(i == 18)
						updatelfoattackspeed(o);
				}

				return;
			}
		}
	}
}
void FMPlusAudioProcessor::updateconnectors(signed char o, bool outputs, signed char* ids) {
	updateconnections = false;

	signed char* apids;
	if(outputs) apids = presets[currentpreset].outputs[o];
	else        apids = presets[currentpreset].inputs [o];

	// remove connections removed for turned on operators
	bool replacable[3];
	bool hasemptyslot = false;
	for(signed char n = 0; n < 3; ++n) {
		signed char id = apids[n];
		replacable[n] = true;
		if(id < 0) {
			hasemptyslot = true;
			continue;
		}
		if(id != 0 && page_op.state.value[0][fmax(0,id-1)] < .5f) continue;
		for(signed char nn = 0; nn < 3; ++nn) if(id == ids[nn]) {
			replacable[n] = false;
			break;
		}
		if(replacable[n]) {
			hasemptyslot = true;
			for(signed char nn = 0; nn < 3; ++nn) {
				if( outputs && presets[currentpreset].inputs [id][nn] == o) {
					presets[currentpreset].inputs [id][nn] = -1;
					break;
				}
				if(!outputs && presets[currentpreset].outputs[id][nn] == o) {
					presets[currentpreset].outputs[id][nn] = -1;
					break;
				}
			}
			apids[n] = -1;
		}
	}

	// mark the new item
	signed char newitem = -1;
	for(signed char n = 0; n < 3; ++n) {
		signed char id = ids[n];
		bool isnew = true;
		for(signed char nn = 0; nn < 3; ++nn) if(id == apids[nn]) {
			isnew = false;
			break;
		}
		if(isnew) {
			newitem = id;
			break;
		}
	}
	if(newitem == -1) return;

	if(!hasemptyslot) for(signed char n = 2; n >= 0; --n) if(replacable[n]) {
		for(signed char nn = 0; nn < 3; ++nn) {
			if( outputs && presets[currentpreset].inputs [apids[n]][nn] == o) {
				presets[currentpreset].inputs [apids[n]][nn] = -1;
				break;
			}
			if(!outputs && presets[currentpreset].outputs[apids[n]][nn] == o) {
				presets[currentpreset].outputs[apids[n]][nn] = -1;
				break;
			}
		}
		apids[n] = -1;
		break;
	}

	// replace removed item w new item, pushing the rest,
	signed char temp;
	float newinfl = .5f;
	float tempinfl;
	for(signed char n = 0; n < 3; ++n) {
		temp = apids[n];
		apids[n] = newitem;
		newitem = temp;

		if(!outputs) {
			tempinfl = presets[currentpreset].influence[o][n];
			presets[currentpreset].influence[o][n] = newinfl;
			newinfl = tempinfl;
		}

		if(temp == -1) break;
	}

	updateconnections = true;
}
void FMPlusAudioProcessor::updatearpspeed() {
	if(page_gen.state.value[ 9][0] > 0)
		midihandle.arpspeed = bpm/(bpmsyncs_f  [(int)page_gen.state.value  [ 9][0]]*samplerate*pow(2,osindex)*60);
	else
		midihandle.arpspeed = 1.f/((page_gen.state.value  [ 8][0]*page_gen.state.value  [ 8][0]*(MAXARP-MINARP)+MINARP)*samplerate*pow(2,osindex));
}
void FMPlusAudioProcessor::updatevibspeed() {
	if(page_gen.state.value[12][0] > 0)
		vibdelta            = bpm/(bpmsyncs_f  [(int)page_gen.state.value  [12][0]]*samplerate*pow(2,osindex)*60);
	else
		vibdelta            = 1.f/((page_gen.state.value  [11][0]*page_gen.state.value  [11][0]*(MAXVIB-MINVIB)+MINVIB)*samplerate*pow(2,osindex));
}
void FMPlusAudioProcessor::updatea(int o) {
	egd[o*5  ]              = 1.f/((page_op.state.value[ 9][o]*page_op.state.value[ 9][o]*(MAXA  -MINA  )+MINA  )*samplerate*pow(2,osindex));
}
void FMPlusAudioProcessor::updated(int o) {
	egd[o*5+1]              = 1.f/((page_op.state.value[10][o]*page_op.state.value[10][o]*(MAXD  -MIND  )+MIND  )*samplerate*pow(2,osindex));
}
void FMPlusAudioProcessor::updater(int o) {
	egd[o*5+3]              = 1.f/((page_op.state.value[12][o]*page_op.state.value[12][o]*(MAXR  -MINR  )+MINR  )*samplerate*pow(2,osindex));
}
void FMPlusAudioProcessor::updatelfospeed(int o) {
	if(page_op.state.value[16][o] > 0)
		lfophasedelta   [o] = bpm/(bpmsynclfo_f[(int)page_op.state.value[16][o]]*samplerate*pow(2,osindex)*60);
	else if(page_op.state.value[15][o] >= 1)
		lfophasedelta   [o] = 0.f;
	else
		lfophasedelta   [o] = 1.f/((page_op.state.value[15][o]*page_op.state.value[15][o]*(MAXLFO-MINLFO)+MINLFO)*samplerate*pow(2,osindex));
	lfophasedelta[o] = fmod(lfophasedelta[o],1.f);
}
void FMPlusAudioProcessor::updatelfoattackspeed(int o) {
	if(page_op.state.value[18][o] > .0001f)
		lfoattackdelta  [o] = 1.f/( page_op.state.value[18][o]*page_op.state.value[18][o]* MAXLFOATT             *samplerate*pow(2,osindex));
	else
		lfoattackdelta  [o] = 1.f;
}

void FMPlusAudioProcessor::movepoint(int p, int c, int o, int index, float x, float y) {
	pages[p]->presets[currentpreset].points[c][o][index].x = x;
	pages[p]->presets[currentpreset].points[c][o][index].y = y;
	pages[p]->curves[c].updatedcurve[o] = true;
}
void FMPlusAudioProcessor::movetension(int p, int c, int o, int index, float tension) {
	pages[p]->presets[currentpreset].points[c][o][index].tension = tension;
	pages[p]->curves[c].updatedcurve[o] = true;
}
void FMPlusAudioProcessor::addpoint(int p, int c, int o, int index, float x, float y) {
	pages[p]->presets[currentpreset].points[c][o].insert(pages[p]->presets[currentpreset].points[c][o].begin()+index,point(x,y,pages[p]->presets[currentpreset].points[c][o][index-1].tension));
	pages[p]->curves[c].updatedcurve[o] = true;
}
void FMPlusAudioProcessor::deletepoint(int p, int c, int o, int index) {
	pages[p]->presets[currentpreset].points[c][o].erase(pages[p]->presets[currentpreset].points[c][o].begin()+index);
	pages[p]->curves[c].updatedcurve[o] = true;
}
const String FMPlusAudioProcessor::curvetostring(int p, int c, int o, const char delimiter) {
	return curve::pointstostring(pages[p]->presets[currentpreset].points[c][o],delimiter);
}
void FMPlusAudioProcessor::curvefromstring(int p, int c, int o, String str, const char delimiter) {
	String revert = curve::pointstostring(pages[p]->presets[currentpreset].points[c][o]);
	try {
		curve::pointsfromstring(pages[p]->presets[currentpreset].points[c][o],str,delimiter);
	} catch(...) {
		curve::pointsfromstring(pages[p]->presets[currentpreset].points[c][o],revert);
	}
	updatevis = true;
	pages[p]->curves[c].updatedcurve[o] = true;
}
void FMPlusAudioProcessor::resetcurve(int p, int c, int o) {
	curve::pointsfromstring(pages[p]->presets[currentpreset].points[c][o],"3,0,0,0.5,0.5,1,0.5,1,0,0.5");
	updatevis = true;
	pages[p]->curves[c].updatedcurve[o] = true;
}

void FMPlusAudioProcessor::resettuning() {
	for(int n = 0; n < 128; ++n)
		midihandle.notes[n].pitch = (440.f/32.f)*pow(2.f,((n-9.f)/12.f));
	page_gen.tuningfile = "Standard";
	midihandle.pitchesupdate();
}
void FMPlusAudioProcessor::updatetuning(File file) {
	if(file.getFullPathName() == "") return;
	params.tuningdir = file.getParentDirectory();
	props.getUserSettings()->setValue("tuningdir",params.tuningdir.getFullPathName());

	File scl;
	File kbm;
	       if(file.hasFileExtension("scl") || file.hasFileExtension("ascl")) {
		scl = file;
		kbm = file.withFileExtension("kbm");
	} else if(file.hasFileExtension("kbm")) {
		kbm = file;
		scl = file.withFileExtension("scl");
	} else return;

	try {
		page_gen.tuningfile = scl.getFileNameWithoutExtension();
		if(scl.existsAsFile()) {
			Tunings::Scale s = Tunings::readSCLFile(scl.getFullPathName().toStdString());
			if(kbm.existsAsFile()) {
				Tunings::KeyboardMapping k = Tunings::readKBMFile(kbm.getFullPathName().toStdString());
				Tunings::Tuning t(s,k);
				for(int n = 0; n < 128; ++n)
					midihandle.notes[n].pitch = t.frequencyForMidiNote(n);
			} else {
				Tunings::Tuning t(s);
				for(int n = 0; n < 128; ++n)
					midihandle.notes[n].pitch = t.frequencyForMidiNote(n);
			}
		} else if(kbm.existsAsFile()) {
			Tunings::KeyboardMapping k = Tunings::readKBMFile(kbm.getFullPathName().toStdString());
			Tunings::Tuning t(k);
			for(int n = 0; n < 128; ++n)
				midihandle.notes[n].pitch = t.frequencyForMidiNote(n);
		}
	} catch(...) {
		resettuning();
		return;
	}

	midihandle.pitchesupdate();
	return;
}
void FMPlusAudioProcessor::resettheme() {
	page_gen.themefile = "Default";
	params.theme[ 0] = 212;
	params.theme[ 1] = 255;
	params.theme[ 2] = 255;
	params.theme[ 3] = 255;
	params.theme[ 4] =   0;
	params.theme[ 5] =   0;
	params.theme[ 6] =   0;
	params.theme[ 7] =  79;
	params.theme[ 8] =  79;
	params.theme[ 9] =  79;
	params.theme[10] = 255;
	params.theme[11] = 194;
	params.theme[12] = 254;

	PropertiesFile* user_settings = props.getUserSettings();
	user_settings->setValue("themefile",page_gen.themefile);
	for(int i = 0; i < 13; ++i)
		user_settings->setValue("theme"+((String)i),(int)params.theme[i]);
}
void FMPlusAudioProcessor::updatetheme(File file) {
	if(file.getFullPathName() == "") return;
	PropertiesFile* user_settings = props.getUserSettings();

	params.themedir = file.getParentDirectory();
	user_settings->setValue("themedir",params.themedir.getFullPathName());

	page_gen.themefile = file.getFileNameWithoutExtension();
	const char delimiter = '\n';
	try {
		std::ifstream ss(file.getFullPathName().toStdString());
		std::string token;

		while(std::getline(ss, token, delimiter)) {
			String param = (String)token;
			if(!param.containsChar('=')) continue;
			       param =          param .upToFirstOccurrenceOf("=",false,false).trim().toLowerCase();
			String value = ((String)token).fromFirstOccurrenceOf("=",false,false).trim().unquoted();

			       if(param == "name"      ) {
				page_gen.themefile = value;
			} else if(param == "opacity_ht") {
				params.theme[0] = (unsigned char)round(value.getFloatValue()*255);
			} else if(value.startsWithChar('#')) {

				int val = 0;
				     if(param == "col_bg" ) val =  1;
				else if(param == "col_fg" ) val =  4;
				else if(param == "col_off") val =  7;
				else if(param == "col_on" ) val = 10;
				else continue;

				     if(value.length() == 4)
					value = ((String)"#")+value[1]+value[1]+value[2]+value[2]+value[3]+value[3];
				else if(value.length() != 7)
					continue;

				for(int i = 0; i < 3; ++i)
					params.theme[val+i] = (unsigned char)value.substring(1+i*2,3+i*2).getHexValue32();
			}
		}
	} catch(...) {
		resettheme();
	}

	for(int i = 0; i < 13; ++i)
		user_settings->setValue("theme"+((String)i),(int)params.theme[i]);
	user_settings->setValue("themefile",page_gen.themefile);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FMPlusAudioProcessor(); }

AudioProcessorValueTreeState::ParameterLayout FMPlusAudioProcessor::create_parameters() { // TODO group
	std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"voices"						,1},"Voices"																		 ,1		,VC		 ,12	));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"pitchbend"					,1},"Pitch Bend"																	 ,0		,24		 ,2		,AudioParameterIntAttributes()	.withStringFromValueFunction(topitchbend		).withValueFromStringFunction(frompitchbend		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"glide"						,1},"Glide"											,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(toglide			).withValueFromStringFunction(fromglide			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"legato"						,1},"Legato"																		 				 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"lfosync"						,1},"LFO Sync"																		 ,0		,3		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(tolfosync			).withValueFromStringFunction(fromlfosync		))); // NOTE, RANDOM, FREE, TRIGGER
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"arpon"						,1},"Arp On"																		 				 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"arpdirection"				,1},"Arp Direction"																	 ,0		,5		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(toarpdirection		).withValueFromStringFunction(fromarpdirection	))); // SEQUENCE, UP, DOWN, U&D, D&U, RANDOM
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"arpspeed"					,1},"Arp Speed"										,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(toarpspeed			).withValueFromStringFunction(fromarpspeed		)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"arpbpm"						,1},"Arp Speed (BPM sync)"															 ,0		,12		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(toarpbpm			).withValueFromStringFunction(fromarpbpm		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"arplength"					,1},"Arp Length"									,juce::NormalisableRange<float	>(0.0f	,1.0f	),1.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(toarplength		).withValueFromStringFunction(fromarplength		)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"vibratoon"					,1},"Vibrato On"																	 				 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"vibratorate"					,1},"Vibrato Rate"									,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tovibratorate		).withValueFromStringFunction(fromvibratorate	)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"vibratobpm"					,1},"Vibrato Rate (BPM sync)"														 ,0		,12		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(tovibratobpm		).withValueFromStringFunction(fromvibratobpm	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"vibratoamount"				,1},"Vibrato Amount"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tovibratoamount	).withValueFromStringFunction(fromvibratoamount	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"vibratoattack"				,1},"Vibrato Attack"								,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tovibratoattack	).withValueFromStringFunction(fromvibratoattack	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"antialias"					,1},"Anti-Alias"									,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.7f	,AudioParameterFloatAttributes().withStringFromValueFunction(toantialias		).withValueFromStringFunction(fromantialias		)));
	for(int o = 0; o < MC; ++o) {
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"o"+(String)o+"on"			,1},"OP"+(String)(o+1)+" On"														 				 ,o<=1	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"pan"			,1},"OP"+(String)(o+1)+" Pan"						,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(topan				).withValueFromStringFunction(frompan			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"amp"			,1},"OP"+(String)(o+1)+" Amplitude"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(toamp				).withValueFromStringFunction(fromamp			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"tone"			,1},"OP"+(String)(o+1)+" Tone"						,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized		).withValueFromStringFunction(fromnormalized	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"velocity"		,1},"OP"+(String)(o+1)+" Velocity"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tovelocity			).withValueFromStringFunction(fromvelocity		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"modat"			,1},"OP"+(String)(o+1)+" Mod Wheel / After-Touch"	,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tomodat			).withValueFromStringFunction(frommodat			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"o"+(String)o+"freqmode"		,1},"OP"+(String)(o+1)+" Frequency Mode"															 ,true	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tofreqmode			).withValueFromStringFunction(fromfreqmode		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"freqmult"		,1},"OP"+(String)(o+1)+" Frequency Multiplier"		,juce::NormalisableRange<float	>(0.0f	,24.0f	),1.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tofreqmult			).withValueFromStringFunction(fromfreqmult		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"freqadd"		,1},"OP"+(String)(o+1)+" Frequency Offset"			,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tofreqadd			).withValueFromStringFunction(fromfreqadd		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"attack"		,1},"OP"+(String)(o+1)+" Attack"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(toattack			).withValueFromStringFunction(fromattack		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"decay"			,1},"OP"+(String)(o+1)+" Decay"						,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(todecay			).withValueFromStringFunction(fromdecay			)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"sustain"		,1},"OP"+(String)(o+1)+" Sustain"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),1.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tonormalized		).withValueFromStringFunction(fromnormalized	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"release"		,1},"OP"+(String)(o+1)+" Release"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(torelease			).withValueFromStringFunction(fromrelease		)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"o"+(String)o+"lfoon"			,1},"OP"+(String)(o+1)+" LFO On"													 				 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"o"+(String)o+"lfotarget"		,1},"OP"+(String)(o+1)+" LFO Target"												 ,0		,3		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(tolfotarget		).withValueFromStringFunction(fromlfotarget		))); // AMPLITUDE, PITCH, PAN, TONE
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"lforate"		,1},"OP"+(String)(o+1)+" LFO Rate"					,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.5f	,AudioParameterFloatAttributes().withStringFromValueFunction(tolforate			).withValueFromStringFunction(fromlforate		)));
	parameters.push_back(std::make_unique<AudioParameterInt		>(ParameterID{"o"+(String)o+"lfobpm"		,1},"OP"+(String)(o+1)+" LFO Rate (BPM sync)"										 ,0		,15		 ,0		,AudioParameterIntAttributes()	.withStringFromValueFunction(tolfobpm			).withValueFromStringFunction(fromlfobpm		)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"lfoinfluence"	,1},"OP"+(String)(o+1)+" LFO Influence"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tolfoinfluence		).withValueFromStringFunction(fromlfoinfluence	)));
	parameters.push_back(std::make_unique<AudioParameterFloat	>(ParameterID{"o"+(String)o+"lfoattack"		,1},"OP"+(String)(o+1)+" LFO Attack"				,juce::NormalisableRange<float	>(0.0f	,1.0f	),0.0f	,AudioParameterFloatAttributes().withStringFromValueFunction(tolfoattack		).withValueFromStringFunction(fromlfoattack		)));
	}
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"filteron"					,1},"Filter On"														 								 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"distortionon"				,1},"Distortion On"													 								 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"phaseron"					,1},"Phaser On"														 								 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"choruson"					,1},"Chorus On"														 								 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"equalizeron"					,1},"Equalizer On"													 								 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"compressoron"				,1},"Compressor On"													 								 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"delayon"						,1},"Delay On"														 								 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	parameters.push_back(std::make_unique<AudioParameterBool	>(ParameterID{"reverbon"					,1},"Reverb On"														 								 ,false	,AudioParameterBoolAttributes()	.withStringFromValueFunction(tobool				).withValueFromStringFunction(frombool			)));
	return { parameters.begin(), parameters.end() };
}
