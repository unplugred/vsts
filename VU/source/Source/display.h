/*
  ==============================================================================

	display.h
	Created: 6 Jul 2021 3:41:33pm
	Author:	 unplugred

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "functions.h"
using namespace juce;

class displayComponent : public Component, public OpenGLRenderer
{
public:
	displayComponent();
	~displayComponent() override;
	void update();

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint(Graphics& g) override;

	void mouseEnter(const MouseEvent& event) override;
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
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

	bool stereo = false;
	int damping = 5;
	int nominal = -18;

	float stereodamp = 0;
	int settingstimer = 0;
	float settingsfade = 0;
	float settingsvelocity = 0;
	int hover = 0;
	int initialdrag = 0;
	bool held = false;
	int initialvalue = 0;
	float stereovelocity = 0;
	float websiteht = 0;

private:
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

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(displayComponent)
};
