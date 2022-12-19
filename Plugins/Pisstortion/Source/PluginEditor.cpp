#include "PluginProcessor.h"
#include "PluginEditor.h"

PisstortionAudioProcessorEditor::PisstortionAudioProcessorEditor (PisstortionAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params)
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 3; y++) {
			int i = x+y*2;
			knobs[i].x = x*106+68;
			knobs[i].y = y*100+144;
			knobs[i].id = params.pots[i].id;
			knobs[i].name = params.pots[i].name;
			knobs[i].value = params.pots[i].normalize(state.values[i]);
			knobs[i].lerpedvalue = knobs[i].value;
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

	for (int i = 0; i < 20; i++) {
		bubbleregen(i);
		bubbles[i].wiggleage = random.nextFloat();
		bubbles[i].moveage = random.nextFloat();
	}

#ifdef BANNER
	setSize(242,462+21);
	banneroffset = 21.f/getHeight();
#else
	setSize(242,462);
#endif
	setResizable(false, false);
	calcvis();
	dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;

	setOpaque(true);
	if((SystemStats::getOperatingSystemType() & SystemStats::OperatingSystemType::MacOSX) != 0)
		context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
PisstortionAudioProcessorEditor::~PisstortionAudioProcessorEditor() {
	for (int i = 0; i < knobcount; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	audioProcessor.apvts.removeParameterListener("oversampling",this);
	stopTimer();
	context.detach();
}

void PisstortionAudioProcessorEditor::newOpenGLContextCreated() {
	compileshader(baseshader,
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 texscale;
out vec2 v_TexCoord;
out vec2 circlecoord;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	v_TexCoord = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	circlecoord = aPos;
})",
//BASE FRAG
R"(#version 150 core
in vec2 v_TexCoord;
in vec2 circlecoord;
uniform sampler2D circletex;
uniform sampler2D basetex;
uniform float dpi;
out vec4 fragColor;
void main(){
	float bubbles = texture(circletex,circlecoord).r;
	vec3 c = max(min((texture(basetex,v_TexCoord).rgb-.5)*dpi+.5,1),0);
	if(c.g >= 1) c.b = 0;
	if(bubbles > 0 && v_TexCoord.y > .7)
		c = vec3(c.r,c.g*(1-bubbles),c.b+c.g*bubbles);
	float gradient = (1-v_TexCoord.y)*.5+bubbles*.3;
	float grayscale = c.g*.95+c.b*.85+(1-c.r-c.g-c.b)*.05;
	fragColor = vec4(vec3(grayscale)+c.r*gradient+c.r*(1-gradient)*vec3(0.984,0.879,0.426),1);
})");

	compileshader(knobshader,
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 texscale;
uniform vec2 knobscale;
uniform float ratio;
uniform vec2 knobpos;
uniform float knobrot;
out vec2 v_TexCoord;
out vec2 circlecoord;
void main(){
	vec2 pos = ((aPos*2-1)*knobscale)/vec2(ratio,1);
	gl_Position = vec4(
		(pos.x*cos(knobrot)-pos.y*sin(knobrot))*ratio-1+knobpos.x,
		pos.x*sin(knobrot)+pos.y*cos(knobrot)-1+knobpos.y,0,1);
	v_TexCoord = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	circlecoord = vec2(gl_Position.x,(gl_Position.y-banner)/(1-banner))*.5+.5;
})",
//KNOB FRAG
R"(#version 150 core
in vec2 v_TexCoord;
in vec2 circlecoord;
uniform sampler2D knobtex;
uniform sampler2D circletex;
uniform float hover;
out vec4 fragColor;
void main(){
	vec3 c = texture(knobtex,v_TexCoord).rgb;
	if(c.r > 0) {
		float bubbles = texture(circletex,circlecoord).r;
		float col = max(min(c.g*4-(1-hover)*3,1),0);
		col = (1-col)*bubbles+col;
		col = .05 + c.b*.8 + col*.1;
		fragColor = vec4(vec3(col),c.r);
	} else fragColor = vec4(0);
})");

	compileshader(visshader,
//VIS VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 texscale;
out vec2 basecoord;
void main(){
	gl_Position = vec4(aPos.x,aPos.y*(1-banner)+banner,0,1);
	basecoord = vec2((aPos.x+1)*texscale.x*.5,1-(1-aPos.y)*texscale.y*.5);
})",
//VIS FRAG
R"(#version 150 core
in vec2 basecoord;
uniform sampler2D basetex;
uniform float alpha;
out vec4 fragColor;
void main(){
	float base = texture(basetex,basecoord).r;
	fragColor = vec4(.05,.05,.05,base <= 0 ? alpha : 0.0);
})");

	compileshader(oversamplingshader,
//OVERSAMPLING VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 texscale;
uniform float selection;
out vec2 basecoord;
out vec2 highlightcoord;
void main(){
	gl_Position = vec4(aPos.x-.5,aPos.y*(1-banner)*.2+.7+banner*.3,0,1);
	basecoord = vec2((aPos.x+.5)*.5*texscale.x,1-(1.5-aPos.y)*.1*texscale.y);
	highlightcoord = vec2((aPos.x-selection)*4.3214285714,aPos.y*3.3-1.1642857143);
})",
//OVERSAMPLING FRAG
R"(#version 150 core
in vec2 basecoord;
in vec2 highlightcoord;
uniform sampler2D basetex;
uniform float alpha;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec3 tex = texture(basetex,basecoord).rgb;
	if(tex.r <= 0) {
		float bleh = 0;
		if(tex.g >= .99 && tex.b > .02) bleh = min(max((tex.b-.5)*dpi+.5,0),1);
		if(highlightcoord.x>0&&highlightcoord.x<1&&highlightcoord.y>0&&highlightcoord.y<1)bleh=1-bleh;
		fragColor = vec4(.05,.05,.05,bleh*alpha);
	} else {
		fragColor = vec4(0);
	}
})");

	compileshader(creditsshader,
//CREDITS VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 texscale;
uniform vec2 basescale;
out vec2 v_TexCoord;
out vec2 basecoord;
void main(){
	gl_Position = vec4(aPos.x*2-1,1-(1-aPos.y*(1-banner)*(57./462.)-banner)*2,0,1);
	v_TexCoord = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	basecoord = vec2(aPos.x*basescale.x,1-(1-aPos.y*(57./462.))*basescale.y);
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 v_TexCoord;
in vec2 basecoord;
uniform sampler2D basetex;
uniform sampler2D creditstex;
uniform float alpha;
uniform float shineprog;
uniform float dpi;
out vec4 fragColor;
void main(){
	float y = (v_TexCoord.y+alpha)*1.1875;
	float creditols = 0;
	float shine = 0;
	vec3 base = texture(basetex,basecoord).rgb;
	if(base.r <= 0) {
		if(y < 1)
			creditols = texture(creditstex,vec2(v_TexCoord.x,y)).b;
		else if(y > 1.1875 && y < 2.1875) {
			y -= 1.1875;
			creditols = texture(creditstex,vec2(v_TexCoord.x,y)).r;
			if(v_TexCoord.x+shineprog < 1 && v_TexCoord.x+shineprog > .582644628099)
				shine = max(min((texture(creditstex,vec2(v_TexCoord.x+shineprog,y)).g*min(base.g+base.b,1)-.5)*dpi+.5,1),0);
		}
	}
	fragColor = vec4(vec3(.05+shine*.8),max(min((creditols-.5)*dpi+.5,1),0));
})");

	compileshader(circleshader,
//CIRCLE VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 pos;
uniform float ratio;
out vec2 v_TexCoord;
void main(){
	float f = .1;
	gl_Position = vec4((aPos*2-1)*vec2(1,ratio)*f+pos*2-1,0,1);
	v_TexCoord = aPos*2-1;
})",
//CIRCLE FRAG
R"(#version 150 core
in vec2 v_TexCoord;
out vec4 fragColor;
void main(){
	float x = v_TexCoord.x*v_TexCoord.x+v_TexCoord.y*v_TexCoord.y;
	float f = .5;
	fragColor = vec4(1,1,1,(x>(1-(1-f)*.5)?(1-x):(x-f))*100);
})");

	framebuffer.initialise(context, 242*dpi, 462*dpi);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize));
	basetex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
void PisstortionAudioProcessorEditor::compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader) {
	shader.reset(new OpenGLShaderProgram(context));
	if(!shader->addVertexShader(vertexshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	if(!shader->addFragmentShader(fragmentshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	shader->link();
}
void PisstortionAudioProcessorEditor::renderOpenGL() {
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	framebuffer.makeCurrentRenderingTarget();
	OpenGLHelpers::clear(Colour::fromRGB(0,0,0));
	circleshader->use();
	circleshader->setUniform("ratio",((float)getWidth())/getHeight());
	coord = context.extensions.glGetAttribLocation(circleshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < 20; i++) {
		circleshader->setUniform("pos",bubbles[i].xoffset+
			sin(bubbles[i].moveage*bubbles[i].movespeed+bubbles[i].moveoffset)*bubbles[i].moveamount+
			sin(bubbles[i].wiggleage)*bubbles[i].wiggleamount,
			bubbles[i].moveage+.05f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);
	framebuffer.releaseAsRenderingTarget();

	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	baseshader->setUniform("circletex",1);
	baseshader->setUniform("banner",banneroffset);
	baseshader->setUniform("dpi",(float)fmax(dpi,1));
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
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	knobshader->setUniform("circletex",1);
	knobshader->setUniform("banner",banneroffset);
	knobshader->setUniform("texscale",108.f/knobtex.getWidth(),108.f/knobtex.getHeight());
	knobshader->setUniform("knobscale",54.f/getWidth(),54.f/getHeight());
	knobshader->setUniform("ratio",((float)getHeight())/getWidth());
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for (int i = 0; i < knobcount; i++) {
		knobshader->setUniform("knobpos",((float)knobs[i].x*2)/getWidth(),2-((float)knobs[i].y*2)/getHeight());
		knobshader->setUniform("knobrot",(float)fmod((knobs[i].lerpedvalue-.5f)*.748f-.125f,1)*-6.2831853072f);
		knobshader->setUniform("hover",knobs[i].hover<0.5?4*knobs[i].hover*knobs[i].hover*knobs[i].hover:1-(float)pow(2-2*knobs[i].hover,3)/2);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	float osalpha = oversamplingalpha;
	if(oversamplingalpha < 1) {
		if(oversamplingalpha > 0)
			osalpha = oversamplingalpha<0.5?4*oversamplingalpha*oversamplingalpha*oversamplingalpha:1-(float)pow(2-2*oversamplingalpha,3)/2;
		glLineWidth(1.3*dpi);
		visshader->use();
		coord = context.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
		context.extensions.glActiveTexture(GL_TEXTURE0);
		basetex.bind();
		visshader->setUniform("basetex",0);
		visshader->setUniform("banner",banneroffset);
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
		oversamplingshader->setUniform("dpi",(float)fmax(dpi,1));
		oversamplingshader->setUniform("banner",banneroffset);
		oversamplingshader->setUniform("alpha",osalpha);
		oversamplingshader->setUniform("selection",.458677686f+oversamplinglerped*.2314049587f);
		oversamplingshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	creditsshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	context.extensions.glActiveTexture(GL_TEXTURE1);
	creditstex.bind();
	creditsshader->setUniform("basetex",0);
	creditsshader->setUniform("dpi",(float)fmax(dpi,1));
	creditsshader->setUniform("banner",banneroffset);
	creditsshader->setUniform("basescale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
	creditsshader->setUniform("creditstex",1);
	creditsshader->setUniform("texscale",242.f/creditstex.getWidth(),48.f/creditstex.getHeight());
	creditsshader->setUniform("alpha",creditsalpha<0.5?4*creditsalpha*creditsalpha*creditsalpha:1-(float)pow(2-2*creditsalpha,3)/2);
	creditsshader->setUniform("shineprog",websiteht);
	coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
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
void PisstortionAudioProcessorEditor::openGLContextClosing() {
	circleshader->release();
	baseshader->release();
	knobshader->release();
	visshader->release();
	oversamplingshader->release();
	creditsshader->release();

	basetex.release();
	knobtex.release();
	creditstex.release();

	framebuffer.release();

#ifdef BANNER
	bannershader->release();
	bannertex.release();
#endif

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void PisstortionAudioProcessorEditor::calcvis() {
	isStereo = knobs[4].value > 0 && knobs[1].value > 0 && knobs[5].value > 0;
	pluginpreset pp;
	for(int i = 0; i < knobcount; i++)
		pp.values[i] = knobs[i].inflate(knobs[i].value);
	for(int c = 0; c < (isStereo ? 2 : 1); c++) {
		for(int i = 0; i < 226; i++) {
			visline[c][i*2] = (i+8)/121.f-1;
			visline[c][i*2+1] = 1-(48+audioProcessor.pisstortion(sin(i/35.8098621957f)*.8f,c,2,pp,false)*38)/231.f;
		}
	}
}
void PisstortionAudioProcessorEditor::paint (Graphics& g) { }

void PisstortionAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) {
		knobs[i].lerpedvalue = knobs[i].lerpedvalue*.6f+knobs[i].value*.4;
		if(knobs[i].hover != (hover==i?1:0))
			knobs[i].hover = fmax(fmin(knobs[i].hover+(hover==i?.07f:-.07f), 1), 0);
	}
	if(held > 0) held--;

	if(oversamplingalpha != (hover<=-4?1:0))
		oversamplingalpha = fmax(fmin(oversamplingalpha+(hover<=-4?.07f:-.07f),1),0);

	float os = oversampling?1:0;
	if(oversamplinglerped != os?1:0) {
		if(fabs(oversamplinglerped-os) <= .001f) oversamplinglerped = os;
		oversamplinglerped = oversamplinglerped*.75f+os*.25f;
	}

	if(creditsalpha != ((hover<=-2&&hover>=-3)?1:0))
		creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.05f:-.05f),1),0);
	if(creditsalpha <= 0) websiteht = -1;
	if(websiteht >= -.227273) websiteht -= .05;

	if(audioProcessor.rmscount.get() > 0) {
		rms = sqrt(audioProcessor.rmsadd.get()/audioProcessor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
		audioProcessor.rmsadd = 0;
		audioProcessor.rmscount = 0;
	} else rms *= .9f;
	rmslerped = rmslerped*.6f+rms*.4f;

	for(int i = 0; i < 20; i++) {
		bubbles[i].moveage += bubbles[i].yspeed*(1+rmslerped*30);
		if(bubbles[i].moveage >= 1)
			bubbleregen(i);
		else
			bubbles[i].wiggleage = fmod(bubbles[i].wiggleage+bubbles[i].wigglespeed,6.2831853072f);
	}

	if (audioProcessor.updatevis.get()) {
		calcvis();
		audioProcessor.updatevis = false;
	}

#ifdef BANNER
	bannerx = fmod(bannerx+.0005f,1.f);
#endif

	context.triggerRepaint();
}
void PisstortionAudioProcessorEditor::bubbleregen(int i) {
	float tradeoff = random.nextFloat();
	bubbles[i].wiggleamount = tradeoff*.05f+random.nextFloat()*.005f;
	bubbles[i].wigglespeed = (1-tradeoff)*.15f+random.nextFloat()*.015f;
	bubbles[i].wiggleage = random.nextFloat()*6.2831853072f;
	bubbles[i].moveamount = random.nextFloat()*.5f;
	bubbles[i].movespeed = random.nextFloat()*5.f;
	bubbles[i].moveoffset = random.nextFloat()*6.2831853072f;
	bubbles[i].moveage = 0;
	bubbles[i].yspeed = random.nextFloat()*.0015f+.00075f;
	bubbles[i].xoffset = random.nextFloat();
}

void PisstortionAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		oversampling = newValue>.5f;
		return;
	}
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		calcvis();
		return;
	}
}
void PisstortionAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
}
void PisstortionAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void PisstortionAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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
void PisstortionAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
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
void PisstortionAudioProcessorEditor::mouseUp(const MouseEvent& event) {
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
			else if(websiteht < -.227273) websiteht = 0.7933884298f;
		}
	}
	held = 1;
}
void PisstortionAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audioProcessor.undoManager.beginNewTransaction();
}
void PisstortionAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int PisstortionAudioProcessorEditor::recalchover(float x, float y) {
	if (x >= 8 && x <= 234 && y >= 8 && y <= 87) {
		if(y < 39 || y > 53) return -4;
		if(x >= 115 && x <= 143) return -5;
		if(x >= 144 && x <= 172) return -6;
		return -4;
	} else if(y >= 403 && y < 462) {
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
