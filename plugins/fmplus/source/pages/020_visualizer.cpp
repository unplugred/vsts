#include "020_visualizer.h"

FMPlusPageVisualizer::FMPlusPageVisualizer() : FMPlusPage() {
	tabname = "Vis";
	params.resize(0);

	dividerx[0] = .3033333f;
	dividery[0] = .25f;
	dividerx[1] = .3033333f;
	dividery[1] = .5f;
	dividerx[2] = .3033333f;
	dividery[2] = .75f;
	dividerx[3] = .3033333f;
}
FMPlusPageVisualizer::~FMPlusPageVisualizer() { }
void FMPlusPageVisualizer::rebuildtab() {
	mesh->addtext(42,124,"To be added...");
}
