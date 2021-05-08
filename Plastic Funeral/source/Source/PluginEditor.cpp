/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

//==============================================================================
FmerAudioProcessorEditor::FmerAudioProcessorEditor (FmerAudioProcessor& p)
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	auto baseimg = ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize);
	mBaseImg.setImage(baseimg);
	addAndMakeVisible(mBaseImg);

	addAndMakeVisible(mVisualizer);

	mFreqKnob.setSliderStyle(Slider::RotaryVerticalDrag);
	mFreqKnob.setRange(0.0f, 1.0f);
	mFreqKnob.setTextBoxStyle(Slider::NoTextBox, true, 50, 20);
	mFreqKnob.setLookAndFeel(&darkKnobb);
	mFreqKnob.addListener(this);
	addAndMakeVisible(mFreqKnob);
	mFreqAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "freq", mFreqKnob);

	mFatKnob.setSliderStyle(Slider::RotaryVerticalDrag);
	mFatKnob.setRange(-20.0f, 20.0f);
	mFatKnob.setTextBoxStyle(Slider::NoTextBox, true, 50, 20);
	mFatKnob.setLookAndFeel(&darkKnobb);
	mFatKnob.addListener(this);
	addAndMakeVisible(mFatKnob);
	mFatAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "fat", mFatKnob);

	mDriveKnob.setSliderStyle(Slider::RotaryVerticalDrag);
	mDriveKnob.setRange(0.0f, 1.0f);
	mDriveKnob.setTextBoxStyle(Slider::NoTextBox, true, 50, 20);
	mDriveKnob.setLookAndFeel(&brightKnobb);
	mDriveKnob.addListener(this);
	addAndMakeVisible(mDriveKnob);
	mDriveAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "drive", mDriveKnob);

	mDryKnob.setSliderStyle(Slider::RotaryVerticalDrag);
	mDryKnob.setRange(0.0f, 1.0f);
	mDryKnob.setTextBoxStyle(Slider::NoTextBox, true, 50, 20);
	mDryKnob.setLookAndFeel(&brightKnobb);
	mDryKnob.addListener(this);
	addAndMakeVisible(mDryKnob);
	mDryAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "dry", mDryKnob);

	mStereoKnob.setSliderStyle(Slider::RotaryVerticalDrag);
	mStereoKnob.setRange(0.0f, 1.0f);
	mStereoKnob.setTextBoxStyle(Slider::NoTextBox, true, 50, 20);
	mStereoKnob.setLookAndFeel(&brightKnobb);
	mStereoKnob.addListener(this);
	addAndMakeVisible(mStereoKnob);
	mStereoAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "stereo", mStereoKnob);

	mGainKnob.setSliderStyle(Slider::RotaryVerticalDrag);
	mGainKnob.setRange(0.0f, 1.0f);
	mGainKnob.setTextBoxStyle(Slider::NoTextBox, true, 50, 20);
	mGainKnob.setLookAndFeel(&brightKnobb);
	mGainKnob.addListener(this);
	addAndMakeVisible(mGainKnob);
	mGainAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "gain", mGainKnob);

	addAndMakeVisible(mCredits);

	setSize (242, 462);
}

FmerAudioProcessorEditor::~FmerAudioProcessorEditor()
{
}

//==============================================================================
void FmerAudioProcessorEditor::paint (Graphics& g)
{
	/*
	g.fillAll(juce::Colours::green);
	g.setColour(juce::Colours::blue);
	g.setFont(18.0f);
	g.drawSingleLineText(std::to_string(audioProcessor.norm), 70, 30, Justification::left);
	*/
}

void FmerAudioProcessorEditor::resized()
{
	audioProcessor.normalizegain();
	calcvis();
	mBaseImg.setBounds(0, 0, 242, 462);
	mVisualizer.setBounds(8,8,226,79);
	mFreqKnob.setBounds(26, 102, 84, 84);
	mFatKnob.setBounds(132, 102, 84, 84);
	mDriveKnob.setBounds(26, 202, 84, 84);
	mDryKnob.setBounds(132, 202, 84, 84);
	mStereoKnob.setBounds(26, 302, 84, 84);
	mGainKnob.setBounds(132, 302, 84, 84);
	mCredits.setBounds(0,403,242,59);
}

void FmerAudioProcessorEditor::sliderValueChanged(Slider* slider)
{
	if(slider == &mFreqKnob)
		audioProcessor.freq = mFreqKnob.getValue();
	else if(slider == &mFatKnob)
		audioProcessor.fat = mFatKnob.getValue();
	else if(slider == &mDriveKnob)
		audioProcessor.drive = mDriveKnob.getValue();
	else if(slider == &mDryKnob)
		audioProcessor.dry = mDryKnob.getValue();
	else if(slider == &mStereoKnob)
		audioProcessor.stereo = mStereoKnob.getValue();
	else if(slider == &mGainKnob)
		audioProcessor.gain = mGainKnob.getValue();
	audioProcessor.normalizegain();
	calcvis();
}
void FmerAudioProcessorEditor::sliderDragStarted(Slider* slider)
{
	audioProcessor.undoManager.beginNewTransaction();
	knobdiff = slider->getValue();
}
void FmerAudioProcessorEditor::sliderDragEnded(Slider* slider)
{
	audioProcessor.undoManager.setCurrentTransactionName(
		(String)((slider->getValue() - knobdiff) >= 0 ? "Increased " : "Decreased ") += slider->getName().toLowerCase());
	audioProcessor.undoManager.beginNewTransaction();
}


void FmerAudioProcessorEditor::calcvis() {
	mVisualizer.isStereo = audioProcessor.stereo > 0 && audioProcessor.dry < 1 && audioProcessor.gain > 0;
	for (int i = 0; i < 226; i++) {
		mVisualizer.visline[0][i] = 40 + audioProcessor.plasticfuneral(sin(i / 35.8098621957f) * .8f, 0, audioProcessor.freq, audioProcessor.fat, audioProcessor.drive, audioProcessor.dry, audioProcessor.stereo, audioProcessor.gain) * audioProcessor.norm * 38;
		if(mVisualizer.isStereo)
			mVisualizer.visline[1][i] = 40 + audioProcessor.plasticfuneral(sin(i / 35.8098621957f) * .8f, 1, audioProcessor.freq, audioProcessor.fat, audioProcessor.drive, audioProcessor.dry, audioProcessor.stereo, audioProcessor.gain) * audioProcessor.norm * 38;
	}
	mVisualizer.repaint();
}
