#include "processor.h"
#include "editor.h"

MagicCarpetAudioProcessorEditor::MagicCarpetAudioProcessorEditor(MagicCarpetAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 360, 420) {
	int i = 0;
	for(int x = 0; x < paramcount; x++) {
		if(x >= 2 && x < 5) continue;
		knobs[i].x = (i%2)*106;
		knobs[i].y = (i/2)*100;
		knobs[i].id = params.pots[x].id;
		knobs[i].name = params.pots[x].name;
		knobs[i].value = params.pots[x].normalize(state.values[x]);
		knobs[i].lerpedvalue = knobs[i].value;
		knobs[i].slowlerpedvalue = knobs[i].value;
		knobs[i].minimumvalue = params.pots[x].minimumvalue;
		knobs[i].maximumvalue = params.pots[x].maximumvalue;
		knobs[i].defaultvalue = params.pots[x].normalize(params.pots[x].defaultvalue);
		knobs[i].str = knobs[i].name;
		if(i >= 2)
			knobs[i].str += "\n"+String(round((pow(knobs[i].value,2)*(MAX_DLY-MIN_DLY)+MIN_DLY)*1000))+"ms";
		add_listener(knobs[i].id);
		++knobcount;
		++i;
	}
	knobs[0].x = 180;
	knobs[0].y = 239;
	knobs[1].x = 120;
	knobs[1].y = 359;
	knobs[2].x = 60;
	knobs[2].y = 179;
	knobs[3].x = 240;
	knobs[3].y = 119;
	knobs[4].x = 300;
	knobs[4].y = 299;
	noise = state.values[2] > .5;
	add_listener("noise");
	knobs[0].indicator = knobs[0].value;
	for(int i = 0; i < knobcount; ++i) {
		if(i > 0) {
			knobs[i].indicator = round(random.nextFloat());
			if(i >= 2)
				indicatortimer[i-2] = (pow(knobs[i].value,2)*(MAX_DLY-MIN_DLY)+MIN_DLY)*30*random.nextFloat();
		}
		knobs[i].lerpedindicator = knobs[i].indicator;
	}
	timex = random.nextFloat()*MathConstants<float>::twoPi;
	timey = random.nextFloat()*MathConstants<float>::twoPi;
	for(int i = 0; i < BUFFERLENGTH; ++i) {
		lineindex = (lineindex+1)%BUFFERLENGTH;
		timex = fmod(timex+(1+fmod(rmslerped*30,1))*SPEED* .01f                                ,MathConstants<float>::twoPi);
		timey = fmod(timey+(1+fmod(rmslerped*30,1))*SPEED*(.11f*knobs[4].slowlerpedvalue+.012f),MathConstants<float>::twoPi);
		float circx = cos(timey)*1.5*knobs[3].slowlerpedvalue;
		float circy = sin(timey)*1.5*knobs[3].slowlerpedvalue*pow(1-knobs[2].slowlerpedvalue,2);
		linedata[lineindex*2  ] = ((circx*cos(timex)-circy*sin(timex))+cos(timex)*(1-knobs[3].slowlerpedvalue))*.8f;
		linedata[lineindex*2+1] = ((circx*sin(timex)+circy*cos(timex))+sin(timex)*(1-knobs[3].slowlerpedvalue))*.8f;
	}

	calcvis();

	for(int i = 0; i < 6; ++i)
		logopos[i] = random.nextFloat();

	setResizable(false,false);
	init(&look_n_feel);
}
MagicCarpetAudioProcessorEditor::~MagicCarpetAudioProcessorEditor() {
	close();
}

void MagicCarpetAudioProcessorEditor::newOpenGLContextCreated() {
	add_font(&font);

	roundedsquareshader = add_shader(
//ROUNDED SQUARE VERT
R"(#version 150 core
in vec2 coords;
uniform vec2 scale;
uniform float banner;
uniform vec2 res;
out vec2 uv;
void main(){
	gl_Position = vec4(1-coords.x*scale.x,(coords.y*scale.y-1)*(1-banner)-banner,0,1);
	uv = (1-coords)*res*.25-1;
})",
//ROUNDED SQUARE FRAG
R"(#version 150 core
in vec2 uv;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec2 clampuv = min(uv,0);
	fragColor = vec4((1-sqrt(clampuv.y*clampuv.y+clampuv.x*clampuv.x))*4*dpi+.5,0,0,1);
})");

	carpetshader = add_shader(
//CARPET VERT
R"(#version 150 core
in vec2 coords;
uniform float banner;
uniform float orientation;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(coords.x,coords.y*(1-banner))*2-1,0,1);
	uv = coords;
	uv.x = uv.x*(6./7.)+mod(orientation,1)/7.;
	if(mod(orientation,2) > 1) uv.xy = uv.yx;
	if(mod(orientation,4) > 2) uv.x = 1-uv.x;
	if(mod(orientation,8) > 4) uv.y = 1-uv.y;
})",
//CARPET FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D carpettex;
uniform float orientation;
out vec4 fragColor;
void main() {
	float o = floor(orientation/8);
	if(o <= 0.5)
		fragColor = vec4(0,0,texture(carpettex,uv).r,1);
	else if(o <= 1.5)
		fragColor = vec4(0,0,texture(carpettex,uv).g,1);
	else
		fragColor = vec4(0,0,texture(carpettex,uv).b,1);
})");

	knobshader = add_shader(
//KNOB VERT
R"(#version 150 core
in vec2 coords;
uniform float knobrot;
uniform vec2 knobscale;
uniform vec2 knobpos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(coords.x*knobscale.x+knobpos.x,(coords.y*knobscale.y+knobpos.y)*(1-banner)-banner,0,1);
	uv = vec2(
		(coords.x-.5)*cos(knobrot)-(coords.y-.5)*sin(knobrot),
		(coords.x-.5)*sin(knobrot)+(coords.y-.5)*cos(knobrot))*2;
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
uniform float dpi;
uniform float indicator;
out vec4 fragColor;
void main(){
	float margin = 1.0/60;
	float circ = sqrt(uv.y*uv.y+uv.x*uv.x);
	float col = 1-circ;
	if(uv.y > 0)
		col = min(col,abs(uv.x)-.3333-margin);
	else
		col = min(col,circ-.3333-margin);
	col = max(col*dpi*30,min(1,(.3333-margin-circ)*dpi*30)*indicator);
	fragColor = vec4(col,0,0,1);
})");

	centershader = add_shader(
//CENTER VERT
R"(#version 150 core
in vec2 coords;
uniform vec4 pos;
uniform float banner;
out vec2 uv;
void main() {
	gl_Position = vec4(coords.x*pos.z+pos.x,(coords.y*pos.w+pos.y)*(1-banner)-banner,0,1);
	uv = coords*2-1;
})",
//CENTER FRAG
R"(#version 150 core
in vec2 uv;
uniform float dpi;
out vec4 fragColor;
void main() {
	vec3 col = abs(vec3(uv,.5-sqrt(uv.y*uv.y+uv.x*uv.x)));
	fragColor = vec4(vec3((.04-min(col.x,min(col.y,col.z)))*35*dpi*.5),1);
})");

	lineshader = add_shader(
//LINE VERT
R"(#version 150 core
in vec3 coords;
uniform float banner;
uniform float ratio;
out float luminocity;
out float linex;
void main() {
	gl_Position = vec4(coords.x,(coords.y*.5+.5)*ratio*(1-banner)*2-1,0,1);
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
	fragColor = vec4(0,min(1,(1-abs(linex))*scaling*luminocity),0,min(1,luminocity));
})");

	logoshader = add_shader(
//LOGO VERT
R"(#version 150 core
in vec2 coords;
uniform vec4 scale;
uniform float banner;
out vec2 uv;
out vec2 logouv;
void main(){
	gl_Position = vec4(coords.x*scale.z-1,(1-(1-coords.y)*scale.w)*(1-banner)-banner,0,1);
	uv = vec2(coords.x*scale.x,1-(1-coords.y)*scale.y);
	logouv = vec2(1-(1-coords.x)*scale.x,coords.y*scale.y);
})",
//LOGO FRAG
R"(#version 150 core
in vec2 uv;
in vec2 logouv;
uniform sampler2D tex;
uniform float dpi;
uniform float logoprog;
uniform vec2 logoposa;
uniform vec2 logoposb;
uniform vec2 logoposc;
out vec4 fragColor;
void main(){
	fragColor = vec4(0,0,0,1);
	if(logoprog > 2) {
		fragColor.r = texture(tex,logouv+logoposa).b*min(1,logoprog-2);
		if(logoprog > 3) {
			fragColor.r = max(fragColor.r,texture(tex,logouv+logoposb).b*min(1,logoprog-3));
			if(logoprog > 4)
				fragColor.g = texture(tex,logouv+logoposc).b*min(1,logoprog-4);
		}
	}
	if(logoprog < 3) {
		vec2 logo = texture(tex,uv).rg;
		fragColor.r = max(fragColor.r,logo.r*min(1,3-logoprog));
		if(logoprog < 2) {
			fragColor.r += logo.g*min(1,2-logoprog);
			if(logoprog < 1)
				fragColor.g = logo.g*min(1,1-logoprog);
		}
	}
	fragColor.rg = (fragColor.rg-.5)*dpi+.5;
})");

	ppshader = add_shader(
//PP VERT
R"(#version 150 core
in vec2 coords;
uniform float banner;
uniform vec2 scale;
uniform vec3 noiseoffsetc;
uniform vec3 noiseoffsetm;
uniform vec3 noiseoffsety;
out vec2 uv;
out vec2 uvc;
out vec2 uvm;
out vec2 uvy;
void main(){
	gl_Position = vec4(vec2(coords.x,coords.y*(1-banner)+banner)*2-1,0,1);
	uv = coords;
	uvc = uv*scale;
	uvm = uvc;
	uvy = uvc;
	uvc += noiseoffsetc.xy;
	if(mod(noiseoffsetc.z,2) > 1) uvc.x = 1-uvc.x;
	if(mod(noiseoffsetc.z,4) > 2) uvc.y = 1-uvc.y;
	uvm += noiseoffsetm.xy;
	if(mod(noiseoffsetm.z,2) > 1) uvm.x = 1-uvm.x;
	if(mod(noiseoffsetm.z,4) > 2) uvm.y = 1-uvm.y;
	uvy += noiseoffsety.xy;
	if(mod(noiseoffsety.z,2) > 1) uvy.x = 1-uvy.x;
	if(mod(noiseoffsety.z,4) > 2) uvy.y = 1-uvy.y;
})",
//PP FRAG
R"(#version 150 core
in vec2 uv;
in vec2 uvc;
in vec2 uvm;
in vec2 uvy;
uniform sampler2D buffertex;
uniform sampler2D noisetex;
uniform vec4 offset;
out vec4 fragColor;
void main(){
	vec3 col = vec3(texture(buffertex,uv+offset.xy).r,texture(buffertex,uv+offset.zw).g,texture(buffertex,uv).b);
	vec2 noisec = texture(noisetex,uvc).rg;
	vec2 noisem = texture(noisetex,uvm).rg;
	vec2 noisey = texture(noisetex,uvy).rg;
	col = (min(max(vec3(noisec.g,noisem.g,noisey.g)+(sqrt(col)*2-1),0),1)*.7+col*.3)*((.5-vec3(noisec.r,noisem.r,noisey.r))*vec3(.5,.7,.5)+1);
	fragColor = vec4(
		(1-col.r*vec3(1			,0.53125	,0.25390625	))*
		(1-col.g*vec3(0.00390625,0.71875	,0.3125		))*
		(1-col.b*vec3(0.00390625,0.09375	,1			))*
		         vec3(0.99609375,0.98046875	,0.98828125	),1);
	//fragColor = texture(noisetex,uvc);
})");

	add_texture(&carpettex, BinaryData::carpet_png, BinaryData::carpet_pngSize);
	add_texture(&logotex, BinaryData::logo_png, BinaryData::logo_pngSize);
	add_texture(&noisetex, BinaryData::noise_png, BinaryData::noise_pngSize, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	add_frame_buffer(&frame_buffer, width, height);

	draw_init();
}
void MagicCarpetAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(carpetshader->getProgramID(),"coords");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	frame_buffer.makeCurrentRenderingTarget();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	OpenGLHelpers::clear(Colour::fromRGB(0,0,0));

	if(noise) {
		roundedsquareshader->use();
		coord = context.extensions.glGetAttribLocation(roundedsquareshader->getProgramID(),"coords");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		roundedsquareshader->setUniform("scale",166.f*2.f/width,53.f*2.f/height);
		roundedsquareshader->setUniform("res",166.f,53.f);
		roundedsquareshader->setUniform("banner",banner_offset);
		roundedsquareshader->setUniform("dpi",scaled_dpi*.6f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);

		font.draw_string(0,0,0,1,0,0,0,0,"NOISE MODE ACTIVATED\nTurn up feedback and\nmodulate delay times",0,1-round(3.f*scaled_dpi)/scaled_dpi/width,1-round(2.f*scaled_dpi)/scaled_dpi/height,1,1);
	}

	for(int i = 0; i < knobcount; i++)
		font.draw_string(1,0,0,1,1,0,0,0,knobs[i].str,0,((float)knobs[i].x)/width,((float)knobs[i].y+31)/height,.5f,0);

	glBlendFunc(GL_ONE, GL_ONE);

	knobshader->use();
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	knobshader->setUniform("knobscale",60.f*2.f/width,60.f*2.f/height);
	knobshader->setUniform("banner",banner_offset);
	knobshader->setUniform("dpi",scaled_dpi*.6f);
	for(int i = 0; i < knobcount; i++) {
		knobshader->setUniform("knobpos",((float)knobs[i].x*2-60.f)/width-1,1-((float)knobs[i].y*2+60.f)/height);
		knobshader->setUniform("knobrot",(knobs[i].lerpedvalue-.5f)*5.5f);
		knobshader->setUniform("indicator",knobs[i].lerpedindicator);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	centershader->use();
	coord = context.extensions.glGetAttribLocation(centershader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	centershader->setUniform("banner",banner_offset);
	centershader->setUniform("dpi",(float)fmax(1.f,scaled_dpi*.6f));
	for(int i = 0; i < 3; ++i) {
		centershader->setUniform("pos",1-(-17.5f+audio_processor.randomui[i*2+1]*72.5)*2/width,1-(-17.5f+audio_processor.randomui[i*2+2]*72.5)*2/height,-35.f*2/width,-35.f*2/height);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	logoshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, logotex.getTextureID());
	logoshader->setUniform("tex",0);
	logoshader->setUniform("banner",banner_offset);
	logoshader->setUniform("dpi",(float)fmax(1.f,scaled_dpi*.3f));
	logoshader->setUniform("logoprog",(float)(floor(logoprog)+fmod(logoprog,1)*(hover==-2?.2:.8)*2));
	logoshader->setUniform("logoposa",round(logopos[0]*.6f*220)/220.f,round(logopos[1]*-.85f*78.f)/78.f);
	logoshader->setUniform("logoposb",round(logopos[2]*.6f*220)/220.f,round(logopos[3]*-.85f*78.f)/78.f);
	logoshader->setUniform("logoposc",round(logopos[4]*.6f*220)/220.f,round(logopos[5]*-.85f*78.f)/78.f);
	logoshader->setUniform("scale",250.f/220.f,102.f/78.f,250.f*2/width,102.f*2/height);
	coord = context.extensions.glGetAttribLocation(logoshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	lineshader->use();
	lineshader->setUniform("banner",banner_offset);
	lineshader->setUniform("scaling",(float)(width*dpi*LINEWIDTH*.35f*(1-powf(1-knobs[0].slowlerpedvalue,2.f))));
	lineshader->setUniform("ratio",((float)width)/height);
	coord = context.extensions.glGetAttribLocation(lineshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*BUFFERLENGTH*6, line, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP,0,BUFFERLENGTH*2);
	context.extensions.glDisableVertexAttribArray(coord);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	carpetshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	carpettex.bind();
	carpetshader->setUniform("carpettex",0);
	carpetshader->setUniform("banner",banner_offset);
	carpetshader->setUniform("orientation",audio_processor.randomui[0]*24);
	coord = context.extensions.glGetAttribLocation(carpetshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	frame_buffer.releaseAsRenderingTarget();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//post processing
	ppshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_buffer.getTextureID());
	ppshader->setUniform("buffertex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	noisetex.bind();
	ppshader->setUniform("noisetex",1);
	ppshader->setUniform("banner",banner_offset);
	ppshader->setUniform("scale",(width*scaled_dpi)/1024.f,(height*scaled_dpi)/1024.f);
	ppshader->setUniform("offset",
			(audio_processor.randomui[ 7]-.5f)*3/width,
			(audio_processor.randomui[ 8]-.5f)*3/height,
			(audio_processor.randomui[ 9]-.5f)*3/width,
			(audio_processor.randomui[10]-.5f)*3/height);
	ppshader->setUniform("noiseoffsetc",
			audio_processor.randomui[11],audio_processor.randomui[12],audio_processor.randomui[13]*4);
	ppshader->setUniform("noiseoffsetm",
			audio_processor.randomui[14],audio_processor.randomui[15],audio_processor.randomui[16]*4);
	ppshader->setUniform("noiseoffsety",
			audio_processor.randomui[17],audio_processor.randomui[18],audio_processor.randomui[19]*4);
	coord = context.extensions.glGetAttribLocation(ppshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void MagicCarpetAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void MagicCarpetAudioProcessorEditor::calcvis() {
	for(int i = 0; i < ((int)floor(1+rmslerped*30)); ++i) {
		for(int i = 0; i < knobcount; i++)
			knobs[i].slowlerpedvalue = knobs[i].slowlerpedvalue*.8f+knobs[i].value*.2f;
		lineindex = (lineindex+1)%BUFFERLENGTH;
		timex = fmod(timex+(1+fmod(rmslerped*30,1))*SPEED* .01f                                ,MathConstants<float>::twoPi);
		timey = fmod(timey+(1+fmod(rmslerped*30,1))*SPEED*(.11f*knobs[4].slowlerpedvalue+.012f),MathConstants<float>::twoPi);
		float circx = cos(timey)*1.5*knobs[3].slowlerpedvalue;
		float circy = sin(timey)*1.5*knobs[3].slowlerpedvalue*pow(1-knobs[2].slowlerpedvalue,2);
		linedata[lineindex*2  ] = ((circx*cos(timex)-circy*sin(timex))+cos(timex)*(1-knobs[3].slowlerpedvalue))*.8f;
		linedata[lineindex*2+1] = ((circx*sin(timex)+circy*cos(timex))+sin(timex)*(1-knobs[3].slowlerpedvalue))*.8f;
	}

	// line calculous
	float nextx = -1;
	float nexty = 0;
	nextx = linedata[(lineindex*2+2)%(BUFFERLENGTH*2)];
	nexty = linedata[(lineindex*2+3)%(BUFFERLENGTH*2)];
	float x = nextx;
	float y = nexty;
	float prevx = nextx;
	float prevy = nexty;
	float angle1 = 0;
	float angle2 = 0;
	for(int i = 0; i < BUFFERLENGTH; ++i) {
		prevx = x;
		prevy = y;
		x = nextx;
		y = nexty;
		if(i < (BUFFERLENGTH-1)) {
			nextx = linedata[((lineindex+i)*2+4)%(BUFFERLENGTH*2)];
			nexty = linedata[((lineindex+i)*2+5)%(BUFFERLENGTH*2)];
		}

		angle1 = std::atan2(y-prevy,x-prevx);
		if(i < (BUFFERLENGTH-1)) angle2 = std::atan2(y-nexty,x-nextx);
		while((angle1-angle2) < -1.5707963268) angle1 += 3.1415926535*2;
		while((angle1-angle2) >  1.5707963268) angle1 -= 3.1415926535*2;
		float angle = (angle1+angle2)*.5;

		line[i*6  ] = x+cos(angle)*LINEWIDTH;
		line[i*6+3] = x-cos(angle)*LINEWIDTH;
		line[i*6+1] = y+sin(angle)*LINEWIDTH;
		line[i*6+4] = y-sin(angle)*LINEWIDTH;
		line[i*6+2] = 1-pow(fmin(1,(1-((float)i)/BUFFERLENGTH)/(knobs[1].slowlerpedvalue+.01f)),4);
		line[i*6+5] = -line[i*6+2];
	}
}
void MagicCarpetAudioProcessorEditor::paint(Graphics& g) { }

void MagicCarpetAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < DLINES; i++) {
		if(indicatortimer[i]++ >= ((pow(knobs[i+2].value,2)*(MAX_DLY-MIN_DLY)+MIN_DLY)*30)) {
			knobs[i+2].indicator = 1-knobs[i+2].indicator;
			knobs[  1].indicator = 1-knobs[  1].indicator;
			indicatortimer[i] = 0;
		}
	}
	for(int i = 0; i < knobcount; i++) {
		knobs[i].lerpedvalue = knobs[i].lerpedvalue*.5f+knobs[i].value*.5f;
		knobs[i].lerpedindicator = knobs[i].lerpedindicator*.35f+knobs[i].indicator*.65f;
	}

	if(logoprog != (hover==-2?5:0)) {
		logoprog = fmax(fmin(logoprog+(hover==-2?.5:-.5),5),0);
		if(logoprog == 0) {
			for(int i = 0; i < 6; ++i)
				logopos[i] = random.nextFloat();
		}
	}

	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;
	rmslerped = rmslerped*.6f+rms*.4f;

	calcvis();

	update();
}

void MagicCarpetAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "noise") noise = newValue > .5;
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		if(i == 0)
			knobs[i].indicator = knobs[i].value;
		if(i >= 2)
			knobs[i].str = knobs[i].name+"\n"+String(round((pow(knobs[i].value,2)*(MAX_DLY-MIN_DLY)+MIN_DLY)*1000))+"ms";
		return;
	}
}
void MagicCarpetAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	hover = recalc_hover(event.x,event.y);
}
void MagicCarpetAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void MagicCarpetAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(dpi < 0) return;
	if(event.mods.isRightButtonDown()) {
		hover = recalc_hover(event.x,event.y);
		std::unique_ptr<PopupMenu> rightclickmenu(new PopupMenu());
		std::unique_ptr<PopupMenu> scalemenu(new PopupMenu());

		int i = 20;
		while(++i < (ui_scales.size()+21))
			scalemenu->addItem(i,(String)round(ui_scales[i-21]*100)+"%",true,(i-21)==ui_scale_index);

		rightclickmenu->setLookAndFeel(&look_n_feel);
		rightclickmenu->addItem(1,"'Copy preset",true);
		rightclickmenu->addItem(2,"'Paste preset",audio_processor.is_valid_preset_string(SystemClipboard::getTextFromClipboard()));
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu("'Scale",*scalemenu);
		rightclickmenu->addSeparator();
		rightclickmenu->addItem(3,"'Randomize");
		rightclickmenu->addItem(4,((String)"'Noise mode: O")+(noise?"N":"FF"));
		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20) {
				set_ui_scale(result-21);
			} else if(result == 1) { //copy preset
				SystemClipboard::copyTextToClipboard(audio_processor.get_preset(audio_processor.currentpreset));
			} else if(result == 2) { //paste preset
				audio_processor.set_preset(SystemClipboard::getTextFromClipboard(), audio_processor.currentpreset);
			} else if(result == 3) { //randomize
				audio_processor.randomize();
			} else if(result == 4) { //toggle noise
				audio_processor.apvts.getParameter("noise")->setValueNotifyingHost(noise?0.f:1.f);
			}
		});
		return;
	}

	initialdrag = hover;
	if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		audio_processor.lerpchanged[hover] = true;
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void MagicCarpetAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
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
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset);

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	}
}
void MagicCarpetAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(hover > -1) {
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((knobs[hover].value-initialvalue)>=0?"Increased ":"Decreased ") += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		if(hover == -2 && prevhover == -2)
			URL("https://vst.unplug.red/").launchInDefaultBrowser();
		else if(hover == -3 && prevhover == -3)
			audio_processor.randomize();
		else if(hover == -4 && prevhover == -4 && noise)
			audio_processor.apvts.getParameter("noise")->setValueNotifyingHost(0.f);
	}
}
void MagicCarpetAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audio_processor.undo_manager.beginNewTransaction();
}
void MagicCarpetAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int MagicCarpetAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if(y < 78) {
		if(x <= 220) return -2;
		if(x > 282) return -3;
		return -1;
	}
	if(y > 367 && x > 194 && noise) return -4;
	for(int i = 0; i < knobcount; i++) {
		float xx = knobs[i].x-x;
		float yy = knobs[i].y-y;
		if((xx*xx+yy*yy) <= 900) return i;
	}
	return -1;
}

LookNFeel::LookNFeel() {
	setColour(PopupMenu::backgroundColourId,Colour::fromFloatRGBA(0.f,0.f,0.f,0.f));
	font = find_font("Consolas|Noto Mono|DejaVu Sans Mono|Menlo|Andale Mono|SF Mono|Lucida Console|Liberation Mono");
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	if(font == "None")
		return Font(18.f*scale,Font::plain);
	return Font(font,"Regular",18.f*scale);
}
void LookNFeel::drawPopupMenuBackground(Graphics &g, int width, int height) {
	g.setColour(bg);
	g.fillRoundedRectangle(0,0,width,height,4.f*scale);
}
void LookNFeel::drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) {
	if(isSeparator) {
		g.setColour(fg);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		return;
	}

	bool removeleft = text.startsWith("'");
	if(isHighlighted && isActive) {
		g.setColour(fg);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		g.setColour(bg);
	} else {
		g.setColour(fg);
	}
	if(textColour != nullptr)
		g.setColour(*textColour);

	auto r = area;
	if(removeleft) r.removeFromLeft(5*scale);
	else r.removeFromLeft(area.getHeight());

	Font font = getPopupMenuFont();
	if(!isActive && removeleft) font.setItalic(true);
	float maxFontHeight = ((float)r.getHeight())/1.45f;
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
int LookNFeel::getPopupMenuBorderSize() {
	return (int)floor(scale);
}
void LookNFeel::getIdealPopupMenuItemSize(const String& text, const bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight)
{
	if(isSeparator) {
		idealWidth = 50*scale;
		idealHeight = (int)ceil(scale);
	} else {
		Font font(getPopupMenuFont());

		if(standardMenuItemHeight > 0 && font.getHeight() > standardMenuItemHeight/1.3f)
			font.setHeight(standardMenuItemHeight/1.3f);

		bool removeleft = text.startsWith("'");
		String newtext = text;
		if(removeleft)
			newtext = text.substring(1);

		int idealheightsingle = (int)floor(font.getHeight()*1.3);

		std::stringstream ss(newtext.trim().toRawUTF8());
		std::string token;
		idealWidth = 0;
		int lines = 0;
		while(std::getline(ss, token, '\n')) {
			idealWidth = fmax(idealWidth,font.getStringWidth(token));
			++lines;
		}

		if(removeleft)
			idealWidth += idealheightsingle*2-5*scale;
		else
			idealWidth += idealheightsingle;

		idealHeight = (int)floor(font.getHeight()*(lines+.3));
	}
}
