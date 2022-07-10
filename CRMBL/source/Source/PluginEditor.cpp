#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace gl;

CRMBLAudioProcessorEditor::CRMBLAudioProcessorEditor (CRMBLAudioProcessor& p, int paramcount, pluginpreset state, potentiometer pots[])
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	//special labels: time, mod, feedback
	//CLOCK
	knobs[0].index = -1;
	knobs[0].x = .4282319003;
	knobs[0].y = .5832025519;
	knobs[0].radius = .4282319003;
	knobs[0].linewidth = .0222989305;
	knobs[0].lineheight = .73;
	knobs[0].r = .0625f;
	//FEEDBACK
	knobs[1].index = 6;
	knobs[1].x = .5095732975;
	knobs[1].y = .7174707256;
	knobs[1].radius = .1885720718;
	knobs[1].linewidth = .050639065;
	knobs[1].r = .5;
	//TIME
	knobs[2].index = 0;
	knobs[2].x = .6480072833;
	knobs[2].y = .6583846315;
	knobs[2].radius = .2376583343;
	knobs[2].linewidth = .0401800064;
	knobs[2].r = .1875;
	//MOD FREQ
	knobs[3].index = 3;
	knobs[3].x = .8327202455;
	knobs[3].y = .5297886311;
	knobs[3].radius = .0990625024;
	knobs[3].linewidth = .0963948332;
	knobs[3].lineheight = 1;
	knobs[3].r = .4375;
	//MOD
	knobs[4].index = 2;
	knobs[4].x = .7837187701;
	knobs[4].y = .470595587;
	knobs[4].radius = .0990625024;
	knobs[4].linewidth = .0963948332;
	knobs[4].lineheight = 1;
	knobs[4].r = .375;
	//DRY/WET
	knobs[5].index = 11;
	knobs[5].x = .4592473633;
	knobs[5].y = .9123757694;
	knobs[5].radius = .2376583343;
	knobs[5].linewidth = .0401800064;
	knobs[5].r = .625;
	//PINGPONG
	knobs[6].index = 4;
	knobs[6].x = .6327409673;
	knobs[6].y = .7939896811;
	knobs[6].radius = .1885720718;
	knobs[6].linewidth = .050639065;
	knobs[6].r = .25;
	//PITCH
	knobs[7].index = 9;
	knobs[7].x = .2221832359;
	knobs[7].y = .3319921158;
	knobs[7].radius = .1885720718;
	knobs[7].linewidth = .050639065;
	knobs[7].r = .5625;
	//CHEW
	knobs[8].index = 8;
	knobs[8].x = .1082867784;
	knobs[8].y = .4330561124;
	knobs[8].radius = .1885720718;
	knobs[8].linewidth = .050639065;
	knobs[8].r = .6875;
	//REVERSE
	knobs[9].index = 7;
	knobs[9].x = .2314544482;
	knobs[9].y = .5413354191;
	knobs[9].radius = .2376583343;
	knobs[9].linewidth = .0401800064;
	knobs[9].r = .3125;
	//LOWPASS
	knobs[10].index = 10;
	knobs[10].x = .3532976591;
	knobs[10].y = .3882975081;
	knobs[10].radius = .2376583343;
	knobs[10].linewidth = .0401800064;
	knobs[10].r = .125;

	for(int i = 0; i < 11; i++) {
		knobs[i].radius += .004;
		knobs[i].x -= .001;
		knobs[i].y += .002;
	}
	for(int i = 1; i < 11; i++) {
		knobs[i].id = pots[knobs[i].index].id;
		knobs[i].name = pots[knobs[i].index].name;
		if(pots[knobs[i].index].smoothtime > 0)
			knobs[i].value = pots[knobs[i].index].normalize(pots[knobs[i].index].smooth.getTargetValue());
		else
			knobs[i].value = pots[knobs[i].index].normalize(state.values[knobs[i].index]);
		knobs[i].minimumvalue = pots[knobs[i].index].minimumvalue;
		knobs[i].maximumvalue = pots[knobs[i].index].maximumvalue;
		knobs[i].defaultvalue = pots[knobs[i].index].normalize(pots[knobs[i].index].defaultvalue);
		audioProcessor.apvts.addParameterListener(knobs[i].id,this);
	}
	for(int i = 0; i < paramcount; i++) {
		if(pots[i].id == "oversampling")
			oversampling = state.values[i] > .5;
		else if(pots[i].id == "hold")
			hold = pots[i].smooth.getTargetValue() >.5;
		else if(pots[i].id == "pingpostfeedback")
			postfb = pots[i].smooth.getTargetValue() >.5;
		else if(pots[i].id == "time")
			time = pots[i].smooth.getTargetValue();
		else if(pots[i].id == "sync")
			sync = state.values[i];
	}
	recalclabels();
	audioProcessor.apvts.addParameterListener("oversampling",this);
	audioProcessor.apvts.addParameterListener("hold",this);
	audioProcessor.apvts.addParameterListener("pingpostfeedback",this);
	audioProcessor.apvts.addParameterListener("sync",this);

	setSize(507,465);
	setResizable(false, false);

	setOpaque(true);
	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
CRMBLAudioProcessorEditor::~CRMBLAudioProcessorEditor() {
	for(int i = 0; i < knobcount; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	audioProcessor.apvts.removeParameterListener("oversampling",this);
	stopTimer();
	context.detach();
}

void CRMBLAudioProcessorEditor::newOpenGLContextCreated() {
	audioProcessor.logger.init(&context,getWidth(),getHeight());

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
uniform float r;
uniform float gb;
void main(){
	vec4 col = texture2D(basetex,uv);
	col.r /= col.a;
	gl_FragColor = vec4(0);
	if(col.r <= (r+.033)) {
		if(col.r >= (r-.03))
			gl_FragColor = vec4(vec3(gb>.5?col.g:col.b),col.a);
		else if(col.r >= (r-.031))
			gl_FragColor = vec4(0,0,0,col.a);
	}
})";
	baseshader.reset(new OpenGLShaderProgram(context));
	baseshader->addVertexShader(basevert);
	baseshader->addFragmentShader(basefrag);
	baseshader->link();

	logovert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})";
	logofrag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D basetex;
uniform float websiteht;
void main(){
	vec4 col = texture2D(basetex,uv);
	col.r /= col.a;
	gl_FragColor = vec4(0);
	if(col.r >= .85)
		gl_FragColor = vec4(vec3(col.g*(1-texture2D(basetex,vec2(min(uv.x+websiteht,.3),uv.y)).b*.5)),col.a);
})";
	logoshader.reset(new OpenGLShaderProgram(context));
	logoshader->addVertexShader(logovert);
	logoshader->addFragmentShader(logofrag);
	logoshader->link();
	
	knobvert =
R"(#version 330 core
in vec2 aPos;
uniform float knobrot;
uniform vec2 knobscale;
uniform vec2 knobpos;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*knobscale+knobpos,0,1);
	uv = vec2(
		(aPos.x-.5)*cos(knobrot)-(aPos.y-.5)*sin(knobrot),
		(aPos.x-.5)*sin(knobrot)+(aPos.y-.5)*cos(knobrot))*2;
})";
	knobfrag =
R"(#version 330 core
in vec2 uv;
uniform int hoverstate;
uniform float thickness;
uniform float lineheight;
uniform vec2 knobscale;
uniform float circle;
void main(){
	float d = 0;
	if(circle > .5) {
		d = sqrt(uv.y*uv.y+uv.x*uv.x);
		if((1-d) > (d-thickness)) {
			float y = uv.y;
			if(y > 0) y = max(y-lineheight,0);
			d = max(d-thickness,0)*100*knobscale.y+max(1-sqrt(y*y+uv.x*uv.x)*2/(1-thickness),0)*2.5;
			gl_FragColor = vec4(vec3(min(d,1)),1);
		} else gl_FragColor = vec4(1,1,1,min((1-d)*100*knobscale.y,1));
	} else {
		float y = uv.y;
		if(y > 0) y = max(y-lineheight,0);
		gl_FragColor = vec4(1,1,1,min(max(1-sqrt(y*y+uv.x*uv.x)*2/(1-thickness),0)*2.5,1));
	}
	//gl_FragColor *= vec4(1,0,0,.5);
})";
	knobshader.reset(new OpenGLShaderProgram(context));
	knobshader->addVertexShader(knobvert);
	knobshader->addFragmentShader(knobfrag);
	knobshader->link();

	numbervert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 size;
uniform vec2 pos;
uniform vec2 index;
uniform int length;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*size*vec2(length,1)+pos,0,1);
	uv = (aPos+index)*vec2(.0625*length,.5);
})";
	numberfrag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D numbertex;
uniform vec3 col;
void main(){
	gl_FragColor = vec4(texture2D(numbertex,uv).r)*vec4(col,1);
})";
	numbershader.reset(new OpenGLShaderProgram(context));
	numbershader->addVertexShader(numbervert);
	numbershader->addFragmentShader(numberfrag);
	numbershader->link();

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::map_png, BinaryData::map_pngSize));
	basetex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	numbertex.loadImage(ImageCache::getFromMemory(BinaryData::numbers_png, BinaryData::numbers_pngSize));
	numbertex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	context.extensions.glGenBuffers(1, &arraybuffer);
}
void CRMBLAudioProcessorEditor::renderOpenGL() {
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	OpenGLHelpers::clear(Colour::fromRGB(100,100,100));

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	baseshader->use();
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	baseshader->setUniform("gb",postfb?1.f:0.f);
	baseshader->setUniform("texscale",507.f/basetex.getWidth(),465.f/basetex.getHeight());
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < knobcount; i++) {
		if(i != 1 && i != 2 && i != 4) {
			baseshader->setUniform("r",knobs[i].r);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	baseshader->setUniform("gb",hold?0.f:1.f);
	baseshader->setUniform("r",.75f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	baseshader->setUniform("gb",oversampling?1.f:0.f);
	baseshader->setUniform("r",.8125f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	logoshader->use();
	coord = context.extensions.glGetAttribLocation(logoshader->getProgramID(),"aPos");
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	logoshader->setUniform("basetex",0);
	logoshader->setUniform("websiteht",websiteht);
	logoshader->setUniform("texscale",507.f/basetex.getWidth(),465.f/basetex.getHeight());
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	float ratio = ((float)getHeight())/getWidth();
	knobshader->use();
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	knobshader->setUniform("circle",1.f);
	for(int i = 0; i < knobcount; i++) {
		if(i == 1 || i == 3) {
			context.extensions.glDisableVertexAttribArray(coord);

			baseshader->use();
			coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
			context.extensions.glActiveTexture(GL_TEXTURE0);
			basetex.bind();
			baseshader->setUniform("basetex",0);
			baseshader->setUniform("texscale",507.f/basetex.getWidth(),465.f/basetex.getHeight());
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			if(i == 1) {
				baseshader->setUniform("r",knobs[1].r);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
				baseshader->setUniform("r",knobs[2].r);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			} else {
				baseshader->setUniform("r",knobs[4].r);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			}
			context.extensions.glDisableVertexAttribArray(coord);

			knobshader->use();
			coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			knobshader->setUniform("circle",1.f);
		}
		knobshader->setUniform("thickness",1-knobs[i].linewidth*2.8f);
		knobshader->setUniform("lineheight",knobs[i].lineheight);
		knobshader->setUniform("knobscale",knobs[i].radius*ratio*2.f,knobs[i].radius*2.f);
		knobshader->setUniform("knobpos",knobs[i].x*2-1,1-knobs[i].y*2);
		knobshader->setUniform("hoverstate",hover==i?1:0);
		if(i == 0) {
			Time time = Time::getCurrentTime();
			knobshader->setUniform("knobrot",((time.getMinutes()+time.getSeconds()/60.f)/60.f)*6.28318530718f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			knobshader->setUniform("circle",0.f);
			knobshader->setUniform("lineheight",.3f);
			knobshader->setUniform("knobrot",((time.getHours()+time.getMinutes()/60.f)/12.f)*6.28318530718f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			knobshader->setUniform("circle",1.f);
		} else {
			knobshader->setUniform("knobrot",(knobs[i].value-.5f)*(i==6?6.2831853072f:5.5f));
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	numbershader->use();
	coord = context.extensions.glGetAttribLocation(numbershader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	numbertex.bind();
	numbershader->setUniform("numbertex",0);
	float l = 16.f/getWidth();
	bool n = pitchnum[0]==-1;
	numbershader->setUniform("size",l,32.f/getHeight());
	numbershader->setUniform("length",1);
	numbershader->setUniform("col",1,1,1);
	for(int i = 0; i < pitchnum[0]; i++) {
		numbershader->setUniform("pos",knobs[7].x*2-1+knobs[7].radius*ratio+(i-pitchnum[0]*.5f)*l,1-knobs[7].y*2+knobs[7].radius*.5f-l);
		numbershader->setUniform("index",pitchnum[i+1],1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	n = timenum[0]==-1;
	for(int i = 0; i < timenum[0]; i++) {
		numbershader->setUniform("pos",knobs[2].x*2-1+knobs[2].radius*ratio+(i-timenum[0]*.5f)*l,1-knobs[2].y*2+knobs[2].radius*.6f-l);
		numbershader->setUniform("index",timenum[i+1],1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	if(audioProcessor.outofrange.get()) {
		numbershader->setUniform("length",13);
		numbershader->setUniform("col",1,0,0);
		numbershader->setUniform("pos",knobs[2].x*2-1+knobs[2].radius*ratio-6.5f*l,1-knobs[2].y*2+knobs[2].radius*.6f+l);
		numbershader->setUniform("index",0.f,0.f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	audioProcessor.logger.drawlog();
}
void CRMBLAudioProcessorEditor::openGLContextClosing() {
	baseshader->release();
	logoshader->release();
	knobshader->release();
	numbershader->release();

	basetex.release();
	numbertex.release();

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void CRMBLAudioProcessorEditor::paint (Graphics& g) { }

void CRMBLAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) {
		if(knobs[i].hoverstate < -1) {
			knobs[i].hoverstate++;
		}
	}
	if(held > 0) held--;

	if(websiteht > -.3) websiteht -= .023;

	if(audioProcessor.rmscount.get() > 0) {
		rms = sqrt(audioProcessor.rmsadd.get()/audioProcessor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
		audioProcessor.rmsadd = 0;
		audioProcessor.rmscount = 0;
	} else rms *= .9f;


	context.triggerRepaint();
}

void CRMBLAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		oversampling = newValue>.5f;
		return;
	} else if(parameterID == "hold") {
		hold = newValue>.5f;
		return;
	} else if(parameterID == "pingpostfeedback") {
		postfb = newValue>.5f;
		return;
	} else if(parameterID == "sync") {
		sync = newValue;
		recalclabels();
		return;
	} else if(parameterID == "time") {
		time = newValue;
		recalclabels();
		return;
	}
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		if(parameterID == "pitch") recalclabels();
		return;
	}
}
void CRMBLAudioProcessorEditor::recalclabels() {
	int num = fabs(round(knobs[7].value*48-24));
	pitchnum[0] = 0;
	if(knobs[7].value != .5) {
		if(knobs[7].value < .5) {
			pitchnum[0]++;
			pitchnum[1] = 10;
		}
		if(num >= 10) {
			pitchnum[0]++;
			pitchnum[pitchnum[0]] = floor(num*.1);
		}
		pitchnum[0]++;
		pitchnum[pitchnum[0]] = fmod(floor(num),10);
		//pitchnum[3] = 11;
		//pitchnum[4] = fmod(floor(num*10),10);
	}
	if(sync <= 0) {
		knobs[2].value = time;
		num = round(pow(knobs[2].value,2)*5980+20);
		timenum[0] = 0;
		if(num >= 100) {
			if(num >= 1000) {
				timenum[0]++;
				timenum[1] = floor(num*.001);
			}
			timenum[0]++;
			timenum[timenum[0]] = fmod(floor(num*.01),10);
		}
		timenum[0] += 2;
		timenum[timenum[0]-1] = fmod(floor(num*.1),10);
		timenum[timenum[0]] = fmod(floor(num),10);
	} else {
		knobs[2].value = (sync-1)/15.f;
		num = round(knobs[2].value*15+1);
		timenum[0] = 4;
		timenum[1] = 12;
		timenum[2] = 13;
		timenum[3] = 14;
		if(num >= 10) {
			timenum[4] = floor(num*.1);
			timenum[0]++;
		}
		timenum[timenum[0]] = fmod(floor(num),10);
	}
}
void CRMBLAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == -5 && prevhover != -5 && websiteht <= -.3) websiteht = .19f;
	if(prevhover != hover && held == 0) {
		if(hover > -1) knobs[hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
	}
}
void CRMBLAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void CRMBLAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	held = -1;
	initialdrag = hover;
	if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audioProcessor.undoManager.beginNewTransaction();
		audioProcessor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	} else if(hover == -2) {
		postfb = !postfb;
		audioProcessor.apvts.getParameter("pingpostfeedback")->setValueNotifyingHost(postfb?1.f:0.f);
		audioProcessor.undoManager.setCurrentTransactionName(postfb?"Pingpong post feedback":"Pingpong pre feedback");
		audioProcessor.undoManager.beginNewTransaction();
	} else if(hover == -3) {
		oversampling = !oversampling;
		audioProcessor.apvts.getParameter("oversampling")->setValueNotifyingHost(oversampling?1.f:0.f);
		audioProcessor.undoManager.setCurrentTransactionName(oversampling?"Turned oversampling on":"Turned oversampling off");
		audioProcessor.undoManager.beginNewTransaction();
	} else if (hover == -4) {
		hold = !hold;
		audioProcessor.apvts.getParameter("hold")->setValueNotifyingHost(hold?1.f:0.f);
		audioProcessor.undoManager.setCurrentTransactionName(hold?"Hold":"Unhold");
		audioProcessor.undoManager.beginNewTransaction();
	}
}
void CRMBLAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(hover == -1) return;
	if(initialdrag > -1) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f)*(hover==7?.2:1);
		if(hover == 7 && !event.mods.isCtrlDown()) {
			audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(round((value-valueoffset)*48)/48.f);
		} else if(knobs[hover].id == "time") {
			if(event.mods.isCtrlDown()) {
				audioProcessor.apvts.getParameter("sync")->setValueNotifyingHost((fmax(value-valueoffset,0)*15+1)*.0625);
			} else {
				if(sync > 0) audioProcessor.apvts.getParameter("sync")->setValueNotifyingHost(0);
				audioProcessor.apvts.getParameter("time")->setValueNotifyingHost(value-valueoffset);
			}
		} else {
			audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset);
		}

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	}
}
void CRMBLAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(hover > -1) {
		audioProcessor.undoManager.setCurrentTransactionName(
			(String)((knobs[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
		audioProcessor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audioProcessor.undoManager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		int prevhover = hover;
		hover = recalchover(event.x,event.y);
		if(prevhover == -5 && hover == -5) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		if(prevhover != -5 && hover == -5 && websiteht <= -.3) websiteht = .19f;
		if(hover > -1) knobs[hover].hoverstate = -4;
	}
	held = 1;
}
void CRMBLAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover > -1) {
		audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
		if(knobs[hover].id == "time")
			audioProcessor.apvts.getParameter("sync")->setValueNotifyingHost(0);
		audioProcessor.undoManager.beginNewTransaction();
	}
}
void CRMBLAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(wheel.deltaY == 0) return;
	if(hover == 7 && !event.mods.isCtrlDown())
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			round(knobs[hover].value*48+(wheel.deltaY>0?1:-1))/48.);
	else if(hover > -1)
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int CRMBLAudioProcessorEditor::recalchover(float x, float y) {
	if (x >= 9 && x <= 160 && y >= 335 && y <= 456) {
		if(x <= 50 && y <= 378) return -4;
		else if(x <= 119 && y >= 382 && y <= 402) return -3;
		else if(y >= 406) return -5;
		return -1;
	} else if(x >= 336 && x <= 381 && y >= 390 && y <= 411) return -2;
	float r = 0, xx = 0, yy = 0;
	for(int i = knobcount-1; i >= 1; i--) {
		r = knobs[i].radius*getHeight()*.5;
		xx = knobs[i].x*getWidth()+r-x;
		yy = knobs[i].y*getHeight()-r-y;
		if(sqrt(xx*xx+yy*yy)<= r) return i;
	}
	return -1;
}
