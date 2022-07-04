#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct knob {
	int x = 0;
	int y = 0;
	float value = .5f;
	int hoverstate = 0;
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
class CRMBLAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	CRMBLAudioProcessorEditor (CRMBLAudioProcessor&, int paramcount, pluginpreset state, potentiometer pots[]);
	~CRMBLAudioProcessorEditor() override;

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

	knob knobs[13];
	int knobcount = 0;
	float visline[452];
private:
	CRMBLAudioProcessor& audioProcessor;

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

	std::unique_ptr<OpenGLShaderProgram> textshader;
	String textfrag;

	int hover = -1;
	int initialdrag = 0;
	int held = 0;
	float initialvalue = 0;
	int needtoupdate = 2;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	std::unique_ptr<OpenGLShaderProgram> knobshader;
	String knobvert;
	String knobfrag;

	std::unique_ptr<OpenGLShaderProgram> blackshader;
	String blackvert;
	String blackfrag;

	std::unique_ptr<OpenGLShaderProgram> visshader;
	String visvert;
	String visfrag;

	float oversamplingalpha = 0;
	float oversamplinglerped = 1;
	bool oversampling = true;
	OpenGLTexture oversamplingtex;
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
	float time = 0;

	Random random;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CRMBLAudioProcessorEditor)
};
