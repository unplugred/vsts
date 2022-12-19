#include "PluginProcessor.h"
#include "PluginEditor.h"

PNCHAudioProcessorEditor::PNCHAudioProcessorEditor (PNCHAudioProcessor& p, float amountt)
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	amount = amountt;
	audioProcessor.apvts.addParameterListener("amount",this);

	float hue = random.nextFloat();
	bool lightness = random.nextFloat() >= .5;
	c1 = Colour::fromHSV(hue,1.f,lightness?1.f:.4f,1.f);
	c2 = Colour::fromHSV(hue+.1666f*(random.nextFloat()>.5?1:-1),1.f,lightness?.4f:1.f,1.f);

#ifdef BANNER
	setSize(128,148+21);
	banneroffset = 21.f/getHeight();
#else
	setSize(128,148);
#endif
	setResizable(false, false);
	dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;

	setOpaque(true);
	if((SystemStats::getOperatingSystemType() & SystemStats::OperatingSystemType::MacOSX != 0)
		context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
PNCHAudioProcessorEditor::~PNCHAudioProcessorEditor() {
	audioProcessor.apvts.removeParameterListener("amount",this);
	stopTimer();
	context.detach();
}

void PNCHAudioProcessorEditor::newOpenGLContextCreated() {
	compileshader(baseshader,
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 letterscale;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = vec2((aPos.x-.0078125)*letterscale.x,1-(1-aPos.y)*letterscale.y);
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform float amount;
uniform vec2 texscale;
uniform vec3 colone;
uniform vec3 coltwo;
out vec4 fragColor;
void main(){
	float id = 0;
	if(uv.x < 0 || uv.x > 5) id = 6;
	else {
		if(uv.y < 0 || uv.x > 1) id = max(floor(uv.x)-floor(uv.y)*5-amount+1,1);
		if(amount >= 1 && id == 5) id = 6;
	}
	if(id >= 6) fragColor = vec4(coltwo,1);
	else {
		float o = texture(tex,vec2(mod(uv.x,1)*texscale.x,1-(1-(mod(uv.y,1)-id))*texscale.y)).r;
		fragColor = vec4(o*colone+(1-o)*coltwo,1);
	}
})");

	compileshader(knobshader,
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 knobrot;
uniform vec2 shake;
uniform float banner;
out vec2 uv;
out vec2 balluv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1+shake,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	balluv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y)-knobrot;
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
in vec2 balluv;
uniform sampler2D tex;
uniform vec3 colone;
uniform vec3 coltwo;
out vec4 fragColor;
void main(){
	vec2 c = texture(tex,uv).gb;
	vec2 ball = texture(tex,balluv).gb;
	float o = (c.r==c.g?1:0)+(ball.r>.5&&ball.g<.5?1:0);
	fragColor = vec4(o*colone+(1-o)*coltwo,((c.r+c.g)>.5)?1:0);
})");

	compileshader(creditsshader,
//CREDITS VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos.x*1.875-0.9375,(aPos.y*(1-banner)+banner*1.03)*1.891892-0.945946,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D creditstex;
uniform float hover;
uniform vec3 colone;
uniform vec3 coltwo;
out vec4 fragColor;
void main(){
	vec3 c = texture(creditstex,uv).rgb;
	float o = (hover==1?c.r:0.0)+c.g+(hover==2?c.b:0.0);
	fragColor = vec4(o*colone+(1-o)*coltwo,1);
})");

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize));
	basetex.bind();
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
void PNCHAudioProcessorEditor::compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader) {
	shader.reset(new OpenGLShaderProgram(context));
	if(!shader->addVertexShader(vertexshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	if(!shader->addFragmentShader(fragmentshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	shader->link();
}
void PNCHAudioProcessorEditor::renderOpenGL() {
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
	baseshader->setUniform("tex",0);
	baseshader->setUniform("texscale",25.f/basetex.getWidth(),21.f/basetex.getHeight());
	baseshader->setUniform("banner",banneroffset);
	baseshader->setUniform("letterscale",getWidth()/25.f,(1-banneroffset)*getHeight()/21.f);
	baseshader->setUniform("amount",(float)floor(amount*31));
	baseshader->setUniform("colone",c1.getFloatRed(),c1.getFloatGreen(),c1.getFloatBlue());
	baseshader->setUniform("coltwo",c2.getFloatRed(),c2.getFloatGreen(),c2.getFloatBlue());
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	if(credits){
		creditsshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		creditstex.bind();
		creditsshader->setUniform("creditstex",0);
		creditsshader->setUniform("texscale",120.f/creditstex.getWidth(),140.f/creditstex.getHeight());
		creditsshader->setUniform("banner",banneroffset);
		creditsshader->setUniform("hover",(float)hover);
		creditsshader->setUniform("colone",c1.getFloatRed(),c1.getFloatGreen(),c1.getFloatBlue());
		creditsshader->setUniform("coltwo",c2.getFloatRed(),c2.getFloatGreen(),c2.getFloatBlue());
		coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
	} else {
		knobshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		basetex.bind();
		knobshader->setUniform("tex",0);
		knobshader->setUniform("texscale",128.f/basetex.getWidth(),148.f/basetex.getHeight());
		knobshader->setUniform("banner",banneroffset);
		float rotato = (amount-.5f)*5.f;
		knobshader->setUniform("knobrot",round(sin(rotato)*38)/128.f,round(cos(rotato)*38)/148.f);
		knobshader->setUniform("shake",.03125f*(random.nextFloat()-.5f)*10.f*rms,.027027f*(1-banneroffset)*(random.nextFloat()-.5f)*10.f*rms);
		knobshader->setUniform("colone",c1.getFloatRed(),c1.getFloatGreen(),c1.getFloatBlue());
		knobshader->setUniform("coltwo",c2.getFloatRed(),c2.getFloatGreen(),c2.getFloatBlue());
		coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	}
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

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
void PNCHAudioProcessorEditor::openGLContextClosing() {
	baseshader->release();
	knobshader->release();
	creditsshader->release();

	basetex.release();
	creditstex.release();

#ifdef BANNER
	bannershader->release();
	bannertex.release();
#endif

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void PNCHAudioProcessorEditor::paint (Graphics& g) { }

void PNCHAudioProcessorEditor::timerCallback() {
	if(audioProcessor.rmscount.get() > 0) {
		rms = sqrt(audioProcessor.rmsadd.get()/audioProcessor.rmscount.get());
		audioProcessor.rmsadd = 0;
		audioProcessor.rmscount = 0;
	} else rms *= .9f;

#ifdef BANNER
	bannerx = fmod(bannerx+.0005f,1.f);
#endif

	context.triggerRepaint();
}

void PNCHAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "amount") amount = newValue;
}
void PNCHAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
}
void PNCHAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void PNCHAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(event.mods.isRightButtonDown()) {
		credits = !credits;
		hover = recalchover(event.x,event.y);
		return;
	}

	held = -1;
	initialdrag = hover;
	if(hover == 0) {
		initialvalue = amount;
		valueoffset = 0;
		audioProcessor.undoManager.beginNewTransaction();
		audioProcessor.apvts.getParameter("amount")->beginChangeGesture();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void PNCHAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(event.mods.isRightButtonDown()) return;
	if(hover == -1) return;
	if(initialdrag == 0) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		audioProcessor.apvts.getParameter("amount")->setValueNotifyingHost(value - valueoffset);

		valueoffset = fmax(fmin(valueoffset,value+.1),value-1.1);
	}
}
void PNCHAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(event.mods.isRightButtonDown()) return;
	if(hover == 0) {
		audioProcessor.undoManager.setCurrentTransactionName(((amount - initialvalue) >= 0 ? "Increased amount" : "Decreased amount"));
		audioProcessor.apvts.getParameter("amount")->endChangeGesture();
		audioProcessor.undoManager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		if(hover == 1) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		else if(hover == 2) credits = false;
		int prevhover = hover;
		hover = recalchover(event.x,event.y);
	}
	held = 1;
}
void PNCHAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover == 0)
		audioProcessor.apvts.getParameter("amount")->setValueNotifyingHost(
			amount+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int PNCHAudioProcessorEditor::recalchover(float x, float y) {
	if (credits) {
		if(x >= 69 && x <= 97 && y >= 93 && y <= 117) return 1;
		if(x >= 8 && x <= 59 && y >= 112 && y <= 137) return 2;
	} else {
		float xx = 60-x;
		float yy = 88-y;
		if(xx*xx+yy*yy<=2601) return 0;
	}
	return -1;
}
