#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPageDelay : public FMPlusPage {
public:
	FMPlusPageDelay();
	~FMPlusPageDelay();

	void rebuildtab() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageDelay)
};
