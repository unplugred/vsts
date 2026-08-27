#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPageVisualizer : public FMPlusPage {
public:
	FMPlusPageVisualizer();
	~FMPlusPageVisualizer();

	void rebuildtab() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageVisualizer)
};
