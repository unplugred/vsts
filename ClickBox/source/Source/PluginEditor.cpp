/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

ClickBoxAudioProcessorEditor::ClickBoxAudioProcessorEditor (ClickBoxAudioProcessor& p) : AudioProcessorEditor (&p), audioProcessor (p) {
	audioProcessor.apvts.addParameterListener("intensity",this);
	sliders[0].id = "intensity";
	sliders[0].name = "Intensity";
	sliders[0].value = audioProcessor.apvts.getParameter("intensity")->getValue();
	sliders[0].hy = 116;
	sliders[0].hh = 146;
	sliders[0].y = .4453125f;

	audioProcessor.apvts.addParameterListener("amount",this);
	sliders[1].id = "amount";
	sliders[1].name = "Amount";
	sliders[1].value = audioProcessor.apvts.getParameter("amount")->getValue();
	sliders[1].hy = 151;
	sliders[1].hw = 126;
	sliders[1].hh = 181;
	sliders[1].y = .58203125f;
	sliders[1].w = .5;

	audioProcessor.apvts.addParameterListener("stereo",this);
	sliders[2].id = "stereo";
	sliders[2].name = "Stereo";
	sliders[2].value = audioProcessor.apvts.getParameter("stereo")->getValue();
	sliders[2].hx = 130;
	sliders[2].hy = 151;
	sliders[2].hh = 181;
	sliders[2].x = .5;
	sliders[2].y = .58203125f;
	sliders[2].w = .5;

	audioProcessor.apvts.addParameterListener("sidechain",this);
	sliders[3].id = "sidechain";
	sliders[3].name = "Side-chain to dry";
	sliders[3].value = audioProcessor.apvts.getParameter("sidechain")->getValue();
	sliders[3].isslider = false;
	sliders[3].hy = 186;
	sliders[3].hh = 216;
	sliders[3].y = .71875f;

	audioProcessor.apvts.addParameterListener("dry",this);
	sliders[4].id = "dry";
	sliders[4].name = "Dry out";
	sliders[4].value = audioProcessor.apvts.getParameter("dry")->getValue();
	sliders[4].isslider = false;
	sliders[4].hy = 221;
	sliders[4].hw = 126;
	sliders[4].hh = 251;
	sliders[4].y = .85546875f;
	sliders[4].h = .14453125f;
	sliders[4].w = .5;

	audioProcessor.apvts.addParameterListener("auto",this);
	sliders[5].id = "auto";
	sliders[5].name = "Auto";
	sliders[5].value = audioProcessor.apvts.getParameter("auto")->getValue();
	sliders[5].hx = 130;
	sliders[5].hy = 221;
	sliders[5].hh = 251;
	sliders[5].x = .5;
	sliders[5].y = .85546875f;
	sliders[5].w = .5;
	sliders[5].h = .14453125f;

	for(int i = 0; i < 6; i++) {
		if(sliders[i].isslider) sliders[i].coloffset = random.nextFloat();
		float r = sliders[i].isslider?(sliders[i].value*3*sliders[i].w+sliders[i].coloffset):random.nextFloat();
		sliders[i].r = getr(r);
		sliders[i].g = getg(r);
		sliders[i].b = getb(r);
	}

	mousecolor = (int)floor(random.nextFloat()*6);
	prevpos[0].col = (int)fmod(mousecolor+1+floor(random.nextFloat()*5),6);

	for(int i = 0; i < 8; i++) randoms[i] = random.nextFloat();

	setSize (256, 256);
	setResizable(false,false);

	openGLContext.setRenderer(this);
	openGLContext.attachTo(*this);

	startTimerHz(30);
}
ClickBoxAudioProcessorEditor::~ClickBoxAudioProcessorEditor() {
	audioProcessor.apvts.removeParameterListener("intensity",this);
	audioProcessor.apvts.removeParameterListener("amount",this);
	audioProcessor.apvts.removeParameterListener("stereo",this);
	audioProcessor.apvts.removeParameterListener("sidechain",this);
	audioProcessor.apvts.removeParameterListener("dry",this);
	audioProcessor.apvts.removeParameterListener("auto",this);
	stopTimer();
	openGLContext.detach();
}

void ClickBoxAudioProcessorEditor::newOpenGLContextCreated() {
	clearvert =
R"(#version 330 core
in vec2 aPos;
void main() {
	gl_Position = vec4(aPos*2-1,0,1);
})";
	clearfrag =
R"(#version 330 core
void main() {
	gl_FragColor = vec4(.10546875,.10546875,.10546875,.15);
})";
	clearshader.reset(new OpenGLShaderProgram(openGLContext));
	clearshader->addVertexShader(clearvert);
	clearshader->addFragmentShader(clearfrag);
	clearshader->link();

	slidervert =
R"(#version 330 core
in vec2 aPos;
uniform vec4 texscale;
uniform vec2 margin;
out vec2 texcoord;
out float sliderpos;
void main() {
	texcoord = vec2(aPos.x*texscale.z+texscale.x,1-((1-aPos.y)*texscale.w+texscale.y));
	gl_Position = vec4(texcoord*2-1,0,1);
	sliderpos = aPos.x*margin.x-margin.y;
})";
	sliderfrag =
R"(#version 330 core
in vec2 texcoord;
in float sliderpos;
uniform sampler2D tex;
uniform vec3 col;
uniform float value;
uniform float hover;
void main() {
	vec2 t = texture2D(tex,texcoord).rb;
	vec3 color = col;
	float alpha = 0;

	if(sliderpos <= value) alpha = t.g;
	else alpha = t.r;
	if(hover > .5 && t.r > .5 && t.g < .5) {
		alpha = 1;
		color += .8;
	}

	gl_FragColor = vec4(color,alpha);
})";
	slidershader.reset(new OpenGLShaderProgram(openGLContext));
	slidershader->addVertexShader(slidervert);
	slidershader->addFragmentShader(sliderfrag);
	slidershader->link();

	creditsvert =
R"(#version 330 core
in vec2 aPos;
uniform float texscale;
uniform vec3 rot;
out vec2 texcoord;
out vec2 shadercoord;
void main() {
	gl_Position = vec4(aPos*vec2(2,2*texscale)-1,0,1);
	shadercoord = (gl_Position.xy+vec2(0,.5))*1.2;
	shadercoord = vec2(
		shadercoord.x*cos(rot.z)-shadercoord.y*sin(rot.z),
		shadercoord.x*sin(rot.z)+shadercoord.y*cos(rot.z));
	texcoord = gl_Position.xy*.5+.5;
})";
	creditsfrag =
R"(#version 330 core
in vec2 texcoord;
in vec2 shadercoord;
uniform sampler2D tex;
uniform float htpos;
uniform vec3 rot;
uniform float lineoffset;
uniform vec3 color;
uniform float htback;
void main() {
	vec4 map = texture2D(tex,texcoord);

	if(map.a < .5) {
		gl_FragColor = vec4(color+htback,1);
	} else if(map.r > .5) {
		float ht = 0;
		if(map.b > .5 && (texcoord.x - htpos) < 1 && (texcoord.x - htpos) > 0)
			ht = texture2D(tex,texcoord-vec2(htpos,0)).g;

		if(ht > .5) {
			gl_FragColor = vec4(color+.8,1);
		} else {
			vec3 ro = vec3(rot.x,0,rot.y);
			vec3 cu = normalize(cross(-ro,vec3(sin(.35),cos(.35),0)));
			vec3 rd = normalize(mat3(cu,normalize(cross(cu,-ro)),ro)*vec3(shadercoord,1));
			float col = 0;
			float b = dot(rd,ro);
			float d = b*b-(dot(ro,ro)-1);
			if(d > 0) {
				d = sqrt(abs(d));
				col = mod((.5-asin(ro.x+rd.x*(d-b))/3.14159265359)*6+lineoffset,1)>.9?1:0;
				col = max(mod((.5-asin(ro.x+rd.x*(-d-b))/3.14159265359)*6+lineoffset,1)>.9?1:0,col);
			}

			if(map.b < .5) col = 1-col;
			gl_FragColor = vec4(col>.5?color:vec3(.10546875),1);
		}
	} else gl_FragColor = vec4(0);
})";
	creditsshader.reset(new OpenGLShaderProgram(openGLContext));
	creditsshader->addVertexShader(creditsvert);
	creditsshader->addFragmentShader(creditsfrag);
	creditsshader->link();

	ppvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 texcoord;
void main() {
	gl_Position = vec4(aPos*2-1,0,1);
	texcoord = aPos;
})";
	ppfrag =
R"(#version 330 core
in vec2 texcoord;
uniform sampler2D tex;
uniform sampler2D noisetex;
uniform float intensity;
uniform vec4 randomsone;
uniform vec4 randomstwo;
uniform float randomsblend;
void main() {
	vec4 col = texture2D(tex,texcoord);

	if(col.r > .2 || col.g > .2 || col.b > .2) {
		float noise = texture2D(noisetex,texcoord+randomsone.xy).g;
		noise += texture2D(noisetex,texcoord+randomsone.zw).g;
		noise += texture2D(noisetex,texcoord+randomstwo.xy).g*(1-randomsblend)+texture2D(noisetex,texcoord+randomstwo.zw).g*randomsblend;
		noise /= 3;

		vec2 coord = texcoord;
		if(mod(coord.x*3482+coord.y*43928,1)>.5) {
		if(noise < intensity) coord.x -= .00390625;
			else if(noise > (1-intensity)) coord.x += .00390625;
		} else {
			if(noise < intensity) coord.y -= .00390625;
			else if(noise > (1-intensity)) coord.y += .00390625;
		}

		col = texture2D(tex,coord);
	}

	gl_FragColor = vec4(col.rgb*col.a+.10546875*(1-col.a),1);
})";
	ppshader.reset(new OpenGLShaderProgram(openGLContext));
	ppshader->addVertexShader(ppvert);
	ppshader->addFragmentShader(ppfrag);
	ppshader->link();

	cursorvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 pos;
uniform float automated;
out vec2 texcoord;
out vec2 basecoord;
void main() {
	gl_Position = vec4(aPos.x*.125-1.03125+pos.x,aPos.y*.125+.90625-pos.y,0,1);
	texcoord = aPos*vec2(.5,1)+vec2(automated*.5,0);
	basecoord = gl_Position.xy*.5+.5;
})";
	cursorfrag =
R"(#version 330 core
in vec2 texcoord;
in vec2 basecoord;
uniform sampler2D tex;
uniform sampler2D base;
uniform vec3 col;
uniform float clamp;
void main() {
	if(clamp > .5 && basecoord.y < .5546875) gl_FragColor = vec4(0);
	else {
		vec2 texx = texture2D(tex,texcoord).rg;
		if(clamp > .5) texx.g *= texture2D(base,basecoord).b;
		gl_FragColor = vec4(texx.g>.5?col:(clamp<.5?(col+.8):vec3(.10546875)),texx.r>.5?1:0);
	}
})";
	cursorshader.reset(new OpenGLShaderProgram(openGLContext));
	cursorshader->addVertexShader(cursorvert);
	cursorshader->addFragmentShader(cursorfrag);
	cursorshader->link();

	slidertex.loadImage(ImageCache::getFromMemory(BinaryData::tex_png,BinaryData::tex_pngSize));
	slidertex.bind();
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

	creditstex.loadImage(ImageCache::getFromMemory(BinaryData::credits_png,BinaryData::credits_pngSize));
	creditstex.bind();
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

	framebuffer.initialise(openGLContext,getWidth(),getHeight());
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

	cursortex.loadImage(ImageCache::getFromMemory(BinaryData::cursor_png,BinaryData::cursor_pngSize));
	cursortex.bind();
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

	openGLContext.extensions.glGenBuffers(1,&arraybuffer);
}
void ClickBoxAudioProcessorEditor::renderOpenGL() {
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER,arraybuffer);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	framebuffer.makeCurrentRenderingTarget();
	auto coord = openGLContext.extensions.glGetAttribLocation(clearshader->getProgramID(),"aPos");
	openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,square,GL_DYNAMIC_DRAW);
	clearshader->use();
	openGLContext.extensions.glEnableVertexAttribArray(coord);
	openGLContext.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	openGLContext.extensions.glDisableVertexAttribArray(coord);

	slidershader->use();
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	slidertex.bind();
	slidershader->setUniform("tex",0);
	slidershader->setUniform("value",0.f);
	slidershader->setUniform("texscale",0.f,0.f,1.f,.4453125f);
	slidershader->setUniform("col",.23828125f,.23828125f,.23828125f);
	slidershader->setUniform("margin",0.f);
	slidershader->setUniform("hover",0.f);
	coord = openGLContext.extensions.glGetAttribLocation(slidershader->getProgramID(),"aPos");
	openGLContext.extensions.glEnableVertexAttribArray(coord);
	openGLContext.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	if(!credits) {
		for(int i = 0; i < 6; i++) {
			slidershader->setUniform("texscale",sliders[i].x,sliders[i].y,sliders[i].w,sliders[i].h);
			slidershader->setUniform("col",sliders[i].r,sliders[i].g,sliders[i].b);
			slidershader->setUniform("value",sliders[i].value);
			slidershader->setUniform("hover",hover==i?1.f:0.f);
			if(sliders[i].w < .75f) {
				if(sliders[i].x > .25f)
					slidershader->setUniform("margin",1.0859375f,.03125f);
				else
					slidershader->setUniform("margin",1.0859375f,.0546875f);
			} else
				slidershader->setUniform("margin",1.0546875f,.02734375f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	} else {
		openGLContext.extensions.glDisableVertexAttribArray(coord);

		creditsshader->use();
		openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
		creditstex.bind();
		creditsshader->setUniform("tex",0);
		creditsshader->setUniform("texscale",.5546875f);
		creditsshader->setUniform("htpos",websiteht);
		creditsshader->setUniform("lineoffset",shadertime*.5f);
		creditsshader->setUniform("rot",
			sin(shadertime*.37f)*3,
			cos(shadertime*.37f)*3,
			shadertime*.13f);
		creditsshader->setUniform("color",getr(shadertime*.02f),getg(shadertime*.02f),getb(shadertime*.02f));
		creditsshader->setUniform("htback",hover==-4?.8f:0.f);

		coord = openGLContext.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
		openGLContext.extensions.glEnableVertexAttribArray(coord);
		openGLContext.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		openGLContext.extensions.glDisableVertexAttribArray(coord);
	}
	openGLContext.extensions.glDisableVertexAttribArray(coord);

	openGLContext.extensions.glBindFramebuffer (GL_FRAMEBUFFER, openGLContext.getFrameBufferID());

	ppshader->use();
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,framebuffer.getTextureID());
	ppshader->setUniform("tex",0);
	openGLContext.extensions.glActiveTexture(GL_TEXTURE1);
	slidertex.bind();
	ppshader->setUniform("noisetex",1);
	ppshader->setUniform("intensity",ppamount);
	ppshader->setUniform("randomsone",randoms[0],randoms[1],randoms[2],randoms[3]);
	ppshader->setUniform("randomstwo",randoms[4],randoms[5],randoms[6],randoms[7]);
	ppshader->setUniform("randomsblend",randomsblend);
	coord = openGLContext.extensions.glGetAttribLocation(ppshader->getProgramID(),"aPos");
	openGLContext.extensions.glEnableVertexAttribArray(coord);
	openGLContext.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	openGLContext.extensions.glDisableVertexAttribArray(coord);

	cursorshader->use();
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	cursortex.bind();
	cursorshader->setUniform("tex",0);
	openGLContext.extensions.glActiveTexture(GL_TEXTURE1);
	slidertex.bind();
	cursorshader->setUniform("base",1);
	cursorshader->setUniform("clamp",1.f);
	coord = openGLContext.extensions.glGetAttribLocation(cursorshader->getProgramID(),"aPos");
	openGLContext.extensions.glEnableVertexAttribArray(coord);
	openGLContext.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);

	for(int i = 6; i >= 0; i--) if(prevpos[i].x > -100 && (i != 6 || prevpos[i].automated)) {
		cursorshader->setUniform("col",colors[prevpos[i].col*3],colors[prevpos[i].col*3+1],colors[prevpos[i].col*3+2]);
		cursorshader->setUniform("pos",prevpos[i].x*2,prevpos[i].y*2);
		cursorshader->setUniform("automated",prevpos[i].automated?1.f:0.f);
		if(i == 0) cursorshader->setUniform("clamp",0.f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}

	openGLContext.extensions.glDisableVertexAttribArray(coord);
}
void ClickBoxAudioProcessorEditor::openGLContextClosing() {
	slidershader->release();
	cursorshader->release();
	creditsshader->release();
	ppshader->release();

	slidertex.release();
	cursortex.release();
	creditstex.release();
	framebuffer.release();

	openGLContext.extensions.glDeleteBuffers(1,&arraybuffer);
}
void ClickBoxAudioProcessorEditor::paint (Graphics& g) {}
void ClickBoxAudioProcessorEditor::resized() {}

void ClickBoxAudioProcessorEditor::timerCallback() {
	shadertime += .02f;
	websiteht -= .05f;

	randomsblend += .03f;
	if(randomsblend >= 1) {
		randomsblend = fmod(randomsblend,1.f);
		int rnd = floor(random.nextFloat()*3)*2;
		randoms[4] = randoms[rnd];
		randoms[5] = randoms[rnd+1];
		randoms[rnd] = randoms[6];
		randoms[rnd+1] = randoms[7];
		randoms[6] = random.nextFloat();
		randoms[7] = random.nextFloat();
	}
	ppamount = ppamount*.7f + .3f*fmin(sqrt(sqrt(audioProcessor.oldi.get()*20)),.4f);

	if(!overridee) {
		prevpos[1].x = floor(audioProcessor.x.get()*246+5)/256.f;
		prevpos[1].y = floor(audioProcessor.y.get()*106+5)/256.f;
		prevpos[1].automated = true;
		if(prevpos[1].x == prevpos[2].x && prevpos[1].y == prevpos[2].y && prevpos[2].automated == true)
			prevpos[2].x = -1000;
		else
			mousecolor = (mousecolor+1)%6;
		prevpos[1].col = mousecolor;
	} else if(prevpos[0].x == prevpos[1].x && prevpos[0].y == prevpos[1].y && prevpos[1].automated == false) {
		prevpos[1].x = -1000;
	} else {
		mousecolor = (mousecolor+1)%6;
		prevpos[0].col = mousecolor;
	}

	openGLContext.triggerRepaint();

	for (int i = 6; i > 0; i--) {
		prevpos[i].x = prevpos[i-1].x;
		prevpos[i].y = prevpos[i-1].y;
		prevpos[i].automated = prevpos[i-1].automated;
		prevpos[i].col = prevpos[i-1].col;
	}
}

void ClickBoxAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	int i = -1;
	if(parameterID=="intensity")i=0;
	else if(parameterID=="amount")i=1;
	else if(parameterID=="stereo")i=2;
	else if(parameterID=="sidechain")i=3;
	else if(parameterID=="dry")i=4;
	else if(parameterID=="auto")i=5;
	else return;
	sliders[i].value = newValue;

	float r = sliders[i].isslider?(sliders[i].value*3*sliders[i].w+sliders[i].coloffset):random.nextFloat();
	sliders[i].r = getr(r);
	sliders[i].g = getg(r);
	sliders[i].b = getb(r);
}
void ClickBoxAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	prevpos[0].x = event.x*.00390625f;
	prevpos[0].y = event.y*.00390625f;
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht < -.78515625) websiteht = .16015625f;
}
void ClickBoxAudioProcessorEditor::mouseEnter(const MouseEvent& event) {
	setMouseCursor(MouseCursor::NoCursor);
}
void ClickBoxAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	setMouseCursor(MouseCursor::NormalCursor);
	prevpos[0].x = -1000;
	hover = -1;
}
void ClickBoxAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(event.mods.isRightButtonDown()) {
		credits = !credits;
		hover = recalchover(event.x,event.y);
		return;
	}
	initialdrag = hover;
	if(hover > -1) {
		initialvalue = sliders[hover].value;
		audioProcessor.undoManager.beginNewTransaction();
		audioProcessor.apvts.getParameter(sliders[hover].id)->beginChangeGesture();
		if(sliders[hover].isslider)
			audioProcessor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(((float)event.x-sliders[hover].hx-2)/(sliders[hover].hw-sliders[hover].hx-4));
		else
			audioProcessor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(1-initialvalue);
	} else if(hover == -2) {
		audioProcessor.undoManager.beginNewTransaction();
		overridee = true;
		audioProcessor.apvts.getParameter("override")->setValueNotifyingHost(1.f);
		audioProcessor.apvts.getParameter("x")->setValueNotifyingHost((event.x-5)*.0040650407f);
		audioProcessor.apvts.getParameter("y")->setValueNotifyingHost((event.y-5)*.0094339623f);
		mousecolor = prevpos[0].col;
	}
}
void ClickBoxAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(event.mods.isRightButtonDown()) return;
	prevpos[0].x = event.x*.00390625f;
	prevpos[0].y = event.y*.00390625f;
	if(initialdrag > -1) {
		if(sliders[initialdrag].isslider)
			audioProcessor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(((float)event.x-sliders[hover].hx-2)/(sliders[hover].hw-sliders[hover].hx-4));
		else {
			hover = recalchover(event.x,event.y)==initialdrag?initialdrag:-1;
			audioProcessor.apvts.getParameter(sliders[initialdrag].id)->setValueNotifyingHost(hover==-1?initialvalue:(1-initialvalue));
		}
	} else if(hover == -2) {
		audioProcessor.apvts.getParameter("x")->setValueNotifyingHost((event.x-5)*.0040650407f);
		audioProcessor.apvts.getParameter("y")->setValueNotifyingHost((event.y-5)*.0094339623f);
	} else if(initialdrag == -3) {
		int prevhover = hover;
		hover = recalchover(event.x,event.y)==-3?-3:-1;
		if(initialdrag == -3 && hover == -3 && prevhover != -3 && websiteht < -.78515625) websiteht = .16015625;
	} else if(initialdrag == -4) {
		hover = recalchover(event.x,event.y)==-4?-4:-1;
	}
}
void ClickBoxAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(event.mods.isRightButtonDown()) return;
	if(hover > -1) {
		audioProcessor.undoManager.setCurrentTransactionName(
			(String)((sliders[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += sliders[hover].name);
		audioProcessor.apvts.getParameter(sliders[hover].id)->endChangeGesture();
		audioProcessor.undoManager.beginNewTransaction();
	} else if(hover == -2) {
		overridee = false;
		audioProcessor.apvts.getParameter("override")->setValueNotifyingHost(0.f);
		audioProcessor.undoManager.setCurrentTransactionName("Altered XY");
		audioProcessor.undoManager.beginNewTransaction();
	} else if(hover == -3) {
		URL("https://vst.unplug.red/").launchInDefaultBrowser();
	} else if(hover == -4) {
		credits = false;
	}
}
void ClickBoxAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	if(!sliders[hover].isslider) return;
	audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += sliders[hover].name);
	audioProcessor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(
		audioProcessor.apvts.getParameter(sliders[hover].id)->getDefaultValue());
	audioProcessor.undoManager.beginNewTransaction();
}
void ClickBoxAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	if(!sliders[hover].isslider) return;
	audioProcessor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(
		sliders[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));

}
int ClickBoxAudioProcessorEditor::recalchover(float x, float y) {
	if(x>=5 && x<=251 && y>=5 && y<=111) return -2;
	if (credits) {
		if(x>=55 && x<=201 && y>=161 && y<=205) return -3;
		if(x>=189 && x<=253 && y>=240 && y<=253) return -4;
	} else for(int i = 0; i < 6; i++) {
		if( x >= sliders[i].hx &&
			x <= sliders[i].hw &&
			y >= sliders[i].hy &&
			y <= sliders[i].hh)
			return hover = i;
	}
	return -1;
}
float ClickBoxAudioProcessorEditor::getr(float hue) {
	float h = hue*6;
	float m = fmod(h,1.f);
	return colors[(int)fmod(floor(h),6)*3]*(1-m)+colors[(int)fmod(ceil(h),6)*3]*m;
}
float ClickBoxAudioProcessorEditor::getg(float hue) {
	float h = hue*6;
	float m = fmod(h,1.f);
	return colors[(int)fmod(floor(h),6)*3+1]*(1-m)+colors[(int)fmod(ceil(h),6)*3+1]*m;
}
float ClickBoxAudioProcessorEditor::getb(float hue) {
	float h = hue*6;
	float m = fmod(h,1.f);
	return colors[(int)fmod(floor(h),6)*3+2]*(1-m)+colors[(int)fmod(ceil(h),6)*3+2]*m;
}