#pragma once
#include "includes.h"

class midihandler {
	public:
	struct note {
		int noteid = -1;
		float velocity = -1;
		float aftertouch = -1;
		int voice = -1;
		bool sustained = false;
		note() {}
		note(int notei, float notevel, int notevoice) {
			noteid = notei;
			velocity = notevel;
			aftertouch = 0; // TODO
			voice = notevoice;
		}
	};
	struct voice {
		void reset(int samplesperblock);
		void noteon();
		void noteoff();
		void tick();

		int noteindex = -1;
		float freq[3] { 1000, 1000, 1000 };

		int eventindex = 0;
		std::vector<int> events;
		bool ison = false;
	};

	float* params;

	bool sustain = false;
	float pitchval = 0;
	float modval = 0;

	float pitches[128];

	note notes[128];
	int notessize = 0;

	voice voices[24];
	int activevoices[24];
	int activevoicessize = 0;
	int inactivevoices[24];
	int inactivevoicessize = 0;

	void reset(int samplesperblock);
	void setvoices(int samplesperblock, int count);
	void newbuffer();
	void processmessage(MidiMessage message);
	void noteon(int noteid, float notevel);
	void noteoff(int noteid, bool sustained);
	void allsoundoff();
	void sustainpedal(bool on);
	void pitchwheel();
	void modwheel();
	void tick();
};
