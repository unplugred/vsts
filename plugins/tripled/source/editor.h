#pragma once
#include "includes.h"
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
	Colour bg1 = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	Colour fg1 = Colour::fromFloatRGBA(1.f,1.f,0.f,1.f);
	Colour bg2 = Colour::fromFloatRGBA(0.f,0.f,1.f,1.f);
	Colour fg2 = Colour::fromFloatRGBA(1.f,1.f,1.f,1.f);
	String font = "n";
};
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
class TripleDAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	TripleDAudioProcessorEditor(TripleDAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~TripleDAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void calcvis();
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

	knob knobs[2+DLINES];
	int knobcount = 0;
	float visline[2][452];
	bool is_stereo = false;
private:
	TripleDAudioProcessor& audio_processor;

	OpenGLTexture basetex;
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	int hover = -1;
	int initialdrag = 0;
	int held = 0;
	float initialvalue = 0;
	int needtoupdate = 2;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	std::shared_ptr<OpenGLShaderProgram> knobshader;

	std::shared_ptr<OpenGLShaderProgram> blackshader;

	std::shared_ptr<OpenGLShaderProgram> visshader;

	float websiteht = -1;
	float creditsalpha = 0;
	OpenGLTexture creditstex;
	std::shared_ptr<OpenGLShaderProgram> creditsshader;

	float rms = 0;
	float time = 0;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TripleDAudioProcessorEditor)
};
