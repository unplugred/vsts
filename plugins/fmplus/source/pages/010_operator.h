#pragma once
#include "../includes.h"
#include "../page.h"

class FMPlusPageOperator : public FMPlusPage {
public:
	FMPlusPageOperator();
	~FMPlusPageOperator();

	void calcvis(int curveupdated) override;
	void rebuildtab() override;
	void addtext(int i) override;
	bool updatevalue(int param) override;
	void getframe() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusPageOperator)
};
