#pragma once
#include "includes.h"
#include "processor.h"
#include "functions.h"
#include "page.h"
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
	Colour bg;
	Colour fg;
	Colour on;
	String font = "n";
};

struct op {
	short pos[2] { 0,0 };

	signed char outputs[3]     { -1, -1, -1 };
	signed char inputs [3]     { -1, -1, -1 };
	unsigned char influence[3] { 50, 50, 50 };

	float indicator = 0;
	int textmesh = -1;
};

class FMPlusAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	FMPlusAudioProcessorEditor(FMPlusAudioProcessor&);
	~FMPlusAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void calcvis(int curveupdated);
	void addop();
	void removeop();
	void updatefxop();
	void resortop();
	void sortconnectors(signed char o, bool outputs);
	void calcline();
	void makeconnector(int x1, int y1, int x2, int y2, int oo, int io, int on, int in, bool isdrag = false);
	void rebuildtab(int tab);
	void updatevalue(int param);
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
	bool keyPressed(const KeyPress& key) override;
	int recalc_hover(float x, float y);
	void updatehighlight(bool update_adsr = true);
	void updateadsr(int adsr = -1);
	void freqselect(int digit, bool calcfreqstep = true);

	int prevx = 0;
	int prevy = 0;
private:
	FMPlusAudioProcessor& audio_processor;
	FMPlusPage* selectedpage;

	meshdata mesh;

	OpenGLTexture basetex;
	OpenGLTexture tabselecttex;
	OpenGLTexture headertex;
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	int hover = -1;
	int initialdrag = -1;
	float initialvalue[2] = {0,0};
	float initialdotvalue[2] = {0,0};
	float initialaxispoint[2] = {0,0};
	float axisvaluediff[2] = {0,0};
	float amioutofbounds[2] = {0,0};
	bool finemode = false;
	int axislock = -1;
	float valueoffset[2] = {0,0};
	Point<int> dragpos = Point<int>(0,0);

	float websiteht = -1;
	OpenGLTexture creditstex;
	std::shared_ptr<OpenGLShaderProgram> creditsshader;

	int tabanimation = 100;
	int opanimation = 100;

	int highlight = -1;
	int freqdigit = -20;
	float freqstep = -1;
	float freqoffset = 0;
	int scrolldigit = -20;
	std::shared_ptr<OpenGLShaderProgram> squareshader;

	int curvehover = -1;

	int linewritepos = -1;
	float prevdpi = -1;
	std::shared_ptr<OpenGLShaderProgram> lineshader;

	bool displayaddremove[2] = { true, true };
	signed char circlehover = -5; // i?0:10 for origin
	signed char circlesnapt = 0; // post sort drag n
	signed char circlesnapo = -1; // snap o
	signed char circlesnapn = 0; // snap n
	bool circlesteppedout = false;
	std::shared_ptr<OpenGLShaderProgram> circleshader;

	OpenGLTexture operatortex;
	op ops[2][MC+1];
	short opz[MC+1] { 0,1,2,3,4,5,6,7,8 };
	dampenedvalue fxdamp;
	float fx_smooth[FXC+1][2];
	short fx_target[FXC+1][2];
	signed char scaleop = -10;
	float scaleoplerp = 1;
	signed char fxanim = -1;
	signed char opdrag = -10;
	signed char opdragpos = -10;
	bool dragoutop = false;
	std::shared_ptr<OpenGLShaderProgram> operatorshader;

	std::shared_ptr<OpenGLShaderProgram> indicatorshader;

	int prevtextindex[4];
	int pagestart = 0;
	OpenGLTexture texttex;
	std::shared_ptr<OpenGLShaderProgram> textshader;

	std::unique_ptr<FileChooser> tuningchooser;
	std::unique_ptr<FileChooser> themechooser;
	void updatetheme();

	float opacity_ht =  .85f;
	float col_bg [3];
	float col_fg [3];
	float col_off[3];
	float col_on [3];
	float col_ht [3];

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusAudioProcessorEditor)
};
