/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "display.h"
using namespace juce;

//==============================================================================
/**
*/
class VuAudioProcessorEditor  : public AudioProcessorEditor, private Timer
{
public:
	VuAudioProcessorEditor (VuAudioProcessor&);
	~VuAudioProcessorEditor() override;

	//==============================================================================
	void paint (Graphics&) override;
	void resized() override;

	void timerCallback() override;

	int prevh = 0;
	int prevw = 0;
private:
	VuAudioProcessor& audioProcessor;
	displayComponent displaycomp;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VuAudioProcessorEditor)
};
