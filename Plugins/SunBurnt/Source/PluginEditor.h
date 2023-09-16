#pragma once
#include "includes.h"
#include "PluginProcessor.h"
#include "tahoma8.h"
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
struct slider {
	float value = .5f;
	String id;
	String name;
	String displayname;
	float minimumvalue = 0.f;
	float maximumvalue = 1.f;
	float defaultvalue = 0.f;
	float normalize(float val) {
		return (val-minimumvalue)/(maximumvalue-minimumvalue);
	}
	float inflate(float val) {
		return val*(maximumvalue-minimumvalue)+minimumvalue;
	}
	std::vector<int> showon;
	std::vector<int> dontshowif;
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
	void recalclabels();
	void recalcsliders();
	void randcubes(int64 seed);
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	int recalchover(float x, float y);

	knob knobs[6];
	int knobcount = 0;
	slider sliders[5];
	int slidercount = 0;
	curve curves[8];
	int curveindex[5] = {0,1,3,5,7};
	float visline[3408];

	std::vector<int> slidersvisible;
	std::vector<String> sliderslabel;
	std::vector<String> slidersvalue;
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

	float panelheight = 0;
	float panelheighttarget = 0;
	bool panelvisible = false;
	std::unique_ptr<OpenGLShaderProgram> panelshader;

	OpenGLTexture overlaytex;
	OpenGLFrameBuffer framebuffer;
	std::unique_ptr<OpenGLShaderProgram> ppshader;

	float length = .65f;
	int sync = 0;

	float time = 1000;
	Random random;
	char randomid[4] {0,0,0,0};
	char hidecube[2] {13,19};
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

	float dpi = -10;
	long unsigned int uiscaleindex = 0;
	float scaleddpi = -10;
	std::vector<float> uiscales;
	bool resetsize = false;

	LookNFeel looknfeel;
	tahoma8 font = tahoma8();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SunBurntAudioProcessorEditor)
};
