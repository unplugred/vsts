#include "PluginProcessor.h"
#include "PluginEditor.h"

CRMBLAudioProcessorEditor::CRMBLAudioProcessorEditor (CRMBLAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor (p), plugmachine_gui(p, 507, 465) {
	//special labels: time, mod, feedback
	//CLOCK
	knobs[0].description = "Computer time";
	knobs[0].index = -1;
	knobs[0].x = .4282319003;
	knobs[0].y = .5832025519;
	knobs[0].radius = .4282319003;
	knobs[0].linewidth = .0222989305;
	knobs[0].lineheight = .73;
	knobs[0].r = .0625f;
	//FEEDBACK
	knobs[1].description = "The amount of the output that goes back into the input.\nWhen turned all the way an infinite delay is created.";
	knobs[1].index = 6;
	knobs[1].x = .5095732975;
	knobs[1].y = .7174707256;
	knobs[1].radius = .1885720718;
	knobs[1].linewidth = .050639065;
	knobs[1].r = .5;
	//TIME
	knobs[2].description = "The amount of time before the signal repeats in milliseconds.\nCan be BPM synced by holding CTRL while dragging.";
	knobs[2].index = 0;
	knobs[2].x = .6480072833;
	knobs[2].y = .6583846315;
	knobs[2].radius = .2376583343;
	knobs[2].linewidth = .0401800064;
	knobs[2].r = .1875;
	//MOD FREQ
	knobs[3].description = "Frequency of LFO on the delay time.";
	knobs[3].index = 3;
	knobs[3].x = .8327202455;
	knobs[3].y = .5297886311;
	knobs[3].radius = .0990625024;
	knobs[3].linewidth = .0963948332;
	knobs[3].lineheight = 1;
	knobs[3].r = .4375;
	//MOD
	knobs[4].description = "Amplitude of LFO on the delay time.";
	knobs[4].index = 2;
	knobs[4].x = .7837187701;
	knobs[4].y = .470595587;
	knobs[4].radius = .0990625024;
	knobs[4].linewidth = .0963948332;
	knobs[4].lineheight = 1;
	knobs[4].r = .375;
	//DRY/WET
	knobs[5].description = "Mix between the processed and unprocessed signal.";
	knobs[5].index = 11;
	knobs[5].x = .4592473633;
	knobs[5].y = .9123757694;
	knobs[5].radius = .2376583343;
	knobs[5].linewidth = .0401800064;
	knobs[5].r = .625;
	//PINGPONG
	knobs[6].description = "Shortens the initial delay for the left or right channels.\nIf set to 'PRE' this also affects the feedback, creating interesting polyrythmic patterns.";
	knobs[6].index = 4;
	knobs[6].x = .6327409673;
	knobs[6].y = .7939896811;
	knobs[6].radius = .1885720718;
	knobs[6].linewidth = .050639065;
	knobs[6].r = .25;
	//PITCH
	knobs[7].description = "Changes the pitch (in semitones) of a pitch shifting effect, applied with each repeat of the signal.\nCan be set to a microtonal value by holding CTRL while dragging.";
	knobs[7].index = 9;
	knobs[7].x = .2221832359;
	knobs[7].y = .3319921158;
	knobs[7].radius = .1885720718;
	knobs[7].linewidth = .050639065;
	knobs[7].r = .5625;
	//CHEW
	knobs[8].description = "An effect which adds gating and harmonics, applied on every repeat to create a \"chewing\" effect.\nAlso exists as an independent plugin under the name PNCH.";
	knobs[8].index = 8;
	knobs[8].x = .1082867784;
	knobs[8].y = .4330561124;
	knobs[8].radius = .1885720718;
	knobs[8].linewidth = .050639065;
	knobs[8].r = .6875;
	//REVERSE
	knobs[9].description = "Blends between forward and revered versions of the delay.";
	knobs[9].index = 7;
	knobs[9].x = .2314544482;
	knobs[9].y = .5413354191;
	knobs[9].radius = .2376583343;
	knobs[9].linewidth = .0401800064;
	knobs[9].r = .3125;
	//LOWPASS
	knobs[10].description = "Applies a low-pass filter, reducing higher frequencies with every repeat.";
	knobs[10].index = 10;
	knobs[10].x = .3532976591;
	knobs[10].y = .3882975081;
	knobs[10].radius = .2376583343;
	knobs[10].linewidth = .0401800064;
	knobs[10].r = .125;

	for(int i = 0; i < knobcount; i++) {
		knobs[i].description = look_n_feel.add_line_breaks(knobs[i].description);
		knobs[i].radius += .004;
		knobs[i].x -= .001;
		knobs[i].y += .002;
	}
	for(int i = 1; i < knobcount; i++) {
		knobs[i].id = params.pots[knobs[i].index].id;
		knobs[i].name = params.pots[knobs[i].index].name;
		knobs[i].value = params.pots[knobs[i].index].normalize(state.values[knobs[i].index]);
		knobs[i].minimumvalue = params.pots[knobs[i].index].minimumvalue;
		knobs[i].maximumvalue = params.pots[knobs[i].index].maximumvalue;
		knobs[i].defaultvalue = params.pots[knobs[i].index].normalize(params.pots[knobs[i].index].defaultvalue);
		add_listener(knobs[i].id);
	}
	hold = params.holdsmooth.getTargetValue() >.5;
	oversampling = params.oversampling;
	for(int i = 0; i < paramcount; i++) {
		if(params.pots[i].id == "pingpostfeedback")
			postfb = state.values[i] > .5;
		else if(params.pots[i].id == "time")
			time = state.values[i];
		else if(params.pots[i].id == "sync")
			sync = state.values[i];
	}
	recalclabels();
	add_listener("oversampling");
	add_listener("hold");
	add_listener("pingpostfeedback");
	add_listener("sync");

	rmsdamp.reset(0,.9,-1,30,1);
	for(int i = 0; i < 32; i++) damparray[i] = 0;

	init(&look_n_feel);
}
CRMBLAudioProcessorEditor::~CRMBLAudioProcessorEditor() {
	close();
}

void CRMBLAudioProcessorEditor::newOpenGLContextCreated() {
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform float offset;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)-offset)*2-1,0,1);
	uv = aPos;
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D basetex;
uniform float r;
uniform float gb;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec4 col = texture(basetex,uv);
	col.r /= col.a;
	col.gba = max(min((col.gba-.5)*dpi+.5,1),0);
	fragColor = vec4(0);
	if(col.r <= (r+.033)) {
		if(col.r >= (r-.03))
			fragColor = vec4(gb>.5?col.g:col.b,0,1,col.a);
		else if(col.r >= (r-.031))
			fragColor = vec4(0,0,1,col.a);
		if(col.r >= .7) fragColor.g = fragColor.r;
	}
})");

	logoshader = add_shader(
//LOGO VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner))*2-1,0,1);
	uv = aPos;
})",
//LOGO FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D basetex;
uniform float websiteht;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec4 col = texture(basetex,uv);
	col.r /= col.a;
	col.ga = max(min((col.ga-.5)*dpi+.5,1),0);
	fragColor = vec4(0);
	if(col.r >= .85)
		fragColor = vec4(vec2(col.g*(1-max(min((texture(basetex,vec2(min(uv.x+websiteht,.3),uv.y)).b-.5)*dpi+.5,1),0)*.5)),1,col.a);
})");

	knobshader = add_shader(
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform float knobrot;
uniform vec2 knobscale;
uniform vec2 knobpos;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*knobscale+knobpos)*vec2(1,1-banner)-vec2(0,banner),0,1);
	uv = vec2(
		(aPos.x-.5)*cos(knobrot)-(aPos.y-.5)*sin(knobrot),
		(aPos.x-.5)*sin(knobrot)+(aPos.y-.5)*cos(knobrot))*2;
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
uniform int hoverstate;
uniform float thickness;
uniform float lineheight;
uniform vec2 knobscale;
uniform float circle;
uniform float bright;
uniform int dark;
out vec4 fragColor;
void main(){
	float d = 0;
	if(circle > .5) {
		d = sqrt(uv.y*uv.y+uv.x*uv.x);
		if((1-d) > (d-thickness)) {
			float y = uv.y;
			if(y > 0) y = max(y-lineheight,0);
			d = max(d-thickness,0)*100*knobscale.y+max(1-sqrt(y*y+uv.x*uv.x)*2/(1-thickness),0)*2.5;
			fragColor = vec4(min(d,1),1,1,1);
		} else fragColor = vec4(1,1,1,min((1-d)*100*knobscale.y,1));
	} else {
		float y = uv.y;
		if(y > 0) y = max(y-lineheight,0);
		fragColor = vec4(1,1,1,min(max(1-sqrt(y*y+uv.x*uv.x)*2/(1-thickness),0)*2.5,1));
	}
	fragColor.r = 1-(1-fragColor.r)*bright;
	if(dark > .5) fragColor = vec4(fragColor.r,fragColor.r,fragColor.r,fragColor.a);
})");

	feedbackshader = add_shader(
//FEEDBACK VERT
R"(#version 150 core
in vec2 aPos;
uniform float pitch;
uniform float banner;
out vec2 uv;
out vec2 ruv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner))*2-1,0,1);
	ruv = aPos;
	uv = vec2(
		(aPos.x-.5)*cos(pitch)-(aPos.y-.5)*sin(pitch),
		(aPos.x-.5)*sin(pitch)+(aPos.y-.5)*cos(pitch))+.5;
})",
//FEEDBACK FRAG
R"(#version 150 core
in vec2 uv;
in vec2 ruv;
uniform sampler2D feedbacktex;
uniform sampler2D maintex;
uniform vec4 time;
uniform float feedback;
uniform float lowpass;
uniform float chew;
uniform float ratio;
uniform vec2 res;
out vec4 fragColor;
vec2 hash(vec2 x) {
	const vec2 k = vec2(0.3183099, 0.3678794);
	x = x*k + k.yx;
	return -1.0 + 2.0*fract(16.0*k*fract(x.x*x.y*(x.x+x.y)));
}
float noise(in vec2 p){
	vec2 i = floor(p);
	vec2 f = fract(p);

	vec2 u = f*f*(3.0 - 2.0*f);

	return mix(mix(dot(hash(i + vec2(0.0,0.0)), f - vec2(0.0,0.0)),
				   dot(hash(i + vec2(1.0,0.0)), f - vec2(1.0,0.0)), u.x),
			   mix(dot(hash(i + vec2(0.0,1.0)), f - vec2(0.0,1.0)),
				   dot(hash(i + vec2(1.0,1.0)), f - vec2(1.0,1.0)), u.x), u.y);
}
vec4 sample(in sampler2D tex, in vec2 puv) {
	if(puv.x < 0 || puv.x > 1 || puv.y < 0 || puv.y > 1) return vec4(0,0,0,1);
	else return texture(tex,puv)*min(puv.x*res.x,1)*min(puv.y*res.y,1)*min((1-puv.x)*res.x,1)*min((1-puv.y)*res.y,1);
}
void main(){
	vec2 nuv = uv;
	if(chew > 0) nuv += vec2(noise(ruv*4),noise((ruv+2)*4))*chew;
	vec2 col1 = sample(maintex,nuv+vec2(time.x*ratio,time.x)).rg;
	vec2 col2 = sample(maintex,nuv+vec2(time.y*ratio,time.y)).rg;
	fragColor   = sample(feedbacktex,nuv+vec2(time.z*ratio,time.z));
	fragColor.g = sample(feedbacktex,nuv+vec2((time.w-.004*lowpass)*ratio,time.w)).g;
	if(lowpass > 0) {
		fragColor.r = fragColor.r*.5+sample(feedbacktex,nuv+vec2((time.z+.008*lowpass)*ratio,time.z)).r*.5;
		fragColor.g = fragColor.g*.5+sample(feedbacktex,nuv+vec2((time.w+.004*lowpass)*ratio,time.w)).g*.5;
		fragColor.b = fragColor.b*.5+sample(feedbacktex,nuv+vec2((time.z-.008*lowpass)*ratio,time.z)).b*.5;
	}
	fragColor   = vec4(vec3(col1.r,col2.r,col1.r)*vec3(col1.g,col2.g,col1.g)+(1-vec3(col1.g,col2.g,col1.g))*feedback*fragColor.rgb,1);
})");

	buffershader = add_shader(
//BUFFER VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = aPos;
})",
//BUFFER FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D feedbacktex;
uniform sampler2D maintex;
uniform float wet;
uniform float reverse;
out vec4 fragColor;
void main(){
	vec4 col = texture(maintex,uv);
	fragColor = vec4(vec3(col.r),1);
	if(col.g < 1)
		fragColor = fragColor*col.b+texture(feedbacktex,uv)*vec4(vec3(wet),1)*(1-col.b);
	fragColor = vec4(abs(fragColor.rgb*fragColor.a-vec3(pow(reverse,.5),reverse,pow(reverse,2.))),1);
})");

	numbershader = add_shader(
//NUMBER VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 size;
uniform vec2 pos;
uniform vec2 index;
uniform int length;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos.x*size.x*length+pos.x,(aPos.y*size.y+(1-pos.y))*(1-banner)+banner,0,1);
	uv = (aPos+index)*vec2(.0625*length,.5);
})",
//NUMBER FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D numbertex;
uniform vec3 col;
out vec4 fragColor;
void main(){
	fragColor = vec4(texture(numbertex,uv).r)*vec4(col,1);
})");

	add_texture(&basetex, BinaryData::map_png, BinaryData::map_pngSize);
	add_texture(&numbertex, BinaryData::numbers_png, BinaryData::numbers_pngSize, GL_NEAREST, GL_NEAREST);

	add_frame_buffer(&feedbackbuffer, width, height);
	add_frame_buffer(&mainbuffer, width, height);

	draw_init();
}
void CRMBLAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mainbuffer.makeCurrentRenderingTarget();
	OpenGLHelpers::clear(Colour::fromRGB(0,0,0));

	baseshader->use();
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("gb",postfb?1.f:0.f);
	baseshader->setUniform("dpi",(float)fmax(scaled_dpi,1));
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < knobcount; i++) {
		knobs[i].yoffset = damparray[((int)round(33+dampreadpos-knobs[i].r*16))%32]*.05f;
		if(i != 1 && i != 2 && i != 4) {
			baseshader->setUniform("r",knobs[i].r);
			baseshader->setUniform("offset",knobs[i].yoffset*(1-banner_offset)*1.15f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	baseshader->setUniform("offset",0.f);
	baseshader->setUniform("gb",hold?0.f:1.f);
	baseshader->setUniform("r",.75f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	baseshader->setUniform("gb",oversampling?1.f:0.f);
	baseshader->setUniform("r",.8125f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	logoshader->use();
	coord = context.extensions.glGetAttribLocation(logoshader->getProgramID(),"aPos");
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	logoshader->setUniform("basetex",0);
	logoshader->setUniform("banner",banner_offset);
	logoshader->setUniform("websiteht",websiteht);
	logoshader->setUniform("dpi",(float)fmax(scaled_dpi,1));
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	float ratio = ((float)height)/width;
	knobshader->use();
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	knobshader->setUniform("banner",banner_offset);
	knobshader->setUniform("circle",1.f);
	knobshader->setUniform("dark",0);
	for(int i = 0; i < knobcount; i++) {
		if(i == 1 || i == 3) {
			context.extensions.glDisableVertexAttribArray(coord);

			baseshader->use();
			coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
			context.extensions.glActiveTexture(GL_TEXTURE0);
			basetex.bind();
			baseshader->setUniform("basetex",0);
			baseshader->setUniform("dpi",(float)fmax(scaled_dpi,1));
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			if(i == 1) {
				baseshader->setUniform("offset",knobs[1].yoffset*(1-banner_offset)*1.15f);
				baseshader->setUniform("r",knobs[1].r);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
				baseshader->setUniform("offset",knobs[2].yoffset*(1-banner_offset)*1.15f);
				baseshader->setUniform("r",knobs[2].r);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			} else {
				baseshader->setUniform("offset",knobs[4].yoffset*(1-banner_offset)*1.15f);
				baseshader->setUniform("r",knobs[4].r);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			}
			context.extensions.glDisableVertexAttribArray(coord);

			knobshader->use();
			coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			knobshader->setUniform("circle",1.f);
		}
		knobshader->setUniform("thickness",1-knobs[i].linewidth*2.8f);
		knobshader->setUniform("lineheight",knobs[i].lineheight);
		knobshader->setUniform("knobscale",knobs[i].radius*ratio*2.f,knobs[i].radius*2.f);
		knobshader->setUniform("bright",1-knobs[i].yoffset*4);
		knobshader->setUniform("knobpos",knobs[i].x*2-1,1-(knobs[i].y+knobs[i].yoffset)*2);
		knobshader->setUniform("hoverstate",hover==i?1:0);
		if(i == 0) {
			Time computertime = Time::getCurrentTime();
			knobshader->setUniform("knobrot",((computertime.getMinutes()+computertime.getSeconds()/60.f)/60.f)*6.28318530718f);
			knobshader->setUniform("dark",1);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			knobshader->setUniform("circle",0.f);
			knobshader->setUniform("lineheight",.3f);
			knobshader->setUniform("knobrot",((computertime.getHours()+computertime.getMinutes()/60.f)/12.f)*6.28318530718f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			knobshader->setUniform("circle",1.f);
			knobshader->setUniform("dark",0);
		} else {
			knobshader->setUniform("knobrot",(knobs[i].value-.5f)*(i==6?6.2831853072f:5.5f));
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	mainbuffer.releaseAsRenderingTarget();
	feedbackbuffer.makeCurrentRenderingTarget();

	feedbackshader->use();
	coord = context.extensions.glGetAttribLocation(feedbackshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, feedbackbuffer.getTextureID());
	feedbackshader->setUniform("feedbacktex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mainbuffer.getTextureID());
	feedbackshader->setUniform("maintex",1);
	feedbackshader->setUniform("banner",banner_offset);
	float osc = audio_processor.lastosc.get();
	float dampmodamp = audio_processor.lastmodamp.get();
	float time = 0;
	bool outofrange = false;
	if(sync > 0) {
		time = ((30*(knobs[2].value*15+1))/audio_processor.lastbpm.get()-MIN_DLY)/(MAX_DLY-MIN_DLY);
		if(time > 1) {
			outofrange = true;
			time = 1;
		}
	} else time = knobs[2].value*knobs[2].value;
	float time1 = (time*(1-dampmodamp)+osc)*.25f+.005f;
	float time2 = time1;
	float time3 = time1;
	float time4 = time1;
	if(knobs[6].value > .5)
		time2 = (time+osc)*(2-knobs[6].value*2)*(1-dampmodamp)*.25f+.0025f;
	else if(knobs[6].value < .5)
		time1 = (time+osc)*(  knobs[6].value*2)*(1-dampmodamp)*.25f+.0025f;
	if(!postfb) {
		time3 = time1;
		time4 = time2;
	}
	feedbackshader->setUniform("time",time1,time2,time3,time4);//mod + modfreq
	feedbackshader->setUniform("pitch",(knobs[7].value-.5f)*3.13f);
	feedbackshader->setUniform("chew",knobs[8].value*knobs[8].value);
	feedbackshader->setUniform("feedback",knobs[1].value);
	feedbackshader->setUniform("lowpass",knobs[10].value);
	feedbackshader->setUniform("ratio",ratio);
	feedbackshader->setUniform("res",(float)height,(float)width);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	feedbackbuffer.releaseAsRenderingTarget();

	buffershader->use();
	coord = context.extensions.glGetAttribLocation(buffershader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, feedbackbuffer.getTextureID());
	buffershader->setUniform("feedbacktex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mainbuffer.getTextureID());
	buffershader->setUniform("maintex",1);
	buffershader->setUniform("banner",banner_offset);
	buffershader->setUniform("wet",knobs[5].value);
	buffershader->setUniform("reverse",knobs[9].value);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	numbershader->use();
	coord = context.extensions.glGetAttribLocation(numbershader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	numbertex.bind();
	numbershader->setUniform("numbertex",0);
	float l = 16.f/width;
	numbershader->setUniform("banner",banner_offset);
	numbershader->setUniform("size",l,32.f/height);
	numbershader->setUniform("length",1);
	numbershader->setUniform("col",fabs(1-pow(knobs[9].value,.5)),fabs(1-knobs[9].value),fabs(1-pow(knobs[9].value,2.)));
	for(int i = 0; i < pitchnum[0]; i++) {
		numbershader->setUniform("pos",knobs[7].x*2-1+knobs[7].radius*ratio+(i-pitchnum[0]*.5f)*l,
				(knobs[7].y+knobs[7].yoffset)*2-knobs[7].radius*.5f+l);
		numbershader->setUniform("index",pitchnum[i+1],1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	for(int i = 0; i < timenum[0]; i++) {
		numbershader->setUniform("pos",knobs[2].x*2-1+knobs[2].radius*ratio+(i-timenum[0]*.5f)*l,
				(knobs[2].y+knobs[2].yoffset)*2-knobs[2].radius*.6f+l);
		numbershader->setUniform("index",timenum[i+1],1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	if(outofrange) {
		numbershader->setUniform("length",13);
		numbershader->setUniform("col",1,0,0);
		numbershader->setUniform("pos",knobs[2].x*2-1+knobs[2].radius*ratio-6.5f*l,
				(knobs[2].y+knobs[2].yoffset)*2-knobs[2].radius*.6f-l);
		numbershader->setUniform("index",0.f,0.f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void CRMBLAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void CRMBLAudioProcessorEditor::paint (Graphics& g) { }

void CRMBLAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) {
		if(knobs[i].hoverstate < -1) {
			knobs[i].hoverstate++;
		}
	}
	if(held > 0) held--;

	if(websiteht > -.3) websiteht -= .023;

	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;

	dampreadpos = (dampreadpos+1)%32;
	damparray[dampreadpos] = rmsdamp.nextvalue(rms,0);

	update();
}

void CRMBLAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		oversampling = newValue>.5f;
		return;
	} else if(parameterID == "hold") {
		hold = newValue>.5f;
		return;
	} else if(parameterID == "pingpostfeedback") {
		postfb = newValue>.5f;
		return;
	} else if(parameterID == "sync") {
		sync = newValue;
		recalclabels();
		return;
	} else if(parameterID == "time") {
		time = newValue;
		recalclabels();
		return;
	}
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		if(parameterID == "pitch") recalclabels();
		return;
	}
}
void CRMBLAudioProcessorEditor::recalclabels() {
	int num = fabs(round(knobs[7].value*48-24));
	pitchnum[0] = 0;
	if(knobs[7].value != .5) {
		if(knobs[7].value < .5) {
			pitchnum[0]++;
			pitchnum[1] = 10;
		}
		if(num >= 10) {
			pitchnum[0]++;
			pitchnum[pitchnum[0]] = floor(num*.1);
		}
		pitchnum[0]++;
		pitchnum[pitchnum[0]] = fmod(floor(num),10);
		//pitchnum[3] = 11;
		//pitchnum[4] = fmod(floor(num*10),10);
	}
	if(sync <= 0) {
		knobs[2].value = time;
		num = round((pow(knobs[2].value,2)*(MAX_DLY-MIN_DLY)+MIN_DLY)*1000);
		timenum[0] = 0;
		if(num >= 100) {
			if(num >= 1000) {
				timenum[0]++;
				timenum[1] = floor(num*.001);
			}
			timenum[0]++;
			timenum[timenum[0]] = fmod(floor(num*.01),10);
		}
		timenum[0] += 2;
		timenum[timenum[0]-1] = fmod(floor(num*.1),10);
		timenum[timenum[0]] = fmod(floor(num),10);
	} else {
		knobs[2].value = (sync-1)/15.f;
		num = round(knobs[2].value*15+1);
		timenum[0] = 4;
		timenum[1] = 12;
		timenum[2] = 13;
		timenum[3] = 14;
		if(num >= 10) {
			timenum[4] = floor(num*.1);
			timenum[0]++;
		}
		timenum[timenum[0]] = fmod(floor(num),10);
	}
}
void CRMBLAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == -5 && prevhover != -5 && websiteht <= -.3) websiteht = .19f;
	if(prevhover != hover && held == 0) {
		if(hover > -1) knobs[hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
	}
}
void CRMBLAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void CRMBLAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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
		rightclickmenu->addItem(3,"'Randomize",true);
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu("'Scale",*scalemenu);

		String description = "";
		if(hover > -1) {
			description = knobs[hover].description;
		} else {
			if(hover == -2)
				description = "If set to 'POST', the ping-pong knob shortens the initial delay for the left or right channels.\nIf set to 'PRE' this also affects the feedback, creating interesting polyrythmic patterns.";
			else if(hover == -3)
				description = "Turns oversampling on/off";
			else if(hover == -4)
				description = "Disables input and sets feedback to 100%, thus holding the current sound until disabled.";
			else if(hover == -5)
				description = "Trust the process!";
			description = look_n_feel.add_line_breaks(description);
		}
		if(description != "") {
			rightclickmenu->addSeparator();
			rightclickmenu->addItem(-1,"'"+description,false);
		}

		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20) {
				set_ui_scale(result-21);
			} else if(result == 1) { //copy preset
				SystemClipboard::copyTextToClipboard(audio_processor.get_preset(audio_processor.currentpreset));
			} else if(result == 2) { //paste preset
				audio_processor.set_preset(SystemClipboard::getTextFromClipboard(), audio_processor.currentpreset);
			} else if(result == 3) {
				audio_processor.randomize();
			}
		});
		return;
	}

	held = -1;
	initialdrag = hover;
	if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		audio_processor.lerpchanged[knobs[hover].index] = true;
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	} else if(hover == -2) {
		postfb = !postfb;
		audio_processor.apvts.getParameter("pingpostfeedback")->setValueNotifyingHost(postfb?1.f:0.f);
		audio_processor.undo_manager.setCurrentTransactionName(postfb?"Pingpong post feedback":"Pingpong pre feedback");
		audio_processor.undo_manager.beginNewTransaction();
	} else if(hover == -3) {
		oversampling = !oversampling;
		audio_processor.apvts.getParameter("oversampling")->setValueNotifyingHost(oversampling?1.f:0.f);
		audio_processor.undo_manager.setCurrentTransactionName(oversampling?"Turned oversampling on":"Turned oversampling off");
		audio_processor.undo_manager.beginNewTransaction();
	} else if (hover == -4) {
		hold = !hold;
		audio_processor.apvts.getParameter("hold")->setValueNotifyingHost(hold?1.f:0.f);
		audio_processor.undo_manager.setCurrentTransactionName(hold?"Hold":"Unhold");
		audio_processor.undo_manager.beginNewTransaction();
	}
}
void CRMBLAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(hover == -1) return;
	if(initialdrag > -1) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f)*(hover==7?.2:1);
		if(hover == 7 && !event.mods.isCtrlDown()) {
			audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(round((value-valueoffset)*48)/48.f);
		} else if(knobs[hover].id == "time") {
			if(event.mods.isCtrlDown()) {
				audio_processor.apvts.getParameter("sync")->setValueNotifyingHost((fmax(value-valueoffset,0)*15+1)*.0625);
			} else {
				if(sync > 0) audio_processor.apvts.getParameter("sync")->setValueNotifyingHost(0);
				audio_processor.apvts.getParameter("time")->setValueNotifyingHost(value-valueoffset);
			}
		} else {
			audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset);
		}

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	}
}
void CRMBLAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(hover > -1) {
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((knobs[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		if(hover == -5) {
			if(prevhover == -5) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht <= -.3) websiteht = .19f;
		}
		if(hover > -1) knobs[hover].hoverstate = -4;
	}
	held = 1;
}
void CRMBLAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover > -1) {
		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
		if(knobs[hover].id == "time")
			audio_processor.apvts.getParameter("sync")->setValueNotifyingHost(0);
		audio_processor.undo_manager.beginNewTransaction();
	}
}
void CRMBLAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(wheel.deltaY == 0) return;
	if(hover == 7 && !event.mods.isCtrlDown())
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			round(knobs[hover].value*48+(wheel.deltaY>0?1:-1))/48.);
	else if(hover > -1)
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int CRMBLAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if (x >= 9 && x <= 160 && y >= 335 && y <= 456) {
		if(x <= 50 && y <= 378) return -4;
		else if(x <= 119 && y >= 382 && y <= 402) return -3;
		else if(y >= 406) return -5;
		return -1;
	} else if(x >= 336 && x <= 381 && (y-knobs[6].yoffset*height) >= 380 && (y-knobs[6].yoffset*height) <= 421) return -2;
	float r = 0, xx = 0, yy = 0;
	for(int i = knobcount-1; i >= 1; i--) {
		r = knobs[i].radius*height*.5;
		xx = knobs[i].x*width+r-x;
		yy = (knobs[i].y+knobs[i].yoffset)*height-r-y;
		if(sqrt(xx*xx+yy*yy)<= r) return i;
	}
	return -1;
}

LookNFeel::LookNFeel() {
	setColour(PopupMenu::backgroundColourId,Colour::fromFloatRGBA(0.f,0.f,0.f,0.f));
	font = find_font("Tahoma|Helvetica Neue|Helvetica|Roboto");
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	return Font(font,"Regular",18.f*scale);
}
void LookNFeel::drawPopupMenuBackground(Graphics &g, int width, int height) {
	g.setColour(fg);
	g.fillRect(0,0,width,height);
	g.setColour(bg);
	g.fillRect(scale,scale,width-2*scale,height-2*scale);
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
		idealHeight = (int)floor(scale);
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
