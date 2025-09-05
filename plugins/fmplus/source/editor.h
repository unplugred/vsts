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
	Colour bg1 = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	Colour fg1 = Colour::fromFloatRGBA(1.f,1.f,0.f,1.f);
	Colour bg2 = Colour::fromFloatRGBA(0.f,0.f,1.f,1.f);
	Colour fg2 = Colour::fromFloatRGBA(1.f,1.f,1.f,1.f);
	String font = "n";
};
struct knob {
	int x = 0;
	int y = 0;
	float value = .5f;
	int hoverstate = 0;
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
class FMPlusAudioProcessorEditor : public AudioProcessorEditor, public plugmachine_gui {
public:
	FMPlusAudioProcessorEditor(FMPlusAudioProcessor&, int paramcount, pluginpreset state, pluginparams params);
	~FMPlusAudioProcessorEditor() override;

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

	knob knobs[6];
	int knobcount = 0;
	float visline[2][452];
	bool is_stereo = false;
private:
	FMPlusAudioProcessor& audio_processor;

	OpenGLTexture basetex;
	OpenGLTexture tabselecttex;
	OpenGLTexture headertex;
	std::shared_ptr<OpenGLShaderProgram> baseshader;

	int hover = -1;
	int initialdrag = 0;
	int held = 0;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);

	float websiteht = -1;

	float rms = 0;
	float time = 0;

	// light theme
	//*/
	float	col_bg			[3] = {	 .894f	,.894f	,.894f	};
	float	col_bg_mod		[3] = {	 1.f	,1.f	,1.f	};
	float	col_conf		[3] = {	 1.f	,1.f	,1.f	};
	float	col_conf_mod	[3] = {	 .894f	,.894f	,.894f	};
	float	col_outline		[3] = {	 0.f	,0.f	,0.f	};
	float	col_outline_mod	[3] = {	 .384f	,.384f	,.384f	};
	float	col_vis			[3] = {	 .894f	,.894f	,.894f	};
	float	col_vis_mod		[3] = {	 .949f	,.949f	,.949f	};
	float	col_highlight	[3] = {	 1.f	,.761f	,.996f	};
	//*/

	// dark theme
	/*/
	float	col_bg			[3] = {	 .106f	,.106f	,.106f	};
	float	col_bg_mod		[3] = {	 0.f	,0.f	,0.f	};
	float	col_conf		[3] = {	 0.f	,0.f	,0.f	};
	float	col_conf_mod	[3] = {	 .106f	,.106f	,.106f	};
	float	col_outline		[3] = {	 .8f	,.8f	,.8f	};
	float	col_outline_mod	[3] = {	 .616f	,.616f	,.616f	};
	float	col_vis			[3] = {	 .106f	,.106f	,.106f	};
	float	col_vis_mod		[3] = {	 .051f	,.051f	,.051f	};
	float	col_highlight	[3] = {	 0.f	,.239f	,.000f	};
	//*/

	// high contrast
	/*/
	float	col_bg			[3] = {	  10.f	, 10.f	, 10.f	};
	float	col_bg_mod		[3] = {	 -10.f	, 10.f	, 10.f	};
	float	col_conf		[3] = {	  10.f	, 10.f	, 10.f	};
	float	col_conf_mod	[3] = {	 -10.f	, 10.f	, 10.f	};
	float	col_outline		[3] = {	 -10.f	,-10.f	,-10.f	};
	float	col_outline_mod	[3] = {	 -10.f	,-10.f	,-10.f	};
	float	col_vis			[3] = {	  10.f	, 10.f	, 10.f	};
	float	col_vis_mod		[3] = {	  10.f	, 10.f	, 10.f	};
	float	col_highlight	[3] = {	  10.f	, 10.f	,-10.f	};
	//*/

	// seoul
	/*/
	float	col_bg			[3] = {	 .145f	,.145f	,.145f	};
	float	col_bg_mod		[3] = {	 .2f	,.196f	,.2f	};
	float	col_conf		[3] = {	 .2f	,.196f	,.2f	};
	float	col_conf_mod	[3] = {	 .294f	,.294f	,.294f	};
	float	col_outline		[3] = {	 .851f	,.851f	,.851f	};
	float	col_outline_mod	[3] = {	 .2f	,.196f	,.2f	};
	float	col_vis			[3] = {	 .145f	,.145f	,.145f	};
	float	col_vis_mod		[3] = {	 .2f	,.196f	,.2f	};
	float	col_highlight	[3] = {	 .604f	,.451f	,.447f	};
	//*/

	// windowmaker
	/*/
	float	col_bg			[3] = {	 .6f	,.6f	,.6f	};
	float	col_bg_mod		[3] = {	 .204f	,.224f	,.227f	};
	float	col_conf		[3] = {	 .929f	,.663f	,.424f	};
	float	col_conf_mod	[3] = {	 .533f	,.38f	,.243f	};
	float	col_outline		[3] = {	 1.f	,1.f	,1.f	};
	float	col_outline_mod	[3] = {	 .11f	,.22f	,.251f	};
	float	col_vis			[3] = {	 .402f	,.412f	,.414f	};
	float	col_vis_mod		[3] = {	 .6f	,.6f	,.6f	};
	float	col_highlight	[3] = {	 .29f	,.573f	,.651f	};
	//*/

	// color test
	/*/
	//float	col_bg			[3] = {	 1.f	,0.f	,0.f	};
	//float	col_bg_mod		[3] = {	 0.f	,1.f	,0.f	};
	//float	col_conf		[3] = {	 0.f	,0.f	,1.f	};
	//float	col_conf_mod	[3] = {	 .5f	,.5f	,0.f	};
	//float	col_outline		[3] = {	 .5f	,0.f	,.5f	};
	//float	col_outline_mod	[3] = {	 0.f	,.5f	,.5f	};
	//float	col_vis			[3] = {	 1.f	,0.f	.0.f	};
	//float	col_vis_mod		[3] = {	 0.f	,1.f	.0.f	};
	//float	col_highlight	[3] = {	 .2f	,.2f	,.2f	};
	//*/

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMPlusAudioProcessorEditor)
};
