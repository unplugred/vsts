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
	bool jpmode = false;
	bool prevjpmode = false;
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

	OpenGLTexture papertex;
	int papercoords[2][108] =
	{{ // EN w: 1897, h: 1355
		 745,  894, 439, 436, 104, 105, //01
		1186,  915, 479, 439,  71,  96, //02
		 241,  868, 502, 419,  53, 104, //03
		1085,  560, 521, 332,  34, 153, //05
		 525,  239, 552, 273,   7, 183, //07
		 247,    1, 549, 233,   9, 224, //09
		   1,  236, 522, 271,  29, 223, //11
		1079,  260, 493, 277,  53, 222, //13
		1175,    1, 450, 257,  92, 226, //15
		   1,  509, 437, 293, 109, 216, //17
		 679,  539, 404, 327, 140, 214, //19
		1608,  560, 288, 353, 198, 216, //23
		   1,  804, 238, 356, 198, 215, //25
		 440,  514, 237, 326, 196, 228, //27
		1574,  266, 255, 292, 177, 230, //29
		1627,    1, 264, 263, 169, 230, //31
		   1,    1, 244, 231, 187, 230, //32
		 798,    1, 375, 236, 130, 218  //crossed
	},{ // JP "w": 1714, "h": 1504
		1000,    1, 425, 454, 105,  96, //00
		 511,  381, 442, 458,  89,  97, //01
		 955,  457, 444, 432,  93,  94, //02
		   1,  799, 459, 420,  88,  94, //03
		 516,    1, 482, 378,  73, 117, //05
		   1,  233, 508, 312,  55, 155, //08
		   1,    1, 513, 230,  43, 224, //11
		   1,  547, 481, 250,  67, 224, //15
		 462,  841, 431, 239, 104, 229, //17
		 895,  891, 411, 269, 116, 225, //19
		   1, 1221, 383, 282, 138, 224, //21
		1427,    1, 286, 308, 180, 220, //25
		 736, 1162, 221, 325, 190, 217, //27
		 959, 1162, 224, 316, 187, 217, //29
		1185, 1162, 241, 299, 170, 220, //31
		1428,  311, 239, 279, 172, 221, //33
		1308,  891, 237, 243, 175, 221, //35
		 386, 1221, 348, 227, 133, 243  //crossed
	}};
	OpenGLTexture handtex;
	int handcoords[384] = { // w: 2048, h: 509
		 741,  331,  77, 148,  94,  62, //000
		   1,  300, 101, 142,  77,  66, //004
		 123,  181, 123, 122,  60,  76, //007
		1712,    1, 139,  95,  54,  86, //010
		   1,    1, 143,  77,  52,  95, //012
		1280,    1, 139,  92,  53,  85, //014
		1489,   95, 120, 118,  63,  67, //017
		1085,  208,  96, 131,  77,  61, //019
		 749,  196,  76, 133,  86,  62, //021
		1183,  213,  92, 131,  80,  61, //023
		1869,   98, 109, 119,  78,  66, //025
		1853,    1, 128,  95,  67,  80, //027
		 146,    1, 133,  77,  61,  89, //029
		 701,   85, 128, 109,  66,  79, //032
		1277,  214, 105, 132,  76,  72, //035
		1970,  219,  77, 146,  87,  63, //037
		1066,  341,  86, 150,  77,  60, //039
		 820,  332,  89, 147,  82,  63, //040
		 248,  185, 126, 123,  61,  77, //042
		 967,   92, 137, 112,  54,  77, //043
		1421,    1, 144,  92,  52,  83, //044
		 859,    1, 145,  86,  51,  89, //045
		1363,   95, 124, 117,  59,  71, //047
		1868,  219, 100, 141,  80,  58, //049
		 207,  310,  79, 143,  94,  59, //050
		 288,  310,  81, 143,  88,  58, //051
		 509,  193, 120, 127,  72,  69, //053
		1106,   94, 131, 112,  65,  82, //054
		1006,    1, 135,  89,  60,  99, //055
		1143,    1, 135,  91,  61,  89, //056
		 631,  196, 116, 130,  70,  71, //058
		 104,  305, 101, 142,  73,  65, //059
		 988,  338,  76, 149,  85,  61, //062
		 469,  322,  88, 145,  82,  63, //063
		1384,  215, 107, 135,  70,  69, //064
		 831,   89, 134, 109,  56,  80, //066
		1567,    1, 143,  92,  52,  84, //067
		 281,    1, 143,  77,  52,  92, //068
		1239,   95, 122, 116,  60,  70, //070
		1493,  215,  91, 138,  81,  59, //072
		1789,  218,  77, 141,  93,  58, //073
		1706,  218,  81, 139,  87,  59, //074
		   1,  179, 120, 119,  72,  69, //076
		 284,   80, 130, 103,  66,  80, //077
		 426,    1, 135,  78,  60,  94, //079
		   1,   80, 133,  97,  63,  86, //080
		1611,   98, 121, 118,  70,  77, //081
		 559,  328,  87, 145,  78,  64, //083
		1333,  352,  73, 156,  87,  58, //109
		1244,  348,  87, 155,  82,  58, //110
		 827,  200, 132, 130,  58,  72, //112
		 556,   84, 143, 107,  52,  85, //113
		 705,    1, 152,  82,  47,  95, //114
		 136,   80, 146,  99,  50,  82, //115
		 376,  186, 131, 125,  56,  68, //116
		 648,  328,  91, 145,  74,  57, //117
		 911,  338,  75, 148,  87,  55, //118
		 371,  313,  96, 144,  81,  57, //119
		 961,  206, 122, 130,  69,  63, //120
		 416,   81, 138, 103,  61,  79, //121
		 563,    1, 140,  81,  58,  95, //122
		1734,   98, 133, 118,  63,  76, //123
		1586,  218, 118, 135,  71,  68, //124
		1154,  346,  88, 153,  84,  60  //125
	};
	float handpos       [8] = { -128,171,1012,3.5f,368,171,1036,3.5f };
	float handposrotated[6] = { -128,171,1012     ,368,171,1036      };
	float handposlerp   [8] = { -128,171,1012,3.5f,368,171,1034,3.5f };
	functions::inertiadampened handposinertiadamp[6];
	functions::dampendvalue handposdamp[8];
	int handhover = -1;
	bool skipmouseup = false;
	int menuindex = 0;
	int hoverid = -1;
	bool itemenabled = false;
	int paperindex = 10000;
	float paperrot = 0;
	float paperrottarget = 0;
	float paperrotoffset = 0;
	bool updatetick = true;
	std::unique_ptr<OpenGLShaderProgram> papershader;

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
