#pragma once
#include "includes.h"
#include "processor.h"
#include "perlin.h"
#include "functions.h"
using namespace gl;

class LookNFeel : public plugmachine_look_n_feel {
public:
	LookNFeel();
	~LookNFeel();
	Font getPopupMenuFont();
	void drawPopupMenuBackground(Graphics &g, int width, int height) override;
	void drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) override; //biggest function ive seen ever
	void getIdealPopupMenuItemSize(const String& text, const bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;
	int getPopupMenuBorderSize() override;
	Colour bg1 = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	Colour fg1 = Colour::fromFloatRGBA(1.f,1.f,0.f,1.f);
	Colour bg2 = Colour::fromFloatRGBA(0.f,0.f,1.f,1.f);
	Colour fg2 = Colour::fromFloatRGBA(1.f,1.f,1.f,1.f);
	String font = "n";
};
struct knob {
	int x[2] {0,0};
	int y[8] {0,0,0,0,0,0,0,0};
	float a[2] {0,0};
	float value = .5f;
	float lerpedvalue[3] {0,0,0};
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
class SucroseAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	SucroseAudioProcessorEditor(SucroseAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~SucroseAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void calcvis();
	void calcnext();
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

	knob knobs[7];
	int knobcount = 0;
	bool is_stereo = false;
private:
	SucroseAudioProcessor& audio_processor;

	OpenGLTexture bgtex;
	OpenGLTexture fgtex;
	float websiteht[2] {-1,-1};
	float logopos[6];
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	int hover = -1;
	int initialdrag = 0;
	int fresh = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);

	perlin noisegen;
	float lastdist[4] {0,0,0,0};
	int writepos = 0;
	functions::dampendvalue angledamp;
	float scribble[4*SPEED*2];
	float moveup[4] {8,4,12,24};
	float visline[(140+SPEED*2*4)*4];
	bool knobswitch[7] {false,false,false,false,false,false,false};
	int linelength = -1;
	float lineprevx = 0;
	float lineprevy = 020;
	float linecurrentx = 0;
	float linecurrenty = 0;
	int linebegun = 0;
	float linedist = 0;
	int linechannel = 0;
	OpenGLTexture linetex;
	std::shared_ptr<OpenGLShaderProgram> lineshader;
	void beginline(int channel, float dist);
	void endline();
	void nextpoint(float x, float y);

	OpenGLFrameBuffer frame_buffer;
	float offset[2] {.005f,.005f};
	Random random;
	std::shared_ptr<OpenGLShaderProgram> ppshader;

	float time = 0;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SucroseAudioProcessorEditor)
};
