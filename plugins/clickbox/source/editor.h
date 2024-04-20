#pragma once
#include "includes.h"
#include "processor.h"
using namespace gl;

class LookNFeel : public plugmachine_look_n_feel {
public:
	LookNFeel();
	~LookNFeel();
	Font getPopupMenuFont();
	int getMenuWindowFlags() override;
	void drawPopupMenuBackground(Graphics &g, int width, int height) override;
	void drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) override; //biggest function ive seen ever
	void getIdealPopupMenuItemSize(const String& text, const bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;
	int getPopupMenuBorderSize() override;
	Colour bg = Colour::fromFloatRGBA(.10546875f,.10546875f,.10546875f,1.f);
	Colour fg = Colour::fromFloatRGBA(.23828125f,.23828125f,.23828125f,1.f);
	String font = "n";
	float colors[18]{
		1.f,0.f,.5625f,
		.4335938f,0.f,1.f,
		0.f,.5664063f,1.f,
		0.f,1.f,.4296875f,
		.5664063f,1.f,0.f,
		1.f,.4296875f,0.f
	};
};

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
class ClickBoxAudioProcessorEditor : public plugmachine_gui {
public:
	ClickBoxAudioProcessorEditor(ClickBoxAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~ClickBoxAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint(Graphics&) override;

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
	int recalc_hover(float x, float y);

	float getr(float hue);
	float getg(float hue);
	float getb(float hue);

private:
	ClickBoxAudioProcessor& audio_processor;

	float colors[18]{
		1.f,0.f,.5625f,
		.4335938f,0.f,1.f,
		0.f,.5664063f,1.f,
		0.f,1.f,.4296875f,
		.5664063f,1.f,0.f,
		1.f,.4296875f,0.f
	};

	int hover = -1;
	int initialdrag = 0;
	float initialvalue = 0;

	std::shared_ptr<OpenGLShaderProgram> clearshader;

	int slidercount = 0;
	slider sliders[6];
	
	Point<int> dragpos;
	OpenGLTexture slidertex;
	std::shared_ptr<OpenGLShaderProgram> slidershader;

	bool trailactive = false;
	int mousecolor = 0;
	bool overridee = false;
	mousepos prevpos[7];
	OpenGLTexture cursortex;
	std::shared_ptr<OpenGLShaderProgram> cursorshader;

	float websiteht = -1;
	bool credits = false;
	float shadertime = 0;
	OpenGLTexture creditstex;
	std::shared_ptr<OpenGLShaderProgram> creditsshader;

	float randoms[8];
	float randomsblend = 0;
	float ppamount = 0;
	OpenGLFrameBuffer framebuffer;
	std::shared_ptr<OpenGLShaderProgram> ppshader;

	Random random;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClickBoxAudioProcessorEditor)
};
