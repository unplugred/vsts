#include "090_delay.h"

FMPlusPageDelay::FMPlusPageDelay() : FMPlusPage() {
	tabname = "Delay";
	params.resize(1);
	params[ 0] = potentiometer("Delay On"	,"delayon"	,"DELAY"	,.001f	,0	,1	,potentiometer::booltype	);
}
FMPlusPageDelay::~FMPlusPageDelay() { }
void FMPlusPageDelay::rebuildtab() {
	mesh->addtext(42,124,"To be added...");
	mesh->boxes[mesh->boxnum++] = box( 0, 31,  2,  8,  8,3);
}
