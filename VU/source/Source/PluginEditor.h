/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "functions.h"
using namespace juce;

class VuAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	VuAudioProcessorEditor (VuAudioProcessor&);
	~VuAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint (Graphics&) override;
	void resized() override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void mouseEnter(const MouseEvent& event) override;
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalchover(float x, float y);

	float leftvu = 0;
	float rightvu = 0;
	float leftrms = 0;
	float rightrms = 0;
	float leftvelocity = 0;
	float rightvelocity = 0;
	float multiplier = 1;

	bool leftpeak = false;
	bool rightpeak = false;
	float leftpeaklerp = 0;
	float rightpeaklerp = 0;
	float leftpeakvelocity = 0;
	float rightpeakvelocity = 0;
	int lefthold = 0;
	int righthold = 0;

	float stereodamp = 0;
	int settingstimer = 0;
	float settingsfade = 0;
	float settingsvelocity = 0;
	int hover = 0;
	int initialdrag = 0;
	bool held = false;
	int initialvalue = 0;
	float valueoffset = 0;
	float stereovelocity = 0;
	float websiteht = 0;
	Point<int> dragpos;

	void timerCallback() override;

	int prevh = 0;
	int prevw = 0;
private:
	VuAudioProcessor& audioProcessor;

	OpenGLContext openGLContext;
	unsigned int arraybuffer;
	OpenGLTexture vutex;
	OpenGLTexture mptex;
	OpenGLTexture lgtex;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};

	std::unique_ptr<OpenGLShaderProgram> vushader;
	String vertshader;
	String fragshader;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VuAudioProcessorEditor)
};
