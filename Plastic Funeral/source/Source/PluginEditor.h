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
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalchover(float x, float y);

	knob knobs[6];
	float visline[2][452];
	bool isStereo = false;
private:
	PFAudioProcessor& audioProcessor;

	int hover = -1;
	int initialdrag = 0;
	int held = 0;
	float initialvalue = 0;
	int needtoupdate = 2;

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

	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos;
	OpenGLTexture knobtex;
	std::unique_ptr<OpenGLShaderProgram> knobshader;
	String knobvert;
	String knobfrag;

	std::unique_ptr<OpenGLShaderProgram> visshader;
	String visvert;
	String visfrag;

	float oversamplingalpha = 0;
	float oversamplinglerped = 1;
	int oversampling = 1;
	std::unique_ptr<OpenGLShaderProgram> oversamplingshader;
	String oversamplingvert;
	String oversamplingfrag;

	float websiteht = -1;
	float creditsalpha = 0;
	OpenGLTexture creditstex;
	std::unique_ptr<OpenGLShaderProgram> creditsshader;
	String creditsvert;
	String creditsfrag;

	float rms = 0;
	OpenGLFrameBuffer framebuffer;
	std::unique_ptr<OpenGLShaderProgram> ppshader;
	String ppvert;
	String ppfrag;

	Random random;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PFAudioProcessorEditor)
};
