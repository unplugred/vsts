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
	Colour bg = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	Colour fg = Colour::fromFloatRGBA(1.f,1.f,1.f,1.f);
	String font = "n";
};
struct knob {
	int x = 0;
	int y = 0;
	float value = 0.5f;
	String interpolatedvalue;
	String id;
	String name;
	String description;
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
class RedBassAudioProcessorEditor : public plugmachine_gui {
public:
	RedBassAudioProcessorEditor(RedBassAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~RedBassAudioProcessorEditor() override;

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

	knob knobs[8];
	int knobcount = 0;
private:
	RedBassAudioProcessor& audio_processor;

	float vis = 0;
	OpenGLFrameBuffer framebuffer;
	std::shared_ptr<OpenGLShaderProgram> feedbackshader;

	float credits = 0;
	OpenGLTexture basetex;
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	int hover = -1;
	int initialdrag = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	std::shared_ptr<OpenGLShaderProgram> knobshader;

	std::shared_ptr<OpenGLShaderProgram> toggleshader;

	OpenGLTexture texttex;
	char textindex[21] = { '0','1','2','3','4','5','6','7','8','9','k','H','z','O','f','f','m','s','d','B','-' };
	std::shared_ptr<OpenGLShaderProgram> textshader;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RedBassAudioProcessorEditor)
};
