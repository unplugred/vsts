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
	Colour bg = Colour::fromFloatRGBA(.97255f,.97255f,.97255f,1.f);
	Colour fg = Colour::fromFloatRGBA(0.f,0.f,0.f,1.f);
	Colour ht = Colour::fromFloatRGBA(.99216f,.69412f,.98824f,1.f);
	String font = "n";
};
class MPaintAudioProcessorEditor : public plugmachine_gui {
public:
	MPaintAudioProcessorEditor(MPaintAudioProcessor&, unsigned char soundd);
	~MPaintAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint(Graphics&) override;

	void timerCallback() override;
	virtual void parameterChanged(const String& parameterID, float newValue);

	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;

private:
	MPaintAudioProcessor& audio_processor;

	unsigned char sound = 0;
	int needtoupdate = 2;
	OpenGLTexture errortex;
	OpenGLTexture icontex;
	OpenGLTexture rulertex;
	std::shared_ptr<OpenGLShaderProgram> shader;

	bool loaded = false;
	bool error = false;
	bool errorhover = false;

	LookNFeel look_n_feel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPaintAudioProcessorEditor)
};
