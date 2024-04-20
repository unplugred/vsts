#pragma once
#include "includes.h"
#include "functions.h"
#include "processor.h"
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
	Colour bg = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	Colour fg = Colour::fromFloatRGBA(1.f,1.f,1.f,1.f);
	String font = "n";
};
struct knob {
	float x = 0;
	float y = 0;
	float radius = 0;
	float linewidth = .1;
	float lineheight = .63;
	float r = 0;
	float yoffset = 0;

	int index = -1;
	String id;
	String name;
	String description;

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
class CRMBLAudioProcessorEditor : public plugmachine_gui {
public:
	CRMBLAudioProcessorEditor(CRMBLAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~CRMBLAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint(Graphics&) override;

	void timerCallback() override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void recalclabels();
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalc_hover(float x, float y);

	knob knobs[11];
	int knobcount = 11;
private:
	CRMBLAudioProcessor& audio_processor;

	float time = 0;
	int sync = 0;

	OpenGLTexture basetex;
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	float websiteht = -1;
	std::shared_ptr<OpenGLShaderProgram> logoshader;

	int hover = -1;
	int initialdrag = 0;
	int held = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	std::shared_ptr<OpenGLShaderProgram> knobshader;

	int pitchnum[4]{1,0,0,0};
	int timenum[6]{1,0,0,0,0,0};
	OpenGLTexture numbertex;
	std::shared_ptr<OpenGLShaderProgram> numbershader;

	bool oversampling = true;
	bool postfb = true;
	bool hold = false;

	float rms = 0;
	functions::inertiadampened rmsdamp;
	float damparray[32];
	int dampreadpos = 0;

	OpenGLFrameBuffer feedbackbuffer;
	std::shared_ptr<OpenGLShaderProgram> feedbackshader;

	OpenGLFrameBuffer mainbuffer;
	std::shared_ptr<OpenGLShaderProgram> buffershader;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CRMBLAudioProcessorEditor)
};
