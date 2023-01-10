#include "PluginProcessor.h"
#include "PluginEditor.h"

PrismaAudioProcessorEditor::PrismaAudioProcessorEditor(PrismaAudioProcessor& p, pluginpreset states, pluginparams pots)
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	modules[0].name = "None";
	modules[0].colors[6] = .0f;
	modules[0].colors[7] = .0f;
	modules[0].colors[8] = .0f;
	modules[0].colors[9] = .9921875f;
	modules[0].colors[10] = .9921875f;
	modules[0].colors[11] = .94921875f;
	modules[0].hovercutoff = 1.f;
	modules[1].name = "Soft Clip";
	modules[1].colors[0] = .6171875f;
	modules[1].colors[1] = .16796875f;
	modules[1].colors[2] = .26953125f;
	modules[1].colors[3] = .921875f;
	modules[1].colors[4] = .8203125f;
	modules[1].colors[5] = .26171875f;
	modules[1].colors[6] = .2890625f;
	modules[1].colors[7] = .54296875f;
	modules[1].colors[8] = .671875f;
	modules[1].subknobs.push_back(subknob(2));
	modules[2].name = "Hard Clip";
	modules[2].colors[0] = .98828125f;
	modules[2].colors[1] = .37109375f;
	modules[2].colors[2] = .015625f;
	modules[2].colors[3] = .125f;
	modules[2].colors[4] = .1484375f;
	modules[2].colors[5] = .171875f;
	modules[2].colors[6] = .8984375f;
	modules[2].colors[7] = .921875f;
	modules[2].colors[8] = .89453125f;
	modules[2].subknobs.push_back(subknob(3,.8f,.3f));
	modules[3].name = "Heavy";
	modules[3].colors[0] = .99609375f;
	modules[3].colors[1] = .28125f;
	modules[3].colors[2] = .1328125f;
	modules[3].colors[3] = .94140625f;
	modules[3].colors[4] = .86328125f;
	modules[3].colors[5] = .734375f;
	modules[3].colors[6] = .9921875f;
	modules[3].colors[7] = .84375f;
	modules[3].colors[8] = .07421875f;
	modules[3].subknobs.push_back(subknob(4,.8f,.5f));
	modules[4].name = "Asym";
	modules[4].colors[0] = .7421875f;
	modules[4].colors[1] = .546875f;
	modules[4].colors[2] = .38671875f;
	modules[4].colors[3] = .9453125f;
	modules[4].colors[4] = .90625f;
	modules[4].colors[5] = .45703125f;
	modules[4].colors[6] = .2109375f;
	modules[4].colors[7] = .24609375f;
	modules[4].colors[8] = .19921875f;
	modules[4].subknobs.push_back(subknob(5,.8f,.5f));
	modules[5].name = "Rectify";
	modules[5].colors[0] = .25f;
	modules[5].colors[1] = .5390625f;
	modules[5].colors[2] = .23046875f;
	modules[5].colors[3] = .9375f;
	modules[5].colors[4] = .79296875f;
	modules[5].colors[5] = .1875f;
	modules[5].colors[6] = .91796875f;
	modules[5].colors[7] = .3203125f;
	modules[5].colors[8] = .54296875f;
	modules[5].colors[9] = .76953125f;
	modules[5].colors[10] = .08984375f;
	modules[5].colors[11] = .34375f;
	modules[5].subknobs.push_back(subknob(6));
	modules[6].name = "Fold";
	modules[6].colors[0] = .96484375f;
	modules[6].colors[1] = .96484375f;
	modules[6].colors[2] = .96484375f;
	modules[6].colors[3] = .98828125f;
	modules[6].colors[4] = .12109375f;
	modules[6].colors[5] = .01171875f;
	modules[6].colors[6] = .625f;
	modules[6].colors[7] = .63671875f;
	modules[6].colors[8] = .59375f;
	modules[6].subknobs.push_back(subknob(7));
	modules[7].name = "Sine Fold";
	modules[7].colors[0] = .53125f;
	modules[7].colors[1] = .37109375f;
	modules[7].colors[2] = .578125f;
	modules[7].colors[3] = .35546875f;
	modules[7].colors[4] = .84765625f;
	modules[7].colors[5] = .5859375f;
	modules[7].colors[6] = .92578125f;
	modules[7].colors[7] = .19140625f;
	modules[7].colors[8] = .44140625f;
	modules[7].colors[9] = .5078125f;
	modules[7].colors[10] = .09375f;
	modules[7].colors[11] = .23828125f;
	modules[7].clip = 0;
	modules[7].subknobs.push_back(subknob(8));
	modules[8].name = "Zero Cross";
	modules[8].colors[0] = .921875f;
	modules[8].colors[1] = .734375f;
	modules[8].colors[2] = .109375f;
	modules[8].colors[3] = .1171875f;
	modules[8].colors[4] = .0859375f;
	modules[8].colors[5] = .41015625f;
	modules[8].colors[6] = .796875f;
	modules[8].colors[7] = .08984375f;
	modules[8].colors[8] = .16796875f;
	modules[8].clip = 8;
	modules[8].subknobs.push_back(subknob(9,-.1f,.8f));
	modules[8].subknobs.push_back(subknob(10,0,0,22));
	modules[9].name = "Bit Crush";
	modules[9].colors[0] = .11328125f;
	modules[9].colors[1] = .5859375f;
	modules[9].colors[2] = .90625f;
	modules[9].colors[3] = .98828125f;
	modules[9].colors[4] = .9296875f;
	modules[9].colors[5] = .2265625f;
	modules[9].colors[6] = .8515625f;
	modules[9].colors[7] = .234375f;
	modules[9].colors[8] = .4765625f;
	modules[9].subknobs.push_back(subknob(11,.8f,.3f));
	modules[9].subknobs.push_back(subknob(12,-.8f,.75f,25,.3f));
	modules[10].name = "Sample Divide";
	modules[10].colors[0] = .99609375f;
	modules[10].colors[1] = .9375f;
	modules[10].colors[2] = .21875f;
	modules[10].colors[3] = .1640625f;
	modules[10].colors[4] = .6015625f;
	modules[10].colors[5] = .90234375f;
	modules[10].colors[6] = .8828125f;
	modules[10].colors[7] = .15625f;
	modules[10].colors[8] = .1484375f;
	modules[10].subknobs.push_back(subknob(13,.8f,.3f));
	modules[11].name = "DC";
	modules[11].colors[0] = .64453125f;
	modules[11].colors[1] = .60546875f;
	modules[11].colors[2] = .58984375f;
	modules[11].colors[3] = .99609375f;
	modules[11].colors[4] = .99609375f;
	modules[11].colors[5] = .99609375f;
	modules[11].colors[6] = .98046875f;
	modules[11].colors[7] = .91015625f;
	modules[11].colors[8] = .203125f;
	modules[11].defaultval = .5f;
	modules[11].subknobs.push_back(subknob(14,.8f,.3f));
	modules[12].name = "Width";
	modules[12].colors[0] = .21484375f;
	modules[12].colors[1] = .2578125f;
	modules[12].colors[2] = .1953125f;
	modules[12].colors[3] = .90234375f;
	modules[12].colors[4] = .546875f;
	modules[12].colors[5] = .0f;
	modules[12].colors[6] = .609375f;
	modules[12].colors[7] = .828125f;
	modules[12].colors[8] = .03515625f;
	modules[12].defaultval = .5f;
	modules[12].subknobs.push_back(subknob(15,0,0,10));
	modules[12].subknobs.push_back(subknob(16,-.4f,.7f,-5));
	modules[13].name = "Gain";
	modules[13].colors[0] = .328125f;
	modules[13].colors[1] = .22265625f;
	modules[13].colors[2] = .69140625f;
	modules[13].colors[3] = .95703125f;
	modules[13].colors[4] = .3046875f;
	modules[13].colors[5] = .67578125f;
	modules[13].colors[6] = .8203125f;
	modules[13].colors[7] = .87890625f;
	modules[13].colors[8] = .9296875f;
	modules[13].defaultval = .35f;
	modules[13].subknobs.push_back(subknob(17,.8f,.5f));
	modules[14].name = "Pan";
	modules[14].colors[0] = .9375f;
	modules[14].colors[1] = .69140625f;
	modules[14].colors[2] = .41015625f;
	modules[14].colors[3] = .72265625f;
	modules[14].colors[4] = .453125f;
	modules[14].colors[5] = .36328125f;
	modules[14].colors[6] = .97265625f;
	modules[14].colors[7] = .953125f;
	modules[14].colors[8] = .55859375f;
	modules[14].hovercutoff = .3f;
	modules[14].defaultval = .5f;
	modules[14].subknobs.push_back(subknob(18,.8f,.5f));
	modules[15].name = "Low Pass";
	modules[15].colors[0] = .32421875f;
	modules[15].colors[1] = .73046875f;
	modules[15].colors[2] = .703125f;
	modules[15].colors[3] = .98828125f;
	modules[15].colors[4] = .98046875f;
	modules[15].colors[5] = .50390625f;
	modules[15].colors[6] = .94921875f;
	modules[15].colors[7] = .55859375f;
	modules[15].colors[8] = .703125f;
	modules[15].colors[9] = .91796875f;
	modules[15].colors[10] = .24609375f;
	modules[15].colors[11] = .5234375f;
	modules[15].clip = 0;
	modules[15].subknobs.push_back(subknob(19,0,0, 8,0));
	modules[15].subknobs.push_back(subknob(20,0,0,18,0.55f));
	modules[15].subknobs.push_back(subknob(21,0,0,31,0.7f));
	modules[16].name = "High Pass";
	modules[16].colors[0] = .63671875f;
	modules[16].colors[1] = .3828125f;
	modules[16].colors[2] = .73046875f;
	modules[16].colors[3] = .5703125f;
	modules[16].colors[4] = .99609375f;
	modules[16].colors[5] = .78515625f;
	modules[16].colors[6] = .99609375f;
	modules[16].colors[7] = .5703125f;
	modules[16].colors[8] = .65625f;
	modules[16].subknobs.push_back(subknob(22));

	audioProcessor.transition = false;
	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			state[0].modulesvalues[b*4+m].value = states.values[b][m];
			audioProcessor.apvts.addParameterListener("b"+(String)b+"m"+(String)m+"val", this);
			state[0].modulesvalues[b*4+m].id = states.id[b][m];
			audioProcessor.apvts.addParameterListener("b"+(String)b+"m"+(String)m+"id", this);
		}
		if(b >= 1) {
			state[0].crossover[b-1] = states.crossover[b-1];
			crossovertruevalue[b-1] = state[0].crossover[b-1];
			audioProcessor.apvts.addParameterListener("b"+(String)b+"cross", this);
		}
		state[0].gain[b] = states.gain[b];
		audioProcessor.apvts.addParameterListener("b"+(String)b+"gain", this);

		truemute[b] = pots.bands[b].mute;
		buttons[b][0] = pots.bands[b].mute || state[0].gain[b] <= .01f;
		audioProcessor.apvts.addParameterListener("b"+(String)b+"mute", this);

		buttons[b][1] = pots.bands[b].solo;
		audioProcessor.apvts.addParameterListener("b"+(String)b+"solo", this);

		buttons[b][2] = pots.bands[b].bypass;
		audioProcessor.apvts.addParameterListener("b"+(String)b+"bypass", this);
	}
	state[0].wet = states.wet;
	audioProcessor.apvts.addParameterListener("wet", this);
	oversampling = pots.oversampling;
	audioProcessor.apvts.addParameterListener("oversampling", this);
	isb = pots.isb;
	audioProcessor.apvts.addParameterListener("ab", this);

	audioProcessor.calccross(crossovertruevalue,state[0].crossover);
	for(int i = 0; i < 3; i++) crossoverlerp[i] = state[0].crossover[i];

	for(int m = 0; m < 16; m++) {
		for(int e = 0; e < modules[state[0].modulesvalues[m].id].subknobs.size(); e++) {
			state[0].modulesvalues[m].lerps.push_back(state[0].modulesvalues[m].value);
			state[0].modulesvalues[m].lerps.push_back(state[0].modulesvalues[m].value);
		}
	}

	state[1] = state[0];

#ifdef BANNER
	setSize(478,561+21);
	banneroffset = 21.f/getHeight();
#else
	setSize(478,561);
#endif
	setResizable(false, false);
	if((SystemStats::getOperatingSystemType() & SystemStats::OperatingSystemType::Windows) != 0)
		dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->dpi/96.f;
	else
		dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;

	setOpaque(true);
	context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
	context.setRenderer(this);
	context.attachTo(*this);

	for(int i = 0; i < 330; i++) {
		vispoly[i*6  ] = i/329.f;
		vispoly[i*6+3] = i/329.f;
		vispoly[i*6+1] = -.1f;
		vispoly[i*6+4] = -.1f;
		vispoly[i*6+2] = 1.f;
		vispoly[i*6+5] = 1.f;
	}
	vispoly[1980] = 1.f;
	vispoly[1983] = 1.f;
	vispoly[1986] = 0.f;
	vispoly[1989] = 0.f;
	vispoly[1981] = -.1f;
	vispoly[1984] = -.1f;
	vispoly[1987] = -.1f;
	vispoly[1990] = -.1f;
	vispoly[1982] = 0.f;
	vispoly[1985] = 0.f;
	vispoly[1988] = 0.f;
	vispoly[1991] = 0.f;
	for(int i = 332; i < 662; i++) {
		vispoly[i*6  ] = vispoly[(i-332)*6];
		vispoly[i*6+3] = vispoly[(i-332)*6];
		vispoly[i*6+1] = vispoly[(i-332)*6+1];
		vispoly[i*6+4] = vispoly[(i-332)*6+1];
		vispoly[i*6+2] = 1.f;
		vispoly[i*6+5] = 0.f;
	}

	startTimerHz(30);
	audioProcessor.dofft = true;
}
PrismaAudioProcessorEditor::~PrismaAudioProcessorEditor() {
	for(int b = 0; b < 4; b++) {
		for(int m = 0; m < 4; m++) {
			audioProcessor.apvts.removeParameterListener("b"+(String)b+"m"+(String)m+"val", this);
			audioProcessor.apvts.removeParameterListener("b"+(String)b+"m"+(String)m+"id", this);
		}
		if(b >= 1) audioProcessor.apvts.removeParameterListener("b"+(String)b+"cross", this);
		audioProcessor.apvts.removeParameterListener("b"+(String)b+"gain", this);
		audioProcessor.apvts.removeParameterListener("b"+(String)b+"mute", this);
		audioProcessor.apvts.removeParameterListener("b"+(String)b+"solo", this);
		audioProcessor.apvts.removeParameterListener("b"+(String)b+"bypass", this);
	}
	audioProcessor.apvts.removeParameterListener("wet", this);
	audioProcessor.apvts.removeParameterListener("oversampling", this);
	audioProcessor.apvts.removeParameterListener("ab", this);

	audioProcessor.dofft = false;

	stopTimer();
	context.detach();
}

void PrismaAudioProcessorEditor::newOpenGLContextCreated() {
	audioProcessor.logger.init(&context,getWidth(),getHeight());

	compileshader(baseshader,
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 size;
uniform float pos;
out vec2 uv;
out vec2 p;
void main(){
	gl_Position = vec4((aPos*size+vec2(0,pos))*2-1,0,1);
	p = (aPos-vec2(.0230125523,.1532976827))*vec2(1.048245614,1.442159383);
	uv = vec2(aPos.x*texscale.x,1-(1-aPos.y)*texscale.y);
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
in vec2 p;
uniform sampler2D basetex;
uniform vec4 selector;
uniform float preset;
uniform vec4 grayscale;
out vec4 fragColor;
void main(){
	vec2 puv = uv;
	float s = 0;
	if(p.x > 0 && p.x < 1 && p.y > 0 && p.y < 1) {
		if(p.x < .25) s = selector.x;
		else if(p.x < .5) s = selector.y;
		else if(p.x < .75) s = selector.z;
		else s = selector.w;
		puv.x += s*.2384937238;
		puv.x += (mod(p.x*4-preset+1,1)-mod(p.x*4,1))*.2384937238;
	}
	if((1-s) < mod(p.x*4,1)) fragColor = vec4(0,0,0,1);
	else fragColor = texture(basetex,puv);
	if(fragColor.g > 0 && p.x > 0 && p.y < 1) {
		if(((fragColor.b*.8) > fragColor.r || (p.x < .25 && p.y > 0) || (p.x < .25 && p.y < 0 && (fragColor.b*.9) > fragColor.r)) && grayscale.x < 1) fragColor.rgb = fragColor.rgb*grayscale.x+(fragColor.r+fragColor.g+fragColor.b)*vec3(.4846328383,.4846328383,.463644802)*(1-grayscale.x);
		else if((fragColor.g*.8) > fragColor.r && grayscale.y < 1) fragColor.rgb = fragColor.rgb*grayscale.y+(fragColor.r+fragColor.g+fragColor.b)*vec3(.4532214506,.4532214506,.43359375)*(1-grayscale.y);
		else if((fragColor.r*.6) > fragColor.b && fragColor.g > fragColor.b && grayscale.z < 1) fragColor.rgb = fragColor.rgb*grayscale.z+(fragColor.r+fragColor.g+fragColor.b)*vec3(.2523088488,.2523088488,.2413820876)*(1-grayscale.z);
		else if((fragColor.r*.6) > fragColor.g && grayscale.w < 1) fragColor.rgb = fragColor.rgb*grayscale.w+(fragColor.r+fragColor.g+fragColor.b)*vec3(.3307291667,.3307291667,.31640625)*(1-grayscale.w);
	}
})");

	compileshader(decalshader,
//DECAL VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 size;
uniform vec2 pos;
uniform vec2 offset;
uniform float rot;
out vec2 uv;
out vec2 clipuv;
void main(){
	gl_Position = vec4(((vec2(
		(aPos.x-.5)*cos(rot)-(aPos.y-.5)*sin(rot),
		(aPos.x-.5)*sin(rot)+(aPos.y-.5)*cos(rot))+.5)*size+pos)*2-1,0,1);
	uv = vec2((aPos.x+offset.x)*texscale.x,1-(1-aPos.y+offset.y)*texscale.y);
	clipuv = aPos;
})",
//DECAL FRAG
R"(#version 150 core
in vec2 uv;
in vec2 clipuv;
uniform sampler2D tex;
uniform vec4 channels;
uniform vec2 clip;
uniform vec3 color;
uniform float dpi;
out vec4 fragColor;
void main(){
	if(clip.x > clipuv.x || clip.y < clipuv.x) fragColor = vec4(0);
	else {
		vec4 col = max(min((texture(tex,uv)*channels-.5)*dpi+.5,1),0);
		fragColor = vec4(color,col.r+col.g+col.b+col.a);
	}
})");

	compileshader(moduleshader,
//MODULE VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 size;
uniform vec2 pos;
uniform float selector;
uniform vec2 highlight;
uniform int id;
out vec2 uv;
out float ht;
out float truex;
void main(){
	gl_Position = vec4(((aPos*vec2(1-abs(selector),1)-vec2(min(selector,0),0))*size+pos)*2-1,0,1);
	uv = vec2((1-(1-aPos.x)*(1-abs(selector))+min(selector,0)+id)*texscale.x,1-(1-aPos.y)*texscale.y);
	truex = 1-(1-aPos.x)*(1-abs(selector));
	ht = 1-(1-aPos.y)*highlight.y+highlight.x;
})",
//MODULE FRAG
R"(#version 150 core
in vec2 uv;
in float ht;
in float truex;
uniform sampler2D tex;
uniform vec3 bg;
uniform vec3 tx;
uniform vec3 kn;
uniform vec3 htclr;
uniform float hover;
uniform float grayscale;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec4 col = texture(tex,uv);
	if(ht > -100) {
		if(ht < 0 || ht > 1) col.g = 0;
		else {
			col.g = col.r;
			col.r = 0;
		}
		col.b = 0;
	}
	if(truex < hover) {
		col.a = col.g;
		col.g = 0;
	} else col.a = 0;
	col = max(min((col-.5)*dpi+.5,1),0);
	fragColor = vec4(bg*col.r+tx*col.g+kn*col.b+htclr*col.a,1);
	if(grayscale < 1) fragColor.rgb = fragColor.rgb*grayscale+(fragColor.r+fragColor.g+fragColor.b)*vec3(.3307291667,.3307291667,.31640625)*(1-grayscale);
})");

	compileshader(elementshader,
//ELEMENT VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 size;
uniform vec2 pos;
uniform vec2 texscale;
uniform float selector;
uniform vec2 modulepos;
uniform vec2 modulesize;
uniform vec2 moduletexscale;
uniform int moduleid;
uniform int id;
uniform float rot;
out vec2 uv;
out vec2 moduleclip;
out vec2 moduleuv;
void main(){
	gl_Position = vec4((vec2(
		(aPos.x-.5)*cos(rot)-(aPos.y-.5)*sin(rot),
		(aPos.x-.5)*sin(rot)+(aPos.y-.5)*cos(rot))+.5)*size+pos,0,1);
	moduleclip = (gl_Position.xy-modulepos)*modulesize;
	if(moduleid >= 0)
		moduleuv = vec2((moduleclip.x+moduleid)*moduletexscale.x,1-(1-moduleclip.y)*moduletexscale.y);
	moduleclip.x -= selector*1.1875*size.x*modulesize.x;
	gl_Position = vec4((gl_Position.xy-vec2(selector*1.1875*size.x,0))*2-1,0,1);
	uv = vec2((aPos.x+floor(id/6.0))*texscale.x,1-(1-aPos.y+mod(id,6))*texscale.y);
})",
//ELEMENT FRAG
R"(#version 150 core
in vec2 uv;
in vec2 moduleclip;
in vec2 moduleuv;
uniform sampler2D tex;
uniform sampler2D moduletex;
uniform vec3 bg;
uniform vec3 tx;
uniform vec3 kn;
uniform int moduleid;
uniform float grayscale;
uniform float dpi;
out vec4 fragColor;
void main(){
	if(moduleclip.x < 0 || moduleclip.x > 1) fragColor = vec4(0);
	else {
		vec4 col = max(min((texture(tex,uv)-.5)*dpi+.5,1),0);
		fragColor = vec4(bg*col.r+tx*col.g+kn*col.b,col.a);
		if(moduleid >= 0) fragColor *= texture(moduletex,moduleuv).b;
	}
	if(grayscale < 1) fragColor.rgb = fragColor.rgb*grayscale+(fragColor.r+fragColor.g+fragColor.b)*vec3(.3307291667,.3307291667,.31640625)*(1-grayscale);
})");

	compileshader(lidshader,
//LID VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 size;
uniform vec2 pos;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*size+pos)*2-1,0,1);
	uv = aPos;
})",
//LID FRAG
R"(#version 150 core
in vec2 uv;
uniform float open;
uniform vec2 clip;
uniform vec3 col;
out vec4 fragColor;
void main(){
	if(clip.x > uv.x || clip.y < uv.x) fragColor = vec4(0);
	else {
		float w = .2;
		vec2 suv = uv*2-1;
		float d = (1-length(suv))*13;
		if(d < 0) fragColor = vec4(0,0,0,max(1+d-w,0)*.1);
		else {
			vec3 coll = col*(1-(1-(sqrt(d)+(uv.y-.5)*3+sin(uv.x*7+uv.y+2)*.5+cos(cos(uv.x*3983)*7+sin(uv.y*3241)*9+2+sin(uv.x*3481+uv.y*2389)*3)*.7))*.05);
			if(open <= .5 && uv.y <= .5) fragColor = vec4(0);
			else if((open > .5 && uv.y > .5) || open >= 1) fragColor = vec4(coll*max(min(d-w*.5,1),.3),1);
			else {
				vec2 s = vec2(1,1/abs(open*2-1));
				float f = length(suv*s);
				float nd = ((f-1)*f/length(suv*s*s))*17;
				if(open <= .5) {
					if(nd < 0) fragColor = vec4(0,0,0,max(nd+1,0));
					else fragColor = vec4(coll*max(min(nd-w,1)*min(d-w*.5,1),.3),1);
				} else {
					if(nd > 0) fragColor = vec4(0,0,0,max(1-nd+w,0));
					else fragColor = vec4(coll*max(min(-nd,1),.3),1);
				}
			}
		}
	}
})");

	compileshader(logoshader,
//LOGO VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 id;
uniform vec2 size;
uniform float pos;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*size+vec2(0,pos))*2-1,0,1);
	uv = vec2((aPos.x+id.x)*texscale.x,1-(1-aPos.y+id.y)*texscale.y);
})",
//LOGO FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform float offset;
uniform vec2 texscale;
out vec4 fragColor;
void main(){
	fragColor = texture(tex,uv);
	if(fragColor.r > .5) fragColor = vec4(vec3(.7421875),texture(tex,uv+vec2(offset*texscale.x,0)).g);
	else fragColor = vec4(0);
})");

	compileshader(visshader,
//VIS VERT
R"(#version 150 core
in vec3 aPos;
uniform vec2 size;
uniform vec2 pos;
out vec2 uv;
out float opacity;
void main(){
	gl_Position = vec4((aPos.xy*size+pos)*2-1,0,1);
	uv = aPos.xy;
	opacity = aPos.z;
})",
//VIS FRAG
R"(#version 150 core
in vec2 uv;
in float opacity;
uniform vec3 low;
uniform vec3 lowmid;
uniform vec3 highmid;
uniform vec3 high;
uniform vec3 cutoff;
out vec4 fragColor;
void main(){
	if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) fragColor = vec4(0);
	else {
		if(uv.x < cutoff.x) fragColor = vec4(low,1);
		else if(uv.x < cutoff.y) fragColor = vec4(lowmid,1);
		else if(uv.x < cutoff.z) fragColor = vec4(highmid,1);
		else fragColor = vec4(high,1);
		fragColor = vec4(fragColor.rgb*min(min(min(abs(uv.x-cutoff.x),abs(uv.x-cutoff.y)),abs(uv.x-cutoff.z))*250,1),opacity);
	}
})");

	compileshader(visbttnshader,
//VIS BUTTON VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 size;
uniform vec2 pos;
uniform vec2 offset;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*size+pos)*2-1,0,1);
	uv = vec2((aPos.x+offset.x)*texscale.x,1-(1-aPos.y+offset.y)*texscale.y);
})",
//VIS BUTTON FRAG
R"(#version 150 core
in vec2 uv;
uniform vec3 clr;
uniform vec3 hlt;
uniform vec3 btn;
uniform sampler2D tex;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec4 col = max(min((texture(tex,uv)-.5)*dpi+.5,1),0);
	float l = col.r*btn.r+col.g*btn.g+col.b*btn.b;
	fragColor = vec4(clr*(1-l)+hlt*l,col.a);
})");

	basetex.loadImage(ImageCache::getFromMemory(BinaryData::base_png, BinaryData::base_pngSize));
	basetex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	selectortex.loadImage(ImageCache::getFromMemory(BinaryData::selector_png, BinaryData::selector_pngSize));
	selectortex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	modulestex.loadImage(ImageCache::getFromMemory(BinaryData::modules_png, BinaryData::modules_pngSize));
	modulestex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	elementstex.loadImage(ImageCache::getFromMemory(BinaryData::elements_png, BinaryData::elements_pngSize));
	elementstex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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
}
void PrismaAudioProcessorEditor::compileshader(std::unique_ptr<OpenGLShaderProgram> &shader, String vertexshader, String fragmentshader) {
	shader.reset(new OpenGLShaderProgram(context));
	if(!shader->addVertexShader(vertexshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	if(!shader->addFragmentShader(fragmentshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	shader->link();
}
void PrismaAudioProcessorEditor::renderOpenGL() {
	float activecolors[4][2][3] {{
			{.34375f,.328125f,.51953125f},
			{.1953125f,.18359375f,.359375f}
		},{
			{.3359375f,.56640625f,.3828125f},
			{.1484375f,.36328125f,.18359375f}
		},{
			{.94921875f,.9140625f,.44140625f},
			{.84765625f,.734375f,0.f}
		},{
			{.84765625f,.40234375f,.5f},
			{.73828125f,.19140625f,.28515625f}
	}};
	float bypasscolors[4][2][3];
	float graycolors[2][3] = {
		{.578125f,.578125f,.5546875f},
		{.40234375f,.40234375f,.3828125f}
	};
	for(int b =- 0; b < 4; b++) {
		for(int i =- 0; i < 2; i++) {
			for(int c =- 0; c < 3; c++) {
				bypasscolors[b][i][c] = activecolors[b][i][c]*bypassease[b]+graycolors[i][c]*(1-bypassease[b]);
				activecolors[b][i][c] = activecolors[b][i][c]*activeease[b]+graycolors[i][c]*(1-activeease[b]);
			}
		}
	}

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int isdouble = (presettransition>0?2:1);

	//BASE
	baseshader->use();
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	baseshader->setUniform("preset",presettransitionease);
	baseshader->setUniform("grayscale",activeease[0],activeease[1],activeease[2],activeease[3]);
	baseshader->setUniform("selector",selectorease[0],selectorease[1],selectorease[2],selectorease[3]);
	baseshader->setUniform("texscale",478.f/basetex.getWidth(),561.f/basetex.getHeight());
	baseshader->setUniform("size",478.f/getWidth(),561.f/getHeight());
	baseshader->setUniform("pos",banneroffset);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	//MODULE BG
	glBlendFunc(GL_ONE, GL_ONE);
	moduleshader->use();
	coord = context.extensions.glGetAttribLocation(moduleshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	modulestex.bind();
	moduleshader->setUniform("tex",0);
	moduleshader->setUniform("texscale",114.f/modulestex.getWidth(),87.f/modulestex.getHeight());
	moduleshader->setUniform("size",114.f/getWidth(),87.f/getHeight());
	moduleshader->setUniform("highlight",-1000.f,0.f);
	moduleshader->setUniform("dpi",dpi);
	for(int i = 0; i < isdouble; i++) {
		for(int m = 0; m < 16; m++) {
			if(selectorease[(int)floor(m*.25f)] < 1) {
				moduleshader->setUniform("id",state[i].modulesvalues[m].id);
				moduleshader->setUniform("pos",(11.f+114.f*floor(m*.25f))/getWidth(),(368.f-78.f*fmod(m,4))/getHeight()+banneroffset);
				moduleshader->setUniform("hover",(!hoverknob&&hover==m&&hoverselector<0)?modules[state[i].modulesvalues[m].id].hovercutoff:0.f);
				if(fmod(m,4) == 0) moduleshader->setUniform("selector",selectorease[(int)floor(m*.25f)]-presettransitionease+i);
				if(state[i].modulesvalues[m].id == 0) {
					moduleshader->setUniform("bg",bypasscolors[(int)floor(m*.25f)][0][0],bypasscolors[(int)floor(m*.25f)][0][1],bypasscolors[(int)floor(m*.25f)][0][2]);
					moduleshader->setUniform("tx",bypasscolors[(int)floor(m*.25f)][1][0],bypasscolors[(int)floor(m*.25f)][1][1],bypasscolors[(int)floor(m*.25f)][1][2]);
					moduleshader->setUniform("grayscale",1.f);
				} else {
					moduleshader->setUniform("bg",modules[state[i].modulesvalues[m].id].colors[0],modules[state[i].modulesvalues[m].id].colors[1],modules[state[i].modulesvalues[m].id].colors[2]);
					moduleshader->setUniform("tx",modules[state[i].modulesvalues[m].id].colors[3],modules[state[i].modulesvalues[m].id].colors[4],modules[state[i].modulesvalues[m].id].colors[5]);
					moduleshader->setUniform("grayscale",bypassease[(int)floor(m*.25f)]);
				}
				if(modules[state[i].modulesvalues[m].id].clip >= 0) moduleshader->setUniform("kn",0,0,0);
				else moduleshader->setUniform("kn",modules[state[i].modulesvalues[m].id].colors[6],modules[state[i].modulesvalues[m].id].colors[7],modules[state[i].modulesvalues[m].id].colors[8]);
				if(modules[state[i].modulesvalues[m].id].colors[9] >= 0)
					moduleshader->setUniform("htclr",modules[state[i].modulesvalues[m].id].colors[9],modules[state[i].modulesvalues[m].id].colors[10],modules[state[i].modulesvalues[m].id].colors[11]);
				else
					moduleshader->setUniform("htclr",modules[state[i].modulesvalues[m].id].colors[6],modules[state[i].modulesvalues[m].id].colors[7],modules[state[i].modulesvalues[m].id].colors[8]);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			}
		}
	}

	//SELECTOR
	context.extensions.glActiveTexture(GL_TEXTURE0);
	selectortex.bind();
	moduleshader->setUniform("texscale",114.f/selectortex.getWidth(),389.f/selectortex.getHeight());
	moduleshader->setUniform("size",114.f/getWidth(),389.f/getHeight());
	moduleshader->setUniform("kn",0.f,0.f,0.f);
	moduleshader->setUniform("grayscale",1.f);
	for(int b = 0; b < 4; b++) {
		if(selectorease[b] > 0) {
			moduleshader->setUniform("pos",(125.f+114.f*b)/getWidth(),86.f/getHeight()+banneroffset);
			moduleshader->setUniform("selector",selectorease[b]+1);
			if(hoverselector == b)
				moduleshader->setUniform("highlight",hover,17.f);
			else
				moduleshader->setUniform("highlight",1.f,1.f);
			moduleshader->setUniform("bg",bypasscolors[b][0][0],bypasscolors[b][0][1],bypasscolors[b][0][2]);
			moduleshader->setUniform("tx",bypasscolors[b][1][0],bypasscolors[b][1][1],bypasscolors[b][1][2]);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	//MODULE KNOBS
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	elementshader->use();
	coord = context.extensions.glGetAttribLocation(elementshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	elementstex.bind();
	elementshader->setUniform("tex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	modulestex.bind();
	elementshader->setUniform("moduletex",1);
	elementshader->setUniform("texscale",192.f/elementstex.getWidth(),192.f/elementstex.getHeight());
	elementshader->setUniform("size",96.f/getWidth(),96.f/getHeight());
	elementshader->setUniform("dpi",(float)fmax(dpi*.5f,1));
	for(int i = 0; i < isdouble; i++) {
		for(int m = 0; m < 16; m++) {
			if(fmod(m,4) == 0) {
				elementshader->setUniform("selector",selectorease[(int)floor(m*.25f)]-presettransitionease+i);
				elementshader->setUniform("grayscale",bypassease[(int)floor(m*.25f)]);
			}
			if(state[i].modulesvalues[m].id > 0 && selectorease[(int)floor(m*.25f)] < 1) {
				elementshader->setUniform("bg",modules[state[i].modulesvalues[m].id].colors[0],modules[state[i].modulesvalues[m].id].colors[1],modules[state[i].modulesvalues[m].id].colors[2]);
				elementshader->setUniform("tx",modules[state[i].modulesvalues[m].id].colors[3],modules[state[i].modulesvalues[m].id].colors[4],modules[state[i].modulesvalues[m].id].colors[5]);
				elementshader->setUniform("kn",modules[state[i].modulesvalues[m].id].colors[6],modules[state[i].modulesvalues[m].id].colors[7],modules[state[i].modulesvalues[m].id].colors[8]);
				elementshader->setUniform("moduleid",modules[state[i].modulesvalues[m].id].clip);
				elementshader->setUniform("modulepos",(11.f+114.f*floor(m*.25f))/getWidth(),(368.f-78.f*fmod(m,4))/getHeight()+banneroffset);
				elementshader->setUniform("modulesize",getWidth()/114.f,getHeight()/87.f);
				elementshader->setUniform("moduletexscale",114.f/modulestex.getWidth(),87.f/modulestex.getHeight());
				for(int e = 0; e < modules[state[i].modulesvalues[m].id].subknobs.size(); e++) {
					elementshader->setUniform("rot",(.5f-state[i].modulesvalues[m].lerps[e*2])*6.28318531f*modules[state[i].modulesvalues[m].id].subknobs[e].rotspeed);
					elementshader->setUniform("pos",
						(34.f+114.f*floor(m*.25f)+sin((state[i].modulesvalues[m].lerps[e*2+1]-.5f)*6.28318531f*modules[state[i].modulesvalues[m].id].subknobs[e].movespeed)*modules[state[i].modulesvalues[m].id].subknobs[e].moveradius)/getWidth (),
						(365.f-78.f*fmod (m,4   )+cos((state[i].modulesvalues[m].lerps[e*2+1]-.5f)*6.28318531f*modules[state[i].modulesvalues[m].id].subknobs[e].movespeed)*modules[state[i].modulesvalues[m].id].subknobs[e].moveradius)/getHeight()+banneroffset);
					elementshader->setUniform("id",modules[state[i].modulesvalues[m].id].subknobs[e].id);
					glDrawArrays(GL_TRIANGLE_STRIP,0,4);
				}
			}
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	//EYES
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	decalshader->use();
	coord = context.extensions.glGetAttribLocation(decalshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	elementstex.bind();
	decalshader->setUniform("tex",0);
	decalshader->setUniform("texscale",192.f/elementstex.getWidth(),192.f/elementstex.getHeight());
	decalshader->setUniform("size",96.f/getWidth(),96.f/getHeight());
	decalshader->setUniform("channels",0.f,0.f,0.f,1.f);
	decalshader->setUniform("offset",0.f,0.f);
	decalshader->setUniform("rot",0.f);
	decalshader->setUniform("color",0.f,0.f,0.f);
	decalshader->setUniform("dpi",(float)fmax(dpi*.5f,1));
	for(int i = 0; i < isdouble; i++) {
		for(int b = 0; b < 4; b++) {
			if(selectorease[b] < 1) {
				float x = 0;
				if(b == 0) {
					x = (eyelerpx*(1-eyelid*.5f)-6*eyelid*.5f)+(presettransitionease-selectorease[0]-i)*114.f;
					decalshader->setUniform("pos",x/getWidth(),(eyelerpy*(1-eyelid*.5f)+51*eyelid*.5f)/getHeight()+banneroffset);
				} else {
					x = -6.f+114.f*b+sin((state[i].crossover[b-1]-.5f)*.8f*6.28318531f)*8.f+(presettransitionease-selectorease[b]-i)*114.f;
					decalshader->setUniform("pos",x/getWidth(),(59.f+cos((state[i].crossover[b-1]-.5f)*.8f*6.28318531f)*8.f)/getHeight()+banneroffset);
				}
				decalshader->setUniform("clip",(11.f+114.f*b-x)/96.f,(125.f+114.f*b-x)/96.f);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);

				x = 46.f+114.f*b+sin((state[i].gain[b]-.5f)*.8f*6.28318531f)*8.f+(presettransitionease-selectorease[b]-i)*114.f;
				decalshader->setUniform("pos",x/getWidth(),(59.f+cos((state[i].gain[b]-.5f)*.8f*6.28318531f)*8.f)/getHeight()+banneroffset);
				decalshader->setUniform("clip",(11.f+114.f*b-x)/96.f,(125.f+114.f*b-x)/96.f);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			}
		}
	}

	//WET KNOB
	decalshader->setUniform("pos",402.f/getWidth(),-1.f/getHeight()+banneroffset);
	decalshader->setUniform("offset",0.f,1.f);
	decalshader->setUniform("rot",(.5f-(state[0].wet*(1-presettransitionease)+state[1].wet*presettransitionease))*6.28318531f*.8f);
	decalshader->setUniform("clip",0.f,1.f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	//QUALITY
	decalshader->setUniform("pos",351.f/getWidth(),24.f/getHeight()+banneroffset);
	decalshader->setUniform("rot",0.f);
	decalshader->setUniform("texscale",48.f/elementstex.getWidth(),48.f/elementstex.getHeight());
	decalshader->setUniform("size",48.f/getWidth(),48.f/getHeight());
	decalshader->setUniform("offset",14.f,21.f);
	decalshader->setUniform("channels",oversampling?1.f:0.f,oversampling?0.f:1.f,0.f,0.f);
	decalshader->setUniform("dpi",dpi);
	if(hover == -10) decalshader->setUniform("color",activecolors[2][1][0]*.6f,activecolors[2][1][1]*.6f,activecolors[2][1][2]*.6f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	//AB
	decalshader->setUniform("offset",12.f,21.f);
	if(hover == -11) decalshader->setUniform("color",activecolors[1][1][0]*.6f,activecolors[1][1][1]*.6f,activecolors[1][1][2]*.6f);
	else decalshader->setUniform("color",0.f,0.f,0.f);
	decalshader->setUniform("channels",isb?0.f:1.f,isb?1.f:0.f,0.f,0.f);
	decalshader->setUniform("pos",251.f/getWidth(),40.f/getHeight()+banneroffset);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	if(hover == -12) decalshader->setUniform("color",activecolors[1][1][0]*.6f,activecolors[1][1][1]*.6f,activecolors[1][1][2]*.6f);
	else decalshader->setUniform("color",0.f,0.f,0.f);
	decalshader->setUniform("pos",207.f/getWidth(),40.f/getHeight()+banneroffset);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	decalshader->setUniform("channels",isb?1.f:0.f,isb?0.f:1.f,0.f,0.f);
	decalshader->setUniform("pos",225.f/getWidth(),40.f/getHeight()+banneroffset);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	decalshader->setUniform("channels",0.f,0.f,1.f,0.f);
	decalshader->setUniform("pos",216.f/getWidth(),40.f/getHeight()+banneroffset);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	if(eyelid > .1 && selectorease[0] < 1) {
		context.extensions.glDisableVertexAttribArray(coord);

	//BLINKING EYELID
		lidshader->use();
		coord = context.extensions.glGetAttribLocation(lidshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		lidshader->setUniform("size",34.f/getWidth(),34.f/getHeight());
		lidshader->setUniform("open",eyelid<.5f?4*eyelid*eyelid*eyelid:1-powf(-2*eyelid+2,3)*.5f);
		lidshader->setUniform("col",
				activecolors[0][0][0]*.3f+activecolors[0][1][0]*.7f,
				activecolors[0][0][1]*.3f+activecolors[0][1][1]*.7f,
				activecolors[0][0][2]*.3f+activecolors[0][1][2]*.7f);
		for(int i = 0; i < isdouble; i++) {
			float x = (presettransitionease-selectorease[0]-i)*114.f;
			lidshader->setUniform("pos",(25.f+x)/getWidth(),90.f/getHeight()+banneroffset);
			lidshader->setUniform("clip",(-x-14.f)/34.f,(-x-14.f+114.f)/34.f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
		context.extensions.glDisableVertexAttribArray(coord);

	//SELECTOR ANIMATION BORDER
		decalshader->use();
		coord = context.extensions.glGetAttribLocation(decalshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	}
	context.extensions.glActiveTexture(GL_TEXTURE0);
	selectortex.bind();
	decalshader->setUniform("tex",0);
	decalshader->setUniform("texscale",114.f/selectortex.getWidth(),389.f/selectortex.getHeight());
	decalshader->setUniform("size",114.f/getWidth(),389.f/getHeight());
	decalshader->setUniform("channels",0.f,0.f,1.f,0.f);
	decalshader->setUniform("offset",0.f,0.f);
	decalshader->setUniform("color",0.f,0.f,0.f);
	decalshader->setUniform("dpi",dpi);
	for(int b = 0; b < 4; b++) {
		if((selectorease[b] > 0 && selectorease[b] < 1) || presettransition > 0) {
			decalshader->setUniform("pos",(11.f+114.f*b)/getWidth(),86.f/getHeight()+banneroffset);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	//LOGO
	logoshader->use();
	coord = context.extensions.glGetAttribLocation(logoshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	elementstex.bind();
	logoshader->setUniform("tex",0);
	logoshader->setUniform("id",3.f,11.f);
	logoshader->setUniform("size",192.f/getWidth(),96.f/getHeight());
	logoshader->setUniform("pos",banneroffset);
	logoshader->setUniform("texscale",192.f/elementstex.getWidth(),96.f/elementstex.getHeight());
	logoshader->setUniform("offset",websiteht);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	//VIS BG
	visshader->use();
	coord = context.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12, opsquare, GL_DYNAMIC_DRAW);
	visshader->setUniform("size",330.f/getWidth(),52.f/getHeight());
	visshader->setUniform("pos",74.f/getWidth(),486.f/getHeight()+banneroffset);
	visshader->setUniform("low",activecolors[0][0][0],activecolors[0][0][1],activecolors[0][0][2]);
	visshader->setUniform("lowmid",activecolors[1][0][0],activecolors[1][0][1],activecolors[1][0][2]);
	visshader->setUniform("highmid",activecolors[2][0][0],activecolors[2][0][1],activecolors[2][0][2]);
	visshader->setUniform("high",activecolors[3][0][0],activecolors[3][0][1],activecolors[3][0][2]);
	visshader->setUniform("cutoff",crossoverlerp[0],crossoverlerp[1],crossoverlerp[2]);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	//VIS SHAPE
	visshader->setUniform("low",activecolors[0][1][0],activecolors[0][1][1],activecolors[0][1][2]);
	visshader->setUniform("lowmid",activecolors[1][1][0],activecolors[1][1][1],activecolors[1][1][2]);
	visshader->setUniform("highmid",activecolors[2][1][0],activecolors[2][1][1],activecolors[2][1][2]);
	visshader->setUniform("high",activecolors[3][1][0],activecolors[3][1][1],activecolors[3][1][2]);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3972, vispoly, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP,0,1324);
	context.extensions.glDisableVertexAttribArray(coord);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	//VIS BUTTONS
	if(hover >= -20 && hover <= -17) {
		float left=74;
		float right=404;
		if(hover > -20) left = crossoverlerp[hover+19]*330+74;
		if(hover < -17) right = crossoverlerp[hover+20]*330+74;
		if((right-left) >= 17) {
			int a = 0;
			if((right-left) <= 47) a = round(left+2.5f);
			else a = round(left+(right-left)*.5f-22.5f);

			visbttnshader->use();
			coord = context.extensions.glGetAttribLocation(visbttnshader->getProgramID(),"aPos");
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			context.extensions.glActiveTexture(GL_TEXTURE0);
			elementstex.bind();
			visbttnshader->setUniform("tex",0);
			visbttnshader->setUniform("offset",12.f,20.f);
			visbttnshader->setUniform("texscale",48.f/elementstex.getWidth(),48.f/elementstex.getHeight());
			visbttnshader->setUniform("size",48.f/getWidth(),48.f/getHeight());
			visbttnshader->setUniform("dpi",dpi);
			for(int i = 0; i < 3; i++) {
				bool ison = buttons[hover+20][i];
				if(isdown && hoverbutton == i) ison = !ison;
				visbttnshader->setUniform(ison?"hlt":"clr",activecolors[hover+20][1][0],activecolors[hover+20][1][1],activecolors[hover+20][1][2]);
				visbttnshader->setUniform(ison?"clr":"hlt",.9921875f,.9921875f,.94921875f);
				if(right-left <= 47)
					visbttnshader->setUniform("pos",((float)a)/getWidth(),(522.f-15*i)/getHeight()+banneroffset);
				else
					visbttnshader->setUniform("pos",((float)a+15*i)/getWidth(),522.f/getHeight()+banneroffset);
				visbttnshader->setUniform("btn",i==0?1.f:0.f,i==1?1.f:0.f,i==2?1.f:0.f);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			}
			context.extensions.glDisableVertexAttribArray(coord);
		}
	}

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
void PrismaAudioProcessorEditor::openGLContextClosing() {
	baseshader->release();
	decalshader->release();
	moduleshader->release();
	elementshader->release();
	lidshader->release();
	logoshader->release();
	visshader->release();
	visbttnshader->release();

	basetex.release();
	selectortex.release();
	modulestex.release();
	elementstex.release();

#ifdef BANNER
	bannershader->release();
	bannertex.release();
#endif

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}
void PrismaAudioProcessorEditor::paint (Graphics& g) { }

void PrismaAudioProcessorEditor::timerCallback() {
	if(websiteht > -.75521f) websiteht -= .07f;

	bool soloed = false;
	for(int b = 0; b < 4; b++) {
		if(buttons[b][1]) {
			soloed = true;
			break;
		}
	}
	if(soloed) {
		for(int b = 0; b < 4; b++) {
			activelerp[b] = fmin(fmax(activelerp[b]+(buttons[b][1]?.15f:-.15f),0),1);
			bypasslerp[b] = fmin(fmax(bypasslerp[b]+((buttons[b][1]&&!buttons[b][2])?.15f:-.15f),0),1);
		}
	} else {
		for(int b = 0; b < 4; b++) {
			activelerp[b] = fmin(fmax(activelerp[b]+(buttons[b][0]?-.15f:.15f),0),1);
			bypasslerp[b] = fmin(fmax(bypasslerp[b]+((buttons[b][0]||buttons[b][2])?-.15f:.15f),0),1);
		}
	}
	for(int b = 0; b < 4; b++) {
		activeease[b] = activelerp[b]<.5f?2*activelerp[b]*activelerp[b]:1-pow(-2*activelerp[b]+2,2)*.5f;
		bypassease[b] = bypasslerp[b]<.5f?2*bypasslerp[b]*bypasslerp[b]:1-pow(-2*bypasslerp[b]+2,2)*.5f;
	}

	if(audioProcessor.transition) {
		state[1] = state[0];
		presettransition = 1-presettransition;
		presettransitionease = presettransition<.5f?4*presettransition*presettransition*presettransition:1-powf(-2*presettransition+2,3)*.5f;
		audioProcessor.transition = false;
	}

	if(presettransition > 0) {
		presettransition = fmax(presettransition-.06f,0);
		presettransitionease = presettransition<.5f?4*presettransition*presettransition*presettransition:1-powf(-2*presettransition+2,3)*.5f;
		for(int b = 0; b < 3; b++) crossoverlerp[b] = state[0].crossover[b]*(1-presettransitionease)+state[1].crossover[b]*presettransitionease;
	}

	for(int b = 0; b < 4; b++) {
		if(selectorlerp[b] != (selectorstate[b]?1:0)) {
			selectorlerp[b] = fmax(fmin(selectorlerp[b]+(selectorstate[b]?.06f:-.06f),1),0);
			selectorease[b] = selectorlerp[b]<.5f?4*selectorlerp[b]*selectorlerp[b]*selectorlerp[b]:1-powf(-2*selectorlerp[b]+2,3)*.5f;
			if(!out) recalchover(lastx,lasty);
		}
	}

	for(int i = 0; i < (presettransition>0?2:1); i++) {
		for(int m = 0; m < 16; m++) {
			for(int e = 0; e < modules[state[i].modulesvalues[m].id].subknobs.size(); e++) {
				state[i].modulesvalues[m].lerps[e*2  ] = state[i].modulesvalues[m].lerps[e*2  ]*modules[state[i].modulesvalues[m].id].subknobs[e].lerprot +state[i].modulesvalues[m].value*(1-modules[state[i].modulesvalues[m].id].subknobs[e].lerprot );
				state[i].modulesvalues[m].lerps[e*2+1] = state[i].modulesvalues[m].lerps[e*2+1]*modules[state[i].modulesvalues[m].id].subknobs[e].lerpmove+state[i].modulesvalues[m].value*(1-modules[state[i].modulesvalues[m].id].subknobs[e].lerpmove);
			}
		}
	}

	if(--nextblink <= 0) {
		if(out) {
			nextblink = random.nextFloat()*200+100;
			float angle = random.nextFloat()*6.2831853072f;
			eyex = -6.f+sin(angle)*8.f;
			eyey = 59.f+cos(angle)*8.f;
			eyelidup = eyelidup || random.nextBool();
		} else {
			nextblink = random.nextFloat()*450+150;
			eyelidup = true;
		}
	}
	if(eyelidup) {
		if(clickedeye) eyelid = fmin(eyelid*.3f+.7f,1);
		else eyelid = fmin(eyelid+.4f,1);
	} else eyelid *= eyeclickcooldown>-30?.9f:.8f;
	if(eyelid >= 1 && eyeclickcooldown < 0) eyelidup = false;
	eyelidcooldown--;
	if(!clickedeye) eyeclickcooldown--;

	eyelerpx = eyelerpx*.7f+eyex*.3f;
	eyelerpy = eyelerpy*.7f+eyey*.3f;

	fftdelta++;
	bool redraw = false;
	if(audioProcessor.nextFFTBlockReady.get()) {
		redraw = true;
		audioProcessor.drawNextFrameOfSpectrum(fftdelta);
		for(int i = 0; i < 330; i++) {
			vispoly[i*6+1] = audioProcessor.scopeData[i];
			if(vispoly[i*6+1] <= .001f) vispoly[i*6+1] = -.1f;
		}
		audioProcessor.nextFFTBlockReady = false;
		fftdelta = 0;
	} else if(fftdelta > 10) {
		redraw = true;
		for(int i = 0; i < 330; i++) {
			if(vispoly[i*6+1] > .001f)
				vispoly[i*6+1] = vispoly[i*6+1]*.95f;
			else vispoly[i*6+1] = -.1f;
		}
	}
	if(redraw) for(int i = 0; i < 330; i++) {
		vispoly[(i+332)*6+1] = vispoly[i*6+1];
		double angle1 = std::atan2(vispoly[i*6+1]-vispoly[i*6-5], 1/329.f);
		double angle2 = std::atan2(vispoly[i*6+1]-vispoly[i*6+7],-1/329.f);
		if(i == 0) angle1 = angle2;
		else if(i == 329) angle2 = angle1;
		while((angle1-angle2)<(-1.5707963268))angle1+=3.1415926535*2;
		while((angle1-angle2)>( 1.5707963268))angle1-=3.1415926535*2;
		double angle = (angle1+angle2)*.5;
		vispoly[(i+332)*6+3] = vispoly[i*6  ]+cos(angle)/(330.f*.8f*dpi);
		vispoly[(i+332)*6+4] = vispoly[i*6+1]+sin(angle)/(52.f*.8f*dpi);
		if(i > 0) vispoly[(i+332)*6+3] = fmax(vispoly[(i+332)*6+3],vispoly[(i+332)*6-3]);
	}

#ifdef BANNER
	bannerx = fmod(bannerx+.0005f,1.f);
#endif

	context.triggerRepaint();
}

void PrismaAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(audioProcessor.transition) {
		state[1] = state[0];
		presettransition = 1-presettransition;
		presettransitionease = presettransition<.5f?4*presettransition*presettransition*presettransition:1-powf(-2*presettransition+2,3)*.5f;
		audioProcessor.transition = false;
	}
	if(parameterID == "ab") {
		isb = newValue > .5;
		return;
	}
	if(parameterID == "oversampling") {
		oversampling = newValue > .5;
		return;
	}
	if(parameterID == "wet") {
		state[0].wet = newValue;
		return;
	}
	int b = std::stoi(std::string(1,parameterID.toStdString()[1]));
	if(parameterID.endsWith("cross")) {
		crossovertruevalue[b-1] = newValue;
		audioProcessor.calccross(crossovertruevalue,state[0].crossover);

		for(int i = 0; i < 3; i++)
			crossoverlerp[i] = state[0].crossover[i]*(1-presettransitionease)+state[1].crossover[i]*presettransitionease;
		return;
	}
	if(parameterID.endsWith("gain")) {
		buttons[b][0] = truemute[b] || newValue <= .01f;
		state[0].gain[b] = newValue;
		return;
	}
	if(parameterID.endsWith("mute")) {
		truemute[b] = newValue > .5;
		buttons[b][0] = newValue > .5 || state[0].gain[b] <= .01f;
		return;
	}
	if(parameterID.endsWith("solo")) {
		buttons[b][1] = newValue > .5;
		return;
	}
	if(parameterID.endsWith("bypass")) {
		buttons[b][2] = newValue > .5;
		return;
	}
	int m = std::stoi(std::string(1,parameterID.toStdString()[3]));
	if(parameterID.endsWith("val")) {
		state[0].modulesvalues[b*4+m].value = newValue;
		return;
	}
	if(parameterID.endsWith("id")) {
		state[0].modulesvalues[b*4+m].id = (int)newValue;
		state[0].modulesvalues[b*4+m].lerps.resize(modules[(int)newValue].subknobs.size()*2);
		for(int e = 0; e < modules[(int)newValue].subknobs.size(); e++) {
			state[0].modulesvalues[b*4+m].lerps[e*2  ] = state[0].modulesvalues[b*4+m].value;
			state[0].modulesvalues[b*4+m].lerps[e*2+1] = state[0].modulesvalues[b*4+m].value;
		}
	}
}
void PrismaAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	out = false;
	lastx = event.x;
	lasty = event.y;

	int prevhover = hover;
	recalchover(event.x,event.y);
	if(hover == -13 && prevhover != -13 && websiteht <= -.75521f) websiteht = .484375f;
}
void PrismaAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	out = true;
	prevout = true;
	nextblink = fmin(nextblink,random.nextFloat()*200+100);
	hovereye = false;
	clickedeye = false;
	hoverknob = false;
	hoverselector = -1;
	hoverbutton = -1;
	hover = -1;
}
void PrismaAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	initialdrag = hover;
	isdown = true;
	if(hovereye) {
		eyelidup = true;
		clickedeye = true;
		eyeclickcooldown = random.nextFloat()*150+50;
	} else if(hoverknob) {
		valueoffset = 0;
		audioProcessor.undoManager.beginNewTransaction();
		if(hover >= 0) {
			initialvalue = state[0].modulesvalues[hover].value;
			audioProcessor.apvts.getParameter("b"+(String)floor(hover*.25f)+"m"+(String)fmod(hover,4)+"val")->beginChangeGesture();
		} else if((hover >= -4 && hover < -1) || (hover >= -16 && hover <= -14)) {
			int b = hover>-10?(hover+4):(hover+16);
			initialvalue = state[0].crossover[b];
			audioProcessor.apvts.getParameter("b"+(String)(b+1)+"cross")->beginChangeGesture();
		} else if(hover >= -8 && hover <= -5) {
			initialvalue = state[0].gain[hover+8];
			audioProcessor.apvts.getParameter("b"+(String)(hover+8)+"gain")->beginChangeGesture();
		} else if(hover == -9) {
			initialvalue = state[0].wet;
			audioProcessor.apvts.getParameter("wet")->beginChangeGesture();
		}
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void PrismaAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(initialdrag == -1) return;
	if(hoverknob) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		if(hover >= 0) {
			audioProcessor.apvts.getParameter("b"+(String)floor(hover*.25f)+"m"+(String)fmod(hover,4)+"val")->setValueNotifyingHost(value-valueoffset);
		} else if((hover >= -4 && hover < -1) || (hover >= -16 && hover <= -14)) {
			int b = 0;
			if(hover > -10) {
				b = hover+4;
			} else {
				value = initialvalue+event.getDistanceFromDragStartX()*(finemode?.0005f:.005f);
				b = hover+16;
			}
			audioProcessor.apvts.getParameter("b"+(String)(b+1)+"cross")->setValueNotifyingHost(fmin(fmax(value-valueoffset,b>0?(state[0].crossover[b-1]+.02f):.02f),b<2?(state[0].crossover[b+1]-.02f):.98f));
		} else if(hover >= -8 && hover <= -5) {
			audioProcessor.apvts.getParameter("b"+(String)(hover+8)+"gain")->setValueNotifyingHost(value-valueoffset);
		} else if(hover == -9) {
			audioProcessor.apvts.getParameter("wet")->setValueNotifyingHost(value-valueoffset);
		}

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	} else if(initialdrag == -13) {
		int prevhover = hover;
		recalchover(event.x,event.y);
		hovereye = false;
		hoverknob = false;
		hoverselector = -1;
		hoverbutton = -1;
		if(hover != initialdrag) hover = -1;
		if(hover == -13 && prevhover != -13 && websiteht < -.75521f) websiteht = .484375f;
	}
}
void PrismaAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	isdown = false;
	if(hoverknob) {
		if(hover >= 0) {
			audioProcessor.undoManager.setCurrentTransactionName(
				(String)((state[0].modulesvalues[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += modules[state[0].modulesvalues[hover].id].name);
			audioProcessor.apvts.getParameter("b"+(String)floor(hover*.25f)+"m"+(String)fmod(hover,4)+"val")->endChangeGesture();
		} else if((hover >= -4 && hover < -1) || (hover >= -16 && hover <= -14)) {
			int b = hover>-10?(hover+4):(hover+16);
			audioProcessor.undoManager.setCurrentTransactionName(
				(String)((state[0].crossover[b] - initialvalue) >= 0 ? "Increased crossover " : "Decreased crossover ") += (String)(b+1));
			audioProcessor.apvts.getParameter("b"+(String)(b+1)+"cross")->endChangeGesture();
		} else if(hover >= -8 && hover <= -5) {
			audioProcessor.undoManager.setCurrentTransactionName(
				(String)((state[0].crossover[hover+8] - initialvalue) >= 0 ? "Increased gain for band " : "Decreased gain for band ") += (String)(hover+8));
			audioProcessor.apvts.getParameter("b"+(String)(hover+8)+"gain")->endChangeGesture();
		} else if(hover == -9) {
			audioProcessor.undoManager.setCurrentTransactionName(
				(state[0].wet - initialvalue) >= 0 ? "Increased Wet" : "Decreased Wet");
			audioProcessor.apvts.getParameter("wet")->endChangeGesture();
		}
		audioProcessor.undoManager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		int prevhover = hover;
		int prevhoverbutton = hoverbutton;
		recalchover(event.x,event.y);
		if(hover == prevhover && hoverbutton == prevhoverbutton) {
			if(hover >= 0) {
				if(hoverselector >= 0) {
					if(state[0].modulesvalues[selectorid].id != hover) audioProcessor.apvts.getParameter("b"+(String)floor(selectorid*.25f)+"m"+(String)fmod(selectorid,4)+"val")->setValueNotifyingHost(modules[hover].defaultval);
					audioProcessor.apvts.getParameter("b"+(String)floor(selectorid*.25f)+"m"+(String)fmod(selectorid,4)+"id")->setValueNotifyingHost(hover/16.f);
					for(int i = 0; i < 4; i++) selectorstate[i] = false;
				} else {
					selectorid = hover;
					int b = (int)floor(hover*.25f);
					for(int i = 0; i < 4; i++) selectorstate[i] = i==b;
				}
			} else if(hover == -10) {
				oversampling = !oversampling;
				audioProcessor.apvts.getParameter("oversampling")->setValueNotifyingHost(oversampling?1.f:0.f);
				audioProcessor.undoManager.setCurrentTransactionName(oversampling?"Turned oversampling on":"Turned oversampling off");
				audioProcessor.undoManager.beginNewTransaction();
			} else if(hover == -11) {
				isb = !isb;
				audioProcessor.undoManager.beginNewTransaction();
				audioProcessor.undoManager.setCurrentTransactionName(isb?"Moved to preset B":"Moved to preset A");
				audioProcessor.apvts.getParameter("ab")->setValueNotifyingHost(isb?1.f:0.f);
			} else if(hover == -12) {
				audioProcessor.undoManager.beginNewTransaction();
				audioProcessor.undoManager.setCurrentTransactionName(isb?"Copied preset B to A":"Copied preset A to B");
				audioProcessor.copypreset(isb);
			} else if(hover == -13) {
				URL("https://vst.unplug.red/").launchInDefaultBrowser();
			} else if(hover >= -20 && hover <= -17 && hoverbutton > -1) {
				if(hoverbutton == 0) {
					bool currentmute = buttons[hover+20][0];
					audioProcessor.undoManager.setCurrentTransactionName((String)(currentmute?"Unmuted band ":"Muted band ") += (String)(hover+20));
					if(currentmute && state[0].gain[hover+20] <= .01f)
						audioProcessor.apvts.getParameter("b"+(String)(hover+20)+"gain")->setValueNotifyingHost(.5f);
					audioProcessor.apvts.getParameter("b"+(String)(hover+20)+"mute")->setValueNotifyingHost(!currentmute);
					audioProcessor.undoManager.beginNewTransaction();
				} else if(hoverbutton == 1) {
					audioProcessor.undoManager.setCurrentTransactionName((String)(buttons[hover+20][1]?"Unsoloed band ":"Soloed band ") += (String)(hover+20));
					audioProcessor.apvts.getParameter("b"+(String)(hover+20)+"solo")->setValueNotifyingHost(!buttons[hover+20][1]);
					audioProcessor.undoManager.beginNewTransaction();
				} else if(hoverbutton == 2) {
					audioProcessor.undoManager.setCurrentTransactionName((String)(buttons[hover+20][2]?"Unbypassed band ":"Bypassed band ") += (String)(hover+20));
					audioProcessor.apvts.getParameter("b"+(String)(hover+20)+"bypass")->setValueNotifyingHost(!buttons[hover+20][2]);
					audioProcessor.undoManager.beginNewTransaction();
				}
			} else for(int i = 0; i < 4; i++) selectorstate[i] = false;
		} else if(hover == -13) websiteht = .484375f;
	}
}
void PrismaAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(!hoverknob) return;
	if(hover >= 0) {
		audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += modules[state[0].modulesvalues[hover].id].name);
		audioProcessor.apvts.getParameter("b"+(String)floor(hover*.25f)+"m"+(String)fmod(hover,4)+"val")->setValueNotifyingHost(modules[state[0].modulesvalues[hover].id].defaultval);
	} else if((hover >= -4 && hover < -1) || (hover >= -16 && hover <= -14)) {
		int b = hover>-10?(hover+4):(hover+16);
		audioProcessor.undoManager.setCurrentTransactionName((String)"Reset crossover " += (String)(b+1));
		audioProcessor.apvts.getParameter("b"+(String)(b+1)+"cross")->setValueNotifyingHost(fmin(fmax((b+1)*.25f,b>0?(state[0].crossover[b-1]+.02f):.02f),b<2?(state[0].crossover[b+1]-.02f):.98f));
	} else if(hover >= -8 && hover <= -5) {
		audioProcessor.undoManager.setCurrentTransactionName((String)"Reset gain for band " += (String)(hover+8));
		audioProcessor.apvts.getParameter("b"+(String)(hover+8)+"gain")->setValueNotifyingHost(.5f);
	} else if(hover == -9) {
		audioProcessor.undoManager.setCurrentTransactionName("Reset Wet");
		audioProcessor.apvts.getParameter("wet")->setValueNotifyingHost(1.f);
	}
	audioProcessor.undoManager.beginNewTransaction();
}
void PrismaAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(!hoverknob) return;
	if(hover >= 0) {
		audioProcessor.apvts.getParameter("b"+(String)floor(hover*.25f)+"m"+(String)fmod(hover,4)+"val")->setValueNotifyingHost(
			state[0].modulesvalues[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	} else if(hover >= -4 && hover < -1) {
		audioProcessor.apvts.getParameter("b"+(String)(hover+5)+"cross")->setValueNotifyingHost(fmin(fmax(
			state[0].crossover[hover+4]+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f)
			,hover>-4?(state[0].crossover[hover+3]+.02f):.02f),hover<-2?(state[0].crossover[hover+5]-.02f):.98f));
	} else if(hover >= -8 && hover <= -5) {
		audioProcessor.apvts.getParameter("b"+(String)(hover+8)+"gain")->setValueNotifyingHost(
			state[0].gain[hover+8]+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	} else if(hover == -9) {
		audioProcessor.apvts.getParameter("wet")->setValueNotifyingHost(
			state[0].wet+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	}
}
void PrismaAudioProcessorEditor::recalchover(float x, float y) {
	//eye position
	if(out) {
		hovereye = false;
		clickedeye = false;
	} else {
		float angle = atan2(x-42+selectorease[0]*114.f,454-y);
		float amp = powf((x-42+selectorease[0]*114.f)*.2f,2)+powf((y-454)*.2f,2);
		hovereye = amp <= 13;
		clickedeye = clickedeye && hovereye;
		amp = fmin(amp,8);
		eyex = -6.f+sin(angle)*amp;
		eyey = 59.f+cos(angle)*amp;
		if(fabs(eyex-eyelerpx)+fabs(eyey-eyelerpy) > 14.f && eyelidcooldown <= 0) {
			eyelidup = !prevout;
			eyelidcooldown = (int)floor(random.nextFloat()*100);
			nextblink = random.nextFloat()*450+150;
		}
	}
	prevout = out;

	hoverselector = -1;
	hoverbutton = -1;
	hoverknob = false;
	if(x <= 11 || y <= 86 || x >= 467 || y >= 475) {
		if(y >= 23 && y <= 75) {
			hover = -1;
			//cross
			for(int b = 0; b < 3; b++) {
				float cx = state[0].crossover[b]*330+74;
				if(x >= (cx-3) && x <= (cx+3)) {
					hover = b-16;
					hoverknob = true;
					return;
				}
			}
			if(hover == -1 && x >= 74 && x <= 404) {
				hover = -17;
				for(int b = 0; b < 3; b++) {
					if(x < state[0].crossover[b]*330+74) {
						hover = b-20;
						break;
					}
				}

				//solo bypass mute
				float left=74;
				float right=404;
				if(hover > -20) left = state[0].crossover[hover+19]*330+74;
				if(hover < -17) right = state[0].crossover[hover+20]*330+74;
				if((right-left) < 17) return;
				if((right-left) <= 47) {
					if(x < round(left+2.5f) || x > round(left+17.5f)) return;
					for(int i = 0; i < 3; i++) {
						if(y >= (24+15*i) && y <= (39+15*i)) {
							hoverbutton = i;
							return;
						}
					}
				} else {
					if(y < 24 || y > 39) return;
					float a = round(left+(right-left)*.5f-22.5f);
					for(int i = 0; i < 3; i++) {
						if(x >= (a+15*i) && x <= (a+15+15*i)) {
							hoverbutton = i;
							return;
						}
					}
				}
			}
			return;
		}
		//wet
		if((powf(450-x,2)+powf(514-y,2)) <= 484) {
			hoverknob = true;
			hover = -9;
			return;
		}
		//oversampling
		if(x >= 351 && y >= 525 && x <= 380 && y <= 536) {
			hover = -10;
			return;
		}
		//a
		if(x >= 251 && y >= 509 && x <= 262 && y <= 520) {
			hover = -11;
			return;
		}
		//a->b
		if(x >= 207 && y >= 509 && x <= 236 && y <= 520) {
			hover = -12;
			return;
		}
		//website
		if(x >= 3 && y >= 514 && x <= 149 && y <= 558) {
			hover = -13;
			return;
		}
		hover = -1;
		return;
	}
	x -= 11;
	//selector
	if((1-selectorease[(int)floor(x/114.f)]) < fmod(x/114.f,1)) {
		hoverselector = (int)floor(x/114.f);
		hover = floor((y-86)*.0437017995f);
		return;
	}
	if(y >= 418) {
		for(int b = 0; b < 4; b++) {
			//cross
			if(b >= 1 && (powf(31+b*114-x,2)+powf(454-y,2)) <= 324) {
				hoverknob = true;
				hover = -5+b;
				return;
			//gain
			} else if((powf(83+b*114-x,2)+powf(454-y,2)) <= 324) {
				hoverknob = true;
				hover = -8+b;
				return;
			}
		}
		hover = -1;
		return;
	}
	if(y <= 106) {
		hover = -1;
		return;
	}
	//module
	hover = floor(x/114.f)*4+floor((y-106)/78.f);
	hoverknob = state[0].modulesvalues[hover].id != 0 && fmod((x/114.f),1) > .3;
}
