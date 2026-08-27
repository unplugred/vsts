#include "050_phaser.h"

FMPlusPagePhaser::FMPlusPagePhaser() : FMPlusPage() {
	tabname = "Phaser";
	params.resize(1);
	params[ 0] = potentiometer("Phaser On"	,"phaseron"	,"PHASER"	,.001f	,0	,1	,potentiometer::booltype	);
}
FMPlusPagePhaser::~FMPlusPagePhaser() { }
void FMPlusPagePhaser::rebuildtab() {
	mesh->addtext(42,124,"To be added...");
	mesh->boxes[mesh->boxnum++] = box( 0, 31,  2,  8,  8,3);
}
