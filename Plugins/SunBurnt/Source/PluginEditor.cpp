#include "PluginProcessor.h"
#include "PluginEditor.h"

SunBurntAudioProcessorEditor::SunBurntAudioProcessorEditor(SunBurntAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : AudioProcessorEditor(&p), audioProcessor(p)
{
	jpmode = params.jpmode;
	curveselection = params.curveselection;

	for(int i = 0; i < paramcount; i++) {
		knobs[i].id = params.pots[i].id;
		knobs[i].name = params.pots[i].name;
		knobs[i].value = params.pots[i].normalize(state.values[i]);
		knobs[i].minimumvalue = params.pots[i].minimumvalue;
		knobs[i].maximumvalue = params.pots[i].maximumvalue;
		knobs[i].defaultvalue = params.pots[i].normalize(params.pots[i].defaultvalue);
		knobcount++;
		audioProcessor.apvts.addParameterListener(knobs[i].id,this);
	}
	for(int i = 0; i < 5; i++) {
		curves[i] = state.curves[i];
		if(i > 0) audioProcessor.apvts.addParameterListener(audioProcessor.curveid[i],this);
	}

	if((SystemStats::getOperatingSystemType() & SystemStats::OperatingSystemType::Windows) != 0)
		dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->dpi/((double)96);
	else
		dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;
	int i = 0;
	double s = 1.f;
	double dif = 100;
	Rectangle<int> r = Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
	int w = r.getWidth();
	int h = r.getHeight();
	double pw = 368;
	double ph = 334;
#ifdef BANNER
	ph += 21/1.5f;
#endif
	while((s/dpi)*pw < w && (s/dpi)*ph < h) {
		uiscales.push_back(s/dpi);
		if(abs(params.uiscale-s/dpi) <= dif) {
			dif = abs(params.uiscale-s/dpi);
			uiscaleindex = i;
		}
		if(i < 4) s += .25f;
		else if(i < 8) s += .5f;
		else if(i < 16) s++;
		i++;
	}
	setSize(pw*uiscales[uiscaleindex],ph*uiscales[uiscaleindex]);
	looknfeel.scale = uiscales[uiscaleindex];
#ifdef BANNER
	banneroffset = 21.f/1.5f*uiscales[uiscaleindex]/getHeight();
#endif
	setResizable(false, false);

	calcvis();
	setOpaque(true);
	context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
	context.setRenderer(this);
	context.attachTo(*this);

	startTimerHz(30);
}
SunBurntAudioProcessorEditor::~SunBurntAudioProcessorEditor() {
	for(int i = 0; i < knobcount; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	for(int i = 1; i < 5; i++) audioProcessor.apvts.addParameterListener(audioProcessor.curveid[i],this);
	stopTimer();
	context.detach();
}

void SunBurntAudioProcessorEditor::newOpenGLContextCreated() {
	audioProcessor.logger.init(&context,getWidth(),getHeight());

	compileshader(baseshader,
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 texscale;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner))*2-1,0,1);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D basetex;
uniform sampler2D disptex;
uniform vec2 texscale;
uniform float selection;
uniform float shineprog;
uniform float dpi;
uniform vec4 randomid;
uniform vec4 randomx;
uniform vec4 randomy;
uniform vec4 randomblend;
out vec4 fragColor;
void main(){
	int section = 0;
	if(uv.y <= .20958) section = 2;
	else if(uv.y <= .83234) section = 1;

	vec3 disp = vec3(texture(disptex,uv).rg,texture(disptex,round(uv*vec2(736,668))/vec2(736,668)).b);
	vec2 dispuv = uv;
	if(disp.b < .9) {
		vec3 r = vec3(-5);
		disp.b = round(disp.b*64);
			 if(disp.b == randomid.x) r = vec3(randomx.x,randomy.x,randomblend.x);
		else if(disp.b == randomid.y) r = vec3(randomx.y,randomy.y,randomblend.y);
		else if(disp.b == randomid.z) r = vec3(randomx.z,randomy.z,randomblend.z);
		else if(disp.b == randomid.w) r = vec3(randomx.w,randomy.w,randomblend.w);
		if(r.x > -4) dispuv += ((disp.gr*2-1)*r.z+r.xy*(1-abs(r.z)))*vec2(.0027174,.002994);
	}

	vec3 col = vec3(0);
	if(section == 1 && uv.x <= .80978) {
		col.rg = texture(basetex,uv).rg;
		col.b = texture(basetex,dispuv).b;
	} else {
		col.rg = texture(basetex,dispuv).rg;
		if(shineprog > -.65761 && section == 0 && uv.x >= .58967) {
			float shine = texture(basetex,uv+vec2(shineprog,0)).g;
			col.b = shine*col.r;
			col.r *= (1-shine);
		} else if(section == 2) col.b = texture(basetex,uv).b;
	}

	if(section == 1 && uv.x >= .80978 && uv.x <= .9837) {
		vec2 selectuv = uv;
		if(selection == 0) selectuv.x -= .5;
		else {
			if(mod(selection,2.) == 0) selectuv.y += .2994;
			if(selection > 2.5) selectuv.x -= .054348;
		}
		col.g = 1-abs(col.r-texture(basetex,selectuv).g);
		col.r = 1;
	} else if(section < 2) col.g = 0;

	fragColor = vec4(max(min((col-.5)*dpi+.5,1),0),1);
})");

	compileshader(visshader,
//VIS VERT
R"(#version 150 core
in vec3 aPos;
uniform float banner;
out float linepos;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner))*2-1,0,1);
	linepos = aPos.z;
})",
//VIS FRAG
R"(#version 150 core
in float linepos;
uniform float dpi;
out vec4 fragColor;
void main(){
	fragColor = vec4(1,0,0,min((1-abs(linepos*2-1))*dpi*2,1));
})");

	compileshader(circleshader,
//CIRCLE VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 knobscale;
uniform vec2 knobpos;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*knobscale+knobpos,0,1);
	uv = aPos*2-1;
})",
//CIRCLE FRAG
R"(#version 150 core
in vec2 uv;
uniform float size;
uniform float fill;
uniform vec3 colin;
uniform vec3 colout;
out vec4 fragColor;
void main(){
	float x = sqrt(uv.x*uv.x+uv.y*uv.y);
	float inblend = min(max((x-fill)*size*.5f,0),1);
	fragColor = vec4(colin*(1-inblend)+colout*inblend,min(max((1-x)*size*.5f,0),1));
})");

	compileshader(ppshader,
//PP VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec4 overlaypos;
uniform float overlayorientation;
out vec2 uv;
out vec2 overlayuv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = aPos;
	overlayuv = uv*overlaypos.xy+overlaypos.zw;
	if(mod(overlayorientation,2.) > .5) overlayuv.x = 1-overlayuv.x;
	if(overlayorientation > 1.5) overlayuv.y = 1-overlayuv.y;
})",
//PP FRAG
R"(#version 150 core
in vec2 uv;
in vec2 overlayuv;
uniform sampler2D buffertex;
uniform sampler2D overlaytex;
uniform vec2 unalignedoffset;
uniform vec2 unalignedspeed;
uniform vec2 unalignedamount;
uniform float chromatic;
out vec4 fragColor;
void main(){
	vec3 colr = texture(buffertex,uv-vec2(chromatic,0)).rgb;
	vec3 colg = texture(buffertex,uv).rgb;
	vec3 colb = texture(buffertex,uv+vec2(chromatic,0)).rgb;
	vec2 unaligneduv = sin(uv*unalignedspeed*4+unalignedoffset*3.14159*2)*2*unalignedamount/vec2(368,334);
	vec3 unaligned = vec3(
		texture(buffertex,uv+unaligneduv-vec2(chromatic,0)).r,
		texture(buffertex,uv+unaligneduv).r,
		texture(buffertex,uv+unaligneduv+vec2(chromatic,0)).r);
	vec3 coldark = vec3(
		min(colr.r,1-colr.g),
		min(colg.r,1-colg.g),
		min(colb.r,1-colb.g));
	vec3 colgray = vec3(
		min(colr.b,1-min(max(colr.r+colr.g,unaligned.r),1)),
		min(colg.b,1-min(max(colg.r+colg.g,unaligned.g),1)),
		min(colb.b,1-min(max(colb.r+colb.g,unaligned.b),1)));
	vec3 colwite = 1-coldark-colgray;
	fragColor = vec4(1-(1-(vec3(.15686,.1451,.13725)*coldark+vec3(.76471,.77647,.77647)*colgray+vec3(.98824,.98824,.97647)*colwite))*(1-vec3(
		texture(overlaytex,overlayuv-vec2(chromatic,0)).r,
		texture(overlaytex,overlayuv).g,
		texture(overlaytex,overlayuv+vec2(chromatic,0)).b)*.18),1);
})");

	baseentex.loadImage(ImageCache::getFromMemory(BinaryData::base_en_png, BinaryData::base_en_pngSize));
	baseentex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	basejptex.loadImage(ImageCache::getFromMemory(BinaryData::base_jp_png, BinaryData::base_jp_pngSize));
	basejptex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	disptex.loadImage(ImageCache::getFromMemory(BinaryData::disp_png, BinaryData::disp_pngSize));
	disptex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	overlaytex.loadImage(ImageCache::getFromMemory(BinaryData::overlay_png, BinaryData::overlay_pngSize));
	overlaytex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

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
	vec2 col = max(min((texture(tex,vec2(mod(uv.x+pos,1.)*texscale.x,uv.y)).rg-.5)*dpi+.5,1),0);
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
}
void SunBurntAudioProcessorEditor::compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader) {
	shader.reset(new OpenGLShaderProgram(context));
	if(!shader->addVertexShader(vertexshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	if(!shader->addFragmentShader(fragmentshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	shader->link();
}
void SunBurntAudioProcessorEditor::renderOpenGL() {
	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);
	float scaleddpi = uiscales[uiscaleindex]*dpi;

	if(prevuiscaleindex != uiscaleindex) {
		framebuffer.release();
		framebuffer.initialise(context, 368*scaleddpi, 334*scaleddpi);
		glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		prevuiscaleindex = uiscaleindex;
	}

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	framebuffer.makeCurrentRenderingTarget();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//base
	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	if(jpmode) basejptex.bind();
	else baseentex.bind();
	baseshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	disptex.bind();
	baseshader->setUniform("disptex",1);
	baseshader->setUniform("banner",banneroffset);
	baseshader->setUniform("texscale",736.f/baseentex.getWidth(),668.f/baseentex.getHeight());
	baseshader->setUniform("selection",(float)curveselection);
	baseshader->setUniform("shineprog",websiteht);
	baseshader->setUniform("randomid",(float)randomid[0],(float)randomid[1],(float)randomid[2],(float)randomid[3]);
	baseshader->setUniform("randomx",randomdir[0],randomdir[1],randomdir[2],randomdir[3]);
	baseshader->setUniform("randomy",randomdir[4],randomdir[5],randomdir[6],randomdir[7]);
	baseshader->setUniform("randomblend",randomblend[0],randomblend[1],randomblend[2],randomblend[3]);
	baseshader->setUniform("dpi",(float)fmax(scaleddpi*.5f,1.f));
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	//vis line
	visshader->use();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	coord = context.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
	visshader->setUniform("banner",banneroffset);
	visshader->setUniform("dpi",scaleddpi);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3408, visline, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP,0,1136);
	context.extensions.glDisableVertexAttribArray(coord);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	//tension points
	circleshader->use();
	coord = context.extensions.glGetAttribLocation(circleshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	circleshader->setUniform("colin",1.f,0.f,0.f);
	circleshader->setUniform("colout",1.f,0.f,0.f);
	circleshader->setUniform("knobscale",17.364f*uiscales[uiscaleindex]/getWidth(),17.364f*uiscales[uiscaleindex]/getHeight());
	circleshader->setUniform("size",8.682f*scaleddpi);
	circleshader->setUniform("fill",.5f);
	int nextpoint = 0;
	for(int i = 0; i < (curves[curveselection].points.size()-1); ++i) {
		if(!curves[curveselection].points[i].enabled) continue;
		++nextpoint;
		while(!curves[curveselection].points[nextpoint].enabled) ++nextpoint;
		if(((curves[curveselection].points[nextpoint].x-curves[curveselection].points[i].x)*1.42f) <= .02 || fabs(curves[curveselection].points[nextpoint].y-curves[curveselection].points[i].y) <= .02) continue;
		double interp = curve::calctension(.5,curves[curveselection].points[i].tension);
		circleshader->setUniform("knobpos",
			(((curves[curveselection].points[i].x+curves[curveselection].points[nextpoint].x)*284.f+24.f-8.682f)*uiscales[uiscaleindex])/getWidth()-1,
			(((curves[curveselection].points[i].y*(1-interp)+curves[curveselection].points[nextpoint].y*interp)*400.f+148.f-8.682f)*uiscales[uiscaleindex])/getHeight()-1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	//control points
	circleshader->setUniform("colin",1.f,1.f,0.f);
	circleshader->setUniform("knobscale",25.364f*uiscales[uiscaleindex]/getWidth(),25.364f*uiscales[uiscaleindex]/getHeight());
	circleshader->setUniform("size",12.682f*scaleddpi);
	circleshader->setUniform("fill",1-9.f/12.682f);
	for(int i = 0; i < curves[curveselection].points.size(); ++i) {
		if(!curves[curveselection].points[i].enabled) continue;
		circleshader->setUniform("knobpos",
			((curves[curveselection].points[i].x*568.f+24.f-12.682f)*uiscales[uiscaleindex])/getWidth()-1,
			((curves[curveselection].points[i].y*400.f+148.f-12.682f)*uiscales[uiscaleindex])/getHeight()-1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	//knob bg
	circleshader->setUniform("knobscale",83.26f*uiscales[uiscaleindex]/getWidth(),83.26f*uiscales[uiscaleindex]/getHeight());
	circleshader->setUniform("size",41.63f*scaleddpi);
	circleshader->setUniform("fill",1-9.f/41.63f);
	for(int i = 0; i < knobcount; ++i) {
		float x = i*121.2f+23.38f;
		float y = 36.42f;
		for(int ii = 0; ii < 4; ++ii) if(randomid[ii] == (i+46)) {
			x += randomdir[ii]*2;
			y += randomdir[ii+4]*2;
			break;
		}
		circleshader->setUniform("knobpos",(x*uiscales[uiscaleindex])/getWidth()-1,(y*uiscales[uiscaleindex])/getHeight()-1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	//knob dots
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	circleshader->setUniform("knobscale",25.f*uiscales[uiscaleindex]/getWidth(),25.f*uiscales[uiscaleindex]/getHeight());
	circleshader->setUniform("size",12.5f*scaleddpi);
	circleshader->setUniform("colin",1.f,0.f,0.f);
	circleshader->setUniform("colout",1.f,0.f,0.f);
	for(int i = 0; i < knobcount; i++) {
		float x = i*121.2f+52.51f+sin((knobs[i].value-.5f)*.8f*6.28318531f)*17.5f;
		float y = 65.55f+cos((knobs[i].value-.5f)*.8f*6.28318531f)*17.5f;
		for(int ii = 0; ii < 4; ++ii) if(randomid[ii] == (i+46)) {
			x += randomdir[ii]*2;
			y += randomdir[ii+4]*2;
			break;
		}
		circleshader->setUniform("knobpos",(x*uiscales[uiscaleindex])/getWidth()-1,(y*uiscales[uiscaleindex])/getHeight()-1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	//curve enable
	circleshader->setUniform("knobscale",33.f*uiscales[uiscaleindex]/getWidth(),33.f*uiscales[uiscaleindex]/getHeight());
	circleshader->setUniform("size",16.5f*scaleddpi);
	circleshader->setUniform("fill",1-5.f/16.5f);
	for(int i = 0; i < 4; i++) {
		float x = 643.5f;
		float y = 351.5f;
		if((i%2) > .5) y -= 200;
		if(i > 1.5) x += 40;
		for(int ii = 0; ii < 4; ++ii) if(randomid[ii] == (i+52)) {
			x += randomdir[ii]*2;
			y += randomdir[ii+4]*2;
			break;
		}
		circleshader->setUniform("knobpos",(x*uiscales[uiscaleindex])/getWidth()-1,(y*uiscales[uiscaleindex])/getHeight()-1);
		circleshader->setUniform("colin",1.f,curves[i+1].enabled?1.f:0.f,0.f);
		circleshader->setUniform("colout",1.f,curves[i+1].enabled?0.f:1.f,0.f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);
	framebuffer.releaseAsRenderingTarget();

	//post processing
	ppshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	ppshader->setUniform("buffertex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	overlaytex.bind();
	ppshader->setUniform("overlaytex",1);
	ppshader->setUniform("overlaypos",368.f/600.f,334.f/600.f,(1-368/600.f)*overlayposx,(1-334/600.f)*overlayposy);
	ppshader->setUniform("overlayorientation",(float)overlayorientation);
	ppshader->setUniform("unalignedoffset",unalignedx[0],unalignedy[0]);
	ppshader->setUniform("unalignedspeed",unalignedx[1],unalignedy[1]);
	ppshader->setUniform("unalignedamount",unalignedx[2],unalignedy[2]);
	ppshader->setUniform("chromatic",.3f/(getWidth()*dpi));
	ppshader->setUniform("banner",banneroffset);
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
	bannershader->setUniform("dpi",(float)fmax(scaleddpi,1.f));
#ifdef BETA
	bannershader->setUniform("texscale",494.f/bannertex.getWidth(),21.f/bannertex.getHeight());
	bannershader->setUniform("size",getWidth()/(494.f/1.5f*uiscales[uiscaleindex]),21.f/1.5f*uiscales[uiscaleindex]/getHeight());
	bannershader->setUniform("free",0.f);
#else
	bannershader->setUniform("texscale",426.f/bannertex.getWidth(),21.f/bannertex.getHeight());
	bannershader->setUniform("size",getWidth()/(426.f/1.5f*uiscales[uiscaleindex]),21.f/1.5f*uiscales[uiscaleindex]/getHeight());
	bannershader->setUniform("free",1.f);
#endif
	bannershader->setUniform("pos",bannerx);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);
#endif

	audioProcessor.logger.drawlog();
}
void SunBurntAudioProcessorEditor::openGLContextClosing() {
	baseshader->release();
	visshader->release();
	circleshader->release();
	ppshader->release();

	baseentex.release();
	basejptex.release();
	disptex.release();
	overlaytex.release();

	framebuffer.release();

#ifdef BANNER
	bannershader->release();
	bannertex.release();
#endif

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void SunBurntAudioProcessorEditor::calcvis() {
	curveiterator iterator;
	iterator.reset(curves[curveselection],283);
	double prevy = 74+iterator.next()*200;
	double currenty = prevy;
	double nexty = prevy;
	double nextnexty = prevy;
	for(int i = 0; i < 568; i++) {
		if((i%2)==0) {
			nexty = nextnexty;
		} else {
			nextnexty = 74+iterator.next()*200;
			nexty = (currenty+nextnexty)*.5;
		}
		double angle1 = std::atan2(currenty-prevy, .5);
		double angle2 = std::atan2(currenty-nexty,-.5);
		while((angle1-angle2)<(-1.5707963268))angle1+=3.1415926535*2;
		while((angle1-angle2)>( 1.5707963268))angle1-=3.1415926535*2;
		double angle = (angle1+angle2)*.5;
		visline[i*6  ] = (i*.5+12+cos(angle)*2.3f)/368.f;
		visline[i*6+3] = (i*.5+12-cos(angle)*2.3f)/368.f;
		visline[i*6+1] = (currenty+sin(angle)*2.3f)/334.f;
		visline[i*6+4] = (currenty-sin(angle)*2.3f)/334.f;
		visline[i*6+2] = 0.f;
		visline[i*6+5] = 1.f;
		prevy = currenty;
		currenty = nexty;
	}
}
void SunBurntAudioProcessorEditor::paint (Graphics& g) { }

void SunBurntAudioProcessorEditor::timerCallback() {
	if(websiteht >= -.65761f) websiteht -= .03f;

	time += .1f;
	if(time >= 1) {
		time = fmod(time,1);
		for(int ii = 0; ii < 4; ++ii) {
			if(random.nextFloat() > .3) {
				if(random.nextFloat() > .1) {
					if(random.nextFloat() > .5) randomid[ii] = floor(random.nextFloat()*64);
					randomdir[ii] = random.nextFloat()*2-1;
					randomdir[ii+4] = random.nextFloat()*2-1;
					randomblend[ii] = pow(random.nextFloat(),.5)*2-1;
				} else randomid[ii] = -1;
			}
		}
		if(random.nextFloat() > .2) {
			for(int i = 0; i < 3; ++i) {
				unalignedx[i] = random.nextFloat();
				unalignedy[i] = random.nextFloat();
			}
		}
		overlayorientation = floor(random.nextFloat()*4);
		overlayposx = random.nextFloat();
		overlayposy = random.nextFloat();
	}

#ifdef BANNER
	bannerx = fmod(bannerx+.0005f,1.f);
#endif

	context.triggerRepaint();
}

void SunBurntAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		return;
	}
	for(int i = 1; i < 5; i++) if(parameterID == audioProcessor.curveid[i]) {
		curves[i].enabled = newValue>.5;
		return;
	}
}
void SunBurntAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == -15 && prevhover != -15 && websiteht <= -.65761f) websiteht = 0;
}
void SunBurntAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void SunBurntAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(event.mods.isRightButtonDown() && !event.mods.isLeftButtonDown()) {
		int i = 0;
		std::unique_ptr<PopupMenu> rightclickmenu(new PopupMenu());
		std::unique_ptr<PopupMenu> scalemenu(new PopupMenu());
		std::unique_ptr<PopupMenu> langmenu(new PopupMenu());
		while(i++ < uiscales.size())
			scalemenu->addItem(i,(String)round(uiscales[i-1]*100)+"%",true,(i-1)==uiscaleindex);
		langmenu->addItem(i++,"English",true,!jpmode);
		langmenu->addItem(i++,String::fromUTF8("日本語"),true,jpmode);
		rightclickmenu->setLookAndFeel(&looknfeel);
		rightclickmenu->addSubMenu(jpmode?(String::fromUTF8("'スケール")):("'Scale"),*scalemenu);
		rightclickmenu->addSubMenu(jpmode?(String::fromUTF8("'言語")):("'Language"),*langmenu);
		rightclickmenu->addSeparator();
		rightclickmenu->addItem(++i,"'Trust the process!",false);
		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result <= uiscales.size()) {
				uiscaleindex = result-1;
#ifdef BANNER
				setSize(368*uiscales[uiscaleindex],(334+21/1.5f)*uiscales[uiscaleindex]);
				banneroffset = 21.f/1.5f*uiscales[uiscaleindex]/getHeight();
#else
				setSize(368*uiscales[uiscaleindex],334*uiscales[uiscaleindex]);
#endif
				audioProcessor.logger.width = getWidth();
				audioProcessor.logger.height = getHeight();
				looknfeel.scale = uiscales[uiscaleindex];
				audioProcessor.setUIScale(uiscales[uiscaleindex]);
			} else {
				result -= uiscales.size()+1;
				if(result <= 1) {
					jpmode = result==1;
					audioProcessor.setLang(result==1);
				}
			}
		});
		return;
	}

	initialdrag = hover;
	if(hover > -1) {
		valueoffset[0] = 0;
		audioProcessor.undoManager.beginNewTransaction();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
		if(hover >= knobcount) {
			int i = round(initialdrag-knobcount);
			if((i%2) == 0) {
				i /= 2;
				initialvalue[0] = curves[curveselection].points[i].x;
				initialvalue[1] = curves[curveselection].points[i].y;
				initialdotvalue[0] = initialvalue[0];
				initialdotvalue[1] = initialvalue[1];
				valueoffset[1] = 0;
			} else {
				i = (i-1)/2;
				initialvalue[0] = curves[curveselection].points[i].tension;
				initialdotvalue[0] = initialvalue[0];
			}
		} else {
			initialvalue[0] = knobs[hover].value;
			audioProcessor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		}
	} else if(hover >= -10 && hover <= -2) {
		if((hover+9)%2 == 0 || hover == -10) {
			curveselection = ceil((hover+10)*.5f);
			audioProcessor.params.curveselection = curveselection;
			calcvis();
		} else {
			int id = (int)(hover*.5)+5;
			bool val = !curves[id].enabled;
			audioProcessor.apvts.getParameter(audioProcessor.curveid[id])->setValueNotifyingHost(val?1.f:0.f);
			audioProcessor.undoManager.setCurrentTransactionName((String)"Turned "+audioProcessor.curvename[id]+(val?" on":" off"));
			audioProcessor.undoManager.beginNewTransaction();
		}
	}
}
void SunBurntAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(initialdrag >= knobcount) {
		int i = initialdrag-knobcount;
		if((i%2) == 0) { // dragging a dot
			i /= 2;
			if(!finemode && event.mods.isAltDown()) { //start of fine mode
				finemode = true;
				initialvalue[0] += event.getDistanceFromDragStartX()*.0045f;
				initialvalue[1] -= event.getDistanceFromDragStartY()*.0045f*1.42f;
			} else if(finemode && !event.mods.isAltDown()) { //end of fine mode
				finemode = false;
				initialvalue[0] -= event.getDistanceFromDragStartX()*.0045f;
				initialvalue[1] += event.getDistanceFromDragStartY()*.0045f*1.42f;
			}

			float valuey = initialvalue[1]-event.getDistanceFromDragStartY()*(finemode?.0005f:.005f)*1.42f;
			float pointy = valuey-valueoffset[1];

			if(i > 0 && i < (curves[curveselection].points.size()-1)) {
				float valuex = initialvalue[0]+event.getDistanceFromDragStartX()*(finemode?.0005f:.005f);
				float pointx = valuex-valueoffset[0];

				if(event.mods.isCtrlDown()) { // one axis
					if(axislock == -1) {
						initialaxispoint[0] = pointx;
						initialaxispoint[1] = pointy;
						axisvaluediff[0] = valuex;
						axisvaluediff[1] = valuey;
						axislock = 0;
					} else if(axislock == 0 && (abs(valuex-axisvaluediff[0])+abs(valuey-axisvaluediff[1])) > .1) {
						if(fabs(valuex-axisvaluediff[0]) > fabs(valuey-axisvaluediff[1]))
							axislock = 1;
						else
							axislock = 2;
					} else if(axislock == 1) {
						valueoffset[1] += valuey-axisvaluediff[1];
						axisvaluediff[1] = valuey;
						pointy = initialaxispoint[1];
					} else if(axislock == 2) {
						valueoffset[0] += valuex-axisvaluediff[0];
						axisvaluediff[0] = valuex;
						pointx = initialaxispoint[0];
					}
				} else axislock = -1;

				if(event.mods.isShiftDown()) { // free mode
					if((i > 1 && pointx < curves[curveselection].points[i-1].x) || (i < (curves[curveselection].points.size()-2) && pointx > curves[curveselection].points[i+1].x)) {
						point pnt = curves[curveselection].points[i];
						int n = 1;
						for(n = 1; n < (curves[curveselection].points.size()-1); ++n) //finding new position
							if(pnt.x < curves[curveselection].points[n].x) break;
						if(n > i) { // move points back
							n--;
							for(int f = i; f <= n; f++)
								curves[curveselection].points[f] = curves[curveselection].points[f+1];
						} else { //move points forward
							for(int f = i; f >= n; f--)
								curves[curveselection].points[f] = curves[curveselection].points[f-1];
						}
						i = n;
						initialdrag = i*2+knobcount;
						hover = initialdrag;
					}
				}

				// clampage to nearby pointe
				float preclampx = pointx;
				float preclampy = pointy;
				bool clamppedx = false;
				bool clamppedy = false;
				float xleft = 0;
				float xright = 1;
				if(!event.mods.isShiftDown()) { // free mode
					if(i > 1) {
						xleft = curves[curveselection].points[i-1].x;
						if(pointx <= curves[curveselection].points[i-1].x) {
							pointx = curves[curveselection].points[i-1].x;
							clamppedx = true;
						}
					}
					if(i < (curves[curveselection].points.size()-2)) {
						xright = curves[curveselection].points[i+1].x;
						if(pointx >= curves[curveselection].points[i+1].x) {
							pointx = curves[curveselection].points[i+1].x;
							clamppedx = true;
						}
					}
				}
				if(pointx < 0) { pointx = 0; clamppedx = true; }
				if(pointx > 1) { pointx = 1; clamppedx = true; }
				if(pointy < 0) { pointy = 0; clamppedy = true; }
				if(pointy > 1) { pointy = 1; clamppedy = true; }
				if(clamppedx)
					amioutofbounds[0] += preclampx-pointx-fmin(fmax(amioutofbounds[0],-.1f),.1f);
				else amioutofbounds[0] = 0;
				if(clamppedy)
					amioutofbounds[1] += preclampy-pointy-fmin(fmax(amioutofbounds[1],-.1f),.1f);
				else amioutofbounds[1] = 0;
				curves[curveselection].points[i].enabled = (fabs(amioutofbounds[0])+fabs(amioutofbounds[1])) < .8;

				curves[curveselection].points[i].x = pointx;
				if(axislock != 2)
					valueoffset[0] = fmax(fmin(valueoffset[0],valuex-xleft+.1f),valuex-xright-.1f);
			}
			curves[curveselection].points[i].y = fmax(fmin(pointy,1),0);
			if(axislock != 1)
				valueoffset[1] = fmax(fmin(valueoffset[1],valuey+.1f),valuey-1.1f);

		} else { //dragging tension
			i = (i-1)/2;
			int dir = curves[curveselection].points[i].y > curves[curveselection].points[i+1].y ? -1 : 1;
			if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
				finemode = true;
				initialvalue[0] -= dir*event.getDistanceFromDragStartY()*.0045f;
			} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
				finemode = false;
				initialvalue[0] += dir*event.getDistanceFromDragStartY()*.0045f;
			}

			float value = initialvalue[0]-dir*event.getDistanceFromDragStartY()*(finemode?.0005f:.005f);
			curves[curveselection].points[i].tension = fmin(fmax(value-valueoffset[0],0),1);

			valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
		}
		calcvis();
	} else if(initialdrag > -1) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue[0] -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue[0] += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue[0]-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset[0]);

		valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
	} else if(initialdrag == -15) {
		int prevhover = hover;
		hover = recalchover(event.x,event.y)==-15?-15:-1;
		if(hover == -15 && prevhover != -15 && websiteht <= -.65761f) websiteht = 0;
	}
}
void SunBurntAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(hover > -1) {
		if(hover >= knobcount) {
			int i = hover-knobcount;
			if((i%2) == 0) {
				i /= 2;
				axislock = -1;
				if((fabs(initialdotvalue[0]-curves[curveselection].points[i].x)+fabs(initialdotvalue[1]-curves[curveselection].points[i].y)) < .00001) {
					curves[curveselection].points[i].x = initialdotvalue[0];
					curves[curveselection].points[i].y = initialdotvalue[1];
					return;
				}
				dragpos.x += (curves[curveselection].points[i].x-initialdotvalue[0])*uiscales[uiscaleindex]*284;
				dragpos.y += (initialdotvalue[1]-curves[curveselection].points[i].y)*uiscales[uiscaleindex]*200;
				if(!curves[curveselection].points[i].enabled) {
					curves[curveselection].points.erase(curves[curveselection].points.begin()+i);
					audioProcessor.deletepoint(i);
				} else audioProcessor.movepoint(i,curves[curveselection].points[i].x,curves[curveselection].points[i].y);
			} else {
				i = (i-1)/2;
				if(fabs(initialdotvalue[0]-curves[curveselection].points[i].tension) < .00001) {
					curves[curveselection].points[i].tension = initialdotvalue[0];
					return;
				}
				float interp = curve::calctension(.5,curves[curveselection].points[i].tension)-curve::calctension(.5,initialdotvalue[0]);
				dragpos.y += (curves[curveselection].points[i].y-curves[curveselection].points[i+1].y)*interp*uiscales[uiscaleindex]*200.f;
				audioProcessor.movetension(i,curves[curveselection].points[i].tension);
			}
		}
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
		if(hover >= knobcount) {
			if(((initialdrag-knobcount)%2) == 0) audioProcessor.undoManager.setCurrentTransactionName("Moved point");
			else audioProcessor.undoManager.setCurrentTransactionName("Moved tension");
		} else {
			audioProcessor.undoManager.setCurrentTransactionName(
				(String)((knobs[hover].value - initialvalue[0]) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
			audioProcessor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		}
		audioProcessor.undoManager.beginNewTransaction();
		axislock = -1;
	} else {
		int prevhover = hover;
		hover = recalchover(event.x,event.y);
		if(hover == -15) {
			if(prevhover == -15) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht <= -.65761f) websiteht = 0;
		}
	}
}
void SunBurntAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover == -14) {
		float x = fmin(fmax((((float)event.x)/uiscales[uiscaleindex]-12)/284.f,0),1);
		float y = fmin(fmax(1-(((float)event.y)/uiscales[uiscaleindex]-60)/200.f,0),1);
		int i = 1;
		for(i = 1; i < curves[curveselection].points.size(); ++i)
			if(x < curves[curveselection].points[i].x) break;
		curves[curveselection].points.insert(curves[curveselection].points.begin()+i,point(x,y,curves[curveselection].points[i-1].tension));
		audioProcessor.addpoint(i,x,y);
		calcvis();
		hover = recalchover(event.x,event.y);
		return;
	}
	if(hover <= -1) return;
	if(hover >= knobcount) {
		int i = initialdrag-knobcount;
		if((i%2) == 0) {
			i /= 2;
			if(i > 0 && i < (curves[curveselection].points.size()-1)) {
				curves[curveselection].points.erase(curves[curveselection].points.begin()+i);
				audioProcessor.deletepoint(i);
			}
		} else {
			i = (i-1)/2;
			if(fabs(curves[curveselection].points[i].tension-.5) < .00001f) return;
			curves[curveselection].points[i].tension = .5f;
			audioProcessor.movetension(i,.5f);
		}
		calcvis();
		hover = recalchover(event.x,event.y);
	} else {
		audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
		audioProcessor.undoManager.beginNewTransaction();
	}
}
void SunBurntAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1 || hover >= knobcount) return;
	audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int SunBurntAudioProcessorEditor::recalchover(float x, float y) {
	x /= uiscales[uiscaleindex];
	y /= uiscales[uiscaleindex];
	//logo
	if(y < 53) {
		if(y >= 7 && x >= 217 && x < 365) return -15;
		return -1;
	}
	if(y <= 267) {
		//dots
		if(x < 300) {
			x -= 12;
			y -= 60;
			for(int i = 0; i < curves[curveselection].points.size(); ++i) {
				float xx = x-curves[curveselection].points[i].x*284;
				float yy = y-(1-curves[curveselection].points[i].y)*200;
				//dot
				if((xx*xx+yy*yy)<=37.1) return i*2+knobcount;

				if(i < (curves[curveselection].points.size()-1)) {
					float interp = curve::calctension(.5,curves[curveselection].points[i].tension);
					xx = x-(curves[curveselection].points[i].x+curves[curveselection].points[i+1].x)*.5f*284.f;
					yy = y-(1-(curves[curveselection].points[i].y*(1-interp)+curves[curveselection].points[i+1].y*interp))*200.f;
					//tension
					if((xx*xx+yy*yy)<=16.7) return i*2+1+knobcount;
				}
			}
			if(x >= 0 && x <= 284 && y >= 0 && y <= 200) return -14;
		//curve select
		} else {
			if(y < 60 || y > 260) return -1;
			for(int i = 0; i < 3; i++) if(x < (320+20*i)) {
				//volume
				if(i == 0) return -10;
				float xx = 310+20*i-x;
				if(y < 160) {
					float yy = 150-y;
					//enable disable
					if((xx*xx+yy*yy)<=64) return i*4-12;
					//curve select
					return i*4-13;
				}
				float yy = 250-y;
				//enable disable
				if((xx*xx+yy*yy)<=64) return i*4-10;
				//curve select
				return i*4-11;
			}
		}
		return -1;
	}
	//knobs
	float xx = 0;
	float yy = 294.975f-y;
	yy *= yy;
	for(int i = 0; i < knobcount; i++) {
		xx = (i*60.6f+32.505f)-x;
		if((xx*xx+yy)<=420.25) return i;
	}
	return -1;
}
LookNFeel::LookNFeel() {
	setColour(PopupMenu::backgroundColourId,Colour::fromFloatRGBA(0.f,0.f,0.f,0.f));
	int prioritycjk = 0;
	int priorityeng = 0;
	auto ft = Font::findAllTypefaceNames();
	for(const auto &q : ft) {
		if(q == "GB18030 Bitmap" && prioritycjk < 1) { //mac
			CJKFont = "GB18030 Bitmap";
			prioritycjk = 1;
		} else if(q == "MS Gothic" && prioritycjk < 2) { //win
			CJKFont = "MS Gothic";
			prioritycjk = 2;
		} else if(q == "MS PGothic" && prioritycjk < 3) { //win
			CJKFont = "MS PGothic";
			prioritycjk = 3;
		} else if(q == "Lantinghei TC" && prioritycjk < 4) { //mac
			CJKFont = "Lantinghei TC";
			prioritycjk = 4;
		} else if(q == "Droid Sans Fallback" && prioritycjk < 5) { //linux
			CJKFont = "Droid Sans Fallback";
			prioritycjk = 5;
		} else if(q == "Noto Sans CJK JP" && prioritycjk < 6) { //linux
			CJKFont = "Noto Sans CJK JP";
			prioritycjk = 6;
		} else if(q == "Microsoft YaHei") { //win
			CJKFont = "Microsoft YaHei";
			prioritycjk = 1000;
			if(priorityeng >= 500) break;

		} else if(q == "Liberation Mono" && priorityeng < 1) { //linux
			ENGFont = "Liberation Mono";
			priorityeng = 1;
		} else if(q == "Lucida Console" && priorityeng < 2) { //win
			ENGFont = "Lucida Console";
			priorityeng = 2;
		} else if(q == "SF Mono" && priorityeng < 3) { //mac
			ENGFont = "SF Mono";
			priorityeng = 3;
		} else if(q == "Andale Mono" && priorityeng < 4) { //mac
			ENGFont = "Andale Mono";
			priorityeng = 4;
		} else if(q == "Menlo" && priorityeng < 5) { //mac
			ENGFont = "Menlo";
			priorityeng = 5;
		} else if(q == "DejaVu Sans Mono" && priorityeng < 6) { //linux
			ENGFont = "DejaVu Sans Mono";
			priorityeng = 6;
		} else if(q == "Noto Mono" && priorityeng < 7) { //linux
			ENGFont = "Noto Mono";
			priorityeng = 7;
		} else if(q == "Consolas") { //win
			ENGFont = "Consolas";
			priorityeng = 1000;
			if(prioritycjk >= 500) break;
		}
	}
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	return Font(ENGFont,"Regular",14.f*scale);
}
int LookNFeel::getMenuWindowFlags() {
	//return ComponentPeer::windowHasDropShadow;
	return 0;
}
void LookNFeel::drawPopupMenuBackground(Graphics &g, int width, int height) {
	g.setColour(bg);
	g.fillRoundedRectangle(0,0,width,height,2.f*scale);
}
void LookNFeel::drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) {
	if(isSeparator) {
		g.setColour(inactive);
		g.fillRoundedRectangle(area.withSizeKeepingCentre(area.getWidth()-10*scale,scale).toFloat(),round(scale)*.5f);
		return;
	}

	if(isHighlighted && isActive) {
		g.setColour(txt);
		g.fillAll();
		g.setColour(bg);
	}

	if(textColour != nullptr)
		g.setColour(*textColour);
	else if(!isActive)
		g.setColour(inactive);
	else if(!isHighlighted)
		g.setColour(txt);

	auto r = area;
	bool removeleft = text.startsWith("'");
	if(removeleft) r.removeFromLeft(5*scale);
	else r.removeFromLeft(area.getHeight());

	Font font = getPopupMenuFont();
	float maxFontHeight = ((float)r.getHeight())/1.45f;
	if(((int)text.toRawUTF8()[1]) < 0) {
		font = Font(CJKFont,"Regular",20.f*scale);
		maxFontHeight = r.getHeight();
	}
	if(!isActive && removeleft) font.setBold(true);
	if(font.getHeight() > maxFontHeight)
		font.setHeight(maxFontHeight);
	g.setFont(font);

	Rectangle<float> iconArea = area.toFloat().withX(area.getX()+(r.getX()-area.getX())*.5f-area.getHeight()*.5f).withWidth(area.getHeight());
	if(icon != nullptr)
		icon->drawWithin(g, iconArea.reduced(2), RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.f);
	else if(isTicked)
		g.fillEllipse(iconArea.reduced(iconArea.getHeight()*.3f));

	if(hasSubMenu) {
		float s = area.getHeight()*.35f;
		float x = area.getX()+area.getWidth()-area.getHeight()*.5f-s*.35f;
		float y = area.getCentreY();
		Path path;
		path.startNewSubPath
				   (x,y-s*.5f);
		path.lineTo(x+s*.7f,y);
		path.lineTo(x,y+s*.5f);
		path.lineTo(x,y-s*.5f);
		g.fillPath(path);
	}

	if(removeleft)
		g.drawFittedText(text.substring(1), r, Justification::centredLeft, 1);
	else
		g.drawFittedText(text, r, Justification::centredLeft, 1);

	if(shortcutKeyText.isNotEmpty()) {
		Font f2 = font;
		f2.setHeight(f2.getHeight()*.75f);
		f2.setHorizontalScale(.95f);
		g.setFont(f2);
		g.drawText(shortcutKeyText, r, Justification::centredRight, true);
	}
}