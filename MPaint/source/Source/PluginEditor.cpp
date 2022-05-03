#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace gl;

MPaintAudioProcessorEditor::MPaintAudioProcessorEditor(MPaintAudioProcessor& p, unsigned char soundd, bool errorr)
	: AudioProcessorEditor(&p), audioProcessor(p)
{
	setSize(468, errorr?180:40);
	setResizable(false, false);
	setOpaque(true);

	sound = soundd;
	error = errorr;
	audioProcessor.apvts.addParameterListener("sound",this);

	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
MPaintAudioProcessorEditor::~MPaintAudioProcessorEditor() {
	audioProcessor.apvts.removeParameterListener("sound",this);
	stopTimer();
	context.detach();
}

void MPaintAudioProcessorEditor::newOpenGLContextCreated() {
	vert =
R"(#version 330 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec4 pos;
uniform int sound;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2*pos.xy-1+pos.zw,0,1);
	uv = vec2((aPos.x+sound)*texscale.x,1-(1-aPos.y)*texscale.y);
})";
	frag =
R"(#version 330 core
in vec2 uv;
uniform sampler2D tex;
uniform int errorhover;
void main(){
	gl_FragColor = texture2D(tex,uv);
	if(errorhover > .5 && uv.x > .8 && uv.y < .15 && gl_FragColor.r < .1)
		gl_FragColor = vec4(.972549,0,0,1);
})";
	shader.reset(new OpenGLShaderProgram(context));
	shader->addVertexShader(vert);
	shader->addFragmentShader(frag);
	shader->link();

	errortex.loadImage(ImageCache::getFromMemory(BinaryData::error_png, BinaryData::error_pngSize));
	errortex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	rulertex.loadImage(ImageCache::getFromMemory(BinaryData::ruler_png, BinaryData::ruler_pngSize));
	rulertex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	icontex.loadImage(ImageCache::getFromMemory(BinaryData::largeicon_png, BinaryData::largeicon_pngSize));
	icontex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	context.extensions.glGenBuffers(1, &arraybuffer);

	audioProcessor.logger.init(&context,getWidth(),getHeight());
}
void MPaintAudioProcessorEditor::renderOpenGL() {
	if(needtoupdate < 0) return;

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	auto coord = context.extensions.glGetAttribLocation(shader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	shader->use();

	shader->setUniform("sound",0);
	shader->setUniform("pos",1.f,1.f,0.f,0.f);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	if(error) {
		errortex.bind();
		shader->setUniform("tex",0);
		shader->setUniform("texscale",234.f/errortex.getWidth(),90.f/errortex.getHeight());
		shader->setUniform("errorhover",errorhover?1:0);

	} else {
		rulertex.bind();
		shader->setUniform("tex",0);
		shader->setUniform("texscale",234.f/rulertex.getWidth(),20.f/rulertex.getHeight());
		shader->setUniform("errorhover",0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);

		context.extensions.glActiveTexture(GL_TEXTURE0);
		icontex.bind();
		shader->setUniform("tex",0);
		shader->setUniform("texscale",16.f/icontex.getWidth(),14.f/icontex.getHeight());
		shader->setUniform("sound",sound);
		shader->setUniform("pos",16.f/234.f,14.f/20.f,8.f/234.f,4.f/20.f);
	}
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	context.extensions.glDisableVertexAttribArray(coord);
	needtoupdate--;

	audioProcessor.logger.drawlog();
}
void MPaintAudioProcessorEditor::openGLContextClosing() {
	shader->release();

	errortex.release();
	rulertex.release();
	icontex.release();

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void MPaintAudioProcessorEditor::paint (Graphics& g) { }

void MPaintAudioProcessorEditor::timerCallback() {
	context.triggerRepaint();
}

void MPaintAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID != "sound") return;
	sound = newValue;
	needtoupdate = 2;
	audioProcessor.undoManager.setCurrentTransactionName((String)("Changed sound to sound") += (String)newValue);
	audioProcessor.undoManager.beginNewTransaction();
}
void MPaintAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	if(!error) return;

	bool prevhover = errorhover;
	errorhover = ((event.x/2) >= 192 && (event.y/2) >= 81);
	if(prevhover != errorhover) needtoupdate = 2;
}
void MPaintAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	errorhover = false;
	needtoupdate = 2;
}
void MPaintAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(error) {
		if(errorhover) {
			error = false;
			audioProcessor.error = false;
			setSize(468, 40);
			audioProcessor.logger.height = 40;
			needtoupdate = 2;
		}
		return;
	}

	int x = event.x/2;
	int y = event.y/2;
	if(x < 24 || x > 233 || y < 5 || y > 19) return;
	if(fmod(x-24,14) == 13) return;

	audioProcessor.apvts.getParameter("sound")->setValueNotifyingHost(floorf((x-24)/14)/14.f);
}
