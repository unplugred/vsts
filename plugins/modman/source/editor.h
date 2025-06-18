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
	Colour bg1 = Colour::fromFloatRGBA(.839f,.839f,.859f,1.f);
	Colour bg2 = Colour::fromFloatRGBA(.659f,.655f,.612f,1.f);
	Colour fg = Colour::fromFloatRGBA(.231f,.243f,.22f,1.f);
	String font = "n";
};
struct knob {
	float x = 0;
	float y = 0;
	float centerx = .5f;
	float centery = .5f;
	float value[MC];
	float defaultvalue[MC];
	String id;
	String name;
	float minimumvalue = 0.f;
	float maximumvalue = 1.f;
	float normalize(float val) {
		return (val-minimumvalue)/(maximumvalue-minimumvalue);
	}
	float inflate(float val) {
		return val*(maximumvalue-minimumvalue)+minimumvalue;
	}
};
struct flower {
	float x = 0;
	float y = 0;
	float s = 0;
	float lx1 = 0;
	float ly1 = 0;
	float lx2 = 0;
	float ly2 = 0;
	float rot = 0;
};
class ModManAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	ModManAudioProcessorEditor(ModManAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~ModManAudioProcessorEditor() override;

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

	knob knobs[6];
	int knobcount = 0;
private:
	ModManAudioProcessor& audio_processor;

	OpenGLTexture basetex;
	OpenGLTexture bannertex;
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	int selectedmodulator = 0;
	int hover = -1;
	int initialdrag = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	OpenGLTexture knobtex;
	std::shared_ptr<OpenGLShaderProgram> knobshader;

	OpenGLTexture flowerstex;
	OpenGLTexture labelstex;
	flower flowers[MC];
	int framecount = 0;
	std::shared_ptr<OpenGLShaderProgram> flowersshader;

	OpenGLTexture tackstex;
	OpenGLTexture numberstex;
	float tackpos[2] = {0,0};
	std::shared_ptr<OpenGLShaderProgram> tackshader;

	OpenGLTexture cubertex;
	float cuberindex = 0;
	float cuberpos[2] = {0,0};
	float cuberposclamp[2] = {0,0};
	float cubersize = 1;
	std::shared_ptr<OpenGLShaderProgram> cubershader;

	OpenGLTexture onofftex;
	int offnum = 0;
	int onnum = 0;
	std::shared_ptr<OpenGLShaderProgram> onoffshader;

	OpenGLTexture logotex;
	OpenGLTexture logoalphatex;
	float logolerp = 0;
	float logoease = 0;
	float websiteht = 0;
	float originpos[3] = {0,0,0};
	float targetpos[3] = {0,0,0};
	std::shared_ptr<OpenGLShaderProgram> logoshader;


	float rms = 0;
	float time = 0;

	Random random;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModManAudioProcessorEditor)
};
