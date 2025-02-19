#pragma once
#include "includes.h"
#include "processor.h"
using namespace gl;

#define LINEWIDTH .008
#define BUFFERLENGTH 1200
#define SPEED 1.5

class ui_font : public cool_font {
public:
	ui_font() {
		image = ImageCache::getFromMemory(BinaryData::text_png,BinaryData::text_pngSize);
		texture_width = 128;
		texture_height = 256;
		line_height = 16;
		in_frame_buffer = true;
		mono = true;
		mono_width = 8;
		mono_height = 16;
	};
};

class LookNFeel : public plugmachine_look_n_feel {
public:
	LookNFeel();
	~LookNFeel();
	Font getPopupMenuFont();
	void drawPopupMenuBackground(Graphics &g, int width, int height) override;
	void drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) override; //biggest function ive seen ever
	void getIdealPopupMenuItemSize(const String& text, const bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;
	int getPopupMenuBorderSize() override;
	Colour bg = Colour::fromFloatRGBA(0.99609375f,0.28125f,0.6875f,1.f);
	Colour fg = Colour::fromFloatRGBA(0.99609375f,0.98046875f,0.98828125f,1.f);
	String font = "n";
};
struct knob {
	int x = 0;
	int y = 0;
	float value = .5f;
	float lerpedvalue = .5f;
	float slowlerpedvalue = .5f;
	float indicator = 1.0f;
	float lerpedindicator = 1.0f;
	String id;
	String name;
	String str;
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
class MagicCarpetAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	MagicCarpetAudioProcessorEditor(MagicCarpetAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~MagicCarpetAudioProcessorEditor() override;

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
private:
	MagicCarpetAudioProcessor& audio_processor;

	bool noise = false;
	std::shared_ptr<OpenGLShaderProgram> roundedsquareshader;

	OpenGLTexture carpettex;
	std::shared_ptr<OpenGLShaderProgram> carpetshader;

	int hover = -1;
	int initialdrag = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);

	ui_font font = ui_font();
	int indicatortimer[DLINES];
	std::shared_ptr<OpenGLShaderProgram> knobshader;

	std::shared_ptr<OpenGLShaderProgram> centershader;

	float rms = 0;
	float rmslerped = 0;
	float timex = 0;
	float timey = 0;
	float linedata[BUFFERLENGTH*2];
	float line[BUFFERLENGTH*6];
	int lineindex = 0;
	std::shared_ptr<OpenGLShaderProgram> lineshader;

	float logoprog = 0;
	float logopos[6];
	OpenGLTexture logotex;
	Random random;
	std::shared_ptr<OpenGLShaderProgram> logoshader;

	OpenGLFrameBuffer frame_buffer;
	OpenGLTexture noisetex;
	std::shared_ptr<OpenGLShaderProgram> ppshader;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MagicCarpetAudioProcessorEditor)
};
