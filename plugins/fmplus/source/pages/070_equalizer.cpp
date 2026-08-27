#include "070_equalizer.h"

FMPlusPageEqualizer::FMPlusPageEqualizer() : FMPlusPage() {
	tabname = "EQ";
	params.resize(1);
	params[ 0] = potentiometer("Equalizer On"	,"equalizeron"	,"EQUALIZER"	,.001f	,0	,1	,potentiometer::booltype	);
}
FMPlusPageEqualizer::~FMPlusPageEqualizer() { }
void FMPlusPageEqualizer::rebuildtab() {
	mesh->addtext(42,124,"To be added...");
	mesh->boxes[mesh->boxnum++] = box( 0, 31,  2,  8,  8,3);
}
