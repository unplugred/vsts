#pragma once
#include "includes.h"
#include "processor.h"
#include "functions.h"
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
	Colour bg1 = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	Colour fg1 = Colour::fromFloatRGBA(1.f,1.f,0.f,1.f);
	Colour bg2 = Colour::fromFloatRGBA(0.f,0.f,1.f,1.f);
	Colour fg2 = Colour::fromFloatRGBA(1.f,1.f,1.f,1.f);
	String font = "n";
};

struct box {
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	int type = 0; // -1 - inactive, 0 - line, 1 - centered, 2 - noline, 3 - tick, 4 - burger
	int knob = -1;
	int textamount = 0;
	bool corner = false;
	int mesh = -1;
	int textmesh = -1;
	box(int pknob = -1, int px = 0, int py = 0, int pw = 0, int ph = 0, int ptype = -1, int ptextamount = 0, bool pcorner = false) {
		knob = pknob;
		x = px;
		y = py;
		w = pw;
		h = ph;
		type = ptype;
		textamount = ptextamount;
		corner = pcorner;
	}
};

struct op {
	int pos[2] { 0,0 };
	std::vector<connection> connections;
	float indicator = 0;
	int textmesh = -1;
};

struct knob {
	float value[MC];
	float valuesmoothed = 0;
	String id;
	String name;
	potentiometer::ptype ttype = potentiometer::ptype::floattype;
	float minimumvalue = 0.f;
	float maximumvalue = 1.f;
	float defaultvalue = 0.f;
	int box = -1;
	float normalize(float val) {
		return (val-minimumvalue)/(maximumvalue-minimumvalue);
	}
	float inflate(float val) {
		return val*(maximumvalue-minimumvalue)+minimumvalue;
	}
};

class FMPlusAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	FMPlusAudioProcessorEditor(FMPlusAudioProcessor&, int pgeneralcount, int pparamcount, pluginpreset state, pluginparams params);
	~FMPlusAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void calcvis(int curveupdated);
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
	void updatehighlight(bool update_adsr = false);
	void freqselect(int digit, bool calcfreqstep = true);

	int prevx = 0;
	int prevy = 0;
	knob knobs[16+19];
	dampenedvalue knobsmooth;
	int knobcount = 0;
	int generalcount = 0;
	int paramcount = 0;
	String tuningfile = "";
	String themefile = "";
	bool presetunsaved = false;
private:
	FMPlusAudioProcessor& audio_processor;

	OpenGLTexture basetex;
	OpenGLTexture tabselecttex;
	OpenGLTexture headertex;
	int selectedtab = -1;
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

	box boxes[24];
	int boxnum = 0;
	float adsr[4*2*2]; // [a,d,s,r],[t,b],[c,t]
	dampenedvalue dampadsr;
	int tabanimation = 100;
	int opanimation = 100;

	float squaremesh[39*6*4];
	int squarelength = -1;
	int highlight = -1;
	int freqdigit = -20;
	float freqstep = -1;
	float freqoffset = 0;
	int scrolldigit = -20;
	std::shared_ptr<OpenGLShaderProgram> squareshader;
	void addsquare(float x, float y, float w, float h, float color = 0, bool corner = false);

	curve curves[MC];
	int lfohover = -1;

	int linewritepos = -1;
	float prevdpi = -1;
	int curvemesh[4];
	float visline[480*3];
	int linelength = -1;
	float lineprevx = 0;
	float lineprevy = 020;
	float linecurrentx = 0;
	float linecurrenty = 0;
	bool linebegun = true;
	std::shared_ptr<OpenGLShaderProgram> lineshader;
	void beginline(float x, float y);
	void endline();
	void nextpoint(float x, float y, bool knee = false);

	bool displayaddremove[2] = { true, true };
	std::shared_ptr<OpenGLShaderProgram> circleshader;

	OpenGLTexture operatortex;
	op ops[MC+1];
	std::shared_ptr<OpenGLShaderProgram> operatorshader;

	std::shared_ptr<OpenGLShaderProgram> indicatorshader;

	int prevtextindex[4];
	float textmesh[289*6*4]; // X Y COL CHAR
	int textlength = -1;
	OpenGLTexture texttex;
	std::shared_ptr<OpenGLShaderProgram> textshader;
	void addtext(float x, float y, String txt, float color = 0);
	void replacetext(int id, String txt, int length = -1);

	std::unique_ptr<FileChooser> tuningchooser;
	std::unique_ptr<FileChooser> themechooser;

	// light theme
	//*/
	float col_bg			[3] = {	 .894f	,.894f	,.894f	};
	float col_bg_mod		[3] = {	 1.f	,1.f	,1.f	};
	float col_conf			[3] = {	 1.f	,1.f	,1.f	};
	float col_conf_mod		[3] = {	 .894f	,.894f	,.894f	};
	float col_outline		[3] = {	 0.f	,0.f	,0.f	};
	float col_outline_mod	[3] = {	 .384f	,.384f	,.384f	};
	float col_vis			[3] = {	 .894f	,.894f	,.894f	};
	float col_vis_mod		[3] = {	 .949f	,.949f	,.949f	};
	float col_highlight		[3] = {	 1.f	,.761f	,.996f	};
	//*/

	// dark theme
	/*/
	float col_bg			[3] = {	 .106f	,.106f	,.106f	};
	float col_bg_mod		[3] = {	 0.f	,0.f	,0.f	};
	float col_conf			[3] = {	 0.f	,0.f	,0.f	};
	float col_conf_mod		[3] = {	 .106f	,.106f	,.106f	};
	float col_outline		[3] = {	 .8f	,.8f	,.8f	};
	float col_outline_mod	[3] = {	 .616f	,.616f	,.616f	};
	float col_vis			[3] = {	 .106f	,.106f	,.106f	};
	float col_vis_mod		[3] = {	 .051f	,.051f	,.051f	};
	float col_highlight		[3] = {	 0.f	,.239f	,.000f	};
	//*/

	// high contrast
	/*/
	float col_bg			[3] = {	  10.f	, 10.f	, 10.f	};
	float col_bg_mod		[3] = {	 -10.f	, 10.f	, 10.f	};
	float col_conf			[3] = {	  10.f	, 10.f	, 10.f	};
	float col_conf_mod		[3] = {	 -10.f	, 10.f	, 10.f	};
	float col_outline		[3] = {	 -10.f	,-10.f	,-10.f	};
	float col_outline_mod	[3] = {	 -10.f	,-10.f	,-10.f	};
	float col_vis			[3] = {	  10.f	, 10.f	, 10.f	};
	float col_vis_mod		[3] = {	  10.f	, 10.f	, 10.f	};
	float col_highlight		[3] = {	  10.f	, 10.f	,-10.f	};
	//*/

	// seoul
	/*/
	float col_bg			[3] = {	 .145f	,.145f	,.145f	};
	float col_bg_mod		[3] = {	 .2f	,.196f	,.2f	};
	float col_conf			[3] = {	 .2f	,.196f	,.2f	};
	float col_conf_mod		[3] = {	 .294f	,.294f	,.294f	};
	float col_outline		[3] = {	 .851f	,.851f	,.851f	};
	float col_outline_mod	[3] = {	 .2f	,.196f	,.2f	};
	float col_vis			[3] = {	 .145f	,.145f	,.145f	};
	float col_vis_mod		[3] = {	 .2f	,.196f	,.2f	};
	float col_highlight		[3] = {	 .604f	,.451f	,.447f	};
	//*/

	// machinery
	/*/
	float col_bg			[3] = {	 .6f	,.6f	,.6f	};
	float col_bg_mod		[3] = {	 .204f	,.224f	,.227f	};
	float col_conf			[3] = {	 .929f	,.663f	,.424f	};
	float col_conf_mod		[3] = {	 .533f	,.38f	,.243f	};
	float col_outline		[3] = {	 1.f	,1.f	,1.f	};
	float col_outline_mod	[3] = {	 .11f	,.22f	,.251f	};
	float col_vis			[3] = {	 .402f	,.412f	,.414f	};
	float col_vis_mod		[3] = {	 .6f	,.6f	,.6f	};
	float col_highlight		[3] = {	 .29f	,.573f	,.651f	};
	//*/

	// color test
	/*/
	float col_bg			[3] = {	 1.f	,0.f	,0.f	};
	float col_bg_mod		[3] = {	 0.f	,1.f	,0.f	};
	float col_conf			[3] = {	 0.f	,0.f	,1.f	};
	float col_conf_mod		[3] = {	 .5f	,.5f	,0.f	};
	float col_outline		[3] = {	 .5f	,0.f	,.5f	};
	float col_outline_mod	[3] = {	 0.f	,.5f	,.5f	};
	float col_vis			[3] = {	 1.f	,0.f	,0.f	};
	float col_vis_mod		[3] = {	 0.f	,1.f	,0.f	};
	float col_highlight		[3] = {	 .2f	,.2f	,.2f	};
	//*/

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusAudioProcessorEditor)
};
