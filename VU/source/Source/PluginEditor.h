#pragma once
#include "includes.h"
#include "PluginProcessor.h"
#include "functions.h"
using namespace gl;

struct knob {
	float value = .5f;
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
class VuAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	VuAudioProcessorEditor(VuAudioProcessor&, int paramcount, pluginpreset state, potentiometer pots[]);
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

	int bypassdetection = 0;

	void timerCallback() override;

	int prevh = 0;
	int prevw = 0;

	knob knobs[3];
	int knobcount = 3;
private:
	VuAudioProcessor& audioProcessor;

	OpenGLContext context;
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

#ifdef BANNER
	float bannerx = 0;
	OpenGLTexture bannertex;
	std::unique_ptr<OpenGLShaderProgram> bannershader;
#endif
	float banneroffset = 0;

	float dpi = 1;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VuAudioProcessorEditor)
};
