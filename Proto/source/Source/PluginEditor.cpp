#include "PluginProcessor.h"
#include "PluginEditor.h"

ProtoAudioProcessorEditor::ProtoAudioProcessorEditor(ProtoAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params)
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 3; y++) {
			int i = x+y*2;
			if(i >= paramcount) break;
			knobs[i].x = x*106+68;
			knobs[i].y = y*100+144;
			knobs[i].id = params.pots[i].id;
			knobs[i].name = params.pots[i].name;
			knobs[i].value = params.pots[i].normalize(state.values[i]);
			knobs[i].minimumvalue = params.pots[i].minimumvalue;
			knobs[i].maximumvalue = params.pots[i].maximumvalue;
			knobs[i].defaultvalue = params.pots[i].normalize(params.pots[i].defaultvalue);
			knobcount++;
			audioProcessor.apvts.addParameterListener(knobs[i].id,this);
		}
	}
	oversampling = params.oversampling;
	oversamplinglerped = oversampling;
	audioProcessor.apvts.addParameterListener("oversampling",this);

	setSize(30+106*2,162+100*3);
	setResizable(false, false);
	calcvis();
	dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->dpi/96.f;

	setOpaque(true);
	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
ProtoAudioProcessorEditor::~ProtoAudioProcessorEditor() {
	for(int i = 0; i < knobcount; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	audioProcessor.apvts.removeParameterListener("oversampling",this);
	stopTimer();
	context.detach();
}

void ProtoAudioProcessorEditor::newOpenGLContextCreated() {
	audioProcessor.logger.init(&context,getWidth(),getHeight());

	compileshader(baseshader,
//BASE VERT
R"(#version 330 core
in vec2 aPos;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	uv = aPos;
})",
//BASE FRAG
R"(#version 330 core
in vec2 uv;
uniform sampler2D basetex;
uniform vec2 texscale;
uniform float offset;
void main(){
	gl_FragColor = texture2D(basetex,vec2(mod(uv.x*texscale.x+offset,1),1-(1-uv.y)*texscale.y));
})");

	compileshader(textshader,
audioProcessor.logger.textvert,
//TEXT VERT
R"(#version 330 core
in vec2 texcoord;
uniform sampler2D tex;
uniform int hoverstate;
void main(){
	gl_FragColor = vec4(vec2(texture2D(tex,texcoord).r),hoverstate,1);
})");

	compileshader(knobshader,
//KNOB VERT
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
		(aPos.x-.5)*sin(knobrot)+(aPos.y-.5)*cos(knobrot));
})",
//KNOB FRAG
R"(#version 330 core
in vec2 uv;
uniform int hoverstate;
void main(){
	gl_FragColor=vec4(vec2((uv.y>0&&abs(uv.x)<.02)?1:0),hoverstate,1);
})");

	compileshader(blackshader,
//BLACK VERT
R"(#version 330 core
in vec2 aPos;
uniform vec4 pos;
void main(){
	gl_Position = vec4(aPos*pos.zw+pos.xy,0,1);
})",
//BLACK FRAG
R"(#version 330 core
uniform int hoverstate;
void main(){
	gl_FragColor = vec4(0,0,hoverstate,1);
})");

	compileshader(visshader,
//VIS VERT
R"(#version 330 core
in vec2 aPos;
void main(){
	gl_Position = vec4(aPos,0,1);
})",
//VIS FRAG
R"(#version 330 core
void main(){
	gl_FragColor = vec4(1,1,0,1);
})");

	compileshader(oversamplingshader,
//OVERSAMPLING VERT
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
uniform float selection;
uniform vec4 pos;
out vec2 uv;
out vec2 highlightcoord;
void main(){
	gl_Position = vec4(aPos*pos.zw+pos.xy,0,1);
	uv = aPos*texscale;
	highlightcoord = vec2((aPos.x-selection)*4.3214285714,aPos.y*3.3-1.1642857143);
})",
//OVERSAMPLING FRAG
R"(#version 330 core
in vec2 uv;
in vec2 highlightcoord;
uniform sampler2D ostex;
void main(){
	float tex = texture2D(ostex,uv).b;
	if(highlightcoord.x>0&&highlightcoord.x<1&&highlightcoord.y>0&&highlightcoord.y<1)tex=1-tex;
	gl_FragColor = vec4(1,1,1,tex);
})");

	compileshader(creditsshader,
//CREDITS VERT
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec4 pos;
out vec2 uv;
out float xpos;
void main(){
	gl_Position = vec4(aPos*pos.zw+pos.xy,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	xpos = aPos.x;
})",
//CREDITS FRAG
R"(#version 330 core
in vec2 uv;
in float xpos;
uniform sampler2D creditstex;
uniform float shineprog;
void main(){
	vec2 creditols = texture2D(creditstex,uv).rb;
	float shine = 0;
	if(xpos+shineprog < .65 && xpos+shineprog > 0)
		shine = texture2D(creditstex,uv+vec2(shineprog,0)).g;
	gl_FragColor = vec4(vec2(creditols.r),shine,creditols.g);
})");

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize));
	basetex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	oversamplingtex.loadImage(ImageCache::getFromMemory(BinaryData::oversampling_png, BinaryData::oversampling_pngSize));
	oversamplingtex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	creditstex.loadImage(ImageCache::getFromMemory(BinaryData::credits_png, BinaryData::credits_pngSize));
	creditstex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	context.extensions.glGenBuffers(1, &arraybuffer);
}
void ProtoAudioProcessorEditor::compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader) {
	shader.reset(new OpenGLShaderProgram(context));
	if(!shader->addVertexShader(vertexshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	if(!shader->addFragmentShader(fragmentshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	shader->link();
}
void ProtoAudioProcessorEditor::renderOpenGL() {
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	baseshader->setUniform("offset",time);
	baseshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	knobshader->use();
	knobshader->setUniform("knobscale",48.f*2.f/getWidth(),48.f*2.f/getHeight());
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < knobcount; i++) {
		knobshader->setUniform("knobpos",((float)knobs[i].x*2-48.f)/getWidth()-1,1-((float)knobs[i].y*2+48.f)/getHeight());
		knobshader->setUniform("knobrot",(knobs[i].value-.5f)*5.5f);
		knobshader->setUniform("hoverstate",hover==i?1:0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	textshader->use();
	for(int i = 0; i < knobcount; i++) {
		textshader->setUniform("hoverstate",hover==i?1:0);
		audioProcessor.logger.drawstring(knobs[i].name,((float)knobs[i].x)/getWidth(),1-((float)knobs[i].y+45)/getHeight(),.5f,1,&textshader);
	}
	textshader->setUniform("hoverstate",0);

	blackshader->use();
	coord = context.extensions.glGetAttribLocation(blackshader->getProgramID(),"aPos");
	blackshader->setUniform("pos",(16.f/getWidth())-1,1-(44.f/getHeight()),2-(32.f/getWidth()),-160.f/getHeight());
	blackshader->setUniform("hoverstate",hover<=-4?1:0);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	if(oversamplingalpha <= 0) {
		glLineWidth(1*dpi);
		visshader->use();
		coord = context.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
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

	else if(oversamplingalpha >= 1) {
		oversamplingshader->use();
		coord = context.extensions.glGetAttribLocation(oversamplingshader->getProgramID(),"aPos");
		context.extensions.glActiveTexture(GL_TEXTURE0);
		oversamplingtex.bind();
		oversamplingshader->setUniform("ostex",0);
		oversamplingshader->setUniform("selection",.458677686f+oversamplinglerped*.2314049587f);
		oversamplingshader->setUniform("texscale",121.f/oversamplingtex.getWidth(),46.f/oversamplingtex.getHeight());
		oversamplingshader->setUniform("pos",-120.f/getWidth(),1-(170.f/getHeight()),242.f/getWidth(),92.f/getHeight());
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	if(creditsalpha >= 1) {
		creditsshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		creditstex.bind();
		creditsshader->setUniform("creditstex",0);
		creditsshader->setUniform("texscale",148.f/creditstex.getWidth(),46.f/creditstex.getHeight());
		creditsshader->setUniform("shineprog",websiteht);
		creditsshader->setUniform("pos",((float)-148)/getWidth(),2/(getHeight()*.5f)-1,148/(getWidth()*.5f), 46/(getHeight()*.5f));
		coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	else if(creditsalpha <= 0)
		audioProcessor.logger.drawstring((String)"Temporary user interface!\nThe look of this VST is \nsubject to change.",0,0,0,1,&textshader);

	audioProcessor.logger.drawstring(audioProcessor.getName()+(String)" (Prototype)",1,1,1,0,&textshader);

	needtoupdate--;
	audioProcessor.logger.drawlog();
}
void ProtoAudioProcessorEditor::openGLContextClosing() {
	baseshader->release();
	knobshader->release();
	textshader->release();
	blackshader->release();
	visshader->release();
	oversamplingshader->release();
	creditsshader->release();

	basetex.release();
	oversamplingtex.release();
	creditstex.release();

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void ProtoAudioProcessorEditor::calcvis() {
	isStereo = knobs[4].value > 0 && knobs[3].value < 1 && knobs[5].value > 0;
	pluginpreset pp;
	for(int i = 0; i < knobcount; i++)
		pp.values[i] = knobs[i].inflate(knobs[i].value);
	for(int c = 0; c < (isStereo ? 2 : 1); c++) {
		for(int i = 0; i < 226; i++) {
			visline[c][i*2] = (i/112.5f)*(1-(16.f/getWidth()))+(16.f/getWidth())-1;
			visline[c][i*2+1] = 1-(62+audioProcessor.plasticfuneral(sin(i/35.8098621957f)*.8f,c,2,pp,audioProcessor.normalizegain(pp.values[1],pp.values[3]))*38)/(getHeight()*.5f);
		}
	}
}
void ProtoAudioProcessorEditor::paint (Graphics& g) { }

void ProtoAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) {
		if(knobs[i].hoverstate < -1) {
			needtoupdate = 2;
			knobs[i].hoverstate++;
		}
	}
	if(held > 0) held--;

	if(oversamplingalpha != (hover<=-4?1:0)) {
		oversamplingalpha = fmax(fmin(oversamplingalpha+(hover<=-4?.17f:-.17f),1),0);
		needtoupdate = 2;
	}

	float os = oversampling?1:0;
	if (oversamplinglerped != os?1:0) {
		needtoupdate = 2;
		if(fabs(oversamplinglerped-os) <= .001f) oversamplinglerped = os;
		oversamplinglerped = oversamplinglerped*.75f+os*.25f;
	}

	if(creditsalpha != ((hover<=-2&&hover>=-3)?1:0)) {
		creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.17f:-.17f),1),0);
		needtoupdate = 2;
	}
	if(creditsalpha <= 0) websiteht = -1;
	if(websiteht > -1 && creditsalpha >= 1) {
		websiteht -= .0815;
		needtoupdate = 2;
	}

	if(audioProcessor.rmscount.get() > 0) {
		rms = sqrt(audioProcessor.rmsadd.get()/audioProcessor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
		audioProcessor.rmsadd = 0;
		audioProcessor.rmscount = 0;
	} else rms *= .9f;

	time = fmod(time+.0002f,1.f);

	context.triggerRepaint();
}

void ProtoAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		oversampling = newValue>.5f;
		needtoupdate = 2;
		return;
	}
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		calcvis();
		needtoupdate = 2;
		return;
	}
}
void ProtoAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht <= -1) websiteht = .65f;
	else if(hover > -2 && prevhover <= -2) websiteht = -2;
	if(prevhover != hover && held == 0) {
		if(hover > -1) knobs[hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
	}
}
void ProtoAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void ProtoAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	held = -1;
	initialdrag = hover;
	if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audioProcessor.undoManager.beginNewTransaction();
		audioProcessor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		audioProcessor.lerpchanged[hover] = true;
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	} else if(hover < -4) {
		oversampling = hover == -6;
		audioProcessor.apvts.getParameter("oversampling")->setValueNotifyingHost(oversampling?1.f:0.f);
		audioProcessor.undoManager.setCurrentTransactionName(oversampling?"Turned oversampling on":"Turned oversampling off");
		audioProcessor.undoManager.beginNewTransaction();
	}
}
void ProtoAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
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

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	} else if (initialdrag == -3) {
		int prevhover = hover;
		hover = recalchover(event.x,event.y)==-3?-3:-2;
		if(hover == -3 && prevhover != -3 && websiteht < -1) websiteht = .65f;
	}
}
void ProtoAudioProcessorEditor::mouseUp(const MouseEvent& event) {
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
		if(hover == -3) {
			if(prevhover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht < -1) websiteht = .65f;
		}
		else if(hover > -1) knobs[hover].hoverstate = -4;
	}
	held = 1;
}
void ProtoAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audioProcessor.undoManager.beginNewTransaction();
}
void ProtoAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int ProtoAudioProcessorEditor::recalchover(float x, float y) {
	if (x >= 8 && x <= (getWidth()-8) && y >= 22 && y <= 102) {
		if(y < 53 || y > 67) return -4;
		float midx = x-(getWidth()*.5);
		if(midx >= -6 && midx <= 22) return -5;
		if(midx >= 23 && midx <= 51) return -6;
		return -4;
	} else if(y >= (getHeight()-48)) {
		if(x >= (getWidth()*.5-74) && x <= (getWidth()*.5+73) && y <= (getHeight()-4)) return -3;
		return -2;
	}
	for(int i = 0; i < knobcount; i++) {
		if(fabs(knobs[i].x-x) <= 24 && fabs(knobs[i].y-y) <= 24) return i;
	}
	return -1;
}
