#pragma once
#include "includes.h"
#include "processor.h"
#include "editor.h"
using namespace gl;

#define PADDING 2
#define MARGIN 2

class settingsfont : public cool_font {
public:
	settingsfont() {
		texture_width = 128;
		texture_height = 256;
		line_height = 16;
		vert =
R"(#version 150 core
in vec2 aPos;
uniform vec4 pos;
uniform vec4 texpos;
uniform float padding;
out vec2 uv;
out float filluv;
void main(){
	gl_Position = vec4((aPos*pos.zw+pos.xy)*2-1,0,1);
	gl_Position.y *= -1;
	uv = (aPos*texpos.zw+texpos.xy);
	uv.y = 1-uv.y;
	filluv = (aPos.x*pos.z+pos.x)/(1-padding*2)-padding;
})";
		frag =
R"(#version 150 core
in vec2 uv;
in float filluv;
uniform sampler2D tex;
uniform vec2 res;
uniform float dpi;
uniform float fill;
uniform float hover;
out vec4 fragColor;
void main(){
	vec2 nuv = uv;
	if(dpi > 1) {
		nuv *= res;
		if(mod(nuv.x,1)>.5) nuv.x = floor(nuv.x)+(1-min((1-mod(nuv.x,1))*dpi*2,.5));
		else nuv.x = floor(nuv.x)+min(mod(nuv.x,1)*dpi*2,.5);
		if(mod(nuv.y,1)>.5) nuv.y = floor(nuv.y)+(1-min((1-mod(nuv.y,1))*dpi*2,.5));
		else nuv.y = floor(nuv.y)+min(mod(nuv.y,1)*dpi*2,.5);
		nuv /= res;
	}
	float text = texture(tex,nuv).r;
	if(hover > .5) text += texture(tex,nuv-vec2(1/res.x,0)).r;
	fragColor = vec4(filluv<=fill?vec3(.388,.388,.612):vec3(1),text);
})";
		mono = true;
		mono_width = 8;
		mono_height = 16;
		is_scaled = true;
	};
};

class ScopeAudioProcessorSettings : public DocumentWindow, public plugmachine_gui {
public:
	ScopeAudioProcessorSettings(ScopeAudioProcessor&, int paramcount, knob*);
	~ScopeAudioProcessorSettings() override;
	void closeButtonPressed() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint(Graphics&) override;

	void timerCallback() override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalc_hover(float x, float y);

	virtual void moved() override;
	bool positioned = false;

	knob* knobs;
	int knobcount = 0;
private:
	ScopeAudioProcessor& audio_processor;

	OpenGLTexture tiletex;
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	int hover = -1;
	int initialdrag = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	std::shared_ptr<OpenGLShaderProgram> knobshader;

	settingsfont font;

	float websiteht = -1;
	OpenGLTexture creditstex;
	std::shared_ptr<OpenGLShaderProgram> creditsshader;

	float time;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopeAudioProcessorSettings)
};
