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
	Colour bg = Colour::fromFloatRGBA(.99608f,.99608f,.95294f,1.f);
	Colour inactive = Colour::fromFloatRGBA(0.f,0.f,0.f,.5f);
	Colour txt = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	Colour highlight_bg = Colour::fromFloatRGBA(.83137f,.41176f,.34902f,1.f);
	Colour highlight_bg2 = Colour::fromFloatRGBA(.35294f,.60784f,.83922f,1.f);
	String font = "n";
};
struct subknob {
	int id = 0;
	float rotspeed = .8f;
	float lerprot = .5f;
	float moveradius = 0;
	float lerpmove = 0;
	float movespeed = .8f;
	subknob(int iid, float irotspeed = .8f, float ilerprot = .4f, float imoveradius = 0, float ilerpmove = 0, float imovespeed = .8f) {
		id = iid;
		rotspeed = irotspeed;
		lerprot = ilerprot;
		moveradius = imoveradius;
		lerpmove = ilerpmove;
		movespeed = imovespeed;
	}
};
struct module {
	String name = "";
	std::vector<subknob> subknobs;
	float colors[12] { 1,1,1, 0,0,0, 0,0,0, -1,-1,-1 };
	float hovercutoff = .4f;
	float defaultval = 0;
	float defaultvaly = 0;
	int clip = -1;
	bool xy = false;
	String description = "";
};
struct modulevalues {
	int id = 0;
	std::vector<float> lerps;
	float value = 0;
	float valuey = 0;
};
struct editorpreset {
	modulevalues modulesvalues[BAND_COUNT*MAX_MOD];
	float crossover[BAND_COUNT==1?1:BAND_COUNT-1];
	float gain[BAND_COUNT];
	float wet = 1;
	editorpreset() {
		for(int b = 0; b < BAND_COUNT; ++b) {
			if(b >= 1) crossover[b-1] = ((float)b)/BAND_COUNT;
			gain[b] = .5f;
		}
	}
};
class PrismaAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	PrismaAudioProcessorEditor(PrismaAudioProcessor&, pluginpreset states, pluginparams pots);
	~PrismaAudioProcessorEditor() override;

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
	void recalc_hover(float x, float y);

private:
	PrismaAudioProcessor& audio_processor;

	float opsquare[12]{
		0.f,0.f,1.f,
		1.f,0.f,1.f,
		0.f,1.f,1.f,
		1.f,1.f,1.f};

	float eyex = -6;
	float eyey = 67;
	float eyelerpx = -6;
	float eyelerpy = 67;
	float eyelid = 1;
	bool eyelidup = true;
	int eyelidcooldown = 0;
	int nextblink = 0;
	bool clickedeye = false;
	int eyeclickcooldown = 0;

	int lastx = 0;
	int lasty = 0;
	bool out = true;
	bool prevout = true;
	bool hovereye = false;
	bool hoverknob = false;
	int hover = -1;
	int hoverselector = -1;
	int hoverbutton = -1;
	bool isdown = false;

	OpenGLTexture basetex;
	std::shared_ptr<OpenGLShaderProgram> baseshader;

#ifdef PRISMON
	OpenGLTexture logomontex;
	std::shared_ptr<OpenGLShaderProgram> logomonshader;
#endif

	int modulecount = DEF_MOD;

	OpenGLTexture selectortex;
	std::shared_ptr<OpenGLShaderProgram> selectorshader;

	bool isb = false;
	bool oversampling = false;
	std::shared_ptr<OpenGLShaderProgram> decalshader;

	module modules[MODULE_COUNT+1];
	OpenGLTexture modulestex;
	std::shared_ptr<OpenGLShaderProgram> moduleshader;

	int initialdrag = -1;
	float initialvalue = 0;
	float initialvaluey = 0;
	bool finemode = false;
	float valueoffset = 0;
	float valueyoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	OpenGLTexture elementstex;
	std::shared_ptr<OpenGLShaderProgram> elementshader;

	std::shared_ptr<OpenGLShaderProgram> lidshader;

	float websiteht = -1;
	std::shared_ptr<OpenGLShaderProgram> logoshader;

	float vispoly[3972];
	int fftdelta = 0;
	std::shared_ptr<OpenGLShaderProgram> visshader;

	bool buttons[BAND_COUNT][3];
	bool truemute[BAND_COUNT];
	float activelerp[BAND_COUNT];
	float bypasslerp[BAND_COUNT];
	float activeease[4] = {1.f,1.f,1.f,1.f};
	float bypassease[BAND_COUNT];
	std::shared_ptr<OpenGLShaderProgram> visbttnshader;

	float selectorlerp[BAND_COUNT];
	float selectorease[4] = {0,0,0,0};
	float selectorscroll[BAND_COUNT];
	float selectorscrolllerp[BAND_COUNT];
	bool selectorstate[BAND_COUNT];
	int selectorid = 0;
	int selectormapping[MODULE_COUNT+1] = {0,1,2,3,4,5,6,7,8,9,10,21,11,13,12,14,18,15,16,17,19,20};

	editorpreset state[2];
	float presettransition = 0;
	float presettransitionease = 0;
	float crossoverlerp[3] = { 99.f, 99.f, 99.f };
	float crossovertruevalue[BAND_COUNT==1?1:BAND_COUNT-1];

	int dragger = 0;
	int dragged = 0;
	float dragcoords[6] = {0.f,0.f,0.f,0.f,0.f,0.f}; // x y rot shadow offset
	float draglerp[4] = {0.f,0.f,0.f,0.f}; // x y rot shadow
	bool dragstate = false;
	int darkindex = 0;

	Random random;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismaAudioProcessorEditor)
};
