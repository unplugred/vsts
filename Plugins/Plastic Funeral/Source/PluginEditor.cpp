#include "PluginProcessor.h"
#include "PluginEditor.h"

PFAudioProcessorEditor::PFAudioProcessorEditor (PFAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params)
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

#ifdef BANNER
	setSize(242,462+21);
	banneroffset = 21.f/getHeight();
#else
	setSize(242,462);
#endif
	setResizable(false, false);
	calcvis();

	setOpaque(true);
	context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
PFAudioProcessorEditor::~PFAudioProcessorEditor() {
	for(int i = 0; i < knobcount; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	audioProcessor.apvts.removeParameterListener("oversampling",this);
	stopTimer();
	context.detach();
}

void PFAudioProcessorEditor::newOpenGLContextCreated() {
	compileshader(baseshader,
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 texscale;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*vec2(2,(1-banner)*2)-1,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D basetex;
out vec4 fragColor;
void main(){
	fragColor = vec4(texture(basetex,uv).r,0,0,1);
})");

	compileshader(knobshader,
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 basescale;
uniform float knobrot;
uniform vec2 knobscale;
uniform vec2 knobpos;
uniform float ratio;
out vec2 uv;
out vec2 hovercoord;
out vec2 basecoord;
void main(){
	vec2 pos = (aPos*2*knobscale-vec2(knobscale.x,knobscale.y*.727272727))/vec2(ratio,1);
	float rot = mod(knobrot,1)*-6.2831853072;
	gl_Position = vec4(
		(pos.x*cos(rot)-pos.y*sin(rot))*ratio-1+knobpos.x,
		pos.x*sin(rot)+pos.y*cos(rot)-1+knobpos.y,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
	hovercoord = (aPos-vec2(.5,.3636363636))/vec2(1.1379310345,1);
	hovercoord = vec2(
		(hovercoord.x*cos(rot*2)-hovercoord.y*sin(rot*2))*1.1379310345+.5,
		hovercoord.x*sin(rot*2)+hovercoord.y*cos(rot*2)+.36363636);
	basecoord = vec2((gl_Position.x*.5+.5)*basescale.x,1-(.5-gl_Position.y*.5)*basescale.y);
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
in vec2 hovercoord;
in vec2 basecoord;
uniform float knobcolor;
uniform float knobrot;
uniform vec2 texscale;
uniform sampler2D knobtex;
uniform sampler2D basetex;
uniform float id;
uniform float hoverstate;
out vec4 fragColor;
void main(){
	float index = floor(knobrot*6);
	fragColor = texture(knobtex,uv-vec2(0,texscale.y*mod(index,6)));
	if(fragColor.r > 0) {
		float lerp = mod(knobrot*6,1);
		float col = fragColor.b*(1-lerp)+texture(knobtex,uv-vec2(0,texscale.y*mod(index+1,6))).b*lerp;
		if(col<.5) col = knobcolor*col*2;
		else col = knobcolor+(col-.5)*.8;
		if(hoverstate != 0) {
			float hover = 0;
			if(hovercoord.x < .95 && hovercoord.x > .05 && hovercoord.y > .005 && hovercoord.y < .78)
				hover = texture(knobtex,vec2(hovercoord.x*texscale.x,1-(1-hovercoord.y+id)*texscale.y)).g;
			if(hoverstate < -1) col += (hoverstate==-3?(1-hover):hover)*.3;
			else col = col*(1-hover)+.01171875*hover;
		}
		fragColor = vec4(col+texture(basetex,basecoord).g-.5,0,0,fragColor.r);
	} else fragColor = vec4(0);
})");

	compileshader(visshader,
//VIS VERT
R"(#version 150 core
in vec3 aPos;
uniform float banner;
uniform vec2 texscale;
out vec2 basecoord;
out float linepos;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)-banner),0,1);
	basecoord = vec2((aPos.x+1)*texscale.x*.5,1-(1-aPos.y)*texscale.y*.5);
	linepos = aPos.z;
})",
//VIS FRAG
R"(#version 150 core
in vec2 basecoord;
in float linepos;
uniform sampler2D basetex;
uniform float alpha;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec2 base = texture(basetex,basecoord).rb;
	float gradient = (base.g<.5?base.g:(1-base.g))*alpha;
	fragColor = vec4(base.r*(1-gradient)+gradient,0,0,min((1-abs(linepos*2-1))*dpi*1.3,1));
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
	gl_Position = vec4(aPos.x-.5,aPos.y*(1-banner)*.2+.7-banner*1.7,0,1);
	basecoord = vec2((aPos.x+.5)*.5*texscale.x,1-(1.5-aPos.y)*.1*texscale.y);
	highlightcoord = vec2((aPos.x-selection)*4.3214285714,aPos.y*3.3-1.1642857143);
})",
//OVERSAMPLING FRAG
R"(#version 150 core
in vec2 basecoord;
in vec2 highlightcoord;
uniform sampler2D basetex;
uniform float alpha;
out vec4 fragColor;
void main(){
	float tex = texture(basetex,basecoord).b;
	if(highlightcoord.x>0&&highlightcoord.x<1&&highlightcoord.y>0&&highlightcoord.y<1)tex=1-tex;
	fragColor = vec4(1,0,0,tex>.5?((1-tex)*alpha):0.0);
})");

	compileshader(creditsshader,
//CREDITS VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 texscale;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos.x*2-1,1-(1-aPos.y*(1-banner)*(59./462.))*2,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D creditstex;
uniform float alpha;
uniform float shineprog;
out vec4 fragColor;
void main(){
	vec2 creditols = texture(creditstex,uv).rb;
	float shine = 0;
	if(uv.x+shineprog < 1 && uv.x+shineprog > .582644628099)
		shine = texture(creditstex,uv+vec2(shineprog,0)).g*creditols.r*.8;
	fragColor = vec4(creditols.g+shine,0,0,alpha);
})");

	compileshader(ppshader,
//PP VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform float shake;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = aPos+vec2(shake,0);
})",
//PP FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D buffertex;
uniform float chroma;
out vec4 fragColor;
void main(){
	fragColor = vec4(vec3((
		texture(buffertex,uv+vec2(chroma,0)).r+
		texture(buffertex,uv-vec2(chroma,0)).r+
		texture(buffertex,uv).r)*.333333333),1.);
})");

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize));
	basetex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	framebuffer.initialise(context, 242*dpi, 462*dpi);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

	audioProcessor.logger.init(&context,banneroffset,getWidth(),getHeight());
}
void PFAudioProcessorEditor::compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader) {
	shader.reset(new OpenGLShaderProgram(context));
	if(!shader->addVertexShader(vertexshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at melody@unplug.red. THANKS!","OK!");
	if(!shader->addFragmentShader(fragmentshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at melody@unplug.red. THANKS!","OK!");
	shader->link();
}
void PFAudioProcessorEditor::renderOpenGL() {
	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	if(context.getRenderingScale() != dpi) {
		dpi = context.getRenderingScale();
		framebuffer.release();
		framebuffer.initialise(context, 242*dpi, 462*dpi);
		glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	if(needtoupdate > 0) {
		framebuffer.makeCurrentRenderingTarget();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		baseshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		basetex.bind();
		baseshader->setUniform("basetex",0);
		baseshader->setUniform("banner",banneroffset);
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
		basetex.bind();
		knobshader->setUniform("basetex",1);
		knobshader->setUniform("texscale",58.f/knobtex.getWidth(),66.f/knobtex.getHeight());
		knobshader->setUniform("knobscale",58.f/getWidth(),66.f/getHeight());
		knobshader->setUniform("ratio",((float)getHeight())/getWidth());
		knobshader->setUniform("basescale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
		coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		knobshader->setUniform("knobcolor",.2265625f);
		for(int i = 0; i < knobcount; i++) {
			if(i == 2) knobshader->setUniform("knobcolor",.61328125f);
			knobshader->setUniform("knobpos",((float)knobs[i].x*2)/getWidth(),2-((float)knobs[i].y*2)/getHeight()-banneroffset*2);
			knobshader->setUniform("knobrot",(knobs[i].value-.5f)*.748f);
			knobshader->setUniform("id",(float)i);
			knobshader->setUniform("hoverstate",(float)(knobs[i].hoverstate==-1?(hover==i?-1:0):knobs[i].hoverstate));
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
		context.extensions.glDisableVertexAttribArray(coord);

		float osalpha = oversamplingalpha;
		if(oversamplingalpha < 1) {
			if(oversamplingalpha > 0)
				osalpha = oversamplingalpha<0.5?4*oversamplingalpha*oversamplingalpha*oversamplingalpha:1-(float)pow(-2*oversamplingalpha+2,3)/2;
			visshader->use();
			coord = context.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
			context.extensions.glActiveTexture(GL_TEXTURE0);
			basetex.bind();
			visshader->setUniform("basetex",0);
			visshader->setUniform("banner",banneroffset);
			visshader->setUniform("alpha",1-osalpha);
			visshader->setUniform("dpi",(float)fmax(dpi,1));
			visshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
			context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2712, visline[0], GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLE_STRIP,0,904);
			if(isStereo) {
				context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2712, visline[1], GL_DYNAMIC_DRAW);
				glDrawArrays(GL_TRIANGLE_STRIP,0,904);
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
			oversamplingshader->setUniform("banner",banneroffset);
			oversamplingshader->setUniform("alpha",osalpha);
			oversamplingshader->setUniform("selection",.458677686f+oversamplinglerped*.2314049587f);
			oversamplingshader->setUniform("texscale",242.f/basetex.getWidth(),462.f/basetex.getHeight());
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			context.extensions.glDisableVertexAttribArray(coord);
		}

		if(creditsalpha > 0) {
			creditsshader->use();
			context.extensions.glActiveTexture(GL_TEXTURE0);
			creditstex.bind();
			creditsshader->setUniform("creditstex",0);
			creditsshader->setUniform("banner",banneroffset);
			creditsshader->setUniform("texscale",242.f/creditstex.getWidth(),59.f/creditstex.getHeight());
			creditsshader->setUniform("alpha",creditsalpha<0.5?4*creditsalpha*creditsalpha*creditsalpha:1-(float)pow(-2*creditsalpha+2,3)/2);
			creditsshader->setUniform("shineprog",websiteht);
			coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			context.extensions.glDisableVertexAttribArray(coord);
		}

		needtoupdate--;
		framebuffer.releaseAsRenderingTarget();
	}

	ppshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	ppshader->setUniform("pptex",0);
	ppshader->setUniform("banner",banneroffset);
	ppshader->setUniform("chroma",rms*.006f);
	ppshader->setUniform("shake",rms*.004f*(random.nextFloat()-.5f));
	coord = context.extensions.glGetAttribLocation(ppshader->getProgramID(),"aPos");
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
void PFAudioProcessorEditor::openGLContextClosing() {
	baseshader->release();
	knobshader->release();
	visshader->release();
	oversamplingshader->release();
	creditsshader->release();
	ppshader->release();

	basetex.release();
	knobtex.release();
	creditstex.release();

	framebuffer.release();

#ifdef BANNER
	bannershader->release();
	bannertex.release();
#endif

	audioProcessor.logger.font.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void PFAudioProcessorEditor::calcvis() {
	isStereo = knobs[4].value > 0 && knobs[3].value < 1 && knobs[5].value > 0;
	pluginpreset pp;
	for(int i = 0; i < knobcount; i++)
		pp.values[i] = knobs[i].inflate(knobs[i].value);
	double prevy = 48;
	double currenty = 48;
	double nexty = 48;
	double nextnexty = 48;
	for(int c = 0; c < (isStereo ? 2 : 1); c++) {
		for(int i = 0; i < 452; i++) {
			if((i%2)==0) {
				nexty = nextnexty;
			} else {
				nextnexty = 48+audioProcessor.plasticfuneral(sin((i*.5+.5)/35.8098621957f)*.8f,c,2,pp,audioProcessor.normalizegain(pp.values[1],pp.values[3]))*38;
				nexty = (currenty+nextnexty)*.5;
			}
			double angle1 = std::atan2(currenty-prevy, .5);
			double angle2 = std::atan2(currenty-nexty,-.5);
			while((angle1-angle2)<(-1.5707963268))angle1+=3.1415926535*2;
			while((angle1-angle2)>( 1.5707963268))angle1-=3.1415926535*2;
			double angle = (angle1+angle2)*.5;
			visline[c][i*6  ] = (i*.5+8+cos(angle)*1.3f)/121.f-1;
			visline[c][i*6+3] = (i*.5+8-cos(angle)*1.3f)/121.f-1;
			visline[c][i*6+1] = 1-(currenty+sin(angle)*1.3f)/231.f;
			visline[c][i*6+4] = 1-(currenty-sin(angle)*1.3f)/231.f;
			visline[c][i*6+2] = 0.f;
			visline[c][i*6+5] = 1.f;
			prevy = currenty;
			currenty = nexty;
		}
	}
}
void PFAudioProcessorEditor::paint (Graphics& g) { }

void PFAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) {
		if(knobs[i].hoverstate < -1) {
			needtoupdate = 2;
			knobs[i].hoverstate++;
		}
	}
	if(held > 0) held--;

	if(oversamplingalpha != (hover<=-4?1:0)) {
		oversamplingalpha = fmax(fmin(oversamplingalpha+(hover<=-4?.07f:-.07f),1),0);
		needtoupdate = 2;
	}

	float os = oversampling?1:0;
	if(oversamplinglerped != os?1:0) {
		needtoupdate = 2;
		if(fabs(oversamplinglerped-os) <= .001f) oversamplinglerped = os;
		oversamplinglerped = oversamplinglerped*.75f+os*.25f;
	}

	if(creditsalpha != ((hover<=-2&&hover>=-3)?1:0)) {
		creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.07f:-.07f),1),0);
		needtoupdate = 2;
	}
	if(creditsalpha <= 0) websiteht = -1;
	if(websiteht >= -.227273) {
		websiteht -= .05;
		needtoupdate = 2;
	}

	if(audioProcessor.rmscount.get() > 0) {
		rms = sqrt(audioProcessor.rmsadd.get()/audioProcessor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
		audioProcessor.rmsadd = 0;
		audioProcessor.rmscount = 0;
	} else rms *= .9f;

#ifdef BANNER
	bannerx = fmod(bannerx+.0005f,1.f);
#endif

	context.triggerRepaint();
}

void PFAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
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
void PFAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
	if(prevhover != hover && held == 0) {
		if(hover > -1) knobs[hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
	}
}
void PFAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void PFAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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
void PFAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
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
void PFAudioProcessorEditor::mouseUp(const MouseEvent& event) {
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
			if(hover > -1) knobs[hover].hoverstate = -4;
		}
	}
	held = 1;
}
void PFAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audioProcessor.undoManager.beginNewTransaction();
}
void PFAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int PFAudioProcessorEditor::recalchover(float x, float y) {
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
