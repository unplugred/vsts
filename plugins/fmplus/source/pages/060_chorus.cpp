#include "060_chorus.h"

FMPlusPageChorus::FMPlusPageChorus() : FMPlusPage() {
	tabname = "Chrs";
	params.resize(1);
	params[ 0] = potentiometer("Chorus On"	,"choruson"	,"CHORUS"	,.001f	,0	,1	,potentiometer::booltype	);
}
FMPlusPageChorus::~FMPlusPageChorus() { }
void FMPlusPageChorus::rebuildtab() {
	mesh->addtext(42,124,"To be added...");
	mesh->boxes[mesh->boxnum++] = box( 0, 31,  2,  8,  8,3);
}
