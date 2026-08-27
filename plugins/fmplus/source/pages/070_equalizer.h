#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPageEqualizer : public FMPlusPage {
public:
	FMPlusPageEqualizer();
	~FMPlusPageEqualizer();

	void rebuildtab() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageEqualizer)
};
