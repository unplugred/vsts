/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

PFAudioProcessorEditor::PFAudioProcessorEditor (PFAudioProcessor& p)
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	knobs[0].id = "freq";
	knobs[0].name = "frequency";
	knobs[0].value = audioProcessor.freq;
	knobs[1].id = "fat";
	knobs[1].name = "fatness";
	knobs[1].value = audioProcessor.fat*.025f+.5f;
	knobs[2].id = "drive";
	knobs[2].name = "drive";
	knobs[2].value = audioProcessor.drive;
	knobs[3].id = "dry";
	knobs[3].name = "dry";
	knobs[3].value = audioProcessor.dry;
	knobs[4].id = "stereo";
	knobs[4].name = "stereo";
	knobs[4].value = audioProcessor.stereo;
	knobs[5].id = "gain";
	knobs[5].name = "gain";
	knobs[5].value = audioProcessor.gain;

	for (int x = 0; x < 2; x++) {
		for (int y = 0; y < 3; y++) {
			knobs[x+y*2].x = x*106+68;
			knobs[x+y*2].y = y*100+144;
			audioProcessor.apvts.addParameterListener(knobs[x+y*2].id, this);
		}
	}

	calcvis();
	setSize (242, 462);

	setOpaque(true);
	openGLContext.setRenderer(this);
	openGLContext.attachTo(*this);

	startTimerHz(30);
}
PFAudioProcessorEditor::~PFAudioProcessorEditor() {
	stopTimer();
	openGLContext.detach();
}

void PFAudioProcessorEditor::newOpenGLContextCreated() {
	basevert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 v_TexCoord;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	v_TexCoord = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})";
	basefrag =
R"(#version 330 core
in vec2 v_TexCoord;
uniform sampler2D basetex;
void main(){
	gl_FragColor = vec4(vec3(texture2D(basetex,v_TexCoord).r),1);
})";
	baseshader.reset(new OpenGLShaderProgram(openGLContext));
	baseshader->addVertexShader(basevert);
	baseshader->addFragmentShader(basefrag);
	baseshader->link();

	knobvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 basescale;
uniform float knobrot;
uniform vec2 knobscale;
uniform vec2 knobpos;
uniform float ratio;
out vec2 v_TexCoord;
out vec2 hovercoord;
out vec2 basecoord;
void main(){
	vec2 pos = (aPos*2*knobscale-vec2(knobscale.x,knobscale.y*.727272727))/vec2(ratio,1);
	float rot = mod(knobrot,1)*-6.2831853072;
	gl_Position = vec4(
		(pos.x*cos(rot)-pos.y*sin(rot))*ratio-1+knobpos.x,
		pos.x*sin(rot)+pos.y*cos(rot)-1+knobpos.y,0,1);
	//noisecoord = (gl_Position.xy*texscale*.5)/knobscale;
	v_TexCoord = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	hovercoord = (aPos-vec2(.5,.3636363636))/vec2(1.1379310345,1);
	hovercoord = vec2(
		(hovercoord.x*cos(rot*2)-hovercoord.y*sin(rot*2))*1.1379310345+.5,
		hovercoord.x*sin(rot*2)+hovercoord.y*cos(rot*2)+.36363636);
	hovercoord = vec2(hovercoord.x*texscale.x,1-(1-hovercoord.y)*texscale.y);
	basecoord = vec2((gl_Position.x*.5+.5)*basescale.x,1-(.5-gl_Position.y*.5)*basescale.y);
})";
	knobfrag =
R"(#version 330 core
in vec2 v_TexCoord;
in vec2 hovercoord;
in vec2 basecoord;
uniform float knobcolor;
uniform float knobrot;
uniform vec2 texscale;
uniform sampler2D knobtex;
uniform sampler2D basetex;
uniform float id;
uniform float hoverstate;
void main(){
	float index = floor(knobrot*6);
	gl_FragColor = texture2D(knobtex,v_TexCoord-vec2(0,texscale.y*mod(index,6)));
	if(gl_FragColor.r > 0) {
		float lerp = mod(knobrot*6,1);
		float col = gl_FragColor.b*(1-lerp)+texture2D(knobtex,v_TexCoord-vec2(0,texscale.y*mod(index+1,6))).b*lerp;
		if(col<.5) col = knobcolor*col*2;
		else col = knobcolor+(col-.5)*.8;
		if(hoverstate != 0) {
			float hover = texture2D(knobtex,hovercoord-vec2(0,texscale.y*id)).g;
			if(hoverstate < -1) col += (hoverstate==-3?(1-hover):hover)*.3;
			else col = col*(1-hover)+.01171875*hover;
		}
		gl_FragColor = vec4(vec3(col+texture2D(basetex,basecoord).g-.5),gl_FragColor.r);
	} else gl_FragColor = vec4(0);
})";
	knobshader.reset(new OpenGLShaderProgram(openGLContext));
	knobshader->addVertexShader(knobvert);
	knobshader->addFragmentShader(knobfrag);
	knobshader->link();

	visvert = 
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 gradcoord;
out vec2 v_TexCoord;
void main(){
	gl_Position = vec4(aPos,0,1);
	//(.5-(80/2+8)/462)*2
	gradcoord = (aPos-vec2(0,.7922077922))*vec2(1.0708,2.04425);
	v_TexCoord = vec2((aPos.x*.5+.5)*texscale.x,1-(.5-aPos.y*.5)*texscale.y);
})";
	visfrag = 
R"(#version 330 core
in vec2 gradcoord;
in vec2 v_TexCoord;
uniform sampler2D basetex;
void main(){
	if(gradcoord.x < 1 && gradcoord.x > -1 && gradcoord.y < .3539823009 && gradcoord.y > -.3539823009) {
		float gradient = .390625-sqrt(gradcoord.x*gradcoord.x+gradcoord.y*gradcoord.y)*.15625;
		gl_FragColor = vec4(vec3(texture2D(basetex,v_TexCoord).r*(1-gradient)+gradient),1);
	} else gl_FragColor = vec4(0);
})";
	visshader.reset(new OpenGLShaderProgram(openGLContext));
	visshader->addVertexShader(visvert);
	visshader->addFragmentShader(visfrag);
	visshader->link();

	creditsvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 v_TexCoord;
void main(){
	gl_Position = vec4(aPos.x*2-1,1-(1-aPos.y*(59./462.))*2,0,1);
	v_TexCoord = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})";
	creditsfrag =
R"(#version 330 core
in vec2 v_TexCoord;
uniform sampler2D creditstex;
uniform float alpha;
uniform float shineprog;
void main(){
	vec2 creditols = texture2D(creditstex,v_TexCoord).rb;
	gl_FragColor = vec4(vec3(creditols.g+texture2D(creditstex,v_TexCoord+vec2(shineprog,0)).g*creditols.r*.8),alpha);
})";
	creditsshader.reset(new OpenGLShaderProgram(openGLContext));
	creditsshader->addVertexShader(creditsvert);
	creditsshader->addFragmentShader(creditsfrag);
	creditsshader->link();

	ppvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 shake;
out vec2 v_TexCoord;
void main(){
	gl_Position = vec4((aPos+shake)*2-1,0,1);
	v_TexCoord = aPos;
})";
	ppfrag =
R"(#version 330 core
in vec2 v_TexCoord;
uniform sampler2D buffertex;
uniform float chroma;
void main(){
	gl_FragColor = vec4(
		texture2D(buffertex,v_TexCoord+chroma).r,
		texture2D(buffertex,v_TexCoord).g,
		texture2D(buffertex,v_TexCoord-chroma).b,1.);
})";
	ppshader.reset(new OpenGLShaderProgram(openGLContext));
	ppshader->addVertexShader(ppvert);
	ppshader->addFragmentShader(ppfrag);
	ppshader->link();

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize));
	basetex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	knobtex.loadImage(ImageCache::getFromMemory(BinaryData::knob_png, BinaryData::knob_pngSize));
	knobtex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	creditstex.loadImage(ImageCache::getFromMemory(BinaryData::credits_png, BinaryData::credits_pngSize));
	creditstex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	openGLContext.extensions.glGenBuffers(1, &arraybuffer);
}
void PFAudioProcessorEditor::renderOpenGL() {
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	baseshader->use();
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	baseshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
	openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	auto coord = openGLContext.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	openGLContext.extensions.glEnableVertexAttribArray(coord);
	openGLContext.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	openGLContext.extensions.glDisableVertexAttribArray(coord);

	knobshader->use();
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	knobtex.bind();
	knobshader->setUniform("knobtex",0);
	openGLContext.extensions.glActiveTexture(GL_TEXTURE1);
	basetex.bind();
	knobshader->setUniform("basetex",1);
	knobshader->setUniform("texscale",58.f/knobtex.getWidth(),66.f/knobtex.getHeight());
	knobshader->setUniform("knobscale",58.f/getWidth(),66.f/getHeight());
	knobshader->setUniform("ratio",((float)getHeight())/getWidth());
	knobshader->setUniform("basescale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
	openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	coord = openGLContext.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	openGLContext.extensions.glEnableVertexAttribArray(coord);
	openGLContext.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	knobshader->setUniform("knobcolor",.2265625f);
	for (int i = 0; i < 6; i++) {
		if(i == 2) knobshader->setUniform("knobcolor",.61328125f);
		knobshader->setUniform("knobpos",((float)knobs[i].x*2)/getWidth(),2-((float)knobs[i].y*2)/getHeight());
		knobshader->setUniform("knobrot",(knobs[i].value-.5f)*.748f);
		knobshader->setUniform("id",(float)i);
		knobshader->setUniform("hoverstate",(float)(knobs[i].hoverstate==-1?(hover==i?-1:0):knobs[i].hoverstate));
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	openGLContext.extensions.glDisableVertexAttribArray(coord);

	glLineWidth(1.3);
	visshader->use();
	coord = openGLContext.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	visshader->setUniform("basetex",0);
	visshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
	openGLContext.extensions.glEnableVertexAttribArray(coord);
	openGLContext.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*452, visline[0], GL_DYNAMIC_DRAW);
	glDrawArrays(GL_LINE_STRIP,0,226);
	if(isStereo) {
		openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*452, visline[1], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINE_STRIP,0,226);
	}
	openGLContext.extensions.glDisableVertexAttribArray(coord);

	creditsshader->use();
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	creditstex.bind();
	creditsshader->setUniform("creditstex",0);
	creditsshader->setUniform("texscale",242.f/creditstex.getWidth(),59.f/creditstex.getHeight());
	creditsshader->setUniform("alpha",creditsalpha<0.5?4*creditsalpha*creditsalpha*creditsalpha:1-(float)pow(-2*creditsalpha+2,3)/2);
	creditsshader->setUniform("shineprog",websiteht);
	openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	coord = openGLContext.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
	openGLContext.extensions.glEnableVertexAttribArray(coord);
	openGLContext.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	openGLContext.extensions.glDisableVertexAttribArray(coord);

	openGLContext.extensions.glDisableVertexAttribArray(coord);
}
void PFAudioProcessorEditor::openGLContextClosing() {
	baseshader->release();
	knobshader->release();
	visshader->release();
	creditsshader->release();
	ppshader->release();

	basetex.release();
	knobtex.release();
	creditstex.release();

	openGLContext.extensions.glDeleteBuffers(1,&arraybuffer);
}
void PFAudioProcessorEditor::calcvis() {
	isStereo = audioProcessor.stereo > 0 && audioProcessor.dry < 1 && audioProcessor.gain > 0;
	for(int c = 0; c < (isStereo ? 2 : 1); c++) {
		for(int i = 0; i < 226; i++) {
			visline[c][i*2] = (i+8)/121.f-1;
			visline[c][i*2+1] = 1-(48+audioProcessor.plasticfuneral(sin(i/35.8098621957f)*.8f,c,audioProcessor.freq,audioProcessor.fat,audioProcessor.drive,audioProcessor.dry,audioProcessor.stereo,audioProcessor.gain)*audioProcessor.norm*38)/231.f;
		}
	}
}
void PFAudioProcessorEditor::paint (Graphics& g) { }

void PFAudioProcessorEditor::timerCallback() {
	creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.07f:-.07f),1),0);
	if(creditsalpha <= 0) websiteht = -1;
	websiteht -= .05;

	for(int i = 0; i < 6; i++) knobs[i].hoverstate = fmin(knobs[i].hoverstate+1,-1);
	if(held > 0) held--;

	openGLContext.triggerRepaint();
}

void PFAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "freq") {
		audioProcessor.freq = newValue;
		knobs[0].value = newValue;
	} else if(parameterID == "fat") {
		audioProcessor.fat = newValue;
		knobs[1].value = newValue*.025f+.5f;
		audioProcessor.normalizegain();
	} else if(parameterID == "drive") {
		audioProcessor.drive = newValue;
		knobs[2].value = newValue;
	} else if(parameterID == "dry") {
		audioProcessor.dry = newValue;
		knobs[3].value = newValue;
		audioProcessor.normalizegain();
	} else if(parameterID == "stereo") {
		audioProcessor.stereo = newValue;
		knobs[4].value = newValue;
	} else if(parameterID == "gain") {
		audioProcessor.gain = newValue;
		knobs[5].value = newValue;
	}
	calcvis();
}
void PFAudioProcessorEditor::mouseEnter(const MouseEvent& event) {
//dlt
}
void PFAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
	if(prevhover != hover && held == 0) {
		if(hover > -1) knobs[hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
	}
}
void PFAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void PFAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	held = -1;
	initialdrag = hover;
	if(hover > -1) {
		initialvalue = knobs[hover].value;
		audioProcessor.undoManager.beginNewTransaction();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void PFAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(hover == -1) return;
	if(initialdrag > -1) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost
			(fmin(fmax(initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f),0),1));
	} else if(initialdrag == -2) {
		int draghover = recalchover(event.x,event.y);
		hover = (draghover>=-2||draghover<=-3)?-2:0;
	} else if (initialdrag == -3) {
		int prevhover = hover;
		hover = recalchover(event.x,event.y)==-3?-3:-2;
		if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
	}
}
void PFAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(hover > -1) {
		audioProcessor.undoManager.setCurrentTransactionName(
			(String)((knobs[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
		audioProcessor.undoManager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		if(hover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		int prevhover = hover;
		hover = recalchover(event.x,event.y);
		if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
	}
	held = 1;
}
int PFAudioProcessorEditor::recalchover(float x, float y) {
	if(y > 403) {
		if(x >= 50 && x <= 196 && y >= 412 && y <= 456) return -3;
		return -2;
	}
	float xx = 0, yy = 0;
	for(int i = 0; i < 6; i++) {
		xx = knobs[i].x-x;
		yy = knobs[i].y-y;
		if((xx*xx+yy*yy)<=576) return i;
	}
	return -1;
}
