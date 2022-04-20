/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct knob {
	int x = 0;
	int y = 0;
	float value = .5f;
	float lerpedvalue = .5f;
	float hover = 0.f;
	String id;
	String name;
	float minimumvalue = 0.f;
	float maximumvalue = 1.f;
	float defaultvalue = 0.f;
	float normalize(float val) {
		return (val-minimumvalue)/(maximumvalue-minimumvalue);
	}
	float inflate(float val) {
		return val*(maximumvalue-minimumvalue)+minimumvalue;
	}
};
struct bubble {
	float wiggleamount = 0;
	float wigglespeed = 0;
	float wiggleage = 0;
	float moveamount = 0;
	float movespeed = 0;
	float moveoffset = 0;
	float moveage = 0;
	float yspeed = 0;
	float xoffset = 0;
};
class PisstortionAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	PisstortionAudioProcessorEditor (PisstortionAudioProcessor&, int paramcount, pluginpreset state, potentiometer pots[]);
	~PisstortionAudioProcessorEditor() override;

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

	void bubbleregen(int i);

	knob knobs[6];
	int knobcount = 0;
	float visline[2][452];
	bool isStereo = false;
private:
	PisstortionAudioProcessor& audioProcessor;

	int hover = -1;
	int initialdrag = 0;
	int held = 0;
	float initialvalue = 0;

	OpenGLContext context;
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
	Point<int> dragpos = Point<int>(0,0);
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
	float rmslerped = 0;
	OpenGLFrameBuffer framebuffer;
	std::unique_ptr<OpenGLShaderProgram> circleshader;
	String circlevert;
	String circlefrag;

	bubble bubbles[20];
	Random random;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PisstortionAudioProcessorEditor)
};
