/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;
using namespace gl;

PFAudioProcessorEditor::PFAudioProcessorEditor (PFAudioProcessor& p, int paramcount, pluginpreset state, potentiometer pots[])
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 3; y++) {
			int i = x+y*2;
			knobs[i].x = x*106+68;
			knobs[i].y = y*100+144;
			knobs[i].id = pots[i].id;
			knobs[i].name = pots[i].name;
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

	setSize (242, 462);
	setResizable(false, false);

	setOpaque(true);
	context.setRenderer(this);
	context.attachTo(*this);

	audioProcessor.debug("Debugging activated");

	startTimerHz(30);
}
PFAudioProcessorEditor::~PFAudioProcessorEditor() {
	for(int i = 0; i < knobcount; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	audioProcessor.apvts.removeParameterListener("oversampling",this);
	stopTimer();
	context.detach();
}

void PFAudioProcessorEditor::newOpenGLContextCreated() {
	basevert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})";
	basefrag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D basetex;
void main(){
	gl_FragColor = vec4(texture2D(basetex,uv).r,0,0,1);
})";
	baseshader.reset(new OpenGLShaderProgram(context));
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
out vec2 uv;
out vec2 hovercoord;
out vec2 basecoord;
void main(){
	vec2 pos = (aPos*2*knobscale-vec2(knobscale.x,knobscale.y*.727272727))/vec2(ratio,1);
	float rot = mod(knobrot,1)*-6.2831853072;
	gl_Position = vec4(
		(pos.x*cos(rot)-pos.y*sin(rot))*ratio-1+knobpos.x,
		pos.x*sin(rot)+pos.y*cos(rot)-1+knobpos.y,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	hovercoord = (aPos-vec2(.5,.3636363636))/vec2(1.1379310345,1);
	hovercoord = vec2(
		(hovercoord.x*cos(rot*2)-hovercoord.y*sin(rot*2))*1.1379310345+.5,
		hovercoord.x*sin(rot*2)+hovercoord.y*cos(rot*2)+.36363636);
	basecoord = vec2((gl_Position.x*.5+.5)*basescale.x,1-(.5-gl_Position.y*.5)*basescale.y);
})";
	knobfrag =
R"(#version 330 core
in vec2 uv;
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
	gl_FragColor = texture2D(knobtex,uv-vec2(0,texscale.y*mod(index,6)));
	if(gl_FragColor.r > 0) {
		float lerp = mod(knobrot*6,1);
		float col = gl_FragColor.b*(1-lerp)+texture2D(knobtex,uv-vec2(0,texscale.y*mod(index+1,6))).b*lerp;
		if(col<.5) col = knobcolor*col*2;
		else col = knobcolor+(col-.5)*.8;
		if(hoverstate != 0) {
			float hover = 0;
			if(hovercoord.x < .95 && hovercoord.x > .05 && hovercoord.y > .005 && hovercoord.y < .78)
				hover = texture2D(knobtex,vec2(hovercoord.x*texscale.x,1-(1-hovercoord.y+id)*texscale.y)).g;
			if(hoverstate < -1) col += (hoverstate==-3?(1-hover):hover)*.3;
			else col = col*(1-hover)+.01171875*hover;
		}
		gl_FragColor = vec4(col+texture2D(basetex,basecoord).g-.5,0,0,gl_FragColor.r);
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
	vec2 base = texture2D(basetex,basecoord).rb;
	float gradient = (base.g<.5?base.g:(1-base.g))*alpha;
	gl_FragColor = vec4(base.r*(1-gradient)+gradient,0,0,1);
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
	highlightcoord = vec2(aPos.x*6.3684210526+selection,aPos.y*3.3-1.1642857143);
})";
	oversamplingfrag =
R"(#version 330 core
in vec2 basecoord;
in vec2 highlightcoord;
uniform sampler2D basetex;
uniform float alpha;
void main(){
	float tex = texture2D(basetex,basecoord).b;
	if(highlightcoord.x>0&&highlightcoord.x<1&&highlightcoord.y>0&&highlightcoord.y<1)tex=1-tex;
	gl_FragColor = vec4(1,0,0,tex>.5?((1-tex)*alpha):0);
})";
	oversamplingshader.reset(new OpenGLShaderProgram(context));
	oversamplingshader->addVertexShader(oversamplingvert);
	oversamplingshader->addFragmentShader(oversamplingfrag);
	oversamplingshader->link();

	creditsvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos.x*2-1,1-(1-aPos.y*(59./462.))*2,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})";
	creditsfrag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D creditstex;
uniform float alpha;
uniform float shineprog;
void main(){
	vec2 creditols = texture2D(creditstex,uv).rb;
	float shine = 0;
	if(uv.x+shineprog < 1 && uv.x+shineprog > .582644628099)
		shine = texture2D(creditstex,uv+vec2(shineprog,0)).g*creditols.r*.8;
	gl_FragColor = vec4(creditols.g+shine,0,0,alpha);
})";
	creditsshader.reset(new OpenGLShaderProgram(context));
	creditsshader->addVertexShader(creditsvert);
	creditsshader->addFragmentShader(creditsfrag);
	creditsshader->link();

	ppvert =
R"(#version 330 core
in vec2 aPos;
uniform float shake;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	uv = aPos+vec2(shake,0);
})";
	ppfrag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D buffertex;
uniform float chroma;
void main(){
	gl_FragColor = vec4(vec3((
		texture2D(buffertex,uv+vec2(chroma,0)).r+
		texture2D(buffertex,uv-vec2(chroma,0)).r+
		texture2D(buffertex,uv).r)*.333333333),1.);
})";
	ppshader.reset(new OpenGLShaderProgram(context));
	ppshader->addVertexShader(ppvert);
	ppshader->addFragmentShader(ppfrag);
	ppshader->link();

#ifdef ENABLE_CONSOLE
	textvert = 
R"(#version 330 core
in vec2 aPos;
uniform vec4 size;
uniform vec2 pos;
uniform float letter;
out vec2 texcoord;
void main(){
	gl_Position = vec4((aPos*2-1+size.xy)*size.zw+pos,0,1);
	texcoord = (aPos+vec2(mod(letter,16),floor((letter+1)*-.0625)))*.0625;
})";
	textfrag =
R"(#version 330 core
in vec2 texcoord;
uniform sampler2D tex;
void main(){
	gl_FragColor = vec4(1)*texture2D(tex,texcoord).r;
	gl_FragColor.a = 1-(1-gl_FragColor.a)*.5;
})";
	textshader.reset(new OpenGLShaderProgram(context));
	textshader->addVertexShader(textvert);
	textshader->addFragmentShader(textfrag);
	textshader->link();

	texttex.loadImage(ImageCache::getFromMemory(BinaryData::txt_png,BinaryData::txt_pngSize));
	texttex.bind();
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
#endif

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

	framebuffer.initialise(context, getWidth(), getHeight());
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	context.extensions.glGenBuffers(1, &arraybuffer);
}
void PFAudioProcessorEditor::renderOpenGL() {
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	if(needtoupdate > 0) {
		framebuffer.makeCurrentRenderingTarget();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		baseshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		basetex.bind();
		baseshader->setUniform("basetex",0);
		baseshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);

		knobshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		knobtex.bind();
		knobshader->setUniform("knobtex",0);
		context.extensions.glActiveTexture(GL_TEXTURE1);
		basetex.bind();
		knobshader->setUniform("basetex",1);
		knobshader->setUniform("texscale",58.f/knobtex.getWidth(),66.f/knobtex.getHeight());
		knobshader->setUniform("knobscale",58.f/getWidth(),66.f/getHeight());
		knobshader->setUniform("ratio",((float)getHeight())/getWidth());
		knobshader->setUniform("basescale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
		coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		knobshader->setUniform("knobcolor",.2265625f);
		for(int i = 0; i < knobcount; i++) {
			if(i == 2) knobshader->setUniform("knobcolor",.61328125f);
			knobshader->setUniform("knobpos",((float)knobs[i].x*2)/getWidth(),2-((float)knobs[i].y*2)/getHeight());
			knobshader->setUniform("knobrot",(knobs[i].value-.5f)*.748f);
			knobshader->setUniform("id",(float)i);
			knobshader->setUniform("hoverstate",(float)(knobs[i].hoverstate==-1?(hover==i?-1:0):knobs[i].hoverstate));
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
			oversamplingshader->setUniform("selection",-.3947368421f-1.5263157895f*(oversamplinglerped-1));
			oversamplingshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			context.extensions.glDisableVertexAttribArray(coord);
		}

		if(creditsalpha > 0) {
			creditsshader->use();
			context.extensions.glActiveTexture(GL_TEXTURE0);
			creditstex.bind();
			creditsshader->setUniform("creditstex",0);
			creditsshader->setUniform("texscale",242.f/creditstex.getWidth(),59.f/creditstex.getHeight());
			creditsshader->setUniform("alpha",creditsalpha<0.5?4*creditsalpha*creditsalpha*creditsalpha:1-(float)pow(-2*creditsalpha+2,3)/2);
			creditsshader->setUniform("shineprog",websiteht);
			coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			context.extensions.glDisableVertexAttribArray(coord);
		}

		needtoupdate--;
		framebuffer.releaseAsRenderingTarget();
	}

	ppshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	ppshader->setUniform("pptex",0);
	ppshader->setUniform("chroma",rms*.006f);
	ppshader->setUniform("shake",rms*.004f*(random.nextFloat()-.5f));
	coord = context.extensions.glGetAttribLocation(ppshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

#ifdef ENABLE_CONSOLE
	if(audioProcessor.debugupdated) {
		std::ostringstream console;
		{
			std::lock_guard<std::mutex> guard(audioProcessor.debugmutex);
			for (int i = 0; i < 16; i++) {
				console << audioProcessor.debuglist[(int)fmod(i+audioProcessor.debugreadpos,16)].toStdString();
				if(i < 15) console << "\n";
			}
		}
		debugtxt = (String)console.str();
		audioProcessor.debugupdated = false;
	}
	
	drawstring(debugtxt,.01,.01,0,1);
#endif
}
#ifdef ENABLE_CONSOLE
void PFAudioProcessorEditor::drawstring(String txty, float x, float y, float xa, float ya) {
	textshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	texttex.bind();
	textshader->setUniform("tex",0);
	float coord = context.extensions.glGetAttribLocation(textshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);

	int i = 0; float c = 0, l = ya-.5;

	std::queue<float> linelength;
	const char* txt = txty.toUTF8();
	while(txt[i] != '\0')
		if(txt[i++] == '\n' || (((++c)+2)*8 > getWidth() && txt[i] != '\n'))
			{ linelength.push((c-1)*2*xa+2*(xa-.5)); l+=ya; c = 0; }
	linelength.push((c-1)*2*xa+2*(xa-.5));

	i = 0; c = 0;
	int ll = linelength.front(); linelength.pop();
	float letterw = 8.f/getWidth();
	float letterh = 16.f/getHeight();
	while(txt[i] != '\0') {
		if(txt[i] == '\n') {
			l--; c = 0;
			ll = linelength.front(); linelength.pop();
		} else {
			textshader->setUniform("size",c*2-ll,l*2,letterw,letterh);
			textshader->setUniform("pos",x*2-1,y*2-1);
			textshader->setUniform("letter",(float)((int)txt[i]));
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			c++;
			if((c+2)*8 > getWidth() && txt[i+1] != '\n') {
				l--; c = 0;
				ll = linelength.front(); linelength.pop();
			}
		}
		i++;
	}

	context.extensions.glDisableVertexAttribArray(coord);
}
#endif
void PFAudioProcessorEditor::openGLContextClosing() {
	baseshader->release();
	knobshader->release();
	visshader->release();
	oversamplingshader->release();
	creditsshader->release();
	ppshader->release();

	basetex.release();
	knobtex.release();
	creditstex.release();

	framebuffer.release();

#ifdef ENABLE_CONSOLE
	textshader->release();
	texttex.release();
#endif

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void PFAudioProcessorEditor::calcvis() {
	float norm = audioProcessor.norm.get();
	isStereo = knobs[4].value > 0 && knobs[3].value < 1 && knobs[5].value > 0;
	pluginpreset pp;
	for(int i = 0; i < knobcount; i++)
		pp.values[i] = knobs[i].inflate(knobs[i].value);
	for(int c = 0; c < (isStereo ? 2 : 1); c++) {
		for(int i = 0; i < 226; i++) {
			visline[c][i*2] = (i+8)/121.f-1;
			visline[c][i*2+1] = 1-(48+audioProcessor.plasticfuneral(sin(i/35.8098621957f)*.8f,c,2,pp)*norm*38)/231.f;
		}
	}
}
void PFAudioProcessorEditor::paint (Graphics& g) { }

void PFAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) {
		if(knobs[i].hoverstate < -1) {
			needtoupdate = 2;
			knobs[i].hoverstate++;
		}
	}
	if(held > 0) held--;

	if(oversamplingalpha != (hover<=-4?1:0)) {
		oversamplingalpha = fmax(fmin(oversamplingalpha+(hover<=-4?.07f:-.07f),1),0);
		needtoupdate = 2;
	}

	if (oversamplinglerped != oversampling) {
		needtoupdate = 2;
		if(fabs(oversamplinglerped-oversampling) <= .001f) oversamplinglerped = oversampling;
		oversamplinglerped = oversamplinglerped*.75f+oversampling*.25f;
	}

	if(creditsalpha != ((hover<=-2&&hover>=-3)?1:0)) {
		creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.07f:-.07f),1),0);
		needtoupdate = 2;
	}
	if(creditsalpha <= 0) websiteht = -1;
	if(websiteht >= -.227273) {
		websiteht -= .05;
		needtoupdate = 2;
	}

	if(audioProcessor.rmscount.get() > 0) {
		rms = sqrt(audioProcessor.rmsadd.get()/audioProcessor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
	}
	audioProcessor.rmsadd = 0;
	audioProcessor.rmscount = 0;

	if (audioProcessor.updatevis.get()) {
		calcvis();
		audioProcessor.updatevis = false;
		needtoupdate = 2;
	}

	context.triggerRepaint();
}

void PFAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		oversampling = newValue;
	} else for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		calcvis();
	}
	needtoupdate = 2;
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
		valueoffset = 0;
		audioProcessor.undoManager.beginNewTransaction();
		audioProcessor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	} else if(hover < -4) {
		oversampling = hover+9;
		audioProcessor.apvts.getParameter("oversampling")->setValueNotifyingHost((oversampling-1)/3.f);
		audioProcessor.undoManager.setCurrentTransactionName(
			(String)("Set Over-Sampling to ") += oversampling);
		audioProcessor.undoManager.beginNewTransaction();
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

		float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset);

		valueoffset = fmax(fmin(valueoffset,value+.1),value-1.1);
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
		audioProcessor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audioProcessor.undoManager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		if(hover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		int prevhover = hover;
		hover = recalchover(event.x,event.y);
		if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
		if(hover > -1) knobs[hover].hoverstate = -4;
	}
	held = 1;
}
void PFAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover > -1) {
		audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].defaultvalue);
		audioProcessor.undoManager.beginNewTransaction();
	}
}
void PFAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover > -1)
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int PFAudioProcessorEditor::recalchover(float x, float y) {
	if (x >= 8 && x <= 234 && y >= 8 && y <= 87) {
		if(y < 39 || y > 53) return -4;
		if(x >= 68 && x <= 87) return -8;
		if(x >= 97 && x <= 116) return -7;
		if(x >= 126 && x <= 145) return -6;
		if(x >= 155 && x <= 174) return -5;
		return -4;
	} else if(y >= 403) {
		if(x >= 50 && x <= 196 && y >= 412 && y <= 456) return -3;
		return -2;
	}
	float xx = 0, yy = 0;
	for(int i = 0; i < knobcount; i++) {
		xx = knobs[i].x-x;
		yy = knobs[i].y-y;
		if((xx*xx+yy*yy)<=576) return i;
	}
	return -1;
}
