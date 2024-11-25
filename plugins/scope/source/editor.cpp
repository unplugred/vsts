#include "processor.h"
#include "editor.h"
#include "settings.h"

ScopeAudioProcessorEditor::ScopeAudioProcessorEditor(ScopeAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 0, 0, 1, 1, true, false) {

	knobcount = paramcount;
	for(int i = 0; i < paramcount; ++i) {
		knobs[i].id = params.pots[i].id;
		knobs[i].name = params.pots[i].name;
		knobs[i].value = params.pots[i].normalize(state.values[i]);
		knobs[i].isbool = params.pots[i].ttype == potentiometer::ptype::booltype;
		knobs[i].minimumvalue = params.pots[i].minimumvalue;
		knobs[i].maximumvalue = params.pots[i].maximumvalue;
		knobs[i].defaultvalue = params.pots[i].normalize(params.pots[i].defaultvalue);
		add_listener(knobs[i].id);
	}
	knobs[1].visible = knobs[0].value < .5f;
	knobs[2].visible = knobs[0].value > .5f;
	bgcol = Colour::fromHSV(knobs[7].value,knobs[8].value,knobs[9].value,1);
	ampdamp	.reset(knobs[3].value		,.08,-1,30);
	griddamp.reset(knobs[6].value		,.08,-1,30);
	rdamp	.reset(bgcol.getFloatRed()	,.15,-1,30);
	gdamp	.reset(bgcol.getFloatGreen(),.15,-1,30);
	bdamp	.reset(bgcol.getFloatBlue()	,.15,-1,30);

	calcvis();

	init();

	set_size(audio_processor.height.get()/.75f,audio_processor.height.get());
#ifdef BANNER
	banner_offset = 21.f/(height+21.f);
	getConstrainer()->setFixedAspectRatio(1.f/.75f-21.f/width);
#else
	getConstrainer()->setFixedAspectRatio(1.f/.75f);
#endif
	setResizable(true,false);
	setResizeLimits(200,150,2000,1500);

	dontscale = false;
}
ScopeAudioProcessorEditor::~ScopeAudioProcessorEditor() {
	if(audio_processor.settingswindow != nullptr) {
		delete audio_processor.settingswindow;
		audio_processor.settingswindow = nullptr;
	}
	close();
}

void ScopeAudioProcessorEditor::resized() {
	if(dontscale) return;
	width = getWidth();
#ifdef BANNER
	height = getHeight()-21;
	banner_offset = 21.f/getHeight();
	float w = height/.75f;
	getConstrainer()->setFixedAspectRatio(1.f/.75f-21.f/w);
#else
	height = getHeight();
#endif
	audio_processor.height = height;
	debug_font.width = getWidth();
	debug_font.height = getHeight();
	debug_font.banner_offset = banner_offset;
	set_frame_buffer_size(&framebuffer,width,height);
}

void ScopeAudioProcessorEditor::newOpenGLContextCreated() {
	clearshader = add_shader(
//CLEAR VERT
R"(#version 150 core
in vec2 coords;
void main() {
	gl_Position = vec4(coords*2-1,0,1);
})",
//CLEAR FRAG
R"(#version 150 core
out vec4 fragColor;
void main() {
	fragColor = vec4(0,0,0,.7);
})");
	lineshader = add_shader(
//LINE VERT
R"(#version 150 core
in vec3 coords;
uniform float banner;
out float luminocity;
out float linex;
void main() {
	gl_Position = vec4(vec2(coords.x,coords.y*(1-banner)-banner),0,1);
	luminocity = abs(coords.z);
	linex = coords.z>0?1:-1;
})",
//LINE FRAG
R"(#version 150 core
in float luminocity;
in float linex;
uniform float scaling;
out vec4 fragColor;
void main() {
	fragColor = vec4(vec3(min(1,luminocity)*.2f*min(1,(1-abs(linex))*scaling*luminocity)),1);
})");
	downscaleshader = add_shader(
//DOWNSCALE VERT
R"(#version 150 core
in vec2 coords;
uniform float banner;
out vec2 uv;
void main() {
	gl_Position = vec4(vec2(coords.x,coords.y*(1-banner))*2-1,0,1);
	uv = coords;
})",
//DOWNSCALE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D rendertex;
uniform float scale;
out vec4 fragColor;
void main() {
	fragColor = vec4(vec3(texture(rendertex,uv*scale).r),0.5);
})");
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 coords;
uniform float banner;
out vec2 uv;
void main() {
	gl_Position = vec4(vec2(coords.x,coords.y*(1-banner)+banner)*2-1,0,1);
	uv = coords;
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D basetex;
uniform sampler2D noisetex;
uniform sampler2D rendertex;
uniform sampler2D downscaletex;
uniform vec2 noiseoffset;
uniform vec2 res;
uniform vec3 bgcol;
uniform float grid;
uniform float gweight[4] = float[](0.1964825501511404, 0.2969069646728344, 0.09447039785044732,0.010381362401148057);
out vec4 fragColor;
vec3 hueshift(vec3 color, float shift) {
	vec3 P = vec3(.55735)*dot(vec3(.55735),color);
	vec3 U = color-P;
	vec3 V = cross(vec3(.55735),U);
	color = U*cos(shift*6.2832)+V*sin(shift*6.2832)+P;
	return color;
}
void main() {
	float render = texture(rendertex,uv).r*gweight[0];
	vec2 uvadd = vec2(.25,.333)*)"+(String)(LINEWIDTH*.7f)+R"(;
	for(int i = 1; i < 4; i++) {
		render += texture(rendertex,uv+i*uvadd           ).r*gweight[i];
		render += texture(rendertex,uv+i*uvadd*vec2(-1,1)).r*gweight[i];
		render += texture(rendertex,uv+i*uvadd*vec2(1,-1)).r*gweight[i];
		render += texture(rendertex,uv+i*uvadd*       -1 ).r*gweight[i];
	}
	render = sin(render*1.5707963268);
	float value = max(bgcol.r,max(bgcol.g,bgcol.b));
	float bloom = texture(downscaletex,uv).r;
	bloom = bloom*bloom*(value+1)*.2;
	vec3 ui = texture(basetex,uv).rgb;
	//ui.b *= value;
	//ui.g = 1-((1-ui.g)*grid);
	//vec3 noise = (1-texture(noisetex,uv*res+noiseoffset).rgb)*.09*(value*.75+.25);
	//vec3 color = hueshift(bgcol,((render+bloom)*ui.r+ui.b+ui.r-1)*-.05);
	//fragColor = vec4(((render+color)*ui.g+bloom-noise)*ui.r+ui.b,1);
	fragColor = vec4(((render+hueshift(bgcol,((render+bloom)*ui.r+ui.b+ui.r-1)*-.05))*(1-((1-ui.g)*grid))+bloom-(1-texture(noisetex,uv*res+noiseoffset).rgb)*.09*(value*.75+.25))*ui.r+ui.b*value,1);
})");

	add_texture(&basetex,BinaryData::base_png,BinaryData::base_pngSize,GL_LINEAR,GL_LINEAR,GL_REPEAT,GL_REPEAT);
	add_texture(&noisetex,BinaryData::noise_png,BinaryData::noise_pngSize,GL_NEAREST,GL_NEAREST,GL_REPEAT,GL_REPEAT);

	add_frame_buffer(&framebuffer,width,height);
	add_frame_buffer(&downscalebuffer,20,15,false,false);

	draw_init();
}
void ScopeAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	context.extensions.glBindBuffer(GL_ARRAY_BUFFER,array_buffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,square,GL_DYNAMIC_DRAW);
	framebuffer.makeCurrentRenderingTarget();

	// BG CLEAR
	clearshader->use();
	auto coord = context.extensions.glGetAttribLocation(clearshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	// LINE
	glBlendFunc(GL_ONE, GL_ONE);
	lineshader->use();
	lineshader->setUniform("banner",banner_offset);
	lineshader->setUniform("scaling",(float)(width*dpi*LINEWIDTH*.15f));
	coord = context.extensions.glGetAttribLocation(lineshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < (knobs[0].value<.5?audio_processor.channelnum:1); ++i) {
		context.extensions.glBufferData(GL_ARRAY_BUFFER,sizeof(float)*linew*6,&line[i*4800],GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP,0,linew*2);
	}
	context.extensions.glDisableVertexAttribArray(coord);
	context.extensions.glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,square,GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	framebuffer.releaseAsRenderingTarget();
	downscalebuffer.makeCurrentRenderingTarget();

	// BLOOM
	downscaleshader->use();
	coord = context.extensions.glGetAttribLocation(downscaleshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,framebuffer.getTextureID());
	downscaleshader->setUniform("rendertex",0);
	downscaleshader->setUniform("banner",banner_offset);
	downscaleshader->setUniform("scale",width/20.f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	downscalebuffer.releaseAsRenderingTarget();

	// BASE
	baseshader->use();
	coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,framebuffer.getTextureID());
	baseshader->setUniform("rendertex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	basetex.bind();
	baseshader->setUniform("basetex",1);
	context.extensions.glActiveTexture(GL_TEXTURE2);
	noisetex.bind();
	baseshader->setUniform("noisetex",2);
	context.extensions.glActiveTexture(GL_TEXTURE2+1);
	glBindTexture(GL_TEXTURE_2D,downscalebuffer.getTextureID());
	baseshader->setUniform("downscaletex",3);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("res",(width*dpi)/512.f,(height*dpi)/512.f); //TODO *dpi????
	baseshader->setUniform("noiseoffset",random.nextFloat(),random.nextFloat());
	baseshader->setUniform("grid",(float)griddamp.nextvalue(knobs[6].value));
	baseshader->setUniform("bgcol",rdamp.nextvalue(bgcol.getFloatRed()),gdamp.nextvalue(bgcol.getFloatGreen()),bdamp.nextvalue(bgcol.getFloatBlue()));
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void ScopeAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void ScopeAudioProcessorEditor::calcvis() {
	if(!audio_processor.frameready.get()) return;

	if(channelnum != (knobs[0].value<.5?audio_processor.channelnum:1)) {
		channelnum = knobs[0].value<.5?audio_processor.channelnum:1;
		line.resize(channelnum*4800);
	}

	// LINE CALCULOUS
	float pixel = 2.f/(linew-1);
	float time = knobs[0].value<.5?(pow(knobs[1].value,5)*240+.05f):(pow(knobs[2].value,4)*16+.05f);
	float amp = Decibels::decibelsToGain(ampdamp.nextvalue(knobs[3].value)*34);
	linew = (time<1?time:(fmod(time,1)+1))*400;
	float luminocity = 1;

	for(int c = 0; c < channelnum; ++c) {
		int readpos = 800-linew;
		float nextx = -1;
		float nexty = 0;
		if(knobs[0].value < .5) { // SCOPE
			nextx = -1;
			nexty = amp*audio_processor.line[readpos+c*800];
		} else { // PANORAMA
			nextx = amp*audio_processor.line[readpos]*.75f;
			nexty = amp*audio_processor.line[readpos+(audio_processor.channelnum>1?800:0)]*-1;
		}
		float x = nextx;
		float y = nexty;
		float prevx = nextx;
		float prevy = nexty;
		float angle1 = 0;
		float angle2 = 0;
		for(int i = 0; i < linew; ++i) {
			int readpos = i+1+(800-linew);
			prevx = x;
			prevy = y;
			x = nextx;
			y = nexty;
			if((i+1) < linew) {
				if(knobs[0].value < .5) { // SCOPE
					nextx = (i+1)*pixel-1;
					nexty = amp*audio_processor.line[readpos+c*800];
				} else { // PANORAMA
					nextx = amp*audio_processor.line[readpos]*.75f;
					nexty = amp*audio_processor.line[readpos+(audio_processor.channelnum>1?800:0)]*-1;
				}
			}
			if(knobs[0].value > .5) luminocity = 1-pow(1-((float)i)/linew,4);

			angle1 = std::atan2(y-prevy,x-prevx);
			if((i+1) < linew) angle2 = std::atan2(y-nexty,x-nextx);
			while((angle1-angle2) < -1.5707963268) angle1 += 3.1415926535*2;
			while((angle1-angle2) >  1.5707963268) angle1 -= 3.1415926535*2;
			float angle = (angle1+angle2)*.5;

			if(audio_processor.linemode[readpos-1]) {
				line[c*4800+i*6  ] = x;
				line[c*4800+i*6+3] = x;
				line[c*4800+i*6+1] = y+LINEWIDTH*.75f;
				line[c*4800+i*6+4] = -line[c*4800+i*6+1];
				line[c*4800+i*6+2] = luminocity+y/(LINEWIDTH*.75f);
			} else {
				line[c*4800+i*6  ] = x+cos(angle)*LINEWIDTH;
				line[c*4800+i*6+3] = x-cos(angle)*LINEWIDTH;
				line[c*4800+i*6+1] = y+sin(angle)*LINEWIDTH*.75f;
				line[c*4800+i*6+4] = y-sin(angle)*LINEWIDTH*.75f;
				line[c*4800+i*6+2] = luminocity;
			}
			line[c*4800+i*6+5] = -line[c*4800+i*6+2];
		}
	}

	audio_processor.frameready = false;
}
void ScopeAudioProcessorEditor::paint(Graphics& g) { }

void ScopeAudioProcessorEditor::timerCallback() {
	audio_processor.sleep = 0;
	calcvis();

	update();

	if(positionwindow && audio_processor.settingswindow != nullptr) {
		if(audio_processor.settingsx.get() != -999 && audio_processor.settingsy.get() != -999)
			audio_processor.settingswindow->setTopLeftPosition(audio_processor.settingsx.get(),audio_processor.settingsy.get());
		else
			audio_processor.settingswindow->centreAroundComponent(this,getWidth(),getHeight());
		positionwindow = false;
	}

	if(firstframe && audio_processor.settingsopen.get()) {
		openwindow();
		firstframe = false;
	}
	audio_processor.settingsopen = audio_processor.settingswindow != nullptr;
}

void ScopeAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		if(parameterID == "xy") {
			knobs[1].visible = knobs[0].value < .5f;
			knobs[2].visible = knobs[0].value > .5f;
		} else if(parameterID == "hue" || parameterID == "saturation" || parameterID == "value") {
			bgcol = Colour::fromHSV(knobs[7].value,knobs[8].value,knobs[9].value,1);
		}
		return;
	}
}
void ScopeAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	openwindow();
}
void ScopeAudioProcessorEditor::openwindow() {
	if(audio_processor.settingswindow == nullptr) {
		audio_processor.settingswindow = new ScopeAudioProcessorSettings(audio_processor,knobcount,knobs);
		positionwindow = true;
	} else audio_processor.settingswindow->toFront(true);
}