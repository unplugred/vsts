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
	Colour c1 = Colour::fromFloatRGBA(.99608f,.99608f,.95294f,1.f);
	Colour c2 = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	String font = "n";
};
class PNCHAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	PNCHAudioProcessorEditor(PNCHAudioProcessor&, float amount);
	~PNCHAudioProcessorEditor() override;

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
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalc_hover(float x, float y);

	float amount = 0.f;
private:
	PNCHAudioProcessor& audio_processor;

	Colour c1 = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	Colour c2 = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);

	OpenGLTexture basetex;
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	int hover = -1;
	int initialdrag = 0;
	int held = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	std::shared_ptr<OpenGLShaderProgram> knobshader;

	bool credits = false;
	OpenGLTexture creditstex;
	std::shared_ptr<OpenGLShaderProgram> creditsshader;

	float rms = 0;
	Random random;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PNCHAudioProcessorEditor)
};
