/*
  ==============================================================================

	Knobs.cpp
	Created: 27 Dec 2020 7:13:10am
	Author:	 ari

  ==============================================================================
*/

#include "components.h"

void VisualizerComponent::paint(Graphics &g)
{
	Path vis;
	for (int c = 0; c < (isStereo ? 2 : 1); c++) {
		vis.startNewSubPath(0,visline[c][0]);
		for (int i = 1; i < 226; i++) {
			vis.lineTo(i,visline[c][i]);
		}
	}

	g.setGradientFill(ColourGradient(
		Colour::fromRGBA(255, 255, 255, 100), 113, 40, 
		Colour::fromRGBA(255, 255, 255, 60), 0, 40, true));
	g.strokePath(vis, PathStrokeType(1.3, PathStrokeType::curved, PathStrokeType::rounded));
}

void coolKnob::drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, Slider& slider)
{
	float calc = ((slider.getValue() - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()) - .5) * 4.7;
	AffineTransform knobpos;
	g.drawImageTransformed(img, knobpos.rotation(calc, 29, 42).translated(x+13,y));
}
brightKnob::brightKnob()
{
	img = ImageCache::getFromMemory(BinaryData::knob1_png, BinaryData::knob1_pngSize);
}
darkKnob::darkKnob()
{
	img = ImageCache::getFromMemory(BinaryData::knob2_png, BinaryData::knob2_pngSize);
}

CreditsComponent::CreditsComponent()
{
	img = ImageCache::getFromMemory(BinaryData::credits_png, BinaryData::credits_pngSize);
}
void CreditsComponent::paint(Graphics &g)
{
	g.setOpacity(opacity);
	g.drawImage(img,0,0,242,59,0,0,242,59);
}
void CreditsComponent::mouseEnter(const MouseEvent &e)
{
	visible = true;
	if(opacity <= 0)
		new DelayedOneShotLambda(50.f/3.f, [this]() { updateOpacity(); });
}
void CreditsComponent::mouseExit(const MouseEvent &e)
{
	visible = false;
	if(opacity >= 1)
		new DelayedOneShotLambda(50.f/3.f, [this]() { updateOpacity(); });
}
void CreditsComponent::updateOpacity() {
	if ((visible && opacity >= 1) || (!visible && opacity <= 0)) {
		opacity = visible ? 1 : 0;
		return;
	}
	opacity += .07*(visible?1:-1);
	new DelayedOneShotLambda(50.f/3.f, [this]() { updateOpacity(); });
	repaint();
}
