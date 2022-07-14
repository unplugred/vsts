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

	setSize (128, 148);
	setResizable(false, false);

	setOpaque(true);
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
	basevert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 letterscale;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	uv = vec2((aPos.x-.0078125)*letterscale.x,1-(1-aPos.y)*letterscale.y);
})";
	basefrag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D tex;
uniform float amount;
uniform vec2 texscale;
uniform vec3 colone;
uniform vec3 coltwo;
void main(){
	float id = 0;
	if(uv.x < 0 || uv.x > 5) id = 6;
	else {
		if(uv.y < 0 || uv.x > 1) id = max(floor(uv.x)-floor(uv.y)*5-amount+1,1);
		if(amount >= 1 && id == 5) id = 6;
	}
	if(id >= 6) gl_FragColor = vec4(coltwo,1);
	else {
		float o = texture2D(tex,vec2(mod(uv.x,1)*texscale.x,1-(1-(mod(uv.y,1)-id))*texscale.y)).r;
		gl_FragColor = vec4(o*colone+(1-o)*coltwo,1);
	}
})";
	baseshader.reset(new OpenGLShaderProgram(context));
	baseshader->addVertexShader(basevert);
	baseshader->addFragmentShader(basefrag);
	baseshader->link();

	knobvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 knobrot;
uniform vec2 shake;
out vec2 uv;
out vec2 balluv;
void main(){
	gl_Position = vec4(aPos*2-1+shake,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	balluv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y)-knobrot;
})";
	knobfrag =
R"(#version 330 core
in vec2 uv;
in vec2 balluv;
uniform sampler2D tex;
uniform vec3 colone;
uniform vec3 coltwo;
void main(){
	vec2 c = texture2D(tex,uv).gb;
	vec2 ball = texture2D(tex,balluv).gb;
	float o = (c.r==c.g?1:0)+(ball.r>.5&&ball.g<.5?1:0);
	gl_FragColor = vec4(o*colone+(1-o)*coltwo,((c.r+c.g)>.5)?1:0);
})";
	knobshader.reset(new OpenGLShaderProgram(context));
	knobshader->addVertexShader(knobvert);
	knobshader->addFragmentShader(knobfrag);
	knobshader->link();

	creditsvert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos.x*1.875-0.9375,aPos.y*1.891892-0.945946,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})";
	creditsfrag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D creditstex;
uniform float hover;
uniform vec3 colone;
uniform vec3 coltwo;
void main(){
	vec3 c = texture2D(creditstex,uv).rgb;
	float o = (hover==1?c.r:0)+c.g+(hover==2?c.b:0);
	gl_FragColor = vec4(o*colone+(1-o)*coltwo,1);
})";
	creditsshader.reset(new OpenGLShaderProgram(context));
	creditsshader->addVertexShader(creditsvert);
	creditsshader->addFragmentShader(creditsfrag);
	creditsshader->link();

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

	context.extensions.glGenBuffers(1, &arraybuffer);

	audioProcessor.logger.init(&context,getWidth(),getHeight());
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
	baseshader->setUniform("letterscale",getWidth()/25.f,getHeight()/21.f);
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
		float rotato = (amount-.5f)*5.f;
		knobshader->setUniform("knobrot",round(sin(rotato)*38)/128.f,round(cos(rotato)*38)/148.f);
		knobshader->setUniform("shake",.03125f*(random.nextFloat()-.5f)*10.f*rms,.027027f*(random.nextFloat()-.5f)*10.f*rms);
		knobshader->setUniform("colone",c1.getFloatRed(),c1.getFloatGreen(),c1.getFloatBlue());
		knobshader->setUniform("coltwo",c2.getFloatRed(),c2.getFloatGreen(),c2.getFloatBlue());
		coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	}
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	audioProcessor.logger.drawlog();
}
void PNCHAudioProcessorEditor::openGLContextClosing() {
	baseshader->release();
	knobshader->release();
	creditsshader->release();

	basetex.release();
	creditstex.release();

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
		if(x >= 8 && x <= 59 && y >= 112 && y <= 132) return 2;
	} else {
		float xx = 60-x;
		float yy = 88-y;
		if(xx*xx+yy*yy<=2601) return 0;
	}
	return -1;
}
