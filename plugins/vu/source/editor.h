#pragma once
#include "includes.h"
#include "processor.h"
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

class VUAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	VUAudioProcessorEditor(VUAudioProcessor&, int paramcount, pluginpreset state, potentiometer pots[]);
	~VUAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint(Graphics&) override;
	void resized() override;

	void timerCallback() override;
	virtual void parameterChanged(const String& parameterID, float newValue);

	void mouseEnter(const MouseEvent& event) override;
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalc_hover(float x, float y);

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
	bool dontscale = true;

	knob knobs[3];
	int knobcount = 3;
private:
	VUAudioProcessor& audio_processor;

	std::shared_ptr<OpenGLShaderProgram> vushader;
	OpenGLTexture vutex;
	OpenGLTexture mptex;
	OpenGLTexture lgtex;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUAudioProcessorEditor)
};
