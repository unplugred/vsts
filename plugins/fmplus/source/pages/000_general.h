#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPageGeneral : public FMPlusPage {
public:
	FMPlusPageGeneral();
	~FMPlusPageGeneral();

	void rebuildtab() override;
	void addtext(int i) override;
	bool updatevalue(int param) override;

	String tuningfile = "";
	String themefile = "";
	String presetname = "";
	bool presetunsaved = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageGeneral)
};
