/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VuAudioProcessorEditor::VuAudioProcessorEditor (VuAudioProcessor& p)
	: AudioProcessorEditor (&p), audioProcessor (p) {

	setResizable(true,true);
	audioProcessor.width = round(audioProcessor.height*((audioProcessor.stereo?64.f:32.f)/19.f));
	setSize(audioProcessor.width, audioProcessor.height);
	setResizeLimits(3,2,3200,950);
	//getConstrainer()->setFixedAspectRatio((audioProcessor.stereo?64.f:32.f)/19.f);

	addAndMakeVisible(displaycomp);
	displaycomp.stereo = audioProcessor.stereo;
	displaycomp.stereodamp = audioProcessor.stereo?1:0;
	displaycomp.damping = audioProcessor.damping;
	displaycomp.nominal = audioProcessor.nominal;
	displaycomp.setBounds(0,0,getWidth(),getHeight());

	startTimerHz(30);

}

VuAudioProcessorEditor::~VuAudioProcessorEditor() {
}

void VuAudioProcessorEditor::timerCallback() {
	if(audioProcessor.buffercount > 0) {
		displaycomp.leftrms = sqrt(audioProcessor.leftvu/audioProcessor.buffercount);
		displaycomp.rightrms = sqrt(audioProcessor.rightvu/audioProcessor.buffercount);
		if(audioProcessor.leftpeak) displaycomp.leftpeak = true;
		if(audioProcessor.rightpeak) displaycomp.rightpeak = true;
	}
	displaycomp.update();

	if (audioProcessor.stereo != displaycomp.stereo) {
		audioProcessor.stereo = displaycomp.stereo;
		audioProcessor.apvts.getParameter("stereo")->setValueNotifyingHost(audioProcessor.stereo);
	}
	if (audioProcessor.damping != displaycomp.damping) {
		audioProcessor.damping = displaycomp.damping;
		audioProcessor.apvts.getParameter("damping")->setValueNotifyingHost(audioProcessor.damping);
	}
	if (audioProcessor.nominal != displaycomp.nominal) {
		audioProcessor.nominal = displaycomp.nominal;
		audioProcessor.apvts.getParameter("nominal")->setValueNotifyingHost(audioProcessor.nominal);
	}
	audioProcessor.buffercount = 0;
	audioProcessor.leftvu = 0;
	audioProcessor.rightvu = 0;
	audioProcessor.leftpeak = 0;
	audioProcessor.rightpeak = 0;

	if (displaycomp.stereodamp > .001 && displaycomp.stereodamp < .999) {
		audioProcessor.width = round(audioProcessor.height*((32.f+displaycomp.stereodamp*32.f)/19.f));
		displaycomp.setBounds(0,0,audioProcessor.width,audioProcessor.height);
	}
	setSize(audioProcessor.width,audioProcessor.height);
	/*
	if (displaycomp.stereodamp > .001 && displaycomp.stereodamp < .999) {
		if(getConstrainer()->getFixedAspectRatio() != 0) getConstrainer()->setFixedAspectRatio(0);
		setSize(round(getHeight()*((32.f+displaycomp.stereodamp*32.f)/19.f)),getHeight());
		audioProcessor.width = getWidth();
		audioProcessor.height = getHeight();
	} else {
		double ratio = (audioProcessor.stereo?64.f:32.f)/19.f;
		if(getConstrainer()->getFixedAspectRatio() != ratio)
			getConstrainer()->setFixedAspectRatio(ratio);
	}
	*/
}

//==============================================================================
void VuAudioProcessorEditor::paint (Graphics& g) {
}

void VuAudioProcessorEditor::resized() {
	int w = getWidth();
	int h = getHeight();
	if(h != audioProcessor.height && h != prevh) {
		prevw = w;
		w = round(h*((32.f+displaycomp.stereodamp*32.f)/19.f));
	} else if(w != audioProcessor.width && w != prevw) {
		prevh = h;
		h = round(w*((19.f-displaycomp.stereodamp*9.5f)/32.f));
	} else return;

	audioProcessor.width = w;
	audioProcessor.height = h;
	displaycomp.setBounds(0,0,w,h);

	/*
	audioProcessor.width = getWidth();
	audioProcessor.height = getHeight();
	displaycomp.setBounds(0,0,getWidth(),getHeight());
	*/
}
