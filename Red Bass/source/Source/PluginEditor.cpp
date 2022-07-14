#include "PluginProcessor.h"
#include "PluginEditor.h"

RedBassAudioProcessorEditor::RedBassAudioProcessorEditor (RedBassAudioProcessor& p, int paramcount, pluginpreset state, potentiometer pots[])
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 4; y++) {
			int i = x+y*2;
			knobs[x*4+y].x = x*248+37;
			knobs[x*4+y].y = y*100+46;
			knobs[i].id = pots[i].id;
			knobs[i].name = pots[i].name;
			if(pots[i].smoothtime > 0)
				knobs[i].value = pots[i].normalize(pots[i].smooth.getTargetValue());
			else
				knobs[i].value = pots[i].normalize(state.values[i]);
			knobs[i].minimumvalue = pots[i].minimumvalue;
			knobs[i].maximumvalue = pots[i].maximumvalue;
			knobs[i].defaultvalue = pots[i].normalize(pots[i].defaultvalue);
			knobcount++;
			audioProcessor.apvts.addParameterListener(knobs[i].id,this);
		}
	}
	freqfreq = audioProcessor.calculatefrequency(knobs[0].value);
	lowpfreq = audioProcessor.calculatelowpass(knobs[4].value);

	setSize (322, 408);
	setResizable(false, false);

	setOpaque(true);
	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
RedBassAudioProcessorEditor::~RedBassAudioProcessorEditor() {
	for (int i = 0; i < knobcount; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	stopTimer();
	context.detach();
}

void RedBassAudioProcessorEditor::newOpenGLContextCreated() {
	feedbackvert =
R"(#version 330 core
in vec2 aPos;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	uv = aPos*2.1+.003;
})";
	feedbackfrag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D buffertex;
uniform float vis;
void main(){
	if(uv.x > 1) gl_FragColor = vec4(vis,vis,vis,1);
	else gl_FragColor = texture2D(buffertex,uv);
})";
	feedbackshader.reset(new OpenGLShaderProgram(context));
	feedbackshader->addVertexShader(feedbackvert);
	feedbackshader->addFragmentShader(feedbackfrag);
	feedbackshader->link();

	basevert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 texuv;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	texuv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	uv = vec2(aPos.x,(aPos.y-0.0318627)*1.0652742);
})";
	basefrag =
R"(#version 330 core
in vec2 texuv;
in vec2 uv;
uniform sampler2D basetex;
uniform sampler2D buffertex;
uniform float hover;
uniform float vis;
void main(){
	vec3 c = texture2D(basetex,texuv).rgb;
	if(c.r > .5) {
		if(uv.x > .2 && uv.x < .8) {
			if((hover > ((uv.x-.42236024844720496894409937888199)*4.5352112676056338028169014084507) && c.b > .5) || vis > uv.y) {
				gl_FragColor = vec4(1,1,1,1);
			} else {
				gl_FragColor = vec4(vec3((mod(uv.x*322,2)+mod(uv.y*383,2))>2.9?.2:0),1);
			}
		} else {
			gl_FragColor = vec4(vis,vis,vis,1);
		}
	} else if(c.g > .5) {
		gl_FragColor = vec4(1,1,1,1);
	} else if(c.b > .5) {
		gl_FragColor = vec4(0,0,0,1);
	} else {
		gl_FragColor = texture2D(buffertex,vec2((uv.x>.5?(1-uv.x):(uv.x))*2,uv.y))*vec4(.5,.5,.5,1);
	}
})";
	baseshader.reset(new OpenGLShaderProgram(context));
	baseshader->addVertexShader(basevert);
	baseshader->addFragmentShader(basefrag);
	baseshader->link();

	knobvert =
R"(#version 330 core
in vec2 aPos;
uniform float ratio;
uniform float knobrot;
uniform vec2 knobpos;
uniform vec2 knobscale;
uniform float ext;
void main(){
	vec2 pos = (vec2(aPos.x,1-(1-aPos.y)*ext)*2*knobscale-vec2(knobscale.x,0))/vec2(ratio,1);
	gl_Position = vec4(
		(pos.x*cos(knobrot)-pos.y*sin(knobrot))*ratio-1+knobpos.x,
		pos.x*sin(knobrot)+pos.y*cos(knobrot)-1+knobpos.y,0,1);
})";
	knobfrag =
R"(#version 330 core
void main(){
	gl_FragColor = vec4(1);
})";
	knobshader.reset(new OpenGLShaderProgram(context));
	knobshader->addVertexShader(knobvert);
	knobshader->addFragmentShader(knobfrag);
	knobshader->link();

	togglevert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 basescale;
uniform float ratio;
uniform vec2 knobpos;
uniform vec2 knobscale;
out vec2 basecoord;
void main(){
	gl_Position = vec4(aPos*2*knobscale-knobscale-1+knobpos,0,1);
	basecoord = vec2((gl_Position.x*.5+.5)*basescale.x,1-(.5-gl_Position.y*.5)*basescale.y);
})";
	togglefrag =
R"(#version 330 core
in vec2 basecoord;
uniform sampler2D basetex;
uniform float toggle;
void main(){
	vec2 c = texture2D(basetex,basecoord).rb;
	if(toggle > .5)
		c.r *= (1-c.g);
	else
		c.r *= c.g;
	gl_FragColor = vec4(1,1,1,c.r);
})";
	toggleshader.reset(new OpenGLShaderProgram(context));
	toggleshader->addVertexShader(togglevert);
	toggleshader->addFragmentShader(togglefrag);
	toggleshader->link();

	textvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 size;
uniform vec2 pos;
uniform int letter;
uniform int length;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*size*vec2(length,1)+pos)*2-1,0,1);
	uv = vec2((aPos.x*length+letter)*.0625,aPos.y);
})";
	textfrag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D tex;
void main(){
	gl_FragColor = vec4(1,1,1,texture2D(tex,uv).r);
})";
	textshader.reset(new OpenGLShaderProgram(context));
	textshader->addVertexShader(textvert);
	textshader->addFragmentShader(textfrag);
	textshader->link();

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize));
	basetex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	texttex.loadImage(ImageCache::getFromMemory(BinaryData::txt_png, BinaryData::txt_pngSize));
	texttex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	framebuffer.initialise(context, 161, 1);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	context.extensions.glGenBuffers(1, &arraybuffer);

	audioProcessor.logger.init(&context,getWidth(),getHeight());
}
void RedBassAudioProcessorEditor::renderOpenGL() {
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	framebuffer.makeCurrentRenderingTarget();
	feedbackshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	feedbackshader->setUniform("buffertex",0);
	feedbackshader->setUniform("vis",vis);
	coord = context.extensions.glGetAttribLocation(feedbackshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);
	framebuffer.releaseAsRenderingTarget();

	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	baseshader->setUniform("buffertex",1);
	baseshader->setUniform("texscale",322.f/basetex.getWidth(),408.f/basetex.getHeight());
	baseshader->setUniform("vis",vis);
	baseshader->setUniform("hover",credits<0.5?4*credits*credits*credits:1-(float)pow(-2*credits+2,3)/2);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	knobshader->use();
	knobshader->setUniform("knobscale",2.f/getWidth(),27.f/getHeight());
	knobshader->setUniform("ratio",((float)getHeight())/getWidth());
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for (int i = 0; i < knobcount; i++) {
		if(i != 5) {
			knobshader->setUniform("ext",(i==0||i==4)?.5f:1.f);
			knobshader->setUniform("knobpos",((float)knobs[i].x*2)/getWidth(),2-((float)knobs[i].y*2)/getHeight());
			knobshader->setUniform("knobrot",(float)fmod((knobs[i].value-.5f)*.748f,1)*-6.2831853072f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	toggleshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	toggleshader->setUniform("basetex",0);
	toggleshader->setUniform("knobscale",54.f/getWidth(),54.f/getHeight());
	toggleshader->setUniform("ratio",((float)getHeight())/getWidth());
	toggleshader->setUniform("basescale",322.f/basetex.getWidth(),408.f/basetex.getHeight());
	toggleshader->setUniform("knobpos",((float)knobs[5].x*2)/getWidth(),2-((float)knobs[5].y*2)/getHeight());
	toggleshader->setUniform("toggle",knobs[5].value>.5?1.f:0.f);
	coord = context.extensions.glGetAttribLocation(toggleshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	for(int i = 0; i < 6; i += 4) {
		int val = i==0?freqfreq:lowpfreq;
		int tl = 3;
		int ext = 2;
		if (val >= 10000) {
			val = roundFloatToInt(val*.001f);
			ext = 3;
			if(knobs[i].value >= 1) tl = 1;
		} else if(val >= 1000) tl = 5;
		  else if(val >= 100 ) tl = 4;

		textshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		texttex.bind();
		textshader->setUniform("tex",0);
		coord = context.extensions.glGetAttribLocation(textshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		for(int t = 0; t < tl; t++) {
			int n = 0;
			if(tl == 1) n = 13;
			else if(t == 0) n = ext==3?10:11;
			else n = fmod(floorf(val*powf(.1f,t-1)),10);
			textshader->setUniform("pos",((float)knobs[i].x-8*t+4*(tl-ext-1))/getWidth(),1-((float)knobs[i].y+8)/getHeight());
			textshader->setUniform("size",8.f/getWidth(),16.f/getHeight());
			textshader->setUniform("letter",n);
			textshader->setUniform("length",t==0?ext:1);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
		context.extensions.glDisableVertexAttribArray(coord);
	}

	audioProcessor.logger.drawlog();
}
void RedBassAudioProcessorEditor::openGLContextClosing() {
	feedbackshader->release();
	baseshader->release();
	knobshader->release();
	toggleshader->release();
	textshader->release();

	basetex.release();
	texttex.release();

	framebuffer.release();

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void RedBassAudioProcessorEditor::paint (Graphics& g) { }

void RedBassAudioProcessorEditor::timerCallback() {
	if(audioProcessor.rmscount.get() > 0) {
		vis = audioProcessor.rmsadd.get()/audioProcessor.rmscount.get();
		audioProcessor.rmsadd = 0;
		audioProcessor.rmscount = 0;
	} else vis *= .9f;

	if(credits!=(hover<-1?1:0))
		credits = fmax(fmin(credits+(hover<-1?.1f:-.1f),1),0);

	context.triggerRepaint();
}

void RedBassAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = newValue;
		     if(i == 0) freqfreq = audioProcessor.calculatefrequency(newValue);
		else if(i == 4) lowpfreq = audioProcessor.calculatelowpass  (newValue);
	}
}
void RedBassAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
}
void RedBassAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void RedBassAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	initialdrag = hover;
	if (hover == 5) {
		audioProcessor.undoManager.beginNewTransaction();
		audioProcessor.undoManager.setCurrentTransactionName(
			((knobs[5].value > .5) ? "Turned Sidechain Monitoring off" : "Turned Sidechain Monitoring on"));
		audioProcessor.undoManager.beginNewTransaction();
		audioProcessor.apvts.getParameter(knobs[5].id)->setValueNotifyingHost(knobs[hover].value>.5?0:1);

	} else if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audioProcessor.undoManager.beginNewTransaction();
		audioProcessor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void RedBassAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(hover == -1) return;
	if(initialdrag > -1 && initialdrag != 5) {
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
	}
}
void RedBassAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(hover > -1 && hover != 5) {
		audioProcessor.undoManager.setCurrentTransactionName(
			(String)((knobs[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
		audioProcessor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audioProcessor.undoManager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		//if(hover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		int prevhover = hover;
		hover = recalchover(event.x,event.y);
	}
}
void RedBassAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover > -1 && hover != 5) {
		audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
		audioProcessor.undoManager.beginNewTransaction();
	}
}
void RedBassAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover > -1 && hover != 5)
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int RedBassAudioProcessorEditor::recalchover(float x, float y) {
	if(x >= 134 && x <= 209 && y >= 144 && y <= 242) {
		if(x >= 135 && x <= 181 && y >= 195 && y <= 241)
			return -3;
		return -2;
	}
	float xx = 0, yy = 0;
	for(int i = 0; i < knobcount; i++) {
		xx = knobs[i].x-x;
		yy = knobs[i].y-y;
		if((xx*xx+yy*yy)<=3364) return i;
	}
	return -1;
}
