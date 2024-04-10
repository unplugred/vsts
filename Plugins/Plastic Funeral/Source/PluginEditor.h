#pragma once
#include "includes.h"
#include "PluginProcessor.h"
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
	Colour bg = Colour::fromFloatRGBA(.1f,.1f,.1f,1.f);
	Colour fg = Colour::fromFloatRGBA(.7f,.7f,.7f,1.f);
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
class PFAudioProcessorEditor : public plugmachine_gui {
public:
	PFAudioProcessorEditor(PFAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~PFAudioProcessorEditor() override;

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

	knob knobs[6];
	int knobcount = 0;
	float visline[2][2712];
	bool is_stereo = false;
private:
	PFAudioProcessor& audio_processor;

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
	OpenGLTexture knobtex;
	std::shared_ptr<OpenGLShaderProgram> knobshader;

	std::shared_ptr<OpenGLShaderProgram> visshader;

	float oversamplingalpha = 0;
	float oversamplinglerped = 1;
	bool oversampling = true;
	std::shared_ptr<OpenGLShaderProgram> oversamplingshader;

	float websiteht = -1;
	float creditsalpha = 0;
	OpenGLTexture creditstex;
	std::shared_ptr<OpenGLShaderProgram> creditsshader;

	float rms = 0;
	OpenGLFrameBuffer framebuffer;
	std::shared_ptr<OpenGLShaderProgram> ppshader;

	Random random;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PFAudioProcessorEditor)
};
