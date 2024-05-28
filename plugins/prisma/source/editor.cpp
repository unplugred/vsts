#include "processor.h"
#include "editor.h"

PrismaAudioProcessorEditor::PrismaAudioProcessorEditor(PrismaAudioProcessor& p, pluginpreset states, pluginparams pots) : audio_processor(p), plugmachine_gui(p, 478, 249+78*states.modulecount) {
	modules[0].name = "None";
	modules[0].description = "";
	modules[0].colors[6] = .0f;
	modules[0].colors[7] = .0f;
	modules[0].colors[8] = .0f;
	modules[0].colors[9] = .9921875f;
	modules[0].colors[10] = .9921875f;
	modules[0].colors[11] = .94921875f;
	modules[0].hovercutoff = 1.f;
	modules[1].name = "Soft Clip";
	modules[1].description = "SOFT CLIP is a gentle saturation that adds odd harmonics and is effective at reducing dynamic range.";
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
	modules[2].description = "HARD CLIP is a aggressive distortion with a digital sound that adds lots of odd harmonics by chopping the signal below and above a certain threshold.";
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
	modules[3].description = "HEAVY is an aggressive distortion that significantly increases the noise floor of the signal and adds odd harmonics.";
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
	modules[4].description = "ASYM is a distortion that operates differently on the positive and negative phases, creating even and odd harmonics.";
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
	modules[5].description = "RECTIFY is a distortion that turns the negative phase into positive phase, and as a results adds even harmonics.\nIt is very spicy when added before another type of distortion.";
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
	modules[6].description = "FOLD is an aggressive digital distortion which adds odd harmonics by folding the signal.";
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
	modules[7].description = "SINE FOLD is a gentler version of fold.";
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
	modules[8].description = "ZERO CROSS is a gentle distoriton which also can act as a gate, creating choppy sounds while adding odd harmonics.\nAlso exists as an independent plugin under the name PNCH.";
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
	modules[9].description = "BIT CRUSH is a module that quantizes the signal, giving it a noisy and digital sound while adding inharmonics.";
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
	modules[10].description = "SAMPLE DIVIDE is a module that downsamples the signal without an anti-aliasing filter, which adds inharmonics and aliasing, giving it a digital sound.";
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
	modules[11].description = "DC is a module that offsets the signal.\nAlone it wont do much, but when applied before a distortion it adds even harmonics.\nThe DC offset is removed at the end of the module chain.";
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
	modules[12].description = "WIDTH is a module which controls the gain of the side-channel, making the signal wider or narrower.\nIt has no effect on signals which are already mono.";
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
	modules[13].description = "GAIN is a module that can increase or decrease the level without adding harmonics.";
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
	modules[14].description = "PAN is a module which can make the signal more prominant in the left or right channel, using a 4.5dB pan law.";
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
	modules[15].description = "LOW PASS is a 12dB/Octave filter that removes high frequencies.\nCan be used to tame a heavier distortion and give it a more interesting tone.";
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
	modules[16].description = "HIGH PASS is a 12dB/Octave filter that removes low frequencies.";
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

	for(int i = 0; i < (MODULE_COUNT+1); ++i)
		modules[i].description = look_n_feel.add_line_breaks(modules[i].description);

	audio_processor.transition = false;
	for(int b = 0; b < BAND_COUNT; ++b) {
		activelerp[b] = 1.f;
		bypasslerp[b] = 1.f;
		bypassease[b] = 1.f;
		selectorlerp[b] = 0;
		selectorstate[b] = false;
		selectorscroll[b] = 0;
		selectorscrolllerp[b] = 0;

		for(int m = 0; m < MAX_MOD; ++m) {
			state[0].modulesvalues[b*MAX_MOD+m].value = states.values[b][m];
			add_listener("b"+(String)b+"m"+(String)m+"val");
			state[0].modulesvalues[b*MAX_MOD+m].id = states.id[b][m];
			add_listener("b"+(String)b+"m"+(String)m+"id");
		}
		if(b >= 1) {
			state[0].crossover[b-1] = states.crossover[b-1];
			crossovertruevalue[b-1] = state[0].crossover[b-1];
			add_listener("b"+(String)b+"cross");
		}
		state[0].gain[b] = states.gain[b];
		add_listener("b"+(String)b+"gain");

		truemute[b] = pots.bands[b].mute;
		buttons[b][0] = pots.bands[b].mute || state[0].gain[b] <= .01f;
		add_listener("b"+(String)b+"mute");

		buttons[b][1] = pots.bands[b].solo;
		add_listener("b"+(String)b+"solo");

		buttons[b][2] = pots.bands[b].bypass;
		add_listener("b"+(String)b+"bypass");
	}
	state[0].wet = states.wet;
	add_listener("wet");
	modulecount = states.modulecount;
	add_listener("modulecount");
	oversampling = pots.oversampling;
	add_listener("oversampling");
	isb = pots.isb;
	add_listener("ab");

	audio_processor.calccross(crossovertruevalue,state[0].crossover);
	for(int i = 0; i < (BAND_COUNT-1); ++i) crossoverlerp[i] = state[0].crossover[i];

	for(int m = 0; m < (BAND_COUNT*MAX_MOD); ++m) {
		for(int e = 0; e < modules[state[0].modulesvalues[m].id].subknobs.size(); ++e) {
			state[0].modulesvalues[m].lerps.push_back(state[0].modulesvalues[m].value);
			state[0].modulesvalues[m].lerps.push_back(state[0].modulesvalues[m].value);
		}
	}

	state[1] = state[0];

	if(BAND_COUNT > 1) {
		for(int i = 0; i < 330; ++i) {
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
		for(int i = 332; i < 662; ++i) {
			vispoly[i*6  ] = vispoly[(i-332)*6];
			vispoly[i*6+3] = vispoly[(i-332)*6];
			vispoly[i*6+1] = vispoly[(i-332)*6+1];
			vispoly[i*6+4] = vispoly[(i-332)*6+1];
			vispoly[i*6+2] = 1.f;
			vispoly[i*6+5] = 0.f;
		}
		audio_processor.dofft = true;
	}

	init(&look_n_feel);
}
PrismaAudioProcessorEditor::~PrismaAudioProcessorEditor() {
	audio_processor.dofft = false;
	close();
}

void PrismaAudioProcessorEditor::newOpenGLContextCreated() {
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform float stretch;
out vec3 uv;
out vec2 p;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	p = vec2((aPos.x-.0230125523)*1.048245614,(aPos.y-.3065953654/stretch)/(1-.6131907308/stretch));
	uv = vec3(aPos.x,1-(1-aPos.y)*stretch,aPos.y*stretch);
})",
//BASE FRAG
R"(#version 150 core
in vec3 uv;
in vec2 p;
uniform sampler2D basetex;
uniform vec4 selector;
uniform float preset;
uniform vec4 grayscale;
out vec4 fragColor;
void main(){
	vec2 puv = uv.xy;
	if(uv.y < .55) {
		if(uv.z > .55)
			puv.y = .55;
		else
			puv.y = uv.z;
	}
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

	decalshader = add_shader(
//DECAL VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 size;
uniform vec2 pos;
uniform vec2 offset;
uniform float banner;
uniform float rot;
out vec2 uv;
out vec2 clipuv;
void main(){
	gl_Position = vec4((vec2(
		(aPos.x-.5)*cos(rot)-(aPos.y-.5)*sin(rot),
		(aPos.x-.5)*sin(rot)+(aPos.y-.5)*cos(rot))+.5)*size+pos,0,1);
	gl_Position = vec4(vec2(gl_Position.x,gl_Position.y*(1-banner)+banner)*2-1,0,1);
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

	moduleshader = add_shader(
//MODULE VERT
R"(#version 150 core
in vec2 aPos;
uniform int modcount;
uniform vec2 size;
uniform vec2 pos;
uniform float selector;
uniform int id;
uniform float banner;
out vec2 uv;
out float truex;
void main(){
	gl_Position = vec4((aPos*vec2(1-abs(selector),1)-vec2(min(selector,0),0))*size+pos,0,1);
	gl_Position = vec4(vec2(gl_Position.x,gl_Position.y*(1-banner)+banner)*2-1,0,1);
	uv = vec2((1-(1-aPos.x)*(1-abs(selector))+min(selector,0)+id)/modcount,aPos.y);
	truex = 1-(1-aPos.x)*(1-abs(selector));
})",
//MODULE FRAG
R"(#version 150 core
in vec2 uv;
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
	if(truex < hover) {
		col.a = col.g;
		col.g = 0;
	} else col.a = 0;
	col = max(min((col-.5)*dpi+.5,1),0);
	fragColor = vec4(bg*col.r+tx*col.g+kn*col.b+htclr*col.a,1);
	if(grayscale < 1) fragColor.rgb = fragColor.rgb*grayscale+(fragColor.r+fragColor.g+fragColor.b)*vec3(.3307291667,.3307291667,.31640625)*(1-grayscale);
})");

	selectorshader = add_shader(
//SELECTOR VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 size;
uniform vec2 pos;
uniform float selector;
uniform vec2 highlight;
uniform float banner;
uniform vec2 scroll;
out vec4 uv;
out float borderx;
out float ht;
void main(){
	gl_Position = vec4((aPos*vec2(1-abs(selector),1)-vec2(min(selector,0),0))*size+pos,0,1);
	gl_Position = vec4(vec2(gl_Position.x,gl_Position.y*(1-banner)+banner)*2-1,0,1);
	uv = vec4(1-(1-aPos.x)*(1-abs(selector))+min(selector,0),aPos.y*scroll.x-scroll.y*(scroll.x-1),1-(1-aPos.y)*scroll.x,aPos.y*scroll.x);
	ht = 1-(1-uv.y)*highlight.y+highlight.x;
})",
//SELECTOR FRAG
R"(#version 150 core
in vec4 uv;
in float ht;
uniform sampler2D tex;
uniform vec3 bg;
uniform vec3 tx;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec2 col = vec2(texture(tex,vec2(uv.x,max(uv.y,0))).r,1-max(texture(tex,vec2(uv.x,max(uv.z,-.5))).b,texture(tex,vec2(uv.x,min(uv.w,.5))).b));
	col = max(min((col-.5)*dpi+.5,1),0);
	fragColor = vec4(((ht>=0&&ht<=1)?tx:bg)*col.r*col.g,1);
})");

	elementshader = add_shader(
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
uniform float banner;
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
	gl_Position = vec4(vec2(gl_Position.x-selector*1.1875*size.x,gl_Position.y*(1-banner)+banner)*2-1,0,1);
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

	lidshader = add_shader(
//LID VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 size;
uniform vec2 pos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x*size.x+pos.x,(aPos.y*size.y+pos.y)*(1-banner)+banner)*2-1,0,1);
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

	logoshader = add_shader(
//LOGO VERT
R"(#version 150 core
in vec2 aPos;
uniform vec3 texscale;
uniform vec2 size;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x*size.x,aPos.y*size.y*(1-banner)+banner)*2-1,0,1);
	uv = vec2(1-(1-aPos.x)*texscale.x,(aPos.y-texscale.z)*texscale.y);
})",
//LOGO FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform float offset;
uniform vec3 texscale;
out vec4 fragColor;
void main(){
	float opacity = texture(tex,uv).r;
	if(opacity > 0)
		fragColor = vec4(vec3(.7421875)*texture(tex,uv+vec2(offset*texscale.x,0)).g*opacity,1);
	else
		fragColor = vec4(0);
})");

	visshader = add_shader(
//VIS VERT
R"(#version 150 core
in vec3 aPos;
uniform vec2 size;
uniform vec2 pos;
uniform float banner;
out vec2 uv;
out float opacity;
void main(){
	gl_Position = vec4(vec2(aPos.x*size.x+pos.x,(aPos.y*size.y+pos.y)*(1-banner)+banner)*2-1,0,1);
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
uniform float dpi;
out vec4 fragColor;
void main(){
	if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) fragColor = vec4(0);
	else {
		if(uv.x < cutoff.x) fragColor = vec4(low,1);
		else if(uv.x < cutoff.y) fragColor = vec4(lowmid,1);
		else if(uv.x < cutoff.z) fragColor = vec4(highmid,1);
		else fragColor = vec4(high,1);
		fragColor = vec4(fragColor.rgb*max(1-(1-min(min(min(abs(uv.x-cutoff.x),abs(uv.x-cutoff.y)),abs(uv.x-cutoff.z))*min(250*max(1,dpi*.7),350),1))*dpi,0),opacity);
	}
})");

	visbttnshader = add_shader(
//VIS BUTTON VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 size;
uniform vec2 pos;
uniform vec2 offset;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x*size.x+pos.x,(aPos.y*size.y+pos.y)*(1-banner)+banner)*2-1,0,1);
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

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize);
	add_texture(&selectortex, BinaryData::selector_png, BinaryData::selector_pngSize, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRROR_CLAMP_TO_EDGE);
	add_texture(&modulestex, BinaryData::modules_png, BinaryData::modules_pngSize);
	add_texture(&elementstex, BinaryData::elements_png, BinaryData::elements_pngSize);

	draw_init();
}
void PrismaAudioProcessorEditor::renderOpenGL() {
	draw_begin();

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
	for(int b = 0; b < BAND_COUNT; ++b) {
		for(int i = 0; i < 2; ++i) {
			for(int c = 0; c < 3; ++c) {
				bypasscolors[b][i][c] = activecolors[b][i][c]*bypassease[b]+graycolors[i][c]*(1-bypassease[b]);
				activecolors[b][i][c] = activecolors[b][i][c]*activeease[b]+graycolors[i][c]*(1-activeease[b]);
			}
		}
	}

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
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
	baseshader->setUniform("stretch",(249+78*modulecount)*2/561.f);
	baseshader->setUniform("preset",presettransitionease);
	baseshader->setUniform("grayscale",activeease[0],activeease[1],activeease[2],activeease[3]);
	baseshader->setUniform("selector",selectorease[0],selectorease[1],selectorease[2],selectorease[3]);
	baseshader->setUniform("banner",banner_offset);
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
	moduleshader->setUniform("modcount",MODULE_COUNT+1);
	moduleshader->setUniform("size",114.f/width,87.f/height);
	moduleshader->setUniform("banner",banner_offset);
	moduleshader->setUniform("dpi",scaled_dpi);
	for(int i = 0; i < isdouble; ++i) {
		for(int m = 0; m < (BAND_COUNT*MAX_MOD); ++m) {
			int b = (int)floor(((float)m)/MAX_MOD);
			if(fmod(m,MAX_MOD) >= modulecount || selectorease[b] >= 1) continue;
			moduleshader->setUniform("id",state[i].modulesvalues[m].id);
			moduleshader->setUniform("pos",(11.f+114.f*b)/width,(56.f+78.f*modulecount-78.f*fmod(m,MAX_MOD))/height);
			moduleshader->setUniform("hover",(!hoverknob&&hover==m&&hoverselector<0)?modules[state[i].modulesvalues[m].id].hovercutoff:0.f);
			if(fmod(m,MAX_MOD) == 0) moduleshader->setUniform("selector",selectorease[b]-presettransitionease+i);
			if(state[i].modulesvalues[m].id == 0) {
				moduleshader->setUniform("bg",bypasscolors[b][0][0],bypasscolors[b][0][1],bypasscolors[b][0][2]);
				moduleshader->setUniform("tx",bypasscolors[b][1][0],bypasscolors[b][1][1],bypasscolors[b][1][2]);
				moduleshader->setUniform("grayscale",1.f);
			} else {
				moduleshader->setUniform("bg",modules[state[i].modulesvalues[m].id].colors[0],modules[state[i].modulesvalues[m].id].colors[1],modules[state[i].modulesvalues[m].id].colors[2]);
				moduleshader->setUniform("tx",modules[state[i].modulesvalues[m].id].colors[3],modules[state[i].modulesvalues[m].id].colors[4],modules[state[i].modulesvalues[m].id].colors[5]);
				moduleshader->setUniform("grayscale",bypassease[b]);
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

	//SELECTOR
	selectorshader->use();
	coord = context.extensions.glGetAttribLocation(selectorshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	selectortex.bind();
	selectorshader->setUniform("tex",0);
	selectorshader->setUniform("size",114.f/width,(77.f+78.f*modulecount)/height);
	selectorshader->setUniform("banner",banner_offset);
	selectorshader->setUniform("dpi",scaled_dpi);
	for(int b = 0; b < BAND_COUNT; ++b) {
		if(selectorease[b] <= 0) continue;
		selectorshader->setUniform("scroll",(77.f+78.f*modulecount)/selectortex.getHeight(),1-selectorscrolllerp[b]);
		selectorshader->setUniform("pos",(125.f+114.f*b)/width,86.f/height);
		selectorshader->setUniform("selector",selectorease[b]+1);
		if(hoverselector == b)
			selectorshader->setUniform("highlight",hover,MODULE_COUNT+1);
		else
			selectorshader->setUniform("highlight",-1.f,MODULE_COUNT+1);
		selectorshader->setUniform("bg",bypasscolors[b][0][0],bypasscolors[b][0][1],bypasscolors[b][0][2]);
		selectorshader->setUniform("tx",bypasscolors[b][1][0],bypasscolors[b][1][1],bypasscolors[b][1][2]);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	//LOGO
	if(websiteht > -.75521f) {
		logoshader->use();
		coord = context.extensions.glGetAttribLocation(logoshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glActiveTexture(GL_TEXTURE0);
		elementstex.bind();
		logoshader->setUniform("tex",0);
		logoshader->setUniform("size",192.f/width,48.f/height);
		logoshader->setUniform("banner",banner_offset);
		logoshader->setUniform("texscale",384.f/elementstex.getWidth(),96.f/elementstex.getHeight(),1.f/48.f);
		logoshader->setUniform("offset",websiteht);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}

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
	elementshader->setUniform("size",96.f/width,96.f/height);
	elementshader->setUniform("dpi",(float)fmax(scaled_dpi*.5f,1));
	elementshader->setUniform("banner",banner_offset);
	for(int i = 0; i < isdouble; ++i) {
		for(int m = 0; m < (BAND_COUNT*MAX_MOD); ++m) {
			if(fmod(m,MAX_MOD) >= modulecount) continue;
			int b = (int)floor(((float)m)/MAX_MOD);
			if(fmod(m,MAX_MOD) == 0) {
				elementshader->setUniform("selector",selectorease[b]-presettransitionease+i);
				elementshader->setUniform("grayscale",bypassease[b]);
			}
			if(state[i].modulesvalues[m].id == 0 || selectorease[b] >= 1) continue;
			elementshader->setUniform("bg",modules[state[i].modulesvalues[m].id].colors[0],modules[state[i].modulesvalues[m].id].colors[1],modules[state[i].modulesvalues[m].id].colors[2]);
			elementshader->setUniform("tx",modules[state[i].modulesvalues[m].id].colors[3],modules[state[i].modulesvalues[m].id].colors[4],modules[state[i].modulesvalues[m].id].colors[5]);
			elementshader->setUniform("kn",modules[state[i].modulesvalues[m].id].colors[6],modules[state[i].modulesvalues[m].id].colors[7],modules[state[i].modulesvalues[m].id].colors[8]);
			elementshader->setUniform("moduleid",modules[state[i].modulesvalues[m].id].clip);
			elementshader->setUniform("modulepos",(11.f+114.f*b)/width,(56.f+78.f*modulecount-78.f*fmod(m,MAX_MOD))/height);
			elementshader->setUniform("modulesize",width/114.f,height/87.f);
			elementshader->setUniform("moduletexscale",114.f/modulestex.getWidth(),87.f/modulestex.getHeight());
			for(int e = 0; e < modules[state[i].modulesvalues[m].id].subknobs.size(); ++e) {
				elementshader->setUniform("rot",(.5f-state[i].modulesvalues[m].lerps[e*2])*6.28318531f*modules[state[i].modulesvalues[m].id].subknobs[e].rotspeed);
				elementshader->setUniform("pos",
					(34.f+114.f*b              +sin((state[i].modulesvalues[m].lerps[e*2+1]-.5f)*6.28318531f*modules[state[i].modulesvalues[m].id].subknobs[e].movespeed)*modules[state[i].modulesvalues[m].id].subknobs[e].moveradius)/width,
					(53.f+78.f*modulecount-78.f*fmod(m,MAX_MOD)+cos((state[i].modulesvalues[m].lerps[e*2+1]-.5f)*6.28318531f*modules[state[i].modulesvalues[m].id].subknobs[e].movespeed)*modules[state[i].modulesvalues[m].id].subknobs[e].moveradius)/height);
				elementshader->setUniform("id",modules[state[i].modulesvalues[m].id].subknobs[e].id);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
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
	decalshader->setUniform("banner",banner_offset);
	decalshader->setUniform("texscale",192.f/elementstex.getWidth(),192.f/elementstex.getHeight());
	decalshader->setUniform("size",96.f/width,96.f/height);
	decalshader->setUniform("channels",0.f,0.f,0.f,1.f);
	decalshader->setUniform("offset",0.f,0.f);
	decalshader->setUniform("rot",0.f);
	decalshader->setUniform("color",0.f,0.f,0.f);
	decalshader->setUniform("dpi",(float)fmax(scaled_dpi*.5f,1));
	for(int i = 0; i < isdouble; ++i) {
		for(int b = 0; b < BAND_COUNT; ++b) {
			if(selectorease[b] >= 1) continue;
			float x = 0;
			if(b == 0) {
				x = (eyelerpx*(1-eyelid*.5f)-6*eyelid*.5f)+(presettransitionease-selectorease[0]-i)*114.f;
				decalshader->setUniform("pos",x/width,(eyelerpy*(1-eyelid*.5f)+51*eyelid*.5f)/height);
			} else {
				x = -6.f+114.f*b+sin((state[i].crossover[b-1]-.5f)*.8f*6.28318531f)*8.f+(presettransitionease-selectorease[b]-i)*114.f;
				decalshader->setUniform("pos",x/width,(59.f+cos((state[i].crossover[b-1]-.5f)*.8f*6.28318531f)*8.f)/height);
			}
			decalshader->setUniform("clip",(11.f+114.f*b-x)/96.f,(125.f+114.f*b-x)/96.f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);

			x = 46.f+114.f*b+sin((state[i].gain[b]-.5f)*.8f*6.28318531f)*8.f+(presettransitionease-selectorease[b]-i)*114.f;
			decalshader->setUniform("pos",x/width,(59.f+cos((state[i].gain[b]-.5f)*.8f*6.28318531f)*8.f)/height);
			decalshader->setUniform("clip",(11.f+114.f*b-x)/96.f,(125.f+114.f*b-x)/96.f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}

	//WET KNOB
	decalshader->setUniform("clip",0.f,1.f);
	if(BAND_COUNT > 1) {
		decalshader->setUniform("pos",402.f/width,-1.f/height);
		decalshader->setUniform("offset",0.f,1.f);
		decalshader->setUniform("rot",(.5f-(state[0].wet*(1-presettransitionease)+state[1].wet*presettransitionease))*6.28318531f*.8f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}

	//QUALITY
	decalshader->setUniform("pos",351.f/width,24.f/height);
	decalshader->setUniform("rot",0.f);
	decalshader->setUniform("texscale",48.f/elementstex.getWidth(),48.f/elementstex.getHeight());
	decalshader->setUniform("size",48.f/width,48.f/height);
	decalshader->setUniform("offset",14.f,21.f);
	decalshader->setUniform("channels",oversampling?1.f:0.f,oversampling?0.f:1.f,0.f,0.f);
	decalshader->setUniform("dpi",scaled_dpi);
	if(hover == -10) decalshader->setUniform("color",activecolors[2][1][0]*.6f,activecolors[2][1][1]*.6f,activecolors[2][1][2]*.6f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	//AB
	decalshader->setUniform("offset",12.f,21.f);
	if(hover == -11) decalshader->setUniform("color",activecolors[1][1][0]*.6f,activecolors[1][1][1]*.6f,activecolors[1][1][2]*.6f);
	else decalshader->setUniform("color",0.f,0.f,0.f);
	decalshader->setUniform("channels",isb?0.f:1.f,isb?1.f:0.f,0.f,0.f);
	decalshader->setUniform("pos",251.f/width,40.f/height);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	if(hover == -12) decalshader->setUniform("color",activecolors[1][1][0]*.6f,activecolors[1][1][1]*.6f,activecolors[1][1][2]*.6f);
	else decalshader->setUniform("color",0.f,0.f,0.f);
	decalshader->setUniform("pos",207.f/width,40.f/height);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	decalshader->setUniform("channels",isb?1.f:0.f,isb?0.f:1.f,0.f,0.f);
	decalshader->setUniform("pos",225.f/width,40.f/height);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	decalshader->setUniform("channels",0.f,0.f,1.f,0.f);
	decalshader->setUniform("pos",216.f/width,40.f/height);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	if(eyelid > .1 && selectorease[0] < 1) {
		context.extensions.glDisableVertexAttribArray(coord);

	//BLINKING EYELID
		if(BAND_COUNT > 1) {
			lidshader->use();
			coord = context.extensions.glGetAttribLocation(lidshader->getProgramID(),"aPos");
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			lidshader->setUniform("size",34.f/width,34.f/height);
			lidshader->setUniform("banner",banner_offset);
			lidshader->setUniform("open",eyelid<.5f?4*eyelid*eyelid*eyelid:1-powf(-2*eyelid+2,3)*.5f);
			lidshader->setUniform("col",
					activecolors[0][0][0]*.3f+activecolors[0][1][0]*.7f,
					activecolors[0][0][1]*.3f+activecolors[0][1][1]*.7f,
					activecolors[0][0][2]*.3f+activecolors[0][1][2]*.7f);
			for(int i = 0; i < isdouble; ++i) {
				float x = (presettransitionease-selectorease[0]-i)*114.f;
				lidshader->setUniform("pos",(25.f+x)/width,90.f/height);
				lidshader->setUniform("clip",(-x-14.f)/34.f,(-x-14.f+114.f)/34.f);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			}
			context.extensions.glDisableVertexAttribArray(coord);
		}

	//SELECTOR ANIMATION BORDER
		decalshader->use();
		coord = context.extensions.glGetAttribLocation(decalshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		decalshader->setUniform("banner",banner_offset);
	}
	context.extensions.glActiveTexture(GL_TEXTURE0);
	selectortex.bind();
	decalshader->setUniform("tex",0);
	decalshader->setUniform("texscale",114.f/selectortex.getWidth(),1.f/selectortex.getHeight());
	decalshader->setUniform("size",114.f/width,(77.f+78.f*modulecount)/height);
	decalshader->setUniform("channels",0.f,0.f,1.f,0.f);
	decalshader->setUniform("offset",0.f,.5f*selectortex.getHeight());
	decalshader->setUniform("color",0.f,0.f,0.f);
	decalshader->setUniform("dpi",scaled_dpi);
	for(int b = 0; b < BAND_COUNT; ++b) {
		if((selectorease[b] > 0 && selectorease[b] < 1) || presettransition > 0) {
			decalshader->setUniform("pos",(11.f+114.f*b)/width,86.f/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	//VIS BG
	if(BAND_COUNT > 1) {
		visshader->use();
		coord = context.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12, opsquare, GL_DYNAMIC_DRAW);
		visshader->setUniform("size",330.f/width,52.f/height);
		visshader->setUniform("banner",banner_offset);
		visshader->setUniform("dpi",scaled_dpi);
		visshader->setUniform("pos",74.f/width,1-75.f/height);
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
		if(hover >= -20 && hover <= (BAND_COUNT-21)) {
			float left=74;
			float right=404;
			if(hover > -20) left = crossoverlerp[hover+19]*330+74;
			if(hover < (BAND_COUNT-21)) right = crossoverlerp[hover+20]*330+74;
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
				visbttnshader->setUniform("banner",banner_offset);
				visbttnshader->setUniform("tex",0);
				visbttnshader->setUniform("offset",12.f,20.f);
				visbttnshader->setUniform("texscale",48.f/elementstex.getWidth(),48.f/elementstex.getHeight());
				visbttnshader->setUniform("size",48.f/width,48.f/height);
				visbttnshader->setUniform("dpi",scaled_dpi);
				for(int i = 0; i < 3; ++i) {
					bool ison = buttons[hover+20][i];
					if(isdown && hoverbutton == i) ison = !ison;
					visbttnshader->setUniform(ison?"hlt":"clr",activecolors[hover+20][1][0],activecolors[hover+20][1][1],activecolors[hover+20][1][2]);
					visbttnshader->setUniform(ison?"clr":"hlt",.9921875f,.9921875f,.94921875f);
					if(right-left <= 47)
						visbttnshader->setUniform("pos",((float)a)/width,1-(39.f+15*i)/height);
					else
						visbttnshader->setUniform("pos",((float)a+15*i)/width,1-39.f/height);
					visbttnshader->setUniform("btn",i==0?1.f:0.f,i==1?1.f:0.f,i==2?1.f:0.f);
					glDrawArrays(GL_TRIANGLE_STRIP,0,4);
				}
				context.extensions.glDisableVertexAttribArray(coord);
			}
		}
	}

	draw_end();
}
void PrismaAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void PrismaAudioProcessorEditor::paint(Graphics& g) {}

void PrismaAudioProcessorEditor::timerCallback() {
	if(websiteht > -.75521f) websiteht -= .07f;

	bool soloed = false;
	for(int b = 0; b < BAND_COUNT; ++b) {
		if(buttons[b][1]) {
			soloed = true;
			break;
		}
	}
	if(soloed) {
		for(int b = 0; b < BAND_COUNT; ++b) {
			activelerp[b] = fmin(fmax(activelerp[b]+(buttons[b][1]?.15f:-.15f),0),1);
			bypasslerp[b] = fmin(fmax(bypasslerp[b]+((buttons[b][1]&&!buttons[b][2])?.15f:-.15f),0),1);
		}
	} else {
		for(int b = 0; b < BAND_COUNT; ++b) {
			activelerp[b] = fmin(fmax(activelerp[b]+(buttons[b][0]?-.15f:.15f),0),1);
			bypasslerp[b] = fmin(fmax(bypasslerp[b]+((buttons[b][0]||buttons[b][2])?-.15f:.15f),0),1);
		}
	}
	for(int b = 0; b < BAND_COUNT; ++b) {
		activeease[b] = activelerp[b]<.5f?2*activelerp[b]*activelerp[b]:1-pow(-2*activelerp[b]+2,2)*.5f;
		bypassease[b] = bypasslerp[b]<.5f?2*bypasslerp[b]*bypasslerp[b]:1-pow(-2*bypasslerp[b]+2,2)*.5f;
	}

	if(audio_processor.transition) {
		state[1] = state[0];
		presettransition = 1-presettransition;
		presettransitionease = presettransition<.5f?4*presettransition*presettransition*presettransition:1-powf(-2*presettransition+2,3)*.5f;
		audio_processor.transition = false;
	}

	if(presettransition > 0) {
		presettransition = fmax(presettransition-.06f,0);
		presettransitionease = presettransition<.5f?4*presettransition*presettransition*presettransition:1-powf(-2*presettransition+2,3)*.5f;
		for(int b = 0; b < (BAND_COUNT-1); ++b) crossoverlerp[b] = state[0].crossover[b]*(1-presettransitionease)+state[1].crossover[b]*presettransitionease;
	}

	float amount = fmax(fmin(powf(selectortex.getHeight()-(77.f+78.f*modulecount),-.12f),1),0);
	for(int b = 0; b < BAND_COUNT; ++b) {
		selectorscrolllerp[b] = selectorscrolllerp[b]*amount+selectorscroll[b]*(1-amount);
		if(selectorlerp[b] != (selectorstate[b]?1:0)) {
			selectorlerp[b] = fmax(fmin(selectorlerp[b]+(selectorstate[b]?.06f:-.06f),1),0);
			selectorease[b] = selectorlerp[b]<.5f?4*selectorlerp[b]*selectorlerp[b]*selectorlerp[b]:1-powf(-2*selectorlerp[b]+2,3)*.5f;
			if(!out) recalc_hover(lastx,lasty);
		}
	}

	for(int i = 0; i < (presettransition>0?2:1); ++i) {
		for(int m = 0; m < (BAND_COUNT*MAX_MOD); ++m) {
			for(int e = 0; e < modules[state[i].modulesvalues[m].id].subknobs.size(); ++e) {
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

	if(BAND_COUNT > 1) {
		fftdelta++;
		bool redraw = false;
		if(audio_processor.nextFFTBlockReady.get()) {
			redraw = true;
			audio_processor.drawNextFrameOfSpectrum(fftdelta);
			for(int i = 0; i < 330; ++i) {
				vispoly[i*6+1] = audio_processor.scopeData[i];
				if(vispoly[i*6+1] <= .001f) vispoly[i*6+1] = -.1f;
			}
			audio_processor.nextFFTBlockReady = false;
			fftdelta = 0;
		} else if(fftdelta > 10) {
			redraw = true;
			for(int i = 0; i < 330; ++i) {
				if(vispoly[i*6+1] > .001f)
					vispoly[i*6+1] = vispoly[i*6+1]*.95f;
				else vispoly[i*6+1] = -.1f;
			}
		}
		if(redraw && dpi > 0) for(int i = 0; i < 330; ++i) {
			vispoly[(i+332)*6+1] = vispoly[i*6+1];
			double angle1 = std::atan2(vispoly[i*6+1]-vispoly[i*6-5], 1/329.f);
			double angle2 = std::atan2(vispoly[i*6+1]-vispoly[i*6+7],-1/329.f);
			if(i == 0) angle1 = angle2;
			else if(i == 329) angle2 = angle1;
			while((angle1-angle2)<(-1.5707963268))angle1+=3.1415926535*2;
			while((angle1-angle2)>( 1.5707963268))angle1-=3.1415926535*2;
			double angle = (angle1+angle2)*.5;
			vispoly[(i+332)*6+3] = vispoly[i*6  ]+cos(angle)/(330.f*.8f*scaled_dpi);
			vispoly[(i+332)*6+4] = vispoly[i*6+1]+sin(angle)/(52.f*.8f*scaled_dpi);
			if(i > 0) vispoly[(i+332)*6+3] = fmax(vispoly[(i+332)*6+3],vispoly[(i+332)*6-3]);
		}
	}

	update();
}

void PrismaAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(audio_processor.transition) {
		state[1] = state[0];
		presettransition = 1-presettransition;
		presettransitionease = presettransition<.5f?4*presettransition*presettransition*presettransition:1-powf(-2*presettransition+2,3)*.5f;
		audio_processor.transition = false;
	}
	if(parameterID == "modulecount") {
		modulecount = newValue;
		set_size(478, 249+78*modulecount);
		if((77.f+78.f*modulecount) >= selectortex.getHeight()) {
			for(int b = 0; b < BAND_COUNT; ++b) {
				selectorscroll[b] = 0;
				selectorscrolllerp[b] = 0;
			}
		}
		return;
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
		audio_processor.calccross(crossovertruevalue,state[0].crossover);

		for(int i = 0; i < (BAND_COUNT-1); ++i)
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
		state[0].modulesvalues[b*MAX_MOD+m].value = newValue;
		return;
	}
	if(parameterID.endsWith("id")) {
		state[0].modulesvalues[b*MAX_MOD+m].id = (int)newValue;
		state[0].modulesvalues[b*MAX_MOD+m].lerps.resize(modules[(int)newValue].subknobs.size()*2);
		for(int e = 0; e < modules[(int)newValue].subknobs.size(); ++e) {
			state[0].modulesvalues[b*MAX_MOD+m].lerps[e*2  ] = state[0].modulesvalues[b*MAX_MOD+m].value;
			state[0].modulesvalues[b*MAX_MOD+m].lerps[e*2+1] = state[0].modulesvalues[b*MAX_MOD+m].value;
		}
	}
}
void PrismaAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	out = false;
	lastx = event.x;
	lasty = event.y;

	int prevhover = hover;
	recalc_hover(event.x,event.y);
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
	if(dpi < 0) return;
	if(event.mods.isRightButtonDown() && !event.mods.isLeftButtonDown()) {
		recalc_hover(event.x,event.y);
		std::unique_ptr<PopupMenu> rightclickmenu(new PopupMenu());
		std::unique_ptr<PopupMenu> scalemenu(new PopupMenu());
		std::unique_ptr<PopupMenu> modulemenu(new PopupMenu());
		int i = 20;
		while(++i < (ui_scales.size()+21))
			scalemenu->addItem(i,(String)round(ui_scales[i-21]*100)+"%",true,(i-21)==ui_scale_index);
		for(int i = MIN_MOD; i <= MAX_MOD; ++i)
			modulemenu->addItem(i+40,(String)i,true,i==modulecount);
		rightclickmenu->setLookAndFeel(&look_n_feel);
		rightclickmenu->addItem(1,"'Copy preset",true);
		rightclickmenu->addItem(2,"'Paste preset",audio_processor.is_valid_preset_string(SystemClipboard::getTextFromClipboard()));
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu("'Scale",*scalemenu);
		rightclickmenu->addSubMenu("'Module count",*modulemenu);

		String description = "";
		if(hover >= 0) {
			if(hoverselector >= 0)
				description = modules[hover].description;
			else
				description = modules[state[0].modulesvalues[hover].id].description;
		} else {
			if(hovereye)
				description = "Clair (mostly harmless)";
			else if(hover <= -2 && hover >= -4)
				description = "Changes the crossover frequency of the band";
			else if(hover <= -5 && hover >= -8)
				description = "Changes the gain of the band's output";
			else if(hover == -9)
				description = "Blends between the original and processed signal";
			else if(hover == -10)
				description = "Turns oversampling on/off";
			else if(hover == -11)
				description = "Switches between presets A and B";
			else if(hover == -12)
				description = (String)"Copies preset "+(isb?"B to A":"A to B");
			else if(hover == -13)
				description = "Trust the process!";
			else if(hover <= -17 && hover >= -20 && hoverbutton > -1) {
				if(hoverbutton == 0)
					description = "Mutes the selected band";
				else if(hoverbutton == 1)
					description = "Solos the selected band";
				else if(hoverbutton == 2)
					description = "Bypasses the selected band";
			}
			description = look_n_feel.add_line_breaks(description);
		}
		if(description != "") {
			rightclickmenu->addSeparator();
			rightclickmenu->addItem(-1,"'"+description,false);
		}

		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 40) {
				audio_processor.apvts.getParameter("modulecount")->setValueNotifyingHost((((float)result-40)-MIN_MOD)/(MAX_MOD-MIN_MOD));
			} else if(result >= 20) {
				set_ui_scale(result-21);
			} else if(result == 1) { //copy preset
				SystemClipboard::copyTextToClipboard(audio_processor.get_preset(audio_processor.currentpreset));
			} else if(result == 2) { //paste preset
				audio_processor.set_preset(SystemClipboard::getTextFromClipboard(), audio_processor.currentpreset);
			}
		});
		return;
	}

	initialdrag = hover;
	isdown = true;
	if(hovereye) {
		eyelidup = true;
		clickedeye = true;
		eyeclickcooldown = random.nextFloat()*150+50;
	} else if(hoverknob) {
		valueoffset = 0;
		audio_processor.undo_manager.beginNewTransaction();
		if(hover >= 0) {
			initialvalue = state[0].modulesvalues[hover].value;
			audio_processor.apvts.getParameter("b"+(String)floor(((float)hover)/MAX_MOD)+"m"+(String)fmod(hover,MAX_MOD)+"val")->beginChangeGesture();
		} else if((hover >= -4 && hover < -1) || (hover >= -16 && hover <= -14)) {
			int b = hover>-10?(hover+4):(hover+16);
			initialvalue = state[0].crossover[b];
			audio_processor.apvts.getParameter("b"+(String)(b+1)+"cross")->beginChangeGesture();
		} else if(hover >= -8 && hover <= -5) {
			initialvalue = state[0].gain[hover+8];
			audio_processor.apvts.getParameter("b"+(String)(hover+8)+"gain")->beginChangeGesture();
		} else if(hover == -9) {
			initialvalue = state[0].wet;
			audio_processor.apvts.getParameter("wet")->beginChangeGesture();
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
			audio_processor.apvts.getParameter("b"+(String)floor(((float)hover)/MAX_MOD)+"m"+(String)fmod(hover,MAX_MOD)+"val")->setValueNotifyingHost(value-valueoffset);
		} else if((hover >= -4 && hover < -1) || (hover >= -16 && hover <= -14)) {
			int b = 0;
			if(hover > -10) {
				b = hover+4;
			} else {
				value = initialvalue+event.getDistanceFromDragStartX()*(finemode?.0005f:.005f);
				b = hover+16;
			}
			audio_processor.apvts.getParameter("b"+(String)(b+1)+"cross")->setValueNotifyingHost(fmin(fmax(value-valueoffset,b>0?(state[0].crossover[b-1]+.02f):.02f),b<(BAND_COUNT-2)?(state[0].crossover[b+1]-.02f):.98f));
		} else if(hover >= -8 && hover <= -5) {
			audio_processor.apvts.getParameter("b"+(String)(hover+8)+"gain")->setValueNotifyingHost(value-valueoffset);
		} else if(hover == -9) {
			audio_processor.apvts.getParameter("wet")->setValueNotifyingHost(value-valueoffset);
		}

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	} else if(initialdrag == -13) {
		int prevhover = hover;
		recalc_hover(event.x,event.y);
		hovereye = false;
		hoverknob = false;
		hoverselector = -1;
		hoverbutton = -1;
		if(hover != initialdrag) hover = -1;
		if(hover == -13 && prevhover != -13 && websiteht < -.75521f) websiteht = .484375f;
	}
}
void PrismaAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	isdown = false;
	if(hoverknob) {
		if(hover >= 0) {
			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((state[0].modulesvalues[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += modules[state[0].modulesvalues[hover].id].name);
			audio_processor.apvts.getParameter("b"+(String)floor(((float)hover)/MAX_MOD)+"m"+(String)fmod(hover,MAX_MOD)+"val")->endChangeGesture();
		} else if((hover >= -4 && hover < -1) || (hover >= -16 && hover <= -14)) {
			int b = hover>-10?(hover+4):(hover+16);
			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((state[0].crossover[b] - initialvalue) >= 0 ? "Increased crossover " : "Decreased crossover ") += (String)(b+1));
			audio_processor.apvts.getParameter("b"+(String)(b+1)+"cross")->endChangeGesture();
		} else if(hover >= -8 && hover <= -5) {
			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((state[0].crossover[hover+8] - initialvalue) >= 0 ? "Increased gain for band " : "Decreased gain for band ") += (String)(hover+8));
			audio_processor.apvts.getParameter("b"+(String)(hover+8)+"gain")->endChangeGesture();
		} else if(hover == -9) {
			audio_processor.undo_manager.setCurrentTransactionName(
				(state[0].wet - initialvalue) >= 0 ? "Increased Wet" : "Decreased Wet");
			audio_processor.apvts.getParameter("wet")->endChangeGesture();
		}
		audio_processor.undo_manager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		int prevhover = hover;
		int prevhoverbutton = hoverbutton;
		recalc_hover(event.x,event.y);
		if(hover == prevhover && hoverbutton == prevhoverbutton) {
			if(hover >= 0) {
				if(hoverselector >= 0) {
					if(state[0].modulesvalues[selectorid].id != hover) audio_processor.apvts.getParameter("b"+(String)floor(((float)selectorid)/MAX_MOD)+"m"+(String)fmod(selectorid,MAX_MOD)+"val")->setValueNotifyingHost(modules[hover].defaultval);
					audio_processor.apvts.getParameter("b"+(String)floor(((float)selectorid)/MAX_MOD)+"m"+(String)fmod(selectorid,MAX_MOD)+"id")->setValueNotifyingHost(((float)hover)/MODULE_COUNT);
					for(int i = 0; i < BAND_COUNT; ++i) selectorstate[i] = false;
				} else {
					selectorid = hover;
					int b = (int)floor(((float)hover)/MAX_MOD);
					for(int i = 0; i < BAND_COUNT; ++i) selectorstate[i] = i==b;
				}
			} else if(hover == -10) {
				oversampling = !oversampling;
				audio_processor.apvts.getParameter("oversampling")->setValueNotifyingHost(oversampling?1.f:0.f);
				audio_processor.undo_manager.setCurrentTransactionName(oversampling?"Turned oversampling on":"Turned oversampling off");
				audio_processor.undo_manager.beginNewTransaction();
			} else if(hover == -11) {
				isb = !isb;
				audio_processor.undo_manager.beginNewTransaction();
				audio_processor.undo_manager.setCurrentTransactionName(isb?"Moved to preset B":"Moved to preset A");
				audio_processor.apvts.getParameter("ab")->setValueNotifyingHost(isb?1.f:0.f);
			} else if(hover == -12) {
				audio_processor.undo_manager.beginNewTransaction();
				audio_processor.undo_manager.setCurrentTransactionName(isb?"Copied preset B to A":"Copied preset A to B");
				audio_processor.copypreset(isb);
			} else if(hover == -13) {
				URL("https://vst.unplug.red/").launchInDefaultBrowser();
			} else if(hover >= -20 && hover <= -17 && hoverbutton > -1) {
				if(hoverbutton == 0) {
					bool currentmute = buttons[hover+20][0];
					audio_processor.undo_manager.setCurrentTransactionName((String)(currentmute?"Unmuted band ":"Muted band ") += (String)(hover+20));
					if(currentmute && state[0].gain[hover+20] <= .01f)
						audio_processor.apvts.getParameter("b"+(String)(hover+20)+"gain")->setValueNotifyingHost(.5f);
					audio_processor.apvts.getParameter("b"+(String)(hover+20)+"mute")->setValueNotifyingHost(!currentmute);
					audio_processor.undo_manager.beginNewTransaction();
				} else if(hoverbutton == 1) {
					audio_processor.undo_manager.setCurrentTransactionName((String)(buttons[hover+20][1]?"Unsoloed band ":"Soloed band ") += (String)(hover+20));
					audio_processor.apvts.getParameter("b"+(String)(hover+20)+"solo")->setValueNotifyingHost(!buttons[hover+20][1]);
					audio_processor.undo_manager.beginNewTransaction();
				} else if(hoverbutton == 2) {
					audio_processor.undo_manager.setCurrentTransactionName((String)(buttons[hover+20][2]?"Unbypassed band ":"Bypassed band ") += (String)(hover+20));
					audio_processor.apvts.getParameter("b"+(String)(hover+20)+"bypass")->setValueNotifyingHost(!buttons[hover+20][2]);
					audio_processor.undo_manager.beginNewTransaction();
				}
			} else for(int i = 0; i < BAND_COUNT; ++i) selectorstate[i] = false;
		} else if(hover == -13) websiteht = .484375f;
	}
}
void PrismaAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(dpi < 0) return;
	if(!hoverknob) return;
	if(hover >= 0) {
		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += modules[state[0].modulesvalues[hover].id].name);
		audio_processor.apvts.getParameter("b"+(String)floor(((float)hover)/MAX_MOD)+"m"+(String)fmod(hover,MAX_MOD)+"val")->setValueNotifyingHost(modules[state[0].modulesvalues[hover].id].defaultval);
	} else if((hover >= -4 && hover < -1) || (hover >= -16 && hover <= -14)) {
		int b = hover>-10?(hover+4):(hover+16);
		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset crossover " += (String)(b+1));
		audio_processor.apvts.getParameter("b"+(String)(b+1)+"cross")->setValueNotifyingHost(fmin(fmax((((float)b)+1)/BAND_COUNT,b>0?(state[0].crossover[b-1]+.02f):.02f),b<(BAND_COUNT-2)?(state[0].crossover[b+1]-.02f):.98f));
	} else if(hover >= -8 && hover <= -5) {
		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset gain for band " += (String)(hover+8));
		audio_processor.apvts.getParameter("b"+(String)(hover+8)+"gain")->setValueNotifyingHost(.5f);
	} else if(hover == -9) {
		audio_processor.undo_manager.setCurrentTransactionName("Reset Wet");
		audio_processor.apvts.getParameter("wet")->setValueNotifyingHost(1.f);
	}
	audio_processor.undo_manager.beginNewTransaction();
}
void PrismaAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(!hoverknob) return;
	if(hover >= 0) {
		audio_processor.apvts.getParameter("b"+(String)floor(((float)hover)/MAX_MOD)+"m"+(String)fmod(hover,MAX_MOD)+"val")->setValueNotifyingHost(
			state[0].modulesvalues[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	} else if(hover >= -4 && hover < -1) {
		int b = hover+4;
		audio_processor.apvts.getParameter("b"+(String)(hover+5)+"cross")->setValueNotifyingHost(fmin(fmax(
			state[0].crossover[hover+4]+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f)
			,hover>-4?(state[0].crossover[hover+3]+.02f):.02f),hover<(BAND_COUNT-6)?(state[0].crossover[hover+5]-.02f):.98f));
	} else if(hover >= -8 && hover <= -5) {
		audio_processor.apvts.getParameter("b"+(String)(hover+8)+"gain")->setValueNotifyingHost(
			state[0].gain[hover+8]+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	} else if(hover == -9) {
		audio_processor.apvts.getParameter("wet")->setValueNotifyingHost(
			state[0].wet+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	}
}
void PrismaAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	//eye position
	if(out || BAND_COUNT <= 1) {
		hovereye = false;
		clickedeye = false;
	} else {
		float angle = atan2(x-42+selectorease[0]*114.f,(height-y)-107);
		float amp = powf((x-42+selectorease[0]*114.f)*.2f,2)+powf((107-(height-y))*.2f,2);
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

	int prevselector = hoverselector;
	hoverselector = -1;
	hoverbutton = -1;
	hoverknob = false;
	if(x <= 11 || y <= 86 || x >= 467 || y >= (height-86)) {
		if((y <= 86 || y >= (height-86)) && prevselector > -1 && (77.f+78.f*modulecount) < selectortex.getHeight())
			selectorscroll[prevselector] = y <= 90 ? 0.f : 1.f;
		if(y >= 23 && y <= 75 && BAND_COUNT > 1) {
			hover = -1;
			//cross
			for(int b = 0; b < (BAND_COUNT-1); ++b) {
				float cx = state[0].crossover[b]*330+74;
				if(x >= (cx-3) && x <= (cx+3)) {
					hover = b-16;
					hoverknob = true;
					return;
				}
			}
			if(hover == -1 && x >= 74 && x <= 404) {
				hover = BAND_COUNT-21;
				for(int b = 0; b < (BAND_COUNT-1); ++b) {
					if(x < state[0].crossover[b]*330+74) {
						hover = b-20;
						break;
					}
				}

				//solo bypass mute
				float left=74;
				float right=404;
				if(hover > -20) left = state[0].crossover[hover+19]*330+74;
				if(hover < (BAND_COUNT-21)) right = state[0].crossover[hover+20]*330+74;
				if((right-left) < 17) return;
				if((right-left) <= 47) {
					if(x < round(left+2.5f) || x > round(left+17.5f)) return;
					for(int i = 0; i < 3; ++i) {
						if(y >= (24+15*i) && y <= (39+15*i)) {
							hoverbutton = i;
							return;
						}
					}
				} else {
					if(y < 24 || y > 39) return;
					float a = round(left+(right-left)*.5f-22.5f);
					for(int i = 0; i < 3; ++i) {
						if(x >= (a+15*i) && x <= (a+15+15*i)) {
							hoverbutton = i;
							return;
						}
					}
				}
			}
			return;
		}
		y = height-y;
		//wet
		if((powf(450-x,2)+powf(47-y,2)) <= 484) {
			hoverknob = true;
			hover = -9;
			return;
		}
		//oversampling
		if(x >= 351 && y >= 21 && x <= 380 && y <= 36) {
			hover = -10;
			return;
		}
		//a
		if(x >= 251 && y >= 41 && x <= 262 && y <= 52) {
			hover = -11;
			return;
		}
		//a->b
		if(x >= 207 && y >= 41 && x <= 236 && y <= 52) {
			hover = -12;
			return;
		}
		//website
		if(x >= 3 && y >= 3 && x <= 149 && y <= 47) {
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
		selectorscroll[hoverselector] = (y-86.f)/fmin(77.f+78.f*modulecount,selectortex.getHeight());
		hover = floor(selectorscroll[hoverselector]*(MODULE_COUNT+1));
		if(hover > MODULE_COUNT)
			hover = -1;
		if((77.f+78.f*modulecount) >= selectortex.getHeight())
			selectorscroll[hoverselector] = 0;
		return;
	}
	if((height-y) <= 143) {
		y = powf(107-(height-y),2);
		for(int b = 0; b < BAND_COUNT; ++b) {
			//cross
			if(b >= 1 && (powf(31+b*114-x,2)+y) <= 324) {
				hoverknob = true;
				hover = -5+b;
				return;
			//gain
			} else if((powf(83+b*114-x,2)+y) <= 324) {
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
	hover = floor(x/114.f)*MAX_MOD+floor((y-106)/78.f);
	hoverknob = state[0].modulesvalues[hover].id != 0 && fmod((x/114.f),1) > .3;
}

LookNFeel::LookNFeel() {
	setColour(PopupMenu::backgroundColourId,Colour::fromFloatRGBA(0.f,0.f,0.f,0.f));
	font = find_font("Arial|Helvetica Neue|Helvetica|Roboto");
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	Font fontt = Font(font,"Regular",14.f*scale);
	fontt.setHorizontalScale(1.2f);
	fontt.setExtraKerningFactor(-.05f);
	return fontt;
}
void LookNFeel::drawPopupMenuBackground(Graphics &g, int width, int height) {
	g.setColour(txt);
	g.fillRect(0,0,width,height);
	g.setColour(bg);
	g.fillRect(1.5f*scale,1.5f*scale,width-3*scale,height-3*scale);
}
void LookNFeel::drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) {
	if(isSeparator) {
		g.setColour(txt);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		return;
	}

	bool removeleft = text.startsWith("'");
	if(isHighlighted && isActive) {
		g.setColour(txt);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		if(removeleft)
			g.setColour(highlight_bg);
		else
			g.setColour(highlight_bg2);
		g.fillRect(1.5f*scale,0.f,area.getWidth()-3*scale,(float)area.getHeight());
		g.setColour(bg);
	}

	if(textColour != nullptr)
		g.setColour(*textColour);
	else if(!isActive)
		g.setColour(inactive);
	else if(!isHighlighted)
		g.setColour(txt);

	auto r = area;
	if(removeleft) r.removeFromLeft(5*scale);
	else r.removeFromLeft(area.getHeight());

	Font font = getPopupMenuFont();
	if(!isActive && removeleft) font.setBold(true);
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
	return (int)floor(1.5f*scale);
}
void LookNFeel::getIdealPopupMenuItemSize(const String& text, const bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight)
{
	if(isSeparator) {
		idealWidth = 50*scale;
		idealHeight = (int)floor(1.5f*scale);
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
