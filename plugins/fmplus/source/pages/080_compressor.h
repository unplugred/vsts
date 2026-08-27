#pragma once
#include "../includes.h"
#include "../page.h"
using namespace gl;

class FMPlusPageCompressor : public FMPlusPage {
public:
	FMPlusPageCompressor();
	~FMPlusPageCompressor();

	void rebuildtab() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageCompressor)
};
