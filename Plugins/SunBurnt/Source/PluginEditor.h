#pragma once
#include "includes.h"
#include "PluginProcessor.h"
using namespace gl;

class LookNFeel : public LookAndFeel_V4 {
public:
	LookNFeel();
	~LookNFeel();
	Font getPopupMenuFont();
	int getMenuWindowFlags() override;
	void drawPopupMenuBackground(Graphics &g, int width, int height) override;
	void drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) override; //biggest function ive seen ever
	Colour bg = Colour::fromFloatRGBA(.15686f,.1451f,.13725f,1.f);
	Colour inactive = Colour::fromFloatRGBA(.76471f,.77647f,.77647f,1.f);
	Colour txt = Colour::fromFloatRGBA(.98824f,.98824f,.97647f,1.f);
	String CJKFont = "n";
	String ENGFont = "n";
	float scale = 1.5f;
};
struct knob {
	float value = .5f;
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
class SunBurntAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	SunBurntAudioProcessorEditor(SunBurntAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~SunBurntAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader);
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void calcvis();
	void paint (Graphics&) override;

	void timerCallback() override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalchover(float x, float y);

	knob knobs[6];
	curve curves[5];
	int knobcount = 0;
	float visline[3408];
private:
	SunBurntAudioProcessor& audioProcessor;

	OpenGLContext context;
	unsigned int arraybuffer;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};

	float websiteht = -1;
	OpenGLTexture baseentex;
	OpenGLTexture basejptex;
	OpenGLTexture disptex;
	bool jpmode = true;
	std::unique_ptr<OpenGLShaderProgram> baseshader;

	int curveselection = 0;
	std::unique_ptr<OpenGLShaderProgram> visshader;

	int hover = -1;
	int initialdrag = 0;
	float initialvalue[2] {0,0};
	float initialdotvalue[2] {0,0};
	float initialaxispoint[2] {0,0};
	float axisvaluediff[2] {0,0};
	float amioutofbounds[2] {0,0};

	float valueoffset[2] {0,0};
	bool finemode = false;
	int axislock = -1;
	Point<int> dragpos = Point<int>(0,0);
	std::unique_ptr<OpenGLShaderProgram> circleshader;

	OpenGLTexture overlaytex;
	OpenGLFrameBuffer framebuffer;
	std::unique_ptr<OpenGLShaderProgram> ppshader;

	float rms = 0;

	float time = 1000;
	Random random;
	char randomid[4] {0,0,0,0};
	float randomdir[8] {0,0,0,0,0,0,0,0};
	float randomblend[4] {0,0,0,0};
	int overlayorientation = 0;
	float overlayposx = 0;
	float overlayposy = 0;
	float unalignedx[3] {0,0,0};
	float unalignedy[3] {0,0,0};

#ifdef BANNER
	float bannerx = 0;
	OpenGLTexture bannertex;
	std::unique_ptr<OpenGLShaderProgram> bannershader;
#endif
	float banneroffset = 0;

	float dpi = 1;
	long unsigned int uiscaleindex = 0;
	int prevuiscaleindex = -10;
	std::vector<float> uiscales;

	LookNFeel looknfeel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SunBurntAudioProcessorEditor)
};
