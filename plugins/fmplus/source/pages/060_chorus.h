#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPageChorus : public FMPlusPage {
public:
	FMPlusPageChorus();
	~FMPlusPageChorus();

	void rebuildtab() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageChorus)
};
