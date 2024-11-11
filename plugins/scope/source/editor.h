#pragma once
#include "includes.h"
#include "functions.h"
#include "processor.h"
using namespace gl;

#define LINEWIDTH 0.01

struct knob {
	float value = .5f;
	int hoverstate = 0;
	String id;
	String name;
	float minimumvalue = 0.f;
	float maximumvalue = 1.f;
	float defaultvalue = 0.f;
	bool visible = true;
	bool isbool = false;
	float normalize(float val) {
		return (val-minimumvalue)/(maximumvalue-minimumvalue);
	}
	float inflate(float val) {
		return val*(maximumvalue-minimumvalue)+minimumvalue;
	}
};
class ScopeAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	ScopeAudioProcessorEditor(ScopeAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~ScopeAudioProcessorEditor() override;

	void resized() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void calcvis();
	void paint(Graphics&) override;

	void timerCallback() override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void mouseDown(const MouseEvent& event) override;

	void openwindow();

	functions::dampendvalue ampdamp;
	std::vector<float> line;
	std::vector<float> osci;
	bool oscimode[800];
	int channelnum = 0;
	int linew = 0;

	bool dontscale = true;

	knob knobs[10];
	int knobcount = 0;

private:
	ScopeAudioProcessor& audio_processor;

	std::shared_ptr<OpenGLShaderProgram> clearshader;

	OpenGLFrameBuffer framebuffer;
	std::shared_ptr<OpenGLShaderProgram> lineshader;

	OpenGLFrameBuffer downscalebuffer;
	std::shared_ptr<OpenGLShaderProgram> downscaleshader;

	OpenGLTexture noisetex;
	OpenGLTexture basetex;
	functions::dampendvalue griddamp;
	functions::dampendvalue rdamp;
	functions::dampendvalue gdamp;
	functions::dampendvalue bdamp;
	Colour bgcol = Colour::fromHSV(0.5f,.85f,.6f,1.f);
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	bool positionwindow = false;
	bool firstframe = true;

	Random random;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopeAudioProcessorEditor)
};
