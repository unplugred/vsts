#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPageReverb : public FMPlusPage {
public:
	FMPlusPageReverb();
	~FMPlusPageReverb();

	void rebuildtab() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageReverb)
};
