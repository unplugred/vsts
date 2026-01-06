#include "midihandler.h"

void midihandler::voice::reset(int samplesperblock) {
	events.resize(fmax(2,round(.1f*samplesperblock)));
	for(int e = 0; e < events.size(); ++e) {
		if(events[e] == 0) break;
		events[e] = 0;
	}
	eventindex = 0;
	noteid = -1;
	ison = false;
}
void midihandler::voice::noteon() {
	if(eventindex >= (events.size()-1))
		return;
	ison = true;
	if(events[eventindex] != 0)
		events[++eventindex] = 0;
}
void midihandler::voice::noteoff() {
	ison = false;
	if(eventindex >= (events.size()-1)) {
		if(events[eventindex] > 0) eventindex *= -1;
		return;
	}
	if(events[eventindex] > 0)
		events[++eventindex] = 0;
	else if(events[eventindex] == 0 && eventindex > 0 && events[eventindex-1] < 0)
		--eventindex;
}
void midihandler::voice::tick() {
	events[eventindex] += ison?1:-1;
}
void midihandler::newbuffer() {
	for(int v = 0; v < voicessize; ++v) {
		for(int e = 0; e < voices[v].events.size(); ++e) {
			if(voices[v].events[e] == 0) break;
			voices[v].events[e] = 0;
		}
		voices[v].eventindex = 0;
	}
}

void midihandler::reset(int samplesperblock, int sr) {
	voicessize = params[0];
	samplerate = sr;
	for(int v = 0; v < voicessize; ++v)
		voices[v].reset(samplesperblock);
	glide.reset(params[2]*params[2]*MAXGLIDE,samplerate,voicessize*3,-666);
	allsoundoff();
}
void midihandler::setvoices(int samplesperblock) {
	glide.reset(params[2]*params[2]*MAXGLIDE,samplerate,params[0]*3,-666);
	if(voicessize < params[0]) { // increase voice count
		for(int v = voicessize; v < params[0]; ++v) {
			voices[v].reset(samplesperblock);

			if(inactivevoicessize < 0) { // restore stolen notes
				int noteindex = inactivevoicessize*-1-1;
				notes[activenotes[noteindex]].unsteal(v);
				if(params[5] < .5f) voices[v].noteon();
				voices[v].noteid = activenotes[noteindex];
				voices[v].freq[0] = notes[(int)fmax(activenotes[noteindex]-params[1],  0)].pitch;
				voices[v].freq[1] = notes[          activenotes[noteindex]               ].pitch;
				voices[v].freq[2] = notes[(int)fmin(activenotes[noteindex]+params[1],127)].pitch;
				if(params[2] == 0) for(int i = 0; i < 3; ++i)
					voices[v].freqsmooth[i] = voices[v].freq[i];
				activevoices[activevoicessize++] = v;
			} else {
				inactivevoices[inactivevoicessize] = v;
			}
			++inactivevoicessize;
		}

	} else { // decrease voice count
		int offset = 0;
		for(int n = 0; n < activenotessize; ++n) {
			while((n+offset) < activenotessize && notes[activenotes[n+offset]].voice >= params[0]) {
				++offset;
			}
			if((n+offset) >= activenotessize) break;
			if(offset > 0) activenotes[n] = activenotes[n+offset];
		}
		activenotessize -= offset;

		for(int n = 0; n < 128; ++n)
			if(notes[n].voice >= params[0])
				notes[n].off();

		if(inactivevoicessize > 0) {
			offset = 0;
			for(int v = 0; v < inactivevoicessize; ++v) {
				while((v+offset) < inactivevoicessize && inactivevoices[v+offset] >= params[0]) {
					voices[inactivevoices[v+offset]].reset(0);
					++offset;
				}
				if((v+offset) >= inactivevoicessize) break;
				if(offset > 0) inactivevoices[v] = inactivevoices[v+offset];
			}
			inactivevoicessize -= offset;
		}

		offset = 0;
		for(int v = 0; v < activevoicessize; ++v) {
			while((v+offset) < activevoicessize && activevoices[v+offset] >= params[0]) {
				voices[activevoices[v+offset]].reset(0);
				++offset;
			}
			if((v+offset) >= activevoicessize) break;
			if(offset > 0) activevoices[v] = activevoices[v+offset];
		}
		activevoicessize -= offset;
	}
	voicessize = params[0];

	arpupdate();
}
void midihandler::processmessage(MidiMessage message) {
	       if(message.isNoteOn()) {
		noteon(message.getNoteNumber(),message.getFloatVelocity());
	} else if(message.isNoteOff()) {
		noteoff(message.getNoteNumber(),sustain);
	} else if(message.isAllSoundOff()) {
		allsoundoff();
	} else if(message.isSustainPedalOn()) {
		sustainpedal(true);
	} else if(message.isSustainPedalOff()) {
		sustainpedal(false);
	} else if(message.isPitchWheel()) {
		pitchwheel(message.getPitchWheelValue()/16383.f);
	} else if(message.isAftertouch()) {
		modwheel(message.getNoteNumber(),.5f);
	} else if(message.isChannelPressure()) {
		modwheel(-1,.5f);
	} else if(message.isController()) {
		modwheel(-1,.5f);
	}
}
void midihandler::noteon(int noteid, float notevel) {
	bool found = false; // seek if note already is on
	for(int n = 0; n < activenotessize; ++n) {
		if(!found && activenotes[n] == noteid) {
			if(notes[noteid].voice == -1) { // note on but stolen
				noteoff(noteid,false);
				break;
			} else { // note on, bring to front
				for(int v = 0; v < (activevoicessize-1); ++v) {
					if(activevoices[v] == notes[noteid].voice)
						found = true;
					if(found)
						activevoices[v] = activevoices[v+1];
				}
				activevoices[activevoicessize-1] = notes[noteid].voice;
				found = true;
				activenotes[activenotessize] = noteid;
				notes[noteid].on(notes[noteid].voice,notevel);
				if(params[5] < .5f && params[3] < .5f) voices[notes[noteid].voice].noteon();
			}
		}
		if(found) activenotes[n] = activenotes[n+1];
	}
	if(found) {
		arpupdate();
		return;
	}

	int voicenum;
	if(inactivevoicessize <= 0) { // steal
		voicenum = activevoices[0];
		notes[activenotes[inactivevoicessize*-1]].steal();
		for(int v = 0; v < (activevoicessize-1); ++v)
			activevoices[v] = activevoices[v+1];
		activevoices[activevoicessize-1] = voicenum;
		if(params[5] < .5f && params[3] < .5f) voices[voicenum].noteon();
	} else { // dont steal
		int voiceindex = 0;
		if(params[3] > .5) {
			voiceindex = inactivevoicessize-1;
			int dist = 1000;
			for(int v = fmax(0,inactivevoicessize-(lastchordsize-activevoicessize)); v < inactivevoicessize; ++v) {
				if(voices[inactivevoices[v]].noteid == -1) continue;
				if(fabs(voices[inactivevoices[v]].noteid-noteid) <= dist) {
					dist = fabs(voices[inactivevoices[v]].noteid-noteid);
					voiceindex = v;
					if(dist == 0) break;
				}
			}
		} else {
			for(int v = 0; v < inactivevoicessize; ++v) {
				if(voices[inactivevoices[v]].noteid == noteid) {
					voiceindex = v;
					break;
				}
			}
		}
		voicenum = inactivevoices[voiceindex];
		for(int v = voiceindex; v < (inactivevoicessize-1); ++v)
			inactivevoices[v] = inactivevoices[v+1];
		activevoices[activevoicessize++] = voicenum;
		if(params[5] < .5f) voices[voicenum].noteon();
	}
	--inactivevoicessize;
	currentchordsize = activevoicessize;

	voices[voicenum].noteid = noteid;
	voices[voicenum].freq[0] = notes[(int)fmax(noteid-params[1],  0)].pitch;
	voices[voicenum].freq[1] = notes[          noteid               ].pitch;
	voices[voicenum].freq[2] = notes[(int)fmin(noteid+params[1],127)].pitch;
	if(params[2] == 0) for(int i = 0; i < 3; ++i)
		voices[voicenum].freqsmooth[i] = voices[voicenum].freq[i];
	if(params[3] < .5 && inactivevoicessize >= 0)
		for(int i = 0; i < 3; ++i)
			glide.setto(voices[voicenum].freq[i],voicenum*3+i);
	activenotes[activenotessize++] = noteid;
	notes[noteid].on(voicenum,notevel);

	arpupdate();
}
void midihandler::noteoff(int noteid, bool sustained) {
	if(sustained) { // dont release sustained notes, mark for later
		if(notes[noteid].voice != -1)
			notes[noteid].sustained = true;
		return;
	}

	int offset = 0;
	for(int n = 0; n < activenotessize; ++n) {
		while((n+offset) < activenotessize && ((noteid == -9 && notes[activenotes[n+offset]].sustained) || activenotes[n+offset] == noteid)) {
			if(notes[activenotes[n+offset]].voice != -1) {
				if(inactivevoicessize < 0) { // restore stolen note
					int noteindex = inactivevoicessize*-1-1;
					int voiceindex = notes[activenotes[n+offset]].voice;
					notes[activenotes[noteindex]].unsteal(voiceindex);
					if(params[5] < .5f && params[3] < .5f) voices[voiceindex].noteon();
					voices[voiceindex].noteid = activenotes[noteindex];
					voices[voiceindex].freq[0] = notes[(int)fmax(activenotes[noteindex]-params[1],  0)].pitch;
					voices[voiceindex].freq[1] = notes[          activenotes[noteindex]               ].pitch;
					voices[voiceindex].freq[2] = notes[(int)fmin(activenotes[noteindex]+params[1],127)].pitch;
					if(params[2] == 0) for(int i = 0; i < 3; ++i)
						voices[voiceindex].freqsmooth[i] = voices[voiceindex].freq[i];

					bool found = false;
					for(int v = (activevoicessize-1); v > 0; --v) {
						if(!found) {
							if(activevoices[v] == voiceindex)
								found = true;
							else
								continue;
						}
						activevoices[v] = activevoices[v-1];
					}
					activevoices[0] = voiceindex;
				} else { //note off
					int voiceindex = notes[activenotes[n+offset]].voice;
					if(params[5] < .5f) voices[voiceindex].noteoff();
					bool found = false;
					for(int v = 0; v < (activevoicessize-1); ++v) {
						if(!found) {
							if(activevoices[v] == voiceindex)
								found = true;
							else
								continue;
						}
						activevoices[v] = activevoices[v+1];
					}
					--activevoicessize;
					inactivevoices[inactivevoicessize] = voiceindex;
				}
				notes[activenotes[n+offset]].off();
			}
			++inactivevoicessize;
			++offset;
		}

		if((n+offset) >= activenotessize) break;
		if(offset > 0)
			activenotes[n] = activenotes[n+offset];
	}
	activenotessize -= offset;
	if(activenotessize == 0) lastchordsize = currentchordsize;

	arpupdate();
}
void midihandler::allsoundoff() {
	for(int v = 0; v < activevoicessize; ++v)
		voices[activevoices[v]].noteoff();
	activenotessize = 0;
	activevoicessize = 0;
	inactivevoicessize = voicessize;
	for(int v = 0; v < inactivevoicessize; ++v) {
		voices[v].noteid = -1;
		inactivevoices[v] = v;
	}
	for(int n = 0; n < 128; ++n) {
		notes[n].velocity = 0;
		notes[n].aftertouch = 0;
		notes[n].off();
	}
	arpupdate();
	glide.current[0] = -666;
}
void midihandler::sustainpedal(bool on) {
	if(!on) noteoff(-9,false);
	sustain = on;
}
void midihandler::pitchwheel(float val) {
	pitchindex = val>=.5f?1:0;
	pitchval = val*2-pitchindex;
}
void midihandler::modwheel(int noteid, float val) {
	modval = .5f;
	// TODO
}

void midihandler::arpset() {
	if(params[5] > .5f) {
		arplastnote = -1;
		for(int v = 0; v < activevoicessize; ++v)
			voices[activevoices[v]].noteoff();
		arpupdate();
	} else {
		for(int v = 0; v < activevoicessize; ++v)
			voices[activevoices[v]].noteon();
	}
}
void midihandler::arpupdate() {
	if(params[5] < .5f) return;
	int size = activevoicessize;
	for(int v = size; v < 48; ++v) arporder[v] = -1;
	if(size == 0) {
		arpdirty = true;
		return;
	}

	for(int v = 0; v < size; ++v)
		arporder[v] = activevoices[v];
	if(activevoicessize > 1 && params[6] != 0) {
		if(params[6] == 5) { // shuffle
			for(int i = size-1; i > 0; --i) {
				int pick = random.nextInt(i+1);
				int temp = arporder[i];
				arporder[i] = arporder[pick];
				arporder[pick] = temp;
			}
		} else { //sort
			bool up = params[6]==1||params[6]==3;
			for(int step = 0; step < (size-1); ++step) {
				for(int i = 0; i < (size-step-1); ++i) {
					if((voices[arporder[i]].noteid>voices[arporder[i+1]].noteid) == up) {
						int temp = arporder[i];
						arporder[i] = arporder[i+1];
						arporder[i+1] = temp;
					}
				}
			}
			if(params[6] >= 3) { // mirror
				size = (size-1)*2;
				for(int v = activevoicessize; v < size; ++v)
					arporder[v] = arporder[size-v];
			}
		}
	}

	if(arplastnote == -1) { // start new arp
		arpon = false;
		arpprogress = 1;
		arpindex = -1;
		arpdirection = params[6]==1||params[6]==3;
	}
	arpdirty = true;
}
void midihandler::pitchesupdate() {
	for(int v = 0; v < voicessize; ++v) {
		if(voices[v].noteid == -1) continue;
		voices[v].freq[0] = notes[(int)fmax(voices[v].noteid-params[1],  0)].pitch;
		voices[v].freq[1] = notes[          voices[v].noteid               ].pitch;
		voices[v].freq[2] = notes[(int)fmin(voices[v].noteid+params[1],127)].pitch;
	}
}

void midihandler::tick() {
	// ---- ARP ----
	if(params[5] > .5f && (arporder[0] != -1 || arplastnote != -1)) {
		arpprogress += arpspeed;
		if(arpon && arpprogress >= params[7]) { // note off
			if(arpdirty) {
				for(int v = 0; v < inactivevoicessize; ++v)
					if(voices[inactivevoices[v]].ison)
						voices[inactivevoices[v]].noteoff();
				for(int v = 0; v < activevoicessize; ++v)
					if(voices[activevoices[v]].ison)
						voices[activevoices[v]].noteoff();
			} else voices[arporder[arpindex]].noteoff();
			arpon = false;
		}
		if(arpprogress >= 1) { // next note
			if(arpdirty)
				for(int v = 0; v < inactivevoicessize; ++v)
					if(voices[inactivevoices[v]].ison)
						voices[inactivevoices[v]].noteoff();
			if(arporder[0] == -1) { // termination
				arplastnote = -1;
			} else { // find closest note
				arpprogress = fmod(arpprogress,1.f);
				if(arpdirty && arplastnote != -1) {
					int dist = 1000;
					bool skipfirsthalf = arporder[2]!=-1&&((params[6]==3&&!arpdirection)||(params[6]==4&&arpdirection));
					for(int v = 0; v < 48; ++v) {
						if(arporder[v] == -1) break;
						if(skipfirsthalf && v != 0) {
							if(arporder[v] == arporder[v+2])
								skipfirsthalf = false;
							continue;
						}
						if(fabs(voices[arporder[v]].noteid-arplastnote) < dist) {
							arpindex = v;
							dist = fabs(voices[arporder[v]].noteid-arplastnote);
							if(dist == 0) break;
						}
					}
				}
				if(arporder[++arpindex] == -1) arpindex = 0; // advance
				voices[arporder[arpindex]].noteon();
				if(arplastnote != -1 && params[6] == 3 || params[6] == 4)
					arpdirection = arplastnote<voices[arporder[arpindex]].noteid;
				arplastnote = voices[arporder[arpindex]].noteid;
				arpdirty = false;
				arpon = true;
			}
		}
	}

	// ---- VOICE TICK ----
	for(int v = 0; v < voicessize; ++v) voices[v].tick();

	// ---- GLIDE ----
	if(params[2] > 0) {
		if(glide.current[0] == -666)
			for(int v = 0; v < voicessize; ++v)
				for(int i = 0; i < 3; ++i)
					glide.setto(voices[v].freq[i],v*3+i);

		for(int v = 0; v < voicessize; ++v)
			for(int i = 0; i < 3; ++i)
				voices[v].freqsmooth[i] = glide.nextvalue(voices[v].freq[i],v*3+i);
	}

	// TODO move on off to timestamps
}
float midihandler::getpitch(int v) {
	return voices[v].freqsmooth[pitchindex]*(1-pitchval)+voices[v].freqsmooth[pitchindex+1]*pitchval;
}
