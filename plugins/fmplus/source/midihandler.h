#pragma once
#include "includes.h"
#include "functions.h"

class midihandler {
	public:
	struct note {
		float pitch;
		float velocity = 0;
		float aftertouch = 0;
		int voice = -1;
		bool sustained = false;
		void on(int notevoice, float notevel) {
			velocity = notevel;
			voice = notevoice;
			sustained = false;
		}
		void off() {
			voice = -1;
			sustained = false;
		}
		void steal() {
			voice = -1;
		}
		void unsteal(int notevoice) {
			voice = notevoice;
		}
	};
	struct voice {
		void reset(int samplesperblock);
		void noteon(int sample);
		void noteoff(int sample);

		int noteid = -1;
		float freq[3] { 1000, 1000, 1000 };
		float freqsmooth[3] { 1000, 1000, 1000 };

		int eventindex = 0;
		std::vector<int> events;
		bool ison = false;
	};

	float* params;
	int samplerate;
	int sample = 1;

	bool sustain = false;
	int pitchindex = 1;
	float pitchval = 0;
	float modval = 0;

	note notes[128];
	voice voices[24];

	int voicessize = 0;
	int activenotes[128];
	int activenotessize = 0;
	int activevoices[24];
	int activevoicessize = 0;
	int inactivevoices[24];
	int inactivevoicessize = 0;

	int lastchordsize = 0;
	int currentchordsize = 0;

	bool arpon = false;
	float arpprogress = 1;
	int arpindex = -1;
	int arplastnote = -1;
	bool arpdirection = false;
	bool arpdirty = false;
	int arporder[48];
	float arpspeed = 0;
	Random random;

	onepolevalue glide;

	void reset(int samplesperblock, int sr);
	void setvoices(int samplesperblock);
	void newbuffer();
	void processmessage(MidiMessage message);

	void noteon(int noteid, float notevel);
	void noteoff(int noteid, bool sustained);
	void allsoundoff();
	void sustainpedal(bool on);
	void pitchwheel(float val);
	void modwheel(int noteid, float val);

	void arpset();
	void arpupdate();
	void pitchesupdate();

	void tick();
	float getpitch(int v);
};
