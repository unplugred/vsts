#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct knob {
	float x = 0;
	float y = 0;
	float radius = 0;
	float linewidth = .1;
	float lineheight = .63;
	float r = 0;

	int index = -1;
	String id;
	String name;

	float value = .5f;
	int hoverstate = 0;
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
class CRMBLAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	CRMBLAudioProcessorEditor (CRMBLAudioProcessor&, int paramcount, pluginpreset state, potentiometer pots[]);
	~CRMBLAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint(Graphics&) override;

	void timerCallback() override;
	float getvis(float r);

	virtual void parameterChanged(const String& parameterID, float newValue);
	void recalclabels();
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalchover(float x, float y);

	knob knobs[11];
	int knobcount = 11;
private:
	CRMBLAudioProcessor& audioProcessor;

	float time = 0;
	int sync = 0;

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

	float websiteht = -1;
	std::unique_ptr<OpenGLShaderProgram> logoshader;
	String logovert;
	String logofrag;

	int hover = -1;
	int initialdrag = 0;
	int held = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	std::unique_ptr<OpenGLShaderProgram> knobshader;
	String knobvert;
	String knobfrag;

	int pitchnum[4]{1,0,0,0};
	int timenum[6]{1,0,0,0,0,0};
	OpenGLTexture numbertex;
	std::unique_ptr<OpenGLShaderProgram> numbershader;
	String numbervert;
	String numberfrag;

	bool oversampling = true;
	bool postfb = true;
	bool hold = false;

	float rms = 0;
	functions::inertiadampened rmsdamp;
	float damparray[32];
	int dampreadpos = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CRMBLAudioProcessorEditor)
};
