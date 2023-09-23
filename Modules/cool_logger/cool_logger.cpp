#include "cool_logger.h"

void CoolLogger::init(OpenGLContext* ctx, float bannero, int w, int h) {
	font.init(ctx,bannero,w,h,0,ImageCache::getFromMemory(dbg_txt_png,dbg_txt_pngSize),128,256,16,true,8,16);
}
void CoolLogger::debug(String str, bool timestamp) {
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
}
void CoolLogger::debug(int str, bool timestamp) {
	debug((String)str,timestamp);
}
void CoolLogger::debug(float str, bool timestamp) {
	debug((String)str,timestamp);
}
void CoolLogger::debug(double str, bool timestamp) {
	debug((String)str,timestamp);
}
void CoolLogger::debug(bool str, bool timestamp) {
	debug((String)(str?"true":"false"),timestamp);
}
void CoolLogger::drawlog() {
	std::lock_guard<std::mutex> guard(debugmutex);
	font.drawstring(1,1,1,1,0,0,0,0.5,debugtxt,0,.01,.99,0,1);
}

void CoolFont::init(OpenGLContext* ctx, float bannero, int w, int h, float _dpi, Image image, int texw, int texh, int lineh, bool is_mono, int monow, int monoh, String vert, String frag) {
	context = ctx;
	width = w;
	height = h;
	texwidth = texw;
	texheight = texh;
	mono = is_mono;
	monowidth = monow;
	monoheight = monoh;
	lineheight = lineh;
	banneroffset = bannero;
	dpi = _dpi;

	shader.reset(new OpenGLShaderProgram(*context));
	if(vert == "")
		shader->addVertexShader(
R"(#version 150 core
in vec2 aPos;
uniform vec4 pos;
uniform vec4 texpos;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*pos.zw+pos.xy)*2-1,0,1);
	gl_Position.y *= -1;
	uv = (aPos*texpos.zw+texpos.xy);
	uv.y = 1-uv.y;
})");
	else
		shader->addVertexShader(vert);
	if(frag == "")
		shader->addFragmentShader(
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform int channel;
uniform vec2 res;
uniform float smoot;
uniform float dpi;
uniform vec4 bg;
uniform vec4 fg;
out vec4 fragColor;
void main(){
	vec2 nuv = uv;
	if(dpi > 1 && smoot < .5) {
		nuv *= res;
		if(mod(nuv.x,1)>.5) nuv.x = floor(nuv.x)+(1-min((1-mod(nuv.x,1))*dpi*2,.5));
		else nuv.x = floor(nuv.x)+min(mod(nuv.x,1)*dpi*2,.5);
		if(mod(nuv.y,1)>.5) nuv.y = floor(nuv.y)+(1-min((1-mod(nuv.y,1))*dpi*2,.5));
		else nuv.y = floor(nuv.y)+min(mod(nuv.y,1)*dpi*2,.5);
		nuv /= res;
	}
	float text = 0;
	if(channel == 1)
		text = texture(tex,nuv).g;
	else if(channel == 2)
		text = texture(tex,nuv).b;
	else
		text = texture(tex,nuv).r;
	if(dpi > 1 && smoot > .5)
		text = (text-.5)*dpi+.5;
	fragColor = text*fg+(1-text)*bg;
})");
	else
		shader->addFragmentShader(frag);
	shader->link();

	tex.loadImage(image);
	tex.bind();
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
}

void CoolFont::drawstringmono(float fgr, float fgg, float fgb, float fga, float bgr, float bgg, float bgb, float bga, String txty, int channel, float x, float y, float xa, float ya) {
	shader->use();
	context->extensions.glActiveTexture(GL_TEXTURE0);
	tex.bind();
	if((fmod(dpi,1) > .00001f && fmod(dpi,1) < .99999f) || smooth) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		shader->setUniform("dpi",dpi*scale);
	} else {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		shader->setUniform("dpi",0.f);
	}
	shader->setUniform("tex",0);
	shader->setUniform("channel",channel);
	shader->setUniform("fg",fgr,fgg,fgb,fga);
	shader->setUniform("bg",bgr,bgg,bgb,bga);
	shader->setUniform("res",texwidth,texheight);
	shader->setUniform("smoot",smooth?1.f:0.f);
	float coord = context->extensions.glGetAttribLocation(shader->getProgramID(),"aPos");
	context->extensions.glEnableVertexAttribArray(coord);
	context->extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);

	int letter = 0;
	int letterx = 0;
	float line = -ya;
	std::queue<float> linelength;
	const char* txt = txty.toUTF8();
	float letterw = ((float)monowidth)*scale/width;
	float letterh = ((float)monoheight)*scale/height;
	while(txt[letter] != '\0') {
		if(txt[letter] == '\n' || (((++letterx)+1)*letterw > 1 && txt[letter] != '\n')) {
			linelength.push(letterx);
			line -= ya;
			letterx = 0;
		}
		++letter;
	}
	linelength.push(letterx);

	letter = 0;
	letterx = 0;
	int currentlinelength = linelength.front();
	linelength.pop();
	while(txt[letter] != '\0') {
		if(txt[letter] != '\n') {
			shader->setUniform("pos",
					x+(letterx-currentlinelength*xa)*letterw,
					y*(1-banneroffset)+line*lineheight*scale/height,letterw,letterh);
			shader->setUniform("texpos",
				.00001f+fmod(txt[letter],16)*.0625f,
				.00001f+floor(txt[letter]*.0625f)*.0625f,
				.0625f,.0625f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			++letterx;
		}
		if(txt[letter] == '\n' || (letterx+1)*letterw > 1) {
			++line;
			letterx = 0;
			currentlinelength = linelength.front();
			linelength.pop();
		}
		++letter;
	}

	context->extensions.glDisableVertexAttribArray(coord);
}
void CoolFont::drawstring(float fgr, float fgg, float fgb, float fga, float bgr, float bgg, float bgb, float bga, String txty, int channel, float x, float y, float xa, float ya) {
	if(mono) {
		drawstringmono(fgr,fgg,fgb,fga,bgr,bgg,bgb,bga,txty,channel,x,y,xa,ya);
		return;
	}

	shader->use();
	context->extensions.glActiveTexture(GL_TEXTURE0);
	tex.bind();
	if((fmod(dpi,1) > .00001f && fmod(dpi,1) < .99999f) || smooth) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		shader->setUniform("dpi",dpi*scale);
	} else {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		shader->setUniform("dpi",0.f);
	}
	shader->setUniform("tex",0);
	shader->setUniform("channel",channel);
	shader->setUniform("fg",fgr,fgg,fgb,fga);
	shader->setUniform("bg",bgr,bgg,bgb,bga);
	shader->setUniform("res",texwidth,texheight);
	shader->setUniform("smoot",smooth?1.f:0.f);
	float coord = context->extensions.glGetAttribLocation(shader->getProgramID(),"aPos");
	context->extensions.glEnableVertexAttribArray(coord);
	context->extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);

	int letter = 0;
	float letterx = 0;
	float line = -ya+1;
	std::queue<float> linelength;
	const char* txt = txty.toUTF8();
	while(txt[letter] != '\0') {
		float letterw = 0;
		if(txt[letter] != '\n') {
			int uvindex = finduvindex(txt[letter],channel);
			if(uvindex >= 0) {
				letterw = ((float)uvmap[channel][uvindex][7])*scale/width;
				letterx += letterw;
			}
		}
		if(txt[letter] == '\n' || letterx > 1) {
			if(letterx > 1)
				letterx -= letterw;
			linelength.push(letterx);
			line -= ya;
			letterx = letterw;
		}
		++letter;
	}
	linelength.push(letterx);

	letter = 0;
	letterx = 0;
	float currentlinelength = linelength.front();
	linelength.pop();
	while(txt[letter] != '\0') {
		int uvindex = finduvindex(txt[letter],channel);
		if(txt[letter] == '\n' || (uvindex != -1 && (letterx+((float)uvmap[channel][uvindex][7])*scale/width) > 1)) {
			++line;
			letterx = 0;
			currentlinelength = linelength.front();
			linelength.pop();
		}
		if(txt[letter] != '\n' && uvindex != -1) {
			float currentx = letterx+uvmap[channel][uvindex][5]*scale/width-currentlinelength*xa;
			shader->setUniform("pos",
				x+currentx,
				y*(1-banneroffset)+(line*lineheight+uvmap[channel][uvindex][6])*scale/height+(currentx+((float)uvmap[channel][uvindex][3])*.5f*scale/width)*slant,
				((float)uvmap[channel][uvindex][3])*scale/width,
				((float)uvmap[channel][uvindex][4])*scale/height);
			shader->setUniform("texpos",
				.00001f+((float)uvmap[channel][uvindex][1])/texwidth,
				.00001f+((float)uvmap[channel][uvindex][2])/texheight,
				((float)uvmap[channel][uvindex][3])/texwidth,
				((float)uvmap[channel][uvindex][4])/texheight);
			shader->setUniform("letter",(float)letter);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			letterx += ((float)uvmap[channel][uvindex][7])*scale/width;
		}
		++letter;
	}

	context->extensions.glDisableVertexAttribArray(coord);
}
int CoolFont::finduvindex(int letter, int channel) {
	int iterator = -1;
	while(true) {
		++iterator;
		if(uvmap[channel][iterator][0] == letter)
			return iterator;
		if(uvmap[channel][iterator][0] == -1)
			return -1;
	}
}
void CoolFont::release() {
	shader->release();
	tex.release();
}
