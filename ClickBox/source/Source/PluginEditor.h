#pragma once
#include "includes.h"
#include "PluginProcessor.h"
using namespace gl;

struct slider {
	float x=0,y=0,w=1,h=.13671875f;
	int hx=5,hy=5,hw=251,hh=30;
	String id = "";
	String name = "";
	float value = 0;
	bool isslider = true;
	float r=1,g=0,b=0;
	float coloffset=0;
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
struct mousepos {
	float x = -1000;
	float y = 0;
	bool automated = false;
	int col = 0;
};
class ClickBoxAudioProcessorEditor	: public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer {
public:
	ClickBoxAudioProcessorEditor (ClickBoxAudioProcessor&, int paramcount, pluginpreset state, potentiometer pots[]);
	~ClickBoxAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader);
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint (Graphics&) override;
	void resized() override;

	void timerCallback() override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void mouseMove(const MouseEvent& event) override;
	void mouseEnter(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalchover(float x, float y);

	float getr(float hue);
	float getg(float hue);
	float getb(float hue);

private:
	float colors[18]{
		1.f,0.f,.5625f,
		.4335938f,0.f,1.f,
		0.f,.5664063f,1.f,
		0.f,1.f,.4296875f,
		.5664063f,1.f,0.f,
		1.f,.4296875f,0.f
	};

	ClickBoxAudioProcessor& audioProcessor;

	int hover = -1;
	int initialdrag = 0;
	float initialvalue = 0;

	OpenGLContext context;
	unsigned int arraybuffer;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};

	std::unique_ptr<OpenGLShaderProgram> clearshader;

	int slidercount = 0;
	slider sliders[6];
	
	Point<int> dragpos;
	OpenGLTexture slidertex;
	std::unique_ptr<OpenGLShaderProgram> slidershader;

	bool trailactive = false;
	int mousecolor = 0;
	bool overridee = false;
	mousepos prevpos[7];
	OpenGLTexture cursortex;
	std::unique_ptr<OpenGLShaderProgram> cursorshader;

	float websiteht = -1;
	bool credits = false;
	float shadertime = 0;
	OpenGLTexture creditstex;
	std::unique_ptr<OpenGLShaderProgram> creditsshader;

	float randoms[8];
	float randomsblend = 0;
	float ppamount = 0;
	OpenGLFrameBuffer framebuffer;
	std::unique_ptr<OpenGLShaderProgram> ppshader;

	Random random;

	float dpi = 1;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClickBoxAudioProcessorEditor)
};
