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

	multiplier = 1.f/Decibels::decibelsToGain((float)audioProcessor.nominal.get());
	stereodamp = audioProcessor.stereo.get()?1:0;

	openGLContext.setRenderer(this);
	openGLContext.attachTo(*this);

	setResizable(true,true);
	int h = audioProcessor.height.get();
	int w = round(h*((stereodamp>.5f?64.f:32.f)/19.f));
	audioProcessor.width = w;
	setSize(w,h);
	setResizeLimits(3,2,3200,950);
	//getConstrainer()->setFixedAspectRatio((audioProcessor.stereo?64.f:32.f)/19.f);

	startTimerHz(30);
}

VuAudioProcessorEditor::~VuAudioProcessorEditor() {
	stopTimer();
	openGLContext.detach();
}

void VuAudioProcessorEditor::resized() {
	int w = getWidth();
	int h = getHeight();
	if(h != audioProcessor.height.get() && h != prevh) {
		prevw = w;
		w = round(h*((32.f+stereodamp*32.f)/19.f));
	} else if(w != audioProcessor.width.get() && w != prevw) {
		prevh = h;
		h = round(w*((19.f-stereodamp*9.5f)/32.f));
	} else return;

	audioProcessor.width = w;
	audioProcessor.height = h;

	/*
	audioProcessor.width = getWidth();
	audioProcessor.height = getHeight();
	displaycomp.setBounds(0,0,getWidth(),getHeight());
	*/
}

void VuAudioProcessorEditor::newOpenGLContextCreated() {
	vertshader =
R"(#version 330 core
in vec2 aPos;
uniform float rotation;
uniform float right;
uniform float stereo;
uniform float stereoinv;
uniform vec2 size;
uniform vec4 lgsize;
out vec2 v_TexCoord;
out vec2 metercoords;
out vec2 txtcoords;
out vec2 lgcoords;
void main(){
	metercoords = aPos;
	if(stereo <= .001) {
		gl_Position = vec4(aPos*2-1,0,1);
		v_TexCoord = (aPos+vec2(2,0))*size;
		txtcoords = aPos;
	} else if(right < .5) {
		gl_Position = vec4(aPos*2-1,0,1);
		v_TexCoord = aPos*vec2(1+stereo,1)*size;
		txtcoords = vec2(aPos.x*(1+stereo)-.5*stereo,aPos.y);
		metercoords.x = metercoords.x*(1+stereo)-.0387*stereo;
	} else {
		gl_Position = vec4((aPos.x+1)*(2-stereoinv)-1,aPos.y*2-1,0,1);
		v_TexCoord = (aPos+vec2(1,0))*size;
		txtcoords = aPos+vec2(1-stereo*.5,0);
		metercoords.x+=.0465;
	}
	metercoords = (metercoords-.5)*vec2(32./19.,1)+vec2(0,.5);
	metercoords.y += stereoinv*.06+.04;
	metercoords = vec2(metercoords.x*cos(rotation)-metercoords.y*sin(rotation)+.5,metercoords.x*sin(rotation)+metercoords.y*cos(rotation));
	metercoords.y -= stereoinv*.06+.04;
	if(right < .5) lgcoords = aPos*lgsize.xy+vec2(lgsize.z-lgsize.x,lgsize.w);
	else lgcoords = aPos*lgsize.xy+vec2(lgsize.z,lgsize.w);
})";
	fragshader =
R"(#version 330 core
in vec2 v_TexCoord;
in vec2 metercoords;
in vec2 txtcoords;
in vec2 lgcoords;
uniform float peak;
uniform vec2 size;
uniform vec2 txtsize;
uniform vec3 lines;
uniform float stereo;
uniform vec4 lineht;
uniform float right;
uniform float pause;
uniform sampler2D vutex;
uniform sampler2D mptex;
uniform sampler2D lgtex;
void main(){

	if(peak >= .999) gl_FragColor = texture2D(vutex,v_TexCoord+vec2(0,1-size.y*2));
	else {
		gl_FragColor = texture2D(vutex,v_TexCoord+vec2(0,1-size.y));
		if(peak > .001) gl_FragColor = gl_FragColor*(1-peak)+texture2D(vutex,v_TexCoord+vec2(0,1-size.y*2))*peak;
	}
	vec4 mask = texture2D(vutex,v_TexCoord+vec2(0,1-size.y*3));
	vec4 ids = texture2D(vutex,v_TexCoord+vec2(0,1-size.y*4));
	if(right < .5 && stereo > .001 && stereo < .999) {
		vec3 single = texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y)).rgb;
		if(peak >= .999) single = texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y*2)).rgb;
		else single = single*(1-peak)+texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y*2)).rgb*peak;
		gl_FragColor = v_TexCoord.x>size.x?gl_FragColor:(gl_FragColor*stereo+vec4(single,1.)*(1-stereo));
		mask = mask*stereo+texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y*3))*(1-stereo);
		ids = ids*stereo+texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y*4))*(1-stereo);
	}

	vec3 meter = vec3(0.);
	if(metercoords.x > .001 && metercoords.y > .001 && metercoords.x < .999 && metercoords.y < .999)
		meter = texture2D(vutex,min(max(metercoords,0),1)*vec2(size.x*.59375,size.y)+vec2(size.x*3,1-size.y)).rgb*ids.b;
	vec3 shadow = vec3(1.);
	if(metercoords.x > .016 && metercoords.y > -.014 && metercoords.x < 1.14 && metercoords.y < .984)
		shadow = 1-texture2D(vutex,min(max(metercoords+vec2(-.015,.015),0),1)*vec2(size.x*.59375,size.y)+vec2(size.x*3,1-size.y*2)).rgb*ids.r*.15;
	gl_FragColor = vec4(gl_FragColor.rgb*(1-meter)*shadow+mask.rgb*meter,1.);

	if(pause > .001) {
		vec2 txdiv = vec2(3.8,7.4);
		float bg = ids.g*pause;
		if(stereo > .001) bg = texture2D(vutex,vec2(min(max(v_TexCoord.x+size.x*(2-stereo*.5),size.x*2),size.x*3),v_TexCoord.y+(1-size.y*4))).g*pause;
		if(abs(txtcoords.x-.5) <= 1/txdiv.x && abs(txtcoords.y-.5) <= 1.5/txdiv.y) {
			bool highlight = false;
			float line = 0;
			if(abs(txtcoords.y-.5) > .5/txdiv.y) {
				if(txtcoords.y>.5) {
					line = 1.5+lines.x;
					highlight = lineht.x>=.5;
				} else {
					line = -.5+lines.z;
					highlight = lineht.z>=.5;
				}
			} else {
				line = .5+lines.y;
				highlight = lineht.y>=.5;
			}
			vec3 txcoordss = texture2D(mptex,((txtcoords-.5)*txdiv-vec2(1,line))*vec2(.5,.03125)).rgb;
			float tx = texture2D(vutex,(txcoordss.rg+mod(((txtcoords-.5)*txdiv*vec2(8,1)-vec2(0,.5)),1.)*vec2(.125,.25))*txtsize+vec2(size.x*3,1-size.y*3)).g*(1-txcoordss.b*(highlight?.4:0));

			gl_FragColor = abs(gl_FragColor-bg)*(1-tx)+max(gl_FragColor-bg,0)*tx;
		} else gl_FragColor = abs(gl_FragColor-bg);

		if(lgcoords.x > 0 && lgcoords.y > 0 && lgcoords.y < 1) {
			vec2 lg = texture2D(lgtex,lgcoords).rb;
			if(lineht.w > -1) gl_FragColor = gl_FragColor*(1-lg.g)+(1-(1-vec4(1.,.4549,.2588,1.))*(1-texture2D(lgtex,lgcoords+vec2(lineht.w,0)).g))*lg.r;
			else gl_FragColor = gl_FragColor*(1-lg.g)+vec4(1.,.4549,.2588,1.)*lg.r;
		}
	}
})";

	vushader.reset(new OpenGLShaderProgram(openGLContext));
	if(
		!vushader->addVertexShader(vertshader) ||
		!vushader->addFragmentShader(fragshader) ||
		!vushader->link()) {
		DBG(vushader->getLastError());
	}

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

	openGLContext.extensions.glGenBuffers(1, &arraybuffer);
}
void VuAudioProcessorEditor::renderOpenGL() {
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);

	openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
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
	vushader->setUniform("lines", 24.f+audioProcessor.nominal.get(), -4.f-audioProcessor.damping.get(), hover==3?31.f:(audioProcessor.stereo.get()?29.f:30.f));
	vushader->setUniform("lineht", hover==1?1.f:0.f,hover==2?1.f:0.f,1.f,websiteht);

	vushader->setUniform("size", 800.f/vutex.getWidth(), 475.f/vutex.getHeight());
	vushader->setUniform("txtsize", 384.f/vutex.getWidth(), 354.f/vutex.getHeight());

	openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	auto coord = openGLContext.extensions.glGetAttribLocation(vushader->getProgramID(),"aPos");
	openGLContext.extensions.glEnableVertexAttribArray(coord);
	openGLContext.extensions.glVertexAttribPointer(coord, 2, GL_FLOAT, GL_FALSE, 0, 0);
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

	openGLContext.extensions.glDisableVertexAttribArray(coord);
}
void VuAudioProcessorEditor::openGLContextClosing() {
	vushader->release();
	vutex.release();
	mptex.release();
	lgtex.release();
	openGLContext.extensions.glDeleteBuffers(1, &arraybuffer);
}
void VuAudioProcessorEditor::paint(Graphics& g) {
}

void VuAudioProcessorEditor::timerCallback() {
	int buffercount = audioProcessor.buffercount.get();
	if(buffercount > 0) {
		leftrms = sqrt(audioProcessor.leftvu.get()/buffercount);
		rightrms = sqrt(audioProcessor.rightvu.get()/buffercount);
		audioProcessor.leftvu = 0;
		audioProcessor.rightvu = 0;
		audioProcessor.buffercount = 0;

		if(audioProcessor.leftpeak.get()) {
			leftpeak = true;
			audioProcessor.leftpeak = false;
		}
		if(audioProcessor.rightpeak.get()) {
			rightpeak = true;
			audioProcessor.rightpeak = false;
		}
	}

	float damping = audioProcessor.damping.get()*.05f;
	bool stereo = audioProcessor.stereo.get();
	leftvu = functions::smoothdamp(leftvu,fmin(leftrms*multiplier,1),&leftvelocity,damping,-1,.03333f);
	if(stereo) rightvu = functions::smoothdamp(rightvu,fmin(rightrms*multiplier,1),&rightvelocity,damping,-1,.03333f);
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

	int h = audioProcessor.height.get();
	int w = 0;
	if (stereodamp > .001 && stereodamp < .999) {
		w = round(h*((32.f+stereodamp*32.f)/19.f));
		audioProcessor.width = w;
	} else w = audioProcessor.width.get();
	setSize(w,h);
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

	openGLContext.triggerRepaint();
}

void VuAudioProcessorEditor::mouseEnter(const MouseEvent& event) {
	settingstimer = 120;
}
void VuAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	settingstimer = fmax(settingstimer,60);
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == 4 && prevhover != 4 && websiteht < -.6) websiteht = 0.6f;
}
void VuAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	if(!held) settingstimer = 0;
}
void VuAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	held = true;
	initialdrag = hover;
	if(hover == 1 || hover == 2) {
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
		if(hover == 1) {
			initialvalue = audioProcessor.nominal.get();
			audioProcessor.apvts.getParameter("nominal")->beginChangeGesture();
		} else {
			initialvalue = audioProcessor.damping.get();
			audioProcessor.apvts.getParameter("damping")->beginChangeGesture();
		}
	}
}
void VuAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(initialdrag == 3) hover = recalchover(event.x,event.y)==3?3:0;
	else if(initialdrag == 4) hover = recalchover(event.x,event.y)==4?4:0;
	else if(hover != 0) {
		float val = (event.getDistanceFromDragStartY()+event.getDistanceFromDragStartX())*-.04f;
		int clampval = initialvalue-(val>0?floor(val):ceil(val));
		if(hover == 1) {
			clampval = fmin(fmax(clampval,-24),-6);
			if(audioProcessor.nominal.get() != clampval) {
				audioProcessor.nominal = clampval;
				multiplier = 1.f/Decibels::decibelsToGain((float)clampval);
				audioProcessor.apvts.getParameter("nominal")->setValueNotifyingHost(clampval);
			}
		} else {
			clampval = fmin(fmax(clampval,1),9);
			if(audioProcessor.damping.get() != clampval) {
				audioProcessor.damping = clampval;
				audioProcessor.apvts.getParameter("damping")->setValueNotifyingHost(clampval);
			}
		}
	}
}
void VuAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(hover == 1 || hover == 2) {
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
		audioProcessor.apvts.getParameter(hover==1?"nominal":"damping")->endChangeGesture();
	} else if(hover == 3) {
		bool stereo = !audioProcessor.stereo.get();
		audioProcessor.stereo = stereo;
		audioProcessor.apvts.getParameter("stereo")->setValueNotifyingHost(stereo);
	} else if(hover == 4) URL("https://vst.unplug.red/").launchInDefaultBrowser();
	held = false;
	hover = recalchover(event.x,event.y);
}
int VuAudioProcessorEditor::recalchover(float x, float y) {
	bool stereo = audioProcessor.stereo.get();
	float xx = (x/getWidth()-.5f)*8*3.8*(stereo+1);
	float yy = (y/getHeight()-.5f)*(7.4f);
	if(xx >= 1 && ((xx <= 8 && audioProcessor.nominal.get() <= -10) || xx <= 7) && yy > -1.5 && yy <= -.5)
		return 1;
	if(xx >= 1 && xx <= 4 && yy > -.5 && yy <= .5)
		return 2;
	if((yy > .5 && yy <= 1.5) && ((!stereo && xx >= -8 && xx <= -2) || (stereo && xx >= -1 && xx <= 3)))
		return 3;
	if(x >= (getWidth()-151) && y >= (getHeight()-49) && x < (getWidth()-1) && y < (getHeight()-1))
		return 4;
	return 0;
}
