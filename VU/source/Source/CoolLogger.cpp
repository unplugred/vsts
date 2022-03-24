/*
  ==============================================================================

    Logger.cpp
    Created: 17 Feb 2022 3:09:02pm
    Author:  unplugred

  ==============================================================================
*/

#include "CoolLogger.h"
using namespace gl;

void CoolLogger::init(OpenGLContext* ctx, int w, int h) {
#ifdef ENABLE_TEXT
	context = ctx;
	width = w;
	height = h;
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
	gl_FragColor = vec4(texture2D(tex,texcoord).r);
	gl_FragColor.a = 1-(1-gl_FragColor.a)*.5;
})";
	textshader.reset(new OpenGLShaderProgram(*context));
	textshader->addVertexShader(textvert);
	textshader->addFragmentShader(textfrag);
	textshader->link();

	texttex.loadImage(ImageCache::getFromMemory(BinaryData::dbg_txt_png,BinaryData::dbg_txt_pngSize));
	texttex.bind();
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
#endif
}

void CoolLogger::debug(String str, bool timestamp) {
#ifdef ENABLE_CONSOLE
	DBG(str);

	if(timestamp)
		debuglist[debugreadpos] = Time::getCurrentTime().toString(false,true,true,true) + " " + str;
	else
		debuglist[debugreadpos] = str;
	debugreadpos = fmod(debugreadpos+1,CONSOLE_LENGTH);

	std::ostringstream console;
	for (int i = 0; i < CONSOLE_LENGTH; i++) {
		console << debuglist[(int)fmod(i+debugreadpos,CONSOLE_LENGTH)].toStdString();
		if(i < (CONSOLE_LENGTH-1)) console << "\n";
	}
	std::lock_guard<std::mutex> guard(debugmutex);
	debugtxt = (String)console.str();
#endif
}
void CoolLogger::debug(int str, bool timestamp) {
#ifdef ENABLE_CONSOLE
	debug((String)str,timestamp);
#endif
}
void CoolLogger::debug(float str, bool timestamp) {
#ifdef ENABLE_CONSOLE
	debug((String)str,timestamp);
#endif
}
void CoolLogger::debug(double str, bool timestamp) {
#ifdef ENABLE_CONSOLE
	debug((String)str,timestamp);
#endif
}
void CoolLogger::debug(bool str, bool timestamp) {
#ifdef ENABLE_CONSOLE
	debug((String)(str?"true":"false"),timestamp);
#endif
}

void CoolLogger::drawstring(String txty, float x, float y, float xa, float ya, std::unique_ptr<OpenGLShaderProgram>* shader) {
#ifdef ENABLE_TEXT
	if(shader == nullptr) shader = &textshader;

	(*shader)->use();
	context->extensions.glActiveTexture(GL_TEXTURE0);
	texttex.bind();
	(*shader)->setUniform("tex",0);
	float coord = context->extensions.glGetAttribLocation((*shader)->getProgramID(),"aPos");
	context->extensions.glEnableVertexAttribArray(coord);
	context->extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);

	int i = 0; float c = 0, l = ya-.5;
	std::queue<float> linelength;
	const char* txt = txty.toUTF8();
	while(txt[i] != '\0')
		if(txt[i++] == '\n' || (((++c)+2)*8 > width && txt[i] != '\n'))
			{ linelength.push((c-1)*2*xa+2*(xa-.5)); l+=ya; c = 0; }
	linelength.push((c-1)*2*xa+2*(xa-.5));

	i = 0; c = 0;
	int ll = linelength.front(); linelength.pop();
	float letterw = 8.f/width;
	float letterh = 16.f/height;
	while(txt[i] != '\0') {
		if(txt[i] == '\n') {
			l--; c = 0;
			ll = linelength.front(); linelength.pop();
		} else {
			(*shader)->setUniform("size",c*2-ll,l*2,letterw,letterh);
			(*shader)->setUniform("pos",x*2-1,y*2-1);
			(*shader)->setUniform("letter",(float)((int)txt[i]));
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			c++;
			if((c+2)*8 > width && txt[i+1] != '\n') {
				l--; c = 0;
				ll = linelength.front(); linelength.pop();
			}
		}
		i++;
	}

	context->extensions.glDisableVertexAttribArray(coord);
#endif
}
void CoolLogger::drawlog() {
#ifdef ENABLE_CONSOLE
	std::lock_guard<std::mutex> guard(debugmutex);
	drawstring(debugtxt,.01,.01,0,1);
#endif
}
void CoolLogger::release() {
#ifdef ENABLE_CONSOLE
	textshader->release();
	texttex.release();
#endif
}
