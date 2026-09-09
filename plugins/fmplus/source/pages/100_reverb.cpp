#include "100_reverb.h"

FMPlusPageReverb::FMPlusPageReverb() : FMPlusPage() {
	tabname = "Rvrb";
	params.resize(1);
	params[ 0] = potentiometer("Reverb On"	,"reverbon"	,"REVERB"	,.001f	,0	,1	,potentiometer::booltype	);
}
FMPlusPageReverb::~FMPlusPageReverb() { }
void FMPlusPageReverb::rebuildtab() {
	mesh->addtext(42,124,"To be added...");
	mesh->boxes[mesh->boxnum++] = box( 0, 31,  2,  8,  8,3);
}
