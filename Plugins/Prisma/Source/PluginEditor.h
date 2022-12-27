#pragma once
#include "includes.h"
#include "PluginProcessor.h"
using namespace gl;

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
	float colors[12] { 0,0,0, 0,0,0, 0,0,0, -1,-1,-1 };
	float hovercutoff = .4f;
	float defaultval = 0;
	int clip = -1;
};
struct modulevalues {
	int id = 0;
	std::vector<float> lerps;
	float value = 0;
};
struct editorpreset {
	modulevalues modulesvalues[16];
	float crossover[3] {.25f,.5f,.75f};
	float gain[4] {.5f,.5f,.5f,.5f};
	float wet = 1;
};
class PrismaAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	PrismaAudioProcessorEditor(PrismaAudioProcessor&, pluginpreset states, pluginparams pots);
	~PrismaAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader);
	void renderOpenGL() override;
	void openGLContextClosing() override;
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
	void recalchover(float x, float y);

private:
	PrismaAudioProcessor& audioProcessor;

	OpenGLContext context;
	unsigned int arraybuffer;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};
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
	std::unique_ptr<OpenGLShaderProgram> baseshader;

	bool isb = false;
	bool oversampling = false;
	OpenGLTexture selectortex;
	std::unique_ptr<OpenGLShaderProgram> decalshader;

	module modules[17];
	OpenGLTexture modulestex;
	std::unique_ptr<OpenGLShaderProgram> moduleshader;

	int initialdrag = -1;
	float initialvalue = 0;
	bool finemode = false;
	float valueoffset = 0;
	Point<int> dragpos = Point<int>(0,0);
	OpenGLTexture elementstex;
	std::unique_ptr<OpenGLShaderProgram> elementshader;

	std::unique_ptr<OpenGLShaderProgram> lidshader;

	float websiteht = -1;
	std::unique_ptr<OpenGLShaderProgram> logoshader;

	float vispoly[3972];
	int fftdelta = 0;
	std::unique_ptr<OpenGLShaderProgram> visshader;

	bool buttons[4][3];
	bool truemute[4] {false,false,false,false};
	float activelerp[4] {1.f,1.f,1.f,1.f};
	float bypasslerp[4] {1.f,1.f,1.f,1.f};
	float activeease[4] {1.f,1.f,1.f,1.f};
	float bypassease[4] {1.f,1.f,1.f,1.f};
	std::unique_ptr<OpenGLShaderProgram> visbttnshader;

#ifdef BANNER
	float bannerx = 0;
	OpenGLTexture bannertex;
	std::unique_ptr<OpenGLShaderProgram> bannershader;
#endif
	float banneroffset = 0;

	float dpi = 1;

	float selectorlerp[4] = {0,0,0,0};
	float selectorease[4] = {0,0,0,0};
	bool selectorstate[4] = {false,false,false,false};
	int selectorid = 0;

	editorpreset state[2];
	float presettransition = 0;
	float presettransitionease = 0;
	float crossoverlerp[3] {.25f,.5f,.75f};
	float crossovertruevalue[3] {.25f,.5f,.75f};

	Random random;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismaAudioProcessorEditor)
};
