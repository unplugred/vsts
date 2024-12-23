#pragma once
#include "includes.h"
#include "processor.h"
#include "functions.h"
using namespace gl;

struct knob {
	float value = .5f;
	String id;
	String name;
	String svalue = "";
	float minimumvalue = 0.f;
	float maximumvalue = 1.f;
	float defaultvalue = 0.f;
	bool isbool = false;
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

	void mouseDown(const MouseEvent& event) override;
	int recalc_hover(float x, float y);
	void openwindow();

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
	float stereovelocity = 0;

	int bypassdetection = 0;
	bool dontscale = true;

	knob knobs[3];
	int knobcount = 3;
private:
	VUAudioProcessor& audio_processor;

	std::shared_ptr<OpenGLShaderProgram> vushader;
	OpenGLTexture vutex;

	bool positionwindow = false;
	bool firstframe = true;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUAudioProcessorEditor)
};
