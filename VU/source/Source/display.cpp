/*
  ==============================================================================

    display.cpp
    Created: 6 Jul 2021 3:41:33pm
    Author:  unplugred

  ==============================================================================
*/

#include "display.h"

displayComponent::displayComponent()
{
	multiplier = 1.f/Decibels::decibelsToGain((float)nominal);

	if(auto* peer = getPeer()) peer->setCurrentRenderingEngine(0);
	openGLContext.setRenderer(this);
	openGLContext.attachTo(*this);
}
displayComponent::~displayComponent()
{
	openGLContext.detach();
}

void displayComponent::newOpenGLContextCreated() {
	vushader.reset(new OpenGLShaderProgram(openGLContext));
	vushader->addVertexShader(vertshader());
	vushader->addFragmentShader(fragshader());
	vushader->link();

	vutex.loadImage(ImageCache::getFromMemory(BinaryData::map_png, BinaryData::map_pngSize));
	vutex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	mptex.loadImage(ImageCache::getFromMemory(BinaryData::txtmap_png, BinaryData::txtmap_pngSize));
	mptex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	lgtex.loadImage(ImageCache::getFromMemory(BinaryData::genuine_soundware_png, BinaryData::genuine_soundware_pngSize));
	lgtex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	openGLContext.extensions.glGenBuffers(0, &arraybuffer);
}
void displayComponent::renderOpenGL() {
	openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	vushader->use();
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	vutex.bind();
	vushader->setUniform("vutex", 0);
	openGLContext.extensions.glActiveTexture(GL_TEXTURE1);
	mptex.bind();
	vushader->setUniform("mptex", 1);
	openGLContext.extensions.glActiveTexture(GL_TEXTURE2);
	lgtex.bind();
	vushader->setUniform("lgtex", 2);

	vushader->setUniform("rotation", (leftvu-.5f)*(1.47f-stereodamp*.025f));
	vushader->setUniform("peak", leftpeaklerp);
	vushader->setUniform("right", 0.f);
	vushader->setUniform("lgsize",
		((float)getWidth())/lgtex.getWidth(),
		((float)getHeight())/lgtex.getHeight(),
		164.f/lgtex.getWidth(),
		(62.f/lgtex.getHeight())*(1-settingsfade));

	vushader->setUniform("stereo", stereodamp);
	vushader->setUniform("stereoinv", 2-(1/(stereodamp*.5f+.5f)));
	vushader->setUniform("pause", settingsfade);
	vushader->setUniform("lines", 24.f+nominal, -4.f-damping, hover==3?31.f:(stereo?29.f:30.f));
	vushader->setUniform("lineht", hover==1?1.f:0.f,hover==2?1.f:0.f,1.f,websiteht);

	vushader->setUniform("size", 800.f/vutex.getWidth(), 475.f/vutex.getHeight());
	vushader->setUniform("txtsize", 384.f/vutex.getWidth(), 354.f/vutex.getHeight());
	
	openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_DYNAMIC_DRAW);
	openGLContext.extensions.glEnableVertexAttribArray(0);
	openGLContext.extensions.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (stereodamp > .001) {
		vushader->setUniform("rotation", (rightvu-.5f)*1.445f);
		vushader->setUniform("peak", rightpeaklerp);
		vushader->setUniform("right", 1.f);
		vushader->setUniform("lgsize",
			((float)getWidth())/lgtex.getWidth()*(1/(stereodamp+1)),
			((float)getHeight())/lgtex.getHeight(),
			164.f/lgtex.getWidth()-(stereodamp/152)*getHeight(),
			(62.f/lgtex.getHeight())*(1-settingsfade));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	openGLContext.extensions.glDisableVertexAttribArray(0);
}
void displayComponent::openGLContextClosing() {
	vushader->release();
	vutex.release();
	mptex.release();
	lgtex.release();
	openGLContext.extensions.glDeleteBuffers(1, &arraybuffer);
}

void displayComponent::update()
{
	leftvu = functions::smoothdamp(leftvu,fmin(leftrms*multiplier,1),&leftvelocity,damping*.05f,-1,.03333f);
	if(stereo) rightvu = functions::smoothdamp(rightvu,fmin(rightrms*multiplier,1),&rightvelocity,damping*.05f,-1,.03333f);

	leftpeaklerp = functions::smoothdamp(leftpeaklerp,leftpeak?1:0,&leftpeakvelocity,0.027f,-1,.03333f);
	rightpeaklerp = functions::smoothdamp(rightpeaklerp,rightpeak?1:0,&leftpeakvelocity,0.027f,-1,.03333f);
	if(leftpeaklerp >= .999 && ++lefthold >= 2) {
		leftpeak = false;
		lefthold = 0;
	}
	if(rightpeaklerp >= .999 && ++righthold >= 2) {
		rightpeak = false;
		righthold = 0;
	}

	settingsfade = functions::smoothdamp(settingsfade,fmin(settingstimer,1),&settingsvelocity,0.3,-1,.03333f);
	settingstimer = held?60:fmax(settingstimer-1,0);
	stereodamp = functions::smoothdamp(stereodamp,stereo?1:0,&stereovelocity,0.3,-1,.03333f);
	websiteht -= .05f;

	openGLContext.triggerRepaint();
}
void displayComponent::mouseEnter(const MouseEvent& event) {
	settingstimer = 120;
}
void displayComponent::mouseMove(const MouseEvent& event) {
	settingstimer = fmax(settingstimer,60);
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == 4 && prevhover != 4 && websiteht < -.6) websiteht = 0.6;
}
void displayComponent::mouseExit(const MouseEvent& event) {
	if(!held) settingstimer = 0;
}
void displayComponent::mouseDown(const MouseEvent& event) {
	held = true;
	initialdrag = hover;
	if(hover == 1) initialvalue = nominal;
	else if(hover == 2) initialvalue = damping;
}
void displayComponent::mouseDrag(const MouseEvent& event) {
	if(initialdrag == 3) hover = recalchover(event.x,event.y)==3?3:0;
	else if(initialdrag == 4) hover = recalchover(event.x,event.y)==4?4:0;
	else if(hover != 0) {
		float val = (event.getDistanceFromDragStartY()+event.getDistanceFromDragStartX())*-.04f;
		val = initialvalue-(val>0?floor(val):ceil(val));
		if(hover == 1) {
			nominal = fmin(fmax(val,-24),-6);
			multiplier = 1.f/Decibels::decibelsToGain((float)nominal);
		} else damping = fmin(fmax(val,1),9);
	}
}
void displayComponent::mouseUp(const MouseEvent& event) {
	if(hover == 3) stereo = !stereo;
	else if(hover == 4) URL("https://vst.unplug.red/").launchInDefaultBrowser();
	held = false;
	hover = recalchover(event.x,event.y);
}
int displayComponent::recalchover(float x, float y) {
	float xx = (x/getWidth()-.5f)*8*3.8*(stereo+1);
	float yy = (y/getHeight()-.5f)*(7.4f);
	if(xx >= 1 && ((xx <= 8 && nominal <= -10) || xx <= 7) && yy > -1.5 && yy <= -.5)
		return 1;
	if(xx >= 1 && xx <= 4 && yy > -.5 && yy <= .5)
		return 2;
	if((yy > .5 && yy <= 1.5) && ((!stereo && xx >= -8 && xx <= -2) || (stereo && xx >= -1 && xx <= 3)))
		return 3;
	if(x >= (getWidth()-151) && y >= (getHeight()-49) && x < (getWidth()-1) && y < (getHeight()-1))
		return 4;
	return 0;
}
