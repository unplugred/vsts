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
class handmedown {
public:
	handmedown() {
		font.uvmap = {{{54,1,1,26,35,6,-2,14},{53,28,1,29,35,6,-2,17},{57,58,1,25,34,6,-2,13},{50,84,1,29,34,6,-2,17},{56,1,37,30,34,6,-2,18},{51,32,37,28,34,6,-1,17},{52,61,37,26,34,6,-2,14},{55,88,37,27,33,6,-1,15},{49,1,72,17,33,6,-1,5},{48,19,72,29,33,6,-1,17},{109,49,72,34,25,6,8,22},{115,84,72,22,24,6,8,10},{46,107,72,17,17,6,14,5},{32,0,0,0,0,0,0,13},{113,103,92,25,36,6,-2,13}},{{54,1,1,26,35,6,-2,14},{53,28,1,29,35,6,-2,17},{57,58,1,25,34,6,-2,13},{50,84,1,29,34,6,-2,17},{56,1,37,30,34,6,-2,18},{51,32,37,28,34,6,-1,17},{52,61,37,26,34,6,-2,14},{55,88,37,27,33,6,-1,15},{49,1,72,17,33,6,-1,5},{48,19,72,29,33,6,-1,17},{109,49,72,34,25,6,8,22},{115,84,72,22,24,6,8,10},{46,107,72,17,17,6,14,5},{32,0,0,0,0,0,0,13},{113,103,92,25,36,6,-2,13}}};
		font.kerning = {{{54,55,-2},{54,57,-3},{53,55,-2},{50,55,-2},{51,55,-2},{109,55,-2},{109,57,-3},{115,55,-2},{115,57,-2}},{{54,55,-2},{54,57,-3},{53,55,-2},{50,55,-2},{51,55,-2},{109,55,-2},{109,57,-3},{115,55,-2},{115,57,-2}}}; //NOT YET IMPLEMENTED; looks good already so idk if ill add this
		font.smooth = true;
		font.slant = -.3f;
		font.scale = .5f;
	}
	void init(OpenGLContext* context, float bannero, int w, int h, float _dpi) {
		font.init(context, bannero, w, h, _dpi, ImageCache::getFromMemory(BinaryData::handmedown_png, BinaryData::handmedown_pngSize),128,128,33,false,0,0,
R"(#version 150 core
in vec2 aPos;
uniform vec4 pos;
uniform vec4 texpos;
uniform float letter;
uniform vec2 time;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*pos.zw+pos.xy)*2-1,0,1);
	gl_Position.y = pow(max(0,letter-time.x+1.5),4)*-0.0003-gl_Position.y;
	uv = (aPos*texpos.zw+texpos.xy);
	uv.y = 1-uv.y;
})",
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform int channel;
uniform vec2 res;
uniform float dpi;
uniform vec4 bg;
uniform vec4 fg;
uniform float letter;
uniform vec2 time;
out vec4 fragColor;
void main(){
	if(letter > time.x || letter < time.y) {
		fragColor = vec4(0);
		return;
	}
	float text = 0;
	if(channel == 1)
		text = texture(tex,uv).g;
	else if(channel == 2)
		text = texture(tex,uv).b;
	else
		text = texture(tex,uv).r;
	if(dpi > 1)
		text = (text-.5)*dpi+.5;
	fragColor = vec4(fg.rgb,text);
})");
	}
	CoolFont font;
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

	OpenGLTexture selecttex;
	int curveselection = 0;
	std::unique_ptr<OpenGLShaderProgram> selectshader;

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

	std::vector<int> slidersvisible;
	std::vector<String> sliderslabel;
	std::vector<String> slidersvalue;
	tahoma8 font = tahoma8();

	OpenGLTexture overlaytex;
	OpenGLFrameBuffer framebuffer;
	std::unique_ptr<OpenGLShaderProgram> ppshader;

	float length = .65f;
	int sync = 0;
	bool changegesturesync = false;

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

	float timeopentime = -1;
	float timeclosetime = -1;
	String timestring = "";
	handmedown timefont = handmedown();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SunBurntAudioProcessorEditor)
};
