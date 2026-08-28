#include "080_compressor.h"

FMPlusPageCompressor::FMPlusPageCompressor() : FMPlusPage() {
	tabname = "Comp";
	params.resize(1);
	params[ 0] = potentiometer("Compressor On"	,"compressoron"	,"COMPRESSOR"	,.001f	,0	,1	,potentiometer::booltype	);
}
FMPlusPageCompressor::~FMPlusPageCompressor() { }
void FMPlusPageCompressor::rebuildtab() {
	mesh->addtext(42,124,"To be added...");
	mesh->boxes[mesh->boxnum++] = box( 0, 31,  2,  8,  8,3);
}
