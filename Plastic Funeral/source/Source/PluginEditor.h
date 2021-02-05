/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "components.h"
using namespace juce;


//==============================================================================
/**
*/
class FmerAudioProcessorEditor : public AudioProcessorEditor,
	public Slider::Listener
{
public:
	FmerAudioProcessorEditor (FmerAudioProcessor&);
	~FmerAudioProcessorEditor() override;

	//==============================================================================
	void paint (Graphics&) override;
	void resized() override;

	void sliderValueChanged(Slider* slider) override;
	void sliderDragStarted(Slider* slider) override;
	void sliderDragEnded(Slider* slider) override;
	void calcvis();

private:
	darkKnob darkKnobb;
	brightKnob brightKnobb;

	ImageComponent mBaseImg;
	VisualizerComponent mVisualizer;
	Slider mFreqKnob;
	Slider mFatKnob;
	Slider mDriveKnob;
	Slider mDryKnob;
	Slider mStereoKnob;
	Slider mGainKnob;
	CreditsComponent mCredits;

	//calculates whether the knob has increased or decreased in value
	//by the end of user drag for absolutely no reason
	float knobdiff;

	std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mFreqAttachment;
	std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mFatAttachment;
	std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mDriveAttachment;
	std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mDryAttachment;
	std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mStereoAttachment;
	std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mGainAttachment;

	FmerAudioProcessor& audioProcessor;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FmerAudioProcessorEditor)
};
