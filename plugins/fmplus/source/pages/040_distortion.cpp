#include "040_distortion.h"

FMPlusPageDistortion::FMPlusPageDistortion() : FMPlusPage() {
	tabname = "Dist";
	params.resize(1);
	params[ 0] = potentiometer("Distortion On"	,"distortionon"	,"DISTORTION"	,.001f	,0	,1	,potentiometer::booltype	);
}
FMPlusPageDistortion::~FMPlusPageDistortion() { }
void FMPlusPageDistortion::rebuildtab() {
	mesh->addtext(42,124,"To be added...");
	mesh->boxes[mesh->boxnum++] = box( 0, 31,  2,  8,  8,3);
}
