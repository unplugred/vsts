#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPageFilter : public FMPlusPage {
public:
	FMPlusPageFilter();
	~FMPlusPageFilter();

	void rebuildtab() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageFilter)
};
