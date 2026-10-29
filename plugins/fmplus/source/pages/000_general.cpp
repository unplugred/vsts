#include "000_general.h"

FMPlusPageGeneral::FMPlusPageGeneral() : FMPlusPage() {
	tabname = "Gen";
	paramstart = -2;
	params.resize(18);
	params[ 0] = potentiometer("Voices"						,"voices"			,"Voices"		,0		,1	,VC	,potentiometer::inttype		);
	params[ 1] = potentiometer("Pitch Bend"					,"pitchbend"		,"Pitch Bend"	,0		,0	,24	,potentiometer::inttype		);
	params[ 2] = potentiometer("Glide"						,"glide"			,"Glide"		,0		);
	params[ 3] = potentiometer("Legato"						,"legato"			,"Legato"		,0		,0	,1	,potentiometer::booltype	);
	params[ 4] = potentiometer("LFO Sync"					,"lfosync"			,"LFO Sync"		,0		,0	,3	,potentiometer::inttype		);
	params[ 5] = potentiometer("Arp On"						,"arpon"			,"Arp"			,0		,0	,1	,potentiometer::booltype	);
	params[ 6] = potentiometer("Arp Direction"				,"arpdirection"		,"Direction"	,0		,0	,5	,potentiometer::inttype		);
	params[ 7] = potentiometer("Arp Octave"					,"arpoctave"		,"Octave"		,0		,1	,3	,potentiometer::inttype		);
	params[ 8] = potentiometer("Arp Length"					,"arplength"		,"Length"		,0		);
	params[ 9] = potentiometer("Arp Speed"					,"arpspeed"			,"Speed"		,0		);
	params[ 9].bpmsync = 1;
	params[ 9].defaultbpm = true;
	params[10] = potentiometer("Arp Speed (BPM sync)"		,"arpbpm"			,"Speed"		,0		,0	,12	,potentiometer::inttype		);
	params[10].bpmsync = 2;
	params[10].defaultbpm = true;
	params[11] = potentiometer("Vibrato On"					,"vibratoon"		,"Vibrato"		,.001f	,0	,1	,potentiometer::booltype	);
	params[12] = potentiometer("Vibrato Shape"				,"vibratoshape"		,"Shape"		,0		,0	,3	,potentiometer::inttype		);
	params[13] = potentiometer("Vibrato Rate"				,"vibratorate"		,"Rate"			,0		);
	params[13].bpmsync = 1;
	params[14] = potentiometer("Vibrato Rate (BPM sync)"	,"vibratobpm"		,"Rate"			,0		,0	,12	,potentiometer::inttype		);
	params[14].bpmsync = 2;
	params[15] = potentiometer("Vibrato Amount"				,"vibratoamount"	,"Amount"		,.001f	);
	params[16] = potentiometer("Vibrato Attack"				,"vibratoattack"	,"Attack"		,0		);
	params[17] = potentiometer("Anti-Alias"					,"antialias"		,"Anti-Alias"	,0		); // 0.0 aa, 0.5 1x, 0.625 2x, 0.75 4x, 0.875 8x
	params[17].saved = false;
}
FMPlusPageGeneral::~FMPlusPageGeneral() { }
void FMPlusPageGeneral::rebuildtab() {
	mesh->boxes[mesh->boxnum++] = box(             -1, 31,  2,120, 10,2,15,true);

	mesh->boxes[mesh->boxnum++] = box(              0,114, 22, 37, 11,0, 4);
	mesh->boxes[mesh->boxnum++] = box(              1,114, 34, 37, 11,0, 4);
	mesh->boxes[mesh->boxnum++] = box(              2,114, 46, 37, 11,0, 5);
	mesh->boxes[mesh->boxnum++] = box(              3,114, 58, 37, 11,0, 3);
	mesh->boxes[mesh->boxnum++] = box(              4,114, 70, 37, 11,0, 4,true);

	sections[0] = 5;
	mesh->boxes[mesh->boxnum++] = box(              5, 31, 91,  8,  8,3);
	mesh->boxes[mesh->boxnum++] = box(              6,107,101, 44, 11,0, 6);
	mesh->boxes[mesh->boxnum++] = box(              7,107,113, 44, 11,0, 1);
	mesh->boxes[mesh->boxnum++] = box(              8,107,125, 44, 11,0, 4);
	mesh->boxes[mesh->boxnum++] = box(              9,107,137, 44, 11,0, 6,true);
	knobbox[10] = mesh->boxnum-1;

	sections[1] = 11;
	mesh->boxes[mesh->boxnum++] = box(             11, 31,158,  8,  8,3);
	mesh->boxes[mesh->boxnum++] = box(             12, 86,168, 65, 11,0, 7);
	mesh->boxes[mesh->boxnum++] = box(             13, 86,180, 65, 11,0, 6);
	knobbox[14] = mesh->boxnum-1;
	mesh->boxes[mesh->boxnum++] = box(             15, 86,192, 65, 11,0, 4);
	mesh->boxes[mesh->boxnum++] = box(             16, 86,204, 65, 11,0, 5,true);

	sections[2] = 17;
	mesh->boxes[mesh->boxnum++] = box(             17,114,224, 37, 11,0, 3);
	mesh->boxes[mesh->boxnum++] = box(             -1, 31,246,111, 10,2,15);
	mesh->boxes[mesh->boxnum++] = box(             -1,143,246,  8, 10,2);
}
void FMPlusPageGeneral::addtext(int i) {
	if(i ==  0) mesh->addtext(mesh->boxes[i].x+109,mesh->boxes[i].y,">",0,true);
	if(i == 17) mesh->addtext(mesh->boxes[i].x-1,mesh->boxes[i].y-11,"Theme:");
	if(i == 18) mesh->addtext(mesh->boxes[i].x,mesh->boxes[i].y,"X");
};
bool FMPlusPageGeneral::updatevalue(int param) {
	if(param == -2) { // preset name
		String text = presetname;
		int textlength = fmin(mesh->boxes[0].textamount,text.length());
		if(textlength == (mesh->boxes[0].textamount-1)) textlength += 1;
		text = text.substring(0,mesh->boxes[0].textamount-(presetunsaved?1:0));
		float x = mesh->boxes[0].x+floor((mesh->boxes[0].w-14)*.5f-textlength*3.5f);
		for(int i = 0; i < fmin(mesh->boxes[0].textamount,textlength+1); ++i) for(int t = 0; t < 6; ++t) {
			if(fmod(fmax(1,fmin(4,t)),2) == 1)
				mesh->text[(mesh->boxes[0].textmesh+i*6+t+1)*4] = (x+i*7  )/mesh->width;
			else
				mesh->text[(mesh->boxes[0].textmesh+i*6+t+1)*4] = (x+i*7+8)/mesh->width;
		}
		if(presetunsaved) text += "*";
		mesh->replacetext(mesh->boxes[0].textmesh,text,mesh->boxes[0].textamount);
		return true;
	}
	if(param == -1) { // theme
		int box = knobbox[15]+(param==-2?1:3);
		String text = themefile.substring(0,mesh->boxes[box].textamount);
		float x = mesh->boxes[box].x+floor(mesh->boxes[box].w*.5f-text.length()*3.5f);
		for(int i = 0; i < mesh->boxes[box].textamount; ++i) for(int t = 0; t < 6; ++t) {
			if(fmod(fmax(1,fmin(4,t)),2) == 1)
				mesh->text[(mesh->boxes[box].textmesh+i*6+t+1)*4] = (x+i*7  )/mesh->width;
			else
				mesh->text[(mesh->boxes[box].textmesh+i*6+t+1)*4] = (x+i*7+8)/mesh->width;
		}
		mesh->replacetext(mesh->boxes[box].textmesh,text,mesh->boxes[box].textamount);
		return true;
	}
	return false;
}
