/*
  ==============================================================================

	Knobs.h
	Created: 27 Dec 2020 7:13:10am
	Author:	 ari

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;

class DelayedOneShotLambda : public Timer
{
public:
	DelayedOneShotLambda(int ms, std::function<void()> fn) :
		func(fn)
	{
		startTimer(ms);
	}
	~DelayedOneShotLambda() { stopTimer(); }

	void timerCallback() override
	{
		auto f = func;
		delete this;
		f();
	}
private:
	std::function<void()> func;
};

class VisualizerComponent : public Component
{
public:
	void paint(Graphics& g) override;
	float visline[2][226];
	bool isStereo;
};

class coolKnob : public LookAndFeel_V4
{
public:
	void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, Slider& slider) override;
protected:
	Image img;
};
class brightKnob : public coolKnob
{
public:
	brightKnob();
};
class darkKnob : public coolKnob
{
public:
	darkKnob();
};

class CreditsComponent : public Component
{
private:
	Image img;
	float opacity = 0;
	bool visible = false;
	void updateOpacity();
public:
	CreditsComponent();
	void paint(Graphics& g) override;
	void mouseEnter(const MouseEvent& e) override;
	void mouseExit(const MouseEvent& e) override;
};

