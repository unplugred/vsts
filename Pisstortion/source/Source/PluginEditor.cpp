/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace gl;

PisstortionAudioProcessorEditor::PisstortionAudioProcessorEditor (PisstortionAudioProcessor& p, int paramcount, pluginpreset state, potentiometer pots[])
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 3; y++) {
			int i = x+y*2;
			knobs[i].x = x*106+68;
			knobs[i].y = y*100+144;
			knobs[i].id = pots[i].id;
			knobs[i].name = pots[i].name;
			if(pots[i].smoothtime > 0)
				knobs[i].value = pots[i].normalize(pots[i].smooth.getTargetValue());
			else
				knobs[i].value = pots[i].normalize(state.values[i]);
			knobs[i].minimumvalue = pots[i].minimumvalue;
			knobs[i].maximumvalue = pots[i].maximumvalue;
			knobs[i].defaultvalue = pots[i].defaultvalue;
			knobcount++;
			audioProcessor.apvts.addParameterListener(knobs[i].id,this);
		}
	}
	for(int i = 0; i < paramcount; i++)
		if(pots[i].id == "oversampling") {
			oversampling = state.values[i];
	}
	oversamplinglerped = oversampling;
	audioProcessor.apvts.addParameterListener("oversampling",this);
	calcvis();

	for (int i = 0; i < 20; i++) {
		bubbleregen(i);
		bubbles[i].wiggleage = random.nextFloat();
		bubbles[i].moveage = random.nextFloat();
	}

	setSize (242, 462);
	setResizable(false, false);

	setOpaque(true);
	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
PisstortionAudioProcessorEditor::~PisstortionAudioProcessorEditor() {
	for (int i = 0; i < 6; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	audioProcessor.apvts.removeParameterListener("oversampling",this);
	stopTimer();
	context.detach();
}

void PisstortionAudioProcessorEditor::newOpenGLContextCreated() {
	basevert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 circlescale;
out vec2 v_TexCoord;
out vec2 circlecoord;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	v_TexCoord = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	circlecoord = vec2(aPos.x*circlescale.x,1-(1-aPos.y)*circlescale.y);
})";
	basefrag =
R"(#version 330 core
in vec2 v_TexCoord;
in vec2 circlecoord;
uniform sampler2D circletex;
uniform sampler2D basetex;
void main(){
	float bubbles = texture2D(circletex,circlecoord).r;
	vec3 c = texture2D(basetex,v_TexCoord).rgb;
	if(c.g >= 1) c.b = 0;
	if(bubbles > 0 && v_TexCoord.y > .7)
		c = vec3(c.r,c.g*(1-bubbles),c.b+c.g*bubbles);
	float gradient = (1-v_TexCoord.y)*.5+bubbles*.3;
	float grayscale = c.g*.95+c.b*.85+(1-c.r-c.g-c.b)*.05;
	gl_FragColor = vec4(vec3(grayscale)+c.r*gradient+c.r*(1-gradient)*vec3(0.984,0.879,0.426),1);
})";
	baseshader.reset(new OpenGLShaderProgram(context));
	baseshader->addVertexShader(basevert);
	baseshader->addFragmentShader(basefrag);
	baseshader->link();

	knobvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 knobscale;
uniform vec2 circlescale;
uniform float ratio;
uniform vec2 knobpos;
uniform float knobrot;
out vec2 v_TexCoord;
out vec2 circlecoord;
void main(){
	vec2 pos = ((aPos*2-1)*knobscale)/vec2(ratio,1);
	float rot = mod(knobrot-.125,1)*-6.2831853072;
	gl_Position = vec4(
		(pos.x*cos(rot)-pos.y*sin(rot))*ratio-1+knobpos.x,
		pos.x*sin(rot)+pos.y*cos(rot)-1+knobpos.y,0,1);
	v_TexCoord = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	circlecoord = vec2((gl_Position.x*.5+.5)*circlescale.x,1-(.5-gl_Position.y*.5)*circlescale.y);
})";
	knobfrag =
R"(#version 330 core
in vec2 v_TexCoord;
in vec2 circlecoord;
uniform sampler2D knobtex;
uniform sampler2D circletex;
uniform float hover;
void main(){
	vec3 c = texture2D(knobtex,v_TexCoord).rgb;
	if(c.r > 0) {
		float bubbles = texture2D(circletex,circlecoord).r;
		float col = max(min(c.g*4-(1-hover)*3,1),0);
		col = (1-col)*bubbles+col;
		col = .05 + c.b*.8 + col*.1;
		gl_FragColor = vec4(vec3(col),c.r);
	} else gl_FragColor = vec4(0);
})";
	knobshader.reset(new OpenGLShaderProgram(context));
	knobshader->addVertexShader(knobvert);
	knobshader->addFragmentShader(knobfrag);
	knobshader->link();

	visvert = 
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 basecoord;
void main(){
	gl_Position = vec4(aPos,0,1);
	basecoord = vec2((aPos.x+1)*texscale.x*.5,1-(1-aPos.y)*texscale.y*.5);
})";
	visfrag = 
R"(#version 330 core
in vec2 basecoord;
uniform sampler2D basetex;
uniform float alpha;
void main(){
	float base = texture2D(basetex,basecoord).r;
	gl_FragColor = vec4(.05,.05,.05,base <= 0 ? alpha : 0);
})";
	visshader.reset(new OpenGLShaderProgram(context));
	visshader->addVertexShader(visvert);
	visshader->addFragmentShader(visfrag);
	visshader->link();

	oversamplingvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
uniform float selection;
out vec2 basecoord;
out vec2 highlightcoord;
void main(){
	gl_Position = vec4(aPos.x-.5,aPos.y*.2+.7,0,1);
	basecoord = vec2((aPos.x+.5)*.5*texscale.x,1-(1.5-aPos.y)*.1*texscale.y);
	highlightcoord = vec2((aPos.x-selection)*4.3214285714,aPos.y*3.3-1.1642857143);
})";
	oversamplingfrag =
R"(#version 330 core
in vec2 basecoord;
in vec2 highlightcoord;
uniform sampler2D basetex;
uniform float alpha;
void main(){
	vec3 tex = texture2D(basetex,basecoord).rgb;
	if(tex.r <= 0) {
		float bleh = (tex.g > .9 && tex.b > .9) ? 1 : 0;
		if(highlightcoord.x>0&&highlightcoord.x<1&&highlightcoord.y>0&&highlightcoord.y<1)bleh=1-bleh;
		gl_FragColor = vec4(.05,.05,.05,bleh*alpha);
	} else {
		gl_FragColor = vec4(0);
	}
})";
	oversamplingshader.reset(new OpenGLShaderProgram(context));
	oversamplingshader->addVertexShader(oversamplingvert);
	oversamplingshader->addFragmentShader(oversamplingfrag);
	oversamplingshader->link();

	creditsvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 basescale;
out vec2 v_TexCoord;
out vec2 basecoord;
void main(){
	gl_Position = vec4(aPos.x*2-1,1-(1-aPos.y*(57./462.))*2,0,1);
	v_TexCoord = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	basecoord = vec2(aPos.x*basescale.x,1-(.5-gl_Position.y*.5)*basescale.y);
})";
	creditsfrag =
R"(#version 330 core
in vec2 v_TexCoord;
in vec2 basecoord;
uniform sampler2D basetex;
uniform sampler2D creditstex;
uniform float alpha;
uniform float shineprog;
void main(){
	float y = (v_TexCoord.y+alpha)*1.1875;
	float creditols = 0;
	float shine = 0;
	vec3 base = texture2D(basetex,basecoord).rgb;
	if(base.r <= 0) {
		if(y < 1)
			creditols = texture2D(creditstex,vec2(v_TexCoord.x,y)).b;
		else if(y > 1.1875 && y < 2.1875) {
			creditols = texture2D(creditstex,vec2(v_TexCoord.x,y-1.1875)).r;
			if(v_TexCoord.x+shineprog < 1 && v_TexCoord.x+shineprog > .582644628099)
				shine = texture2D(creditstex,v_TexCoord+vec2(shineprog,alpha)).g*min(base.g+base.b,1);
		}
	}
	gl_FragColor = vec4(vec3(.05+shine*.8),creditols);
})";
	creditsshader.reset(new OpenGLShaderProgram(context));
	creditsshader->addVertexShader(creditsvert);
	creditsshader->addFragmentShader(creditsfrag);
	creditsshader->link();

	circlevert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 pos;
uniform float ratio;
out vec2 v_TexCoord;
void main(){
	float f = .1;
	gl_Position = vec4((aPos*2-1)*vec2(1,ratio)*f+pos*2-1,0,1);
	v_TexCoord = aPos*2-1;
})";
	circlefrag =
R"(#version 330 core
in vec2 v_TexCoord;
void main(){
	float x = v_TexCoord.x*v_TexCoord.x+v_TexCoord.y*v_TexCoord.y;
	float f = .5;
	gl_FragColor = vec4(1,1,1,(x>(1-(1-f)*.5)?(1-x):(x-f))*100);
})";
	circleshader.reset(new OpenGLShaderProgram(context));
	circleshader->addVertexShader(circlevert);
	circleshader->addFragmentShader(circlefrag);
	circleshader->link();

	framebuffer.initialise(context, getWidth(), getHeight());
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize));
	basetex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	context.extensions.glGenBuffers(1, &arraybuffer);

	audioProcessor.logger.init(&context,getWidth(),getHeight());
}
void PisstortionAudioProcessorEditor::renderOpenGL() {
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	framebuffer.makeCurrentRenderingTarget();
	OpenGLHelpers::clear(Colour::fromRGB(0,0,0));
	circleshader->use();
	circleshader->setUniform("ratio",((float)getWidth())/getHeight());
	coord = context.extensions.glGetAttribLocation(circleshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < 20; i++) {
		circleshader->setUniform("pos",bubbles[i].xoffset+
			sin(bubbles[i].moveage*bubbles[i].movespeed+bubbles[i].moveoffset)*bubbles[i].moveamount+
			sin(bubbles[i].wiggleage)*bubbles[i].wiggleamount,
			bubbles[i].moveage+.05f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);
	framebuffer.releaseAsRenderingTarget();

	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	baseshader->setUniform("circletex",1);
	baseshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
	baseshader->setUniform("circlescale",((float)getWidth())/framebuffer.getWidth(),((float)getHeight())/framebuffer.getHeight());
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	knobshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	knobtex.bind();
	knobshader->setUniform("knobtex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	knobshader->setUniform("circletex",1);
	knobshader->setUniform("texscale",54.f/knobtex.getWidth(),54.f/knobtex.getHeight());
	knobshader->setUniform("knobscale",54.f/getWidth(),54.f/getHeight());
	knobshader->setUniform("circlescale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
	knobshader->setUniform("ratio",((float)getHeight())/getWidth());
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for (int i = 0; i < 6; i++) {
		knobshader->setUniform("knobpos",((float)knobs[i].x*2)/getWidth(),2-((float)knobs[i].y*2)/getHeight());
		knobshader->setUniform("knobrot",(knobs[i].lerpedvalue-.5f)*.748f);
		knobshader->setUniform("hover",knobs[i].hover<0.5?4*knobs[i].hover*knobs[i].hover*knobs[i].hover:1-(float)pow(-2*knobs[i].hover+2,3)/2);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	float osalpha = oversamplingalpha;
	if(oversamplingalpha < 1) {
		if(oversamplingalpha > 0)
			osalpha = oversamplingalpha<0.5?4*oversamplingalpha*oversamplingalpha*oversamplingalpha:1-(float)pow(-2*oversamplingalpha+2,3)/2;
		glLineWidth(1.3);
		visshader->use();
		coord = context.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
		context.extensions.glActiveTexture(GL_TEXTURE0);
		basetex.bind();
		visshader->setUniform("basetex",0);
		visshader->setUniform("alpha",1-osalpha);
		visshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*452, visline[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINE_STRIP,0,226);
		if(isStereo) {
			context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*452, visline[1], GL_DYNAMIC_DRAW);
			glDrawArrays(GL_LINE_STRIP,0,226);
		}
		context.extensions.glDisableVertexAttribArray(coord);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	}

	if(oversamplingalpha > 0) {
		oversamplingshader->use();
		coord = context.extensions.glGetAttribLocation(oversamplingshader->getProgramID(),"aPos");
		context.extensions.glActiveTexture(GL_TEXTURE0);
		basetex.bind();
		oversamplingshader->setUniform("basetex",0);
		oversamplingshader->setUniform("alpha",osalpha);
		oversamplingshader->setUniform("selection",.458677686f+oversamplinglerped*.2314049587f);
		oversamplingshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	creditsshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	context.extensions.glActiveTexture(GL_TEXTURE1);
	creditstex.bind();
	creditsshader->setUniform("basetex",0);
	creditsshader->setUniform("basescale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
	creditsshader->setUniform("creditstex",1);
	creditsshader->setUniform("texscale",242.f/creditstex.getWidth(),48.f/creditstex.getHeight());
	creditsshader->setUniform("alpha",creditsalpha<0.5?4*creditsalpha*creditsalpha*creditsalpha:1-(float)pow(-2*creditsalpha+2,3)/2);
	creditsshader->setUniform("shineprog",websiteht);
	coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	audioProcessor.logger.drawlog();
}
void PisstortionAudioProcessorEditor::openGLContextClosing() {
	circleshader->release();
	baseshader->release();
	knobshader->release();
	visshader->release();
	oversamplingshader->release();
	creditsshader->release();

	basetex.release();
	knobtex.release();
	creditstex.release();

	framebuffer.release();

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void PisstortionAudioProcessorEditor::calcvis() {
	isStereo = knobs[4].value > 0 && knobs[1].value > 0 && knobs[5].value > 0;
	pluginpreset pp;
	for(int i = 0; i < knobcount; i++)
		pp.values[i] = knobs[i].inflate(knobs[i].value);
	for(int c = 0; c < (isStereo ? 2 : 1); c++) {
		for(int i = 0; i < 226; i++) {
			visline[c][i*2] = (i+8)/121.f-1;
			visline[c][i*2+1] = 1-(48+audioProcessor.pisstortion(sin(i/35.8098621957f)*.8f,c,2,pp,false)*38)/231.f;
		}
	}
}
void PisstortionAudioProcessorEditor::paint (Graphics& g) { }

void PisstortionAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < 6; i++) {
		knobs[i].lerpedvalue = knobs[i].lerpedvalue*.6f+knobs[i].value*.4;
		if(knobs[i].hover != (hover==i?1:0))
			knobs[i].hover = fmax(fmin(knobs[i].hover+(hover==i?.07f:-.07f), 1), 0);
	}
	if(held > 0) held--;

	if(oversamplingalpha != (hover<=-4?1:0))
		oversamplingalpha = fmax(fmin(oversamplingalpha+(hover<=-4?.07f:-.07f),1),0);

	float os = oversampling?1:0;
	if(oversamplinglerped != os?1:0) {
		if(fabs(oversamplinglerped-os) <= .001f) oversamplinglerped = os;
		oversamplinglerped = oversamplinglerped*.75f+os*.25f;
	}

	if(creditsalpha != ((hover<=-2&&hover>=-3)?1:0))
		creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.05f:-.05f),1),0);
	if(creditsalpha <= 0) websiteht = -1;
	if(websiteht >= -.227273 && creditsalpha >= .5) websiteht -= .05;

	if(audioProcessor.rmscount.get() > 0) {
		rms = sqrt(audioProcessor.rmsadd.get()/audioProcessor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
	}
	rmslerped = rmslerped*.6f+rms*.4f;
	audioProcessor.rmsadd = 0;
	audioProcessor.rmscount = 0;

	for(int i = 0; i < 20; i++) {
		bubbles[i].moveage += bubbles[i].yspeed*(1+rmslerped*30);
		if(bubbles[i].moveage >= 1)
			bubbleregen(i);
		else
			bubbles[i].wiggleage = fmod(bubbles[i].wiggleage+bubbles[i].wigglespeed,6.2831853072f);
	}

	if (audioProcessor.updatevis.get()) {
		calcvis();
		audioProcessor.updatevis = false;
	}

	context.triggerRepaint();
}
void PisstortionAudioProcessorEditor::bubbleregen(int i) {
	float tradeoff = random.nextFloat();
	bubbles[i].wiggleamount = tradeoff*.05f+random.nextFloat()*.005f;
	bubbles[i].wigglespeed = (1-tradeoff)*.15f+random.nextFloat()*.015f;
	bubbles[i].wiggleage = random.nextFloat()*6.2831853072f;
	bubbles[i].moveamount = random.nextFloat()*.5f;
	bubbles[i].movespeed = random.nextFloat()*5.f;
	bubbles[i].moveoffset = random.nextFloat()*6.2831853072f;
	bubbles[i].moveage = 0;
	bubbles[i].yspeed = random.nextFloat()*.0015f+.00075f;
	bubbles[i].xoffset = random.nextFloat();
}

void PisstortionAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		oversampling = newValue>.5f;
		return;
	}
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		calcvis();
		return;
	}
}
void PisstortionAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
}
void PisstortionAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void PisstortionAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	held = -1;
	initialdrag = hover;
	if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audioProcessor.undoManager.beginNewTransaction();
		audioProcessor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	} else if(hover < -4) {
		oversampling = hover == -6;
		audioProcessor.apvts.getParameter("oversampling")->setValueNotifyingHost(oversampling?1.f:0.f);
		audioProcessor.undoManager.setCurrentTransactionName(
			(String)("Set Over-Sampling to ") += oversampling);
		audioProcessor.undoManager.beginNewTransaction();
	}
}
void PisstortionAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(hover == -1) return;
	if(initialdrag > -1) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset);

		valueoffset = fmax(fmin(valueoffset,value+.1),value-1.1);
	} else if (initialdrag == -3) {
		int prevhover = hover;
		hover = recalchover(event.x,event.y)==-3?-3:-2;
		if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
	}
}
void PisstortionAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(hover > -1) {
		audioProcessor.undoManager.setCurrentTransactionName(
			(String)((knobs[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
		audioProcessor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
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
void PisstortionAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover > -1) {
		audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
		audioProcessor.undoManager.beginNewTransaction();
	}
}
void PisstortionAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover > -1)
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int PisstortionAudioProcessorEditor::recalchover(float x, float y) {
	if (x >= 8 && x <= 234 && y >= 8 && y <= 87) {
		if(y < 39 || y > 53) return -4;
		if(x >= 115 && x <= 143) return -5;
		if(x >= 144 && x <= 172) return -6;
		return -4;
	} else if(y >= 403) {
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
