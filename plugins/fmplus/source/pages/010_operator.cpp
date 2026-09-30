#include "010_operator.h"
#include "../oscillator.h"

FMPlusPageOperator::FMPlusPageOperator() : FMPlusPage() {
	tabname = "OP#";
	instances = MC;

	linesnum = 3;
	dividerx[0] = .3866666f; // - 1
	dividery[0] = .25f;      // 2 -
	dividery[1] = .5f;       // 4 -

	adsrnum = 1;

	curvesnum = 1;
	curves.resize(1);
	curves[0].channels = VC;
	curves[0].linenum = 3;
	curves[0].animid = 4;

	framedatavals = 2;

	params.resize(19);
	params[ 0] = potentiometer("On"							,"on"			,"OPERATOR_X"	,.001f	,0	,1	,potentiometer::booltype	);
	params[ 1] = potentiometer("Pan"						,"pan"			,"Pan"			,.001f	);
	params[ 2] = potentiometer("Amplitude"					,"amp"			,"Amp"			,.001f	);
	params[ 3] = potentiometer("Tone"						,"tone"			,"Tone"			,.001f	);
	params[ 3].updateline = 1;
	params[ 4] = potentiometer("Velocity"					,"velocity"		,"Velocity"		,.001f	);
	params[ 5] = potentiometer("Mod Wheel / After-Touch"	,"modat"		,"Mod W/Aft.T"	,.001f	);
	params[ 6] = potentiometer("Frequency Mode"				,"freqmode"		,"Mode"			,0		,0	,1	,potentiometer::booltype	);
	params[ 7] = potentiometer("Frequency Multiplier"		,"freqmult"		,"*"			,0		,0	,24	);
	params[ 8] = potentiometer("Frequency Offset"			,"freqadd"		,"+"			,0		);
	params[ 9] = potentiometer("Attack"						,"attack"		,"Attack"		,0		);
	params[ 9].adsrstage =  0;
	params[ 9].adsr = 0;
	params[ 9].updateline = 2;
	params[10] = potentiometer("Decay"						,"decay"		,"Decay"		,0		);
	params[10].adsrstage =  1;
	params[10].adsr = 0;
	params[10].updateline = 2;
	params[11] = potentiometer("Sustain"					,"sustain"		,"Sustain"		,.001f	);
	params[11].adsrstage = 12;
	params[11].adsr = 0;
	params[11].updateline = 2;
	params[12] = potentiometer("Release"					,"release"		,"Release"		,0		);
	params[12].adsrstage =  3;
	params[12].adsr = 0;
	params[12].updateline = 2;
	params[13] = potentiometer("LFO On"						,"lfoon"		,"LFO"			,.001f	,0	,1	,potentiometer::booltype	);
	params[14] = potentiometer("LFO Target"					,"lfotarget"	,"Target"		,0		,0	,3	,potentiometer::inttype		);
	params[15] = potentiometer("LFO Rate"					,"lforate"		,"Rate"			,0		);
	params[15].bpmsync = 1;
	params[16] = potentiometer("LFO Rate (BPM sync)"		,"lfobpm"		,"Rate"			,0		,0	,15	,potentiometer::inttype		);
	params[16].bpmsync = 2;
	params[17] = potentiometer("LFO Influence"				,"lfoinfluence"	,"Influence"	,.001f	);
	params[18] = potentiometer("LFO Attack"					,"lfoattack"	,"Attack"		,0		);
}
FMPlusPageOperator::~FMPlusPageOperator() { }
void FMPlusPageOperator::calcvis(int lineupdated) {
	if(lineupdated == 1) {
		morphosc osc;
		osc.update(params[3].uismooth);
		int box = knobbox[3];
		int h = mesh->boxes[box].h;
		int x = mesh->boxes[box].x;
		int y = mesh->height-h-mesh->boxes[box].y;
		mesh->linewritepos = mesh->linesegments[0];
		mesh->beginline(x,y+(osc.calc(-1)*-.5f+.5f)*h);
		for(int i = 0; i < (h+1); ++i)
			mesh->nextpoint(x+i+1,y+(osc.calc((((float)i+1)/h)*2-1)*.5f+.5f)*h);
		mesh->endline();
		mesh->linesegments[1] = mesh->linewritepos;
		return;
	}

	if(lineupdated == 2) {
		int box = knobbox[9];
		int w = 120;
		int h = mesh->boxes[box].h;
		int x = mesh->boxes[box].x;
		int y = mesh->height-mesh->boxes[box].y;
		float currentx = adsr[0][0]+.5f;
		mesh->linewritepos = mesh->linesegments[1];
		mesh->beginline(x,y-h+11);
		mesh->nextpoint(x+currentx,y,true);
		currentx += adsr[0][4]+1.f;
		mesh->nextpoint(x+currentx,y-(1-params[11].uismooth)*(h-11),true);
		currentx += adsr[0][8]+1.f;
		mesh->nextpoint(x+currentx,y-(1-params[11].uismooth)*(h-11),true);
		mesh->nextpoint(x+w,y-h+11,true);
		mesh->nextpoint(x+w,y-h+11);
		mesh->endline();
		mesh->linesegments[2] = mesh->linewritepos;
		return;
	}

	if(lineupdated == 3) {
		int box = knobbox[14]-1;
		int w = mesh->boxes[box].w;
		int h = mesh->boxes[box].h;
		int x = mesh->boxes[box].x;
		int y = mesh->height-h-mesh->boxes[box].y;
		mesh->linewritepos = mesh->linesegments[2];
		curveiterator iterator;
		iterator.reset(curves[0].cui[mesh->o],w);
		mesh->beginline(x,y+iterator.next()*h);
		for(int i = 0; i < (w+1); ++i)
			mesh->nextpoint(x+i+1,y+iterator.next()*h);
		mesh->endline();
		mesh->linesegments[3] = mesh->linewritepos;
		return;
	}
}
void FMPlusPageOperator::rebuildtab() {
	mesh->boxes[mesh->boxnum++] = box( 0, 31,  2,  8,  8,3);
	mesh->boxes[mesh->boxnum++] = box( 1, 65, 12, 51, 11,1, 5);
	mesh->boxes[mesh->boxnum++] = box( 2, 65, 24, 51, 11,0, 4);
	mesh->boxes[mesh->boxnum++] = box( 3,117,  2, 33, 33,2);
	mesh->boxes[mesh->boxnum++] = box( 4,121, 36, 29, 11,0, 4);
	mesh->boxes[mesh->boxnum++] = box( 5,121, 48, 29, 11,0, 4,true);

	sections[0] = 6;
	mesh->boxes[mesh->boxnum++] = box( 6, 60, 76,  8, 10,2, 1);
	mesh->boxes[mesh->boxnum++] = box(-1, 69, 69, 29,  6,2);
	mesh->boxes[mesh->boxnum++] = box( 7, 69, 76, 29, 10,2, 4);
	mesh->boxes[mesh->boxnum++] = box(-1, 69, 87, 29,  6,2);
	mesh->boxes[mesh->boxnum++] = box(-1, 99, 76,  8, 10,2, 1);
	mesh->boxes[mesh->boxnum++] = box(-1,108, 69, 29,  6,2);
	mesh->boxes[mesh->boxnum++] = box( 8,108, 76, 29, 10,2, 4);
	mesh->boxes[mesh->boxnum++] = box(-1,108, 87, 29,  6,2);

	sections[1] = 9;
	mesh->boxes[mesh->boxnum++] = box( 9, 31,103,120, 43,4);
	mesh->boxes[mesh->boxnum++] = box(10, 31,103,120, 43,4);
	mesh->boxes[mesh->boxnum++] = box(11, 31,103,120, 43,4);
	mesh->boxes[mesh->boxnum++] = box(12, 31,103,120, 43,4, 5,true);

	sections[2] = 13;
	mesh->boxes[mesh->boxnum++] = box(13, 31,159,  8,  8,3);
	mesh->boxes[mesh->boxnum++] = box(-1, 31,168,120, 40,2);
	mesh->boxes[mesh->boxnum-1].curve = 0;
	mesh->boxes[mesh->boxnum++] = box(14,107,209, 44, 11,0, 5);
	mesh->boxes[mesh->boxnum++] = box(15,107,221, 44, 11,0, 5);
	knobbox[16] = mesh->boxnum-1;
	mesh->boxes[mesh->boxnum++] = box(17,107,233, 44, 11,1, 5);
	mesh->boxes[mesh->boxnum++] = box(18,107,245, 44, 11,0, 5,true);
}
void FMPlusPageOperator::addtext(int i) {
	if(i ==  6) mesh->addtext(30,mesh->boxes[i].y,"Freq");
	if(i ==  7) mesh->addtext(mesh->boxes[i].x+11,mesh->boxes[i].y-2,"<",0,true);
	if(i ==  9) mesh->addtext(mesh->boxes[i].x+11,mesh->boxes[i].y-2,">",0,true);
	if(i == 11) mesh->addtext(mesh->boxes[i].x+11,mesh->boxes[i].y-2,"<",0,true);
	if(i == 12) mesh->addsquare(mesh->boxes[i].x,mesh->boxes[i].y,8,mesh->boxes[i].h,0);
	if(i == 13) mesh->addtext(mesh->boxes[i].x+11,mesh->boxes[i].y-2,">",0,true);
	if(i == 12) mesh->addtext(137,mesh->boxes[i].y,"Hz");
}
bool FMPlusPageOperator::updatevalue(int param) {
	if(param == 0)
		mesh->replacetext(mesh->boxes[knobbox[param]].textmesh-6,(String)(mesh->tab-2));
	if(param == 3)
		return true;
	if(param >= 9 && param <= 12) {
		float padsr[4] = {
			powf(valuenorm[ 9][mesh->o],2)*(MAXA-MINA)+MINA,
			powf(valuenorm[10][mesh->o],2)*(MAXD-MIND)+MIND,
			1,
			powf(valuenorm[12][mesh->o],2)*(MAXR-MINR)+MINR
		};
		float scaling = (120.f-3)/(padsr[0]+padsr[1]+padsr[2]+padsr[3]);
		for(int p = 0; p < 4; ++p) padsr[p] *= scaling;

		for(int s = 0; s < 2; ++s) {
			float prevovershoot = 0;
			int prevnonclampedsections = 4;
			for(int i = 0; i < 3; ++i) {
				float overshoot = -120.f+3;
				int nonclampedsections = 4;
				for(int p = 0; p < 4; ++p) {
					padsr[p] -= (prevovershoot/prevnonclampedsections);
					if(padsr[p] <= (s==0?1:8)) {
						--nonclampedsections;
						padsr[p] = s==0?1:8;
					}
					overshoot += padsr[p];
				}
				if(overshoot < .5f) break;
				prevnonclampedsections = nonclampedsections;
				prevovershoot = overshoot;
			}

			for(int p = 0; p < 4; ++p) adsr[0][p*4+s*2+1] = padsr[p];
		}
		float currentx = mesh->boxes[knobbox[9]].x;
		for(int p = 0; p < 4; ++p) {
			mesh->boxes[knobbox[9+p]].x = round(              currentx);
			mesh->boxes[knobbox[9+p]].w = round(adsr[0][p*4+2+1]+currentx)-round(currentx);
			currentx += adsr[0][p*4+2+1]+1;
		}
		return true;
	}
	return false;
}
void FMPlusPageOperator::getframe() {
	mesh->lineht[1] = mesh->framedata[0];
	mesh->lineht[2] = mesh->framedata[1];
	if(mesh->p == 1) {
		     if(mesh->lineht[1] <= 0)
			mesh->lineht[1] = -1;
		else if(mesh->lineht[1] <  1)
			mesh->lineht[1] *=                       (adsr[0][ 0]+.5f)                   /120.f;
		else if(mesh->lineht[1] <  2)
			mesh->lineht[1] =   ((mesh->lineht[1]-1)*(adsr[0][ 4]+1.f)+(adsr[0][ 0]+.5f))/120.f;
		else if(mesh->lineht[1] <= 3)
			mesh->lineht[1] = 1-((3-mesh->lineht[1])*(adsr[0][ 8]+1.f)+(adsr[0][12]-.5f))/120.f;
		else if(mesh->lineht[1] <  4)
			mesh->lineht[1] = 1- (4-mesh->lineht[1])*(adsr[0][12]+.5f)                   /120.f;
		else
			mesh->lineht[1] = -1;
	}
}
