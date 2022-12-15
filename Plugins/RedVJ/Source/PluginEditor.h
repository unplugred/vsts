#pragma once
#include "includes.h"
#include "PluginProcessor.h"
using namespace gl;

extern "C" {
#include "lua-5.4.3/lua.h"
#include "lua-5.4.3/lauxlib.h"
#include "lua-5.4.3/lualib.h"
}

/*
video channel
image channel
image channel
camera channel
	size (letterbox, fit, stretch)
	scale (0-infinity)
	position (x,y)
	video controls
	path/source
	alpha
	hue shift
	background (color/channel)
color channel
	r g b
	h s v
	alpha
	hex
shader preset channel

type:
float 0-1
int 0-127
color 0-1,0-1,0-1,0-1
string ""
path ""
channel 0-8
*/
class imagevalue {
public:
	int cachetimer = 120;
	int uses = 1;
	File path = "";
	OpenGLTexture tex;
	bool loaded = false;
	bool exists = true;
	bool freed = false;
};
class imports {
public:
	String type = "float";
	String name = "";
	float value = 0;
	unsigned int color;

	String miditype = "";
	int midichannel = 0;

	String miditype = "";
	int midinote = -1;
	int midichannel = -1;
	int midicontrollernumber = -1;
	String midiidentifier = "";
	String midiname = "";
};
class exports {
public:
	String type = "float";
	String name = "";
	File path = "";
	float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
	int device = 0;
	int channel = 0;
	int id = -1;
	/*
camera: name, device
channel: name, channel
video: name, path
image: name, path
float: name, x
vec2: name, x, y
vec3: name, x, y, z
vec4: name, x, y, z, w
	*/
};
class channelstate {
public:
	std::vector<imports> luaimports;
	std::vector<exports> luaexports;

	File path = "";
	String luac = "NULL";
	String vert = "";
	String frag = "";

	bool on = false;
	bool fft = false;
};
class state {
public:
	String statename = "New state";
	channelstate channellist[9];
};
class channel {
public:
	Atomic<bool> resetshader = false;
	std::unique_ptr<OpenGLShaderProgram> shader;

	OpenGLFrameBuffer tex;
	int texh = 0, texw = 0;

	lua_State* L;

	String shadererror;
};
class RedVJAudioProcessorEditor	 : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, public CameraDevice::Listener, public KeyListener, private Timer, public MidiInputCallback
{
public:
	RedVJAudioProcessorEditor (RedVJAudioProcessor&);
	~RedVJAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint (Graphics&) override;
	void resized() override;

	void imageReceived(const Image& image) override;
	void timerCallback() override;
	void parameterChanged(const String& parameterID, float newValue) override;
	bool keyPressed(const KeyPress& key, Component* originatingComponent) override;
	void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) override;

	void updatevalue(int channel, int parameter, float value);
	void updatevalue(int channel, int parameter, int value);
	void loadshader(File filename, int channel);
	bool checkLua(int channel, int results);
	static int lua_Debug(lua_State* lel);
	static int lua_GetFFT(lua_State* lel);

	static RedVJAudioProcessorEditor* singleton;

	RedVJAudioProcessor& audioProcessor;

	channel* getchannel(int channel);
	channelstate* getchannelstate(int channel);
	channelstate* getchannelstate(int channel, int statee);

private:
	void refreshexports(int channel);

	std::map<int, imagevalue*> images;
	std::vector<imagevalue*> todelete;
	std::vector<state> statelist;
	channel channellist[9];
	int currentstate = 0;
	int outchannel = 0;
	int selectedchannel = 0;
	int currentimgid = 0;
	int fftusecount = 0;

	std::unique_ptr<FileChooser> flchs = std::make_unique<FileChooser>("Select shader",File::getSpecialLocation(File::userHomeDirectory),"*.rvjs, *.rvjp, *.rvjl");

	OpenGLContext context;
	unsigned int arraybuffer;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};

	CameraDevice* cam = CameraDevice::openDevice(0);
	OpenGLTexture camtex;
	Image camimg;
	Atomic<bool> drawncam = false;
	Atomic<int> camw = 0;
	Atomic<int> camh = 0;
	Desktop& desktop = Desktop::getInstance();

	String defaultvert;
	String defaultfrag;
	String blackvert;
	String blackfrag;

	std::unique_ptr<OpenGLShaderProgram> quadshader;
	String quadvert;
	String quadfrag;

	OpenGLTexture texttex;
	std::unique_ptr<OpenGLShaderProgram> textshader;
	String textvert;
	String textfrag;

	Random random;
	AudioDeviceManager devicemanager;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RedVJAudioProcessorEditor)
};
