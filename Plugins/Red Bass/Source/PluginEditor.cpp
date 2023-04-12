#include "PluginProcessor.h"
#include "PluginEditor.h"

RedBassAudioProcessorEditor::RedBassAudioProcessorEditor (RedBassAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params)
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 4; y++) {
			int i = x+y*2;
			int ii = i<5?i:(i-1);
			knobs[x*4+y].x = x*248+37;
			knobs[x*4+y].y = y*100+46;
			if(i != 5) {
				knobs[i].id = params.pots[ii].id;
				knobs[i].name = params.pots[ii].name;
				knobs[i].value = params.pots[ii].normalize(state.values[ii]);
				knobs[i].minimumvalue = params.pots[ii].minimumvalue;
				knobs[i].maximumvalue = params.pots[ii].maximumvalue;
				knobs[i].defaultvalue = params.pots[ii].normalize(params.pots[ii].defaultvalue);
				audioProcessor.apvts.addParameterListener(knobs[i].id,this);
			}
			knobcount++;
		}
	}
	knobs[5].id = "monitor";
	knobs[5].name = "Monitor Sidechain";
	knobs[5].value = params.monitorsmooth.getTargetValue() >.5;
	audioProcessor.apvts.addParameterListener("monitor",this);

	freqfreq = audioProcessor.calculatefrequency(knobs[0].value);
	lowpfreq = audioProcessor.calculatelowpass(knobs[4].value);

#ifdef BANNER
	setSize(322,408+21);
	banneroffset = 21.f/getHeight();
#else
	setSize(322,408);
#endif
	setResizable(false, false);
	if((SystemStats::getOperatingSystemType() & SystemStats::OperatingSystemType::Windows) != 0)
		dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->dpi/96.f;
	else
		dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;

	setOpaque(true);
	context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
RedBassAudioProcessorEditor::~RedBassAudioProcessorEditor() {
	for (int i = 0; i < knobcount; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	audioProcessor.apvts.removeParameterListener("monitor",this);
	stopTimer();
	context.detach();
}

void RedBassAudioProcessorEditor::newOpenGLContextCreated() {
	compileshader(feedbackshader,
//FEEDBACK VERT
R"(#version 150 core
in vec2 aPos;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	uv = aPos*2.1+.003;
})",
//FEEDBACK FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D buffertex;
uniform float vis;
out vec4 fragColor;
void main(){
	if(uv.x > 1) fragColor = vec4(vis,vis,vis,1);
	else fragColor = texture(buffertex,uv);
})");

	compileshader(baseshader,
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 texscale;
out vec2 texuv;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	texuv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	uv = vec2(aPos.x,(aPos.y-0.0318627)*1.0652742);
})",
//BASE FRAG
R"(#version 150 core
in vec2 texuv;
in vec2 uv;
uniform sampler2D basetex;
uniform sampler2D buffertex;
uniform float hover;
uniform float vis;
out vec4 fragColor;
void main(){
	vec3 c = texture(basetex,texuv).rgb;
	if(c.r > .5) {
		if(uv.x > .2 && uv.x < .8) {
			if((hover > ((uv.x-.42236024844720496894409937888199)*4.5352112676056338028169014084507) && c.b > .5) || vis > uv.y) {
				fragColor = vec4(1,1,1,1);
			} else {
				fragColor = vec4(vec3((mod(uv.x*322,2)+mod(uv.y*383,2))>2.9?0.2:0.0),1);
			}
		} else {
			fragColor = vec4(vis,vis,vis,1);
		}
	} else if(c.g > .5) {
		fragColor = vec4(1,1,1,1);
	} else if(c.b > .5) {
		fragColor = vec4(0,0,0,1);
	} else {
		fragColor = texture(buffertex,vec2((uv.x>.5?(1-uv.x):(uv.x))*2,uv.y))*vec4(.5,.5,.5,1);
	}
})");

	compileshader(knobshader,
//KNOB VERT
R"(#version 150 core
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
})",
//KNOB FRAG
R"(#version 150 core
out vec4 fragColor;
void main(){
	fragColor = vec4(1);
})");

	compileshader(toggleshader,
//TOGGLE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 basescale;
uniform vec2 knobpos;
uniform vec2 knobscale;
out vec2 basecoord;
void main(){
	gl_Position = vec4(aPos*2*knobscale-knobscale-1+knobpos,0,1);
	basecoord = vec2((gl_Position.x*.5+.5)*basescale.x,1-(.5-gl_Position.y*.5)*basescale.y);
	gl_Position.y = gl_Position.y*(1-banner)+banner;
})",
//TOGGLE FRAG
R"(#version 150 core
in vec2 basecoord;
uniform sampler2D basetex;
uniform float toggle;
out vec4 fragColor;
void main(){
	vec2 c = texture(basetex,basecoord).rb;
	if(toggle > .5)
		c.r *= (1-c.g);
	else
		c.r *= c.g;
	fragColor = vec4(1,1,1,c.r);
})");

	compileshader(textshader,
//TEXT VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 size;
uniform vec2 pos;
uniform int letter;
uniform int length;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*size*vec2(length,1)+pos)*2-1,0,1);
	uv = vec2((aPos.x*length+letter)*.0625,aPos.y);
})",
//TEXT FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
out vec4 fragColor;
void main(){
	fragColor = vec4(1,1,1,texture(tex,uv).r);
})");

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize));
	basetex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	texttex.loadImage(ImageCache::getFromMemory(BinaryData::txt_png, BinaryData::txt_pngSize));
	texttex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	framebuffer.initialise(context, 161*dpi, 1);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#ifdef BANNER
	compileshader(bannershader,
//BANNER VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 size;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*vec2(1,size.y))*2-1,0,1);
	uv = vec2(aPos.x*size.x,1-(1-aPos.y)*texscale.y);
})",
//BANNER FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform vec2 texscale;
uniform float pos;
uniform float free;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec2 col = max(min((texture(tex,vec2(mod(uv.x+pos,1)*texscale.x,uv.y)).rg-.5)*dpi+.5,1),0);
	fragColor = vec4(vec3(col.r*free+col.g*(1-free)),1);
})");

	bannertex.loadImage(ImageCache::getFromMemory(BinaryData::banner_png, BinaryData::banner_pngSize));
	bannertex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#endif

	context.extensions.glGenBuffers(1, &arraybuffer);

	audioProcessor.logger.init(&context,getWidth(),getHeight());
}
void RedBassAudioProcessorEditor::compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader) {
	shader.reset(new OpenGLShaderProgram(context));
	if(!shader->addVertexShader(vertexshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	if(!shader->addFragmentShader(fragmentshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	shader->link();
}
void RedBassAudioProcessorEditor::renderOpenGL() {
	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
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
	baseshader->setUniform("banner",banneroffset);
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
	toggleshader->setUniform("banner",banneroffset);
	toggleshader->setUniform("knobscale",54.f/322.f,54.f/408.f);
	toggleshader->setUniform("basescale",322.f/basetex.getWidth(),408.f/basetex.getHeight());
	toggleshader->setUniform("knobpos",((float)knobs[5].x*2)/getWidth(),2-((float)knobs[5].y*2)/408);
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

#ifdef BANNER
	bannershader->use();
	coord = context.extensions.glGetAttribLocation(bannershader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	bannertex.bind();
	bannershader->setUniform("tex",0);
	bannershader->setUniform("dpi",dpi);
#ifdef BETA
	bannershader->setUniform("texscale",494.f/bannertex.getWidth(),21.f/bannertex.getHeight());
	bannershader->setUniform("size",getWidth()/494.f,21.f/getHeight());
	bannershader->setUniform("free",0.f);
#else
	bannershader->setUniform("texscale",426.f/bannertex.getWidth(),21.f/bannertex.getHeight());
	bannershader->setUniform("size",getWidth()/426.f,21.f/getHeight());
	bannershader->setUniform("free",1.f);
#endif
	bannershader->setUniform("pos",bannerx);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);
#endif

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

#ifdef BANNER
	bannershader->release();
	bannertex.release();
#endif

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

#ifdef BANNER
	bannerx = fmod(bannerx+.0005f,1.f);
#endif

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
		audioProcessor.lerpchanged[hover<5?hover:(hover-1)] = true;
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
		if((xx*xx+yy*yy)<=841) return i;
	}
	return -1;
}
