#pragma once
#include "includes.h"
#include "PluginProcessor.h"
using namespace gl;

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
class PFAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	PFAudioProcessorEditor (PFAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~PFAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader);
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
	int knobcount = 0;
	float visline[2][2712];
	bool isStereo = false;
private:
	PFAudioProcessor& audioProcessor;

	OpenGLContext context;
	unsigned int arraybuffer;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};

	OpenGLTexture basetex;
	std::unique_ptr<OpenGLShaderProgram> baseshader;

	int hover = -1;
	int initialdrag = 0;
	int held = 0;
	float initialvalue = 0;
	int needtoupdate = 2;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	OpenGLTexture knobtex;
	std::unique_ptr<OpenGLShaderProgram> knobshader;

	std::unique_ptr<OpenGLShaderProgram> visshader;

	float oversamplingalpha = 0;
	float oversamplinglerped = 1;
	bool oversampling = true;
	std::unique_ptr<OpenGLShaderProgram> oversamplingshader;

	float websiteht = -1;
	float creditsalpha = 0;
	OpenGLTexture creditstex;
	std::unique_ptr<OpenGLShaderProgram> creditsshader;

	float rms = 0;
	OpenGLFrameBuffer framebuffer;
	std::unique_ptr<OpenGLShaderProgram> ppshader;

	Random random;

#ifdef BANNER
	float bannerx = 0;
	OpenGLTexture bannertex;
	std::unique_ptr<OpenGLShaderProgram> bannershader;
#endif
	float banneroffset = 0;

	float dpi = 1;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PFAudioProcessorEditor)
};
