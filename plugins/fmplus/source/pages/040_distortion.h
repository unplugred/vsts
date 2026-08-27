#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPageDistortion : public FMPlusPage {
public:
	FMPlusPageDistortion();
	~FMPlusPageDistortion();

	void rebuildtab() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageDistortion)
};
