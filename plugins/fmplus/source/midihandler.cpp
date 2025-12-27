#include "midihandler.h"

void midihandler::voice::reset(int samplesperblock) {
	events.resize(fmax(2,round(.1f*samplesperblock)));
	for(int e = 0; e < events.size(); ++e) {
		if(events[e] == 0) break;
		events[e] = 0;
	}
	eventindex = 0;
	noteindex = -1;
	ison = false;
}
void midihandler::voice::noteon() {
	ison = true;
	if(eventindex >= (events.size()-1))
		return;
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
	for(int v = 0; v < params[0]; ++v) {
		for(int e = 0; e < voices[v].events.size(); ++e) {
			if(voices[v].events[e] == 0) break;
			voices[v].events[e] = 0;
		}
		voices[v].eventindex = 0;
	}
}

void midihandler::reset(int samplesperblock) {
	notessize = 0;
	activevoicessize = 0;
	inactivevoicessize = params[0];
	for(int v = 0; v < params[0]; ++v) {
		voices[v].reset(samplesperblock);
		inactivevoices[v] = v;
	}
}
void midihandler::setvoices(int samplesperblock, int count) {
	if(params[0] < count) {
		for(int v = params[0]; v < count; ++v) {
			voices[v].reset(samplesperblock);

			if(inactivevoicessize < 0) {
				int noteindex = inactivevoicessize*-1-1;
				notes[noteindex].voice = v;
				voices[v].noteon();
				voices[v].noteindex = noteindex;
				voices[v].freq[0] = pitches[(int)fmax(notes[noteindex].noteid-1,  0)];
				voices[v].freq[1] = pitches[          notes[noteindex].noteid       ];
				voices[v].freq[2] = pitches[(int)fmin(notes[noteindex].noteid+1,127)];
				activevoices[activevoicessize++] = v;
			} else {
				inactivevoices[inactivevoicessize] = v;
			}
			++inactivevoicessize;
		}

	} else {
		int offset = 0;
		for(int n = 0; n < notessize; ++n) {
			while((n+offset) < notessize && notes[n+offset].voice >= count) {
				voices[notes[n+offset].voice].noteindex = -1;
				++offset;
			}
			if((n+offset) >= notessize) break;
			if(offset > 0) {
				notes[n] = notes[n+offset];
				voices[notes[n].voice].noteindex = n;
			}
		}
		notessize -= offset;

		if(inactivevoicessize > 0) {
			offset = 0;
			for(int v = 0; v < inactivevoicessize; ++v) {
				while((v+offset) < inactivevoicessize && inactivevoices[v+offset] >= count) {
					voices[activevoices[v+offset]].reset(0);
					++offset;
				}
				if((v+offset) >= inactivevoicessize) break;
				if(offset > 0) inactivevoices[v] = inactivevoices[v+offset];
			}
			inactivevoicessize -= offset;
		}

		offset = 0;
		for(int v = 0; v < activevoicessize; ++v) {
			while((v+offset) < activevoicessize && activevoices[v+offset] >= count) {
				voices[activevoices[v+offset]].reset(0);
				++offset;
			}
			if((v+offset) >= activevoicessize) break;
			if(offset > 0) activevoices[v] = activevoices[v+offset];
		}
		activevoicessize -= offset;
		inactivevoicessize -= offset;
	}
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
		pitchwheel();
	} else if(message.isAftertouch() || message.isChannelPressure() || message.isController()) {
		modwheel();
	}
}
void midihandler::noteon(int noteid, float notevel) {
	bool found = false;
	for(int n = 0; n < notessize; ++n) {
		if(!found && notes[n].noteid == noteid) {
			if(notes[n].voice == -1) {
				noteoff(noteid,false);
				break;
			} else {
				for(int v = 0; v < (activevoicessize-1); ++v) {
					if(activevoices[v] == notes[n].voice)
						found = true;
					if(found)
						activevoices[v] = activevoices[v+1];
				}
				found = true;
				activevoices[activevoicessize-1] = notes[n].voice;
				voices[notes[n].voice].noteon();
				notes[notessize] = note(noteid,notevel,notes[n].voice);
			}
		}
		if(found) {
			notes[n] = notes[n+1];
			voices[notes[n].voice].noteindex = n;
		}
	}
	if(found) return;

	int voicenum;
	if(inactivevoicessize <= 0) {
		voicenum = activevoices[0];
		notes[inactivevoicessize*-1].voice = -1;
		for(int v = 0; v < (activevoicessize-1); ++v)
			activevoices[v] = activevoices[v+1];
		activevoices[activevoicessize-1] = voicenum;
	} else {
		voicenum = inactivevoices[0];
		for(int v = 0; v < (inactivevoicessize-1); ++v)
			inactivevoices[v] = inactivevoices[v+1];
		activevoices[activevoicessize++] = voicenum;
	}
	--inactivevoicessize;

	voices[voicenum].noteon();
	voices[voicenum].noteindex = notessize;
	voices[voicenum].freq[0] = pitches[(int)fmax(noteid-1,  0)];
	voices[voicenum].freq[1] = pitches[          noteid       ];
	voices[voicenum].freq[2] = pitches[(int)fmin(noteid+1,127)];
	notes[notessize++] = note(noteid,notevel,voicenum);
}
void midihandler::noteoff(int noteid, bool sustained) {
	if(sustained) {
		for(int n = 0; n < notessize; ++n)
			if(notes[n].noteid == noteid)
				notes[n].sustained = true;
		return;
	}

	int offset = 0;
	for(int n = 0; n < notessize; ++n) {
		while((n+offset) < notessize && ((noteid == -9 && notes[n+offset].sustained) || notes[n+offset].noteid == noteid)) {
			if(notes[n+offset].voice != -1) {
				if(inactivevoicessize < 0) {
					int noteindex = inactivevoicessize*-1-1;
					notes[noteindex].voice = notes[n+offset].voice;
					voices[notes[noteindex].voice].noteon();
					voices[notes[noteindex].voice].noteindex = noteindex;
					voices[notes[noteindex].voice].freq[0] = pitches[(int)fmax(notes[noteindex].noteid-1,  0)];
					voices[notes[noteindex].voice].freq[1] = pitches[          notes[noteindex].noteid       ];
					voices[notes[noteindex].voice].freq[2] = pitches[(int)fmin(notes[noteindex].noteid+1,127)];

					bool found = false;
					for(int v = (activevoicessize-1); v > 0; --v) {
						if(!found) {
							if(activevoices[v] == notes[noteindex].voice)
								found = true;
							else
								continue;
						}
						activevoices[v] = activevoices[v-1];
					}
					activevoices[0] = notes[noteindex].voice;
				} else {
					voices[notes[n+offset].voice].noteoff();
					voices[notes[n+offset].voice].noteindex = -1;
					bool found = false;
					for(int v = 0; v < (activevoicessize-1); ++v) {
						if(!found) {
							if(activevoices[v] == notes[n+offset].voice)
								found = true;
							else
								continue;
						}
						activevoices[v] = activevoices[v+1];
					}
					--activevoicessize;
					inactivevoices[inactivevoicessize] = notes[n+offset].voice;
				}
			}
			++inactivevoicessize;
			++offset;
		}

		if((n+offset) >= notessize) break;
		if(offset > 0) {
			notes[n] = notes[n+offset];
			voices[notes[n].voice].noteindex = n;
		}
	}
	notessize -= offset;
}
void midihandler::allsoundoff() {
	for(int v = 0; v < activevoicessize; ++v) {
		voices[activevoices[v]].noteoff();
		inactivevoices[inactivevoicessize++] = activevoices[v];
	}
	activevoicessize = 0;
	// TODO cut env
}
void midihandler::sustainpedal(bool on) {
	if(!on) noteoff(-9,false);
	sustain = on;
}
void midihandler::pitchwheel() {
	// TODO
}
void midihandler::modwheel() {
	// TODO
}
void midihandler::tick() {
	// TODO arp
	for(int v = 0; v < params[0]; ++v) voices[v].tick();
	// TODO vibrato
	// TODO portamento
}
