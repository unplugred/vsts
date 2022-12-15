#pragma once
#include "includes.h"
#include "PluginProcessor.h"
using namespace gl;

struct knob {
	int x = 0;
	int y = 0;
	float value = 0.5f;
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
class RedBassAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	RedBassAudioProcessorEditor (RedBassAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~RedBassAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader);
	void renderOpenGL() override;
	void openGLContextClosing() override;
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

	knob knobs[8];
	int knobcount = 0;
private:
	RedBassAudioProcessor& audioProcessor;

	OpenGLContext context;
	unsigned int arraybuffer;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};

	float vis = 0;
	OpenGLFrameBuffer framebuffer;
	std::unique_ptr<OpenGLShaderProgram> feedbackshader;

	float credits = 0;
	OpenGLTexture basetex;
	std::unique_ptr<OpenGLShaderProgram> baseshader;

	int hover = -1;
	int initialdrag = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	std::unique_ptr<OpenGLShaderProgram> knobshader;

	std::unique_ptr<OpenGLShaderProgram> toggleshader;

	OpenGLTexture texttex;
	std::unique_ptr<OpenGLShaderProgram> textshader;

	int freqfreq = 20;
	int lowpfreq = 150;

#ifdef BANNER
	float bannerx = 0;
	OpenGLTexture bannertex;
	std::unique_ptr<OpenGLShaderProgram> bannershader;
#endif
	float banneroffset = 0;

	float dpi = 1;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RedBassAudioProcessorEditor)
};
