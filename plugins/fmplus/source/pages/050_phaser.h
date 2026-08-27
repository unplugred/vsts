#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPagePhaser : public FMPlusPage {
public:
	FMPlusPagePhaser();
	~FMPlusPagePhaser();

	void rebuildtab() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPagePhaser)
};
