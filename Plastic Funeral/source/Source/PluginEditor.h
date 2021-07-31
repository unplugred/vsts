/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
using namespace juce;


struct knob {
	int x = 0;
	int y = 0;
	float value = 0.5f;
	int hoverstate = 0;
	String id;
	String name;
};
class PFAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	PFAudioProcessorEditor (PFAudioProcessor&);
	~PFAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void calcvis();
	void paint (Graphics&) override;

	void timerCallback() override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void mouseEnter(const MouseEvent& event) override;
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	int recalchover(float x, float y);

	knob knobs[6];
	float visline[2][452];
	bool isStereo;
private:
	PFAudioProcessor& audioProcessor;

	int hover = 0;
	int initialdrag = 0;
	bool held = false;
	float initialvalue = 0;
	int websiteht = 0;
	float creditsalpha = 0;
	bool finemode = false;

	OpenGLContext openGLContext;
	unsigned int arraybuffer;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};

	OpenGLTexture basetex;
	std::unique_ptr<OpenGLShaderProgram> baseshader;
	String basevert;
	String basefrag;

	OpenGLTexture knobtex;
	std::unique_ptr<OpenGLShaderProgram> knobshader;
	String knobvert;
	String knobfrag;

	std::unique_ptr<OpenGLShaderProgram> visshader;
	String visvert;
	String visfrag;

	OpenGLTexture creditstex;
	std::unique_ptr<OpenGLShaderProgram> creditsshader;
	String creditsvert;
	String creditsfrag;

	std::unique_ptr<OpenGLShaderProgram> ppshader;
	String ppvert;
	String ppfrag;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PFAudioProcessorEditor)
};
