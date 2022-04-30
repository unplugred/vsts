#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MPaintAudioProcessorEditor : public AudioProcessorEditor, public OpenGLRenderer, public AudioProcessorValueTreeState::Listener, private Timer
{
public:
	MPaintAudioProcessorEditor(MPaintAudioProcessor&, unsigned char soundd);
	~MPaintAudioProcessorEditor() override;

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
	void paint (Graphics&) override;

	void timerCallback() override;

	virtual void parameterChanged(const String& parameterID, float newValue);
	void mouseDown(const MouseEvent& event) override;

private:
	MPaintAudioProcessor& audioProcessor;

	OpenGLContext context;
	unsigned int arraybuffer;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};


	unsigned char sound = 0;
	int needtoupdate = 2;
	OpenGLTexture icontex;
	OpenGLTexture rulertex;
	std::unique_ptr<OpenGLShaderProgram> shader;
	String vert;
	String frag;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MPaintAudioProcessorEditor)
};
