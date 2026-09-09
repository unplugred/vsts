#include "030_filter.h"

FMPlusPageFilter::FMPlusPageFilter() : FMPlusPage() {
	tabname = "Fltr";
	params.resize(1);
	params[ 0] = potentiometer("Filter On"	,"filteron"	,"FILTER"	,.001f	,0	,1	,potentiometer::booltype	);
}
FMPlusPageFilter::~FMPlusPageFilter() { }
void FMPlusPageFilter::rebuildtab() {
	mesh->addtext(42,124,"To be added...");
	mesh->boxes[mesh->boxnum++] = box( 0, 31,  2,  8,  8,3);
}
