#include "processor.h"
#include "editor.h"

SunBurntAudioProcessorEditor::SunBurntAudioProcessorEditor(SunBurntAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 368, 334, 1.5f)
{
	jpmode = params.jpmode;
	prevjpmode = jpmode;
	curveselection = params.curveselection;

	for(int i = 0; i < paramcount; ++i)
		add_listener(params.pots[i].id);
	for(int i = 0; i < 7; i++) {
		if(i == 4) {
			sync = state.values[i];
			continue;
		} else if(i == 3) length = state.values[i];
		int newi = i>4?i-1:i;
		knobs[newi].id = params.pots[i].id;
		knobs[newi].name = params.pots[i].name;
		knobs[newi].value = params.pots[i].normalize(state.values[i]);
		knobs[newi].minimumvalue = params.pots[i].minimumvalue;
		knobs[newi].maximumvalue = params.pots[i].maximumvalue;
		knobs[newi].defaultvalue = params.pots[i].normalize(params.pots[i].defaultvalue);
		++knobcount;
	}
	for(int i = 0; i < 5; ++i) {
		sliders[i].id = params.pots[i+11].id;
		sliders[i].name = params.pots[i+11].name;
		if(params.pots[i+11].name == "Shimmer pitch")
			sliders[i].displayname = "pitch";
		else
			sliders[i].displayname = params.pots[i+11].name.toLowerCase().removeCharacters("-");
		sliders[i].value = params.pots[i+11].normalize(state.values[i+11]);
		sliders[i].minimumvalue = params.pots[i+11].minimumvalue;
		sliders[i].maximumvalue = params.pots[i+11].maximumvalue;
		sliders[i].defaultvalue = params.pots[i+11].normalize(params.pots[i+11].defaultvalue);
		sliders[i].showon = params.pots[i+11].showon;
		sliders[i].dontshowif = params.pots[i+11].dontshowif;
		++slidercount;
	}
	for(int i = 1; i < 5; i++)
		curveindex[i] = state.values[6+i];
	for(int i = 0; i < 8; i++)
		curves[i] = state.curves[i];

	knobs[0].description = "Gain of unprocessed signal.";
	knobs[1].description = "Gain of processed signal.";
	knobs[2].description = "The amount of reflections in the reverb.\nAt low values the reverb begins to sound like a delay.\nWhen turned all the way down, the plugin enters tap-delay mode in which every point in the volume curve represents a tap.\nThis knob is irrelevant if the Density curve is enabled.";
	knobs[3].description = "Length of the reverb in seconds.\nWhen rotated while holding CTRL, BPM sync mode can be enabled.";
	knobs[4].description = "Depth of vibrato applied to wet input before entering the reverb.\nThis helps create a thicker and wider sound.";
	knobs[5].description = "Speed of vibrato applied to wet input before entering the reverb.\nThis helps create a thicker and wider sound.";
	sliders[0].description = "Cut-off frequency of filter applied on the wet signal to remove unwanted low frequency content.";
	sliders[1].description = "Resonance of High-Pass filter applied on the wet signal.\nCan be used for creative sound design at higher values when the High-Pass curve is enabled.";
	sliders[2].description = "Cut-off frequency of filter applied on the wet signal to remove unwanted high frequency content.";
	sliders[3].description = "Resonance of Low-Pass filter applied on the wet signal.\nCan be used for creative sound design at higher values when the Low-Pass curve is enabled.";
	sliders[4].description = "Amount of semitones shifted by the pitch shifter.";
	curvedescriptions[0] = "Empty slot.";
	curvedescriptions[1] = "Curve representing the cut-off frequency of a filter which removes lower frequencies.";
	curvedescriptions[2] = "Curve representing the resonance of the High-Pass filter, which can be used for creative sound design.";
	curvedescriptions[3] = "Curve representing the cut-off frequency of a filter which removes higher frequencies.";
	curvedescriptions[4] = "Curve representing the resonance of the Low-Pass filter, which can be used for creative sound design.";
	curvedescriptions[5] = "Curve representing the panning of the reverb along its tail.\nCan be used for creative effects.";
	curvedescriptions[6] = "Curve representing the amount of reflections in the reverb along its tail.\nAt low values the reverb begins to sound like a delay.\nWhen turned all the way down, the plugin enters tap-delay mode in which every point in the volume curve represents a tap.";
	curvedescriptions[7] = "Curve representing a blend between a regular and pitched version of the signal along its tail.\nCan be used to add a bit of sparkle at the end of a tail, or exploited for more creative effects.";

	for(int i = 0; i < 6; i++)
		knobs[i].description = look_n_feel.add_line_breaks(knobs[i].description);
	for(int i = 0; i < 5; i++)
		sliders[i].description = look_n_feel.add_line_breaks(sliders[i].description);
	for(int i = 0; i < 8; i++)
		curvedescriptions[i] = look_n_feel.add_line_breaks(curvedescriptions[i]);

	calcvis();
	randcubes(state.seed);

	for(int h = 0; h < 2; ++h) {
		handposrotated[h*3+1] = 191-random.nextFloat()*40;
		handposdamp       [h*4  ].reset(handposrotated[h*3  ],.07,-1,15,2);
		handposdamp       [h*4+1].reset(handposrotated[h*3+1],.07,-1,15,2);
		handposdamp       [h*4+2].reset(handposrotated[h*3+2],.07,-1,15,2);
		handposdamp       [h*4+3].reset(handpos       [h*4+3],.07,-1,15,2);
		handposinertiadamp[h*3  ].reset(handposrotated[h*3  ],.2 ,-1,15,2);
		handposinertiadamp[h*3+1].reset(handposrotated[h*3+1],.2 ,-1,15,2);
		handposinertiadamp[h*3+2].reset(handposrotated[h*3+2],.1 ,-1,15,2);
	}

	init(&look_n_feel);
}
SunBurntAudioProcessorEditor::~SunBurntAudioProcessorEditor() {
	close();
}

void SunBurntAudioProcessorEditor::newOpenGLContextCreated() {
	add_font(&font);
	add_font(&timefont);
	recalcsliders();

	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner))*2-1,0,1);
	uv = aPos;
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D basetex;
uniform sampler2D basetexjp;
uniform sampler2D disptex;
uniform vec2 texscale;
uniform float shineprog;
uniform float dpi;
uniform float isjp;
uniform vec2 hidecube;
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
	if(disp.b < .95) {
		vec3 r = vec3(-5);
		disp.b = round(disp.b*64);
		if(disp.b == hidecube.x || disp.b == hidecube.y) {
			fragColor = vec4(0,0,0,1);
			return;
		}
			 if(disp.b == randomid.x) r = vec3(randomx.x,randomy.x,randomblend.x);
		else if(disp.b == randomid.y) r = vec3(randomx.y,randomy.y,randomblend.y);
		else if(disp.b == randomid.z) r = vec3(randomx.z,randomy.z,randomblend.z);
		else if(disp.b == randomid.w) r = vec3(randomx.w,randomy.w,randomblend.w);
		if(r.x > -4) dispuv += ((disp.gr*2-1)*r.z+r.xy*(1-abs(r.z)))*vec2(.0027174,.002994);
	}

	vec3 col = vec3(0);
	if(section == 1) {
		col = vec3(texture(basetex,uv).r,0,texture(basetex,dispuv).b);
	} else if(section == 2) {
		if(isjp > .5)
			col = vec3(texture(basetexjp,dispuv*vec2(1,4)).rg,texture(basetexjp,uv*vec2(1,4)).b);
		else
			col = vec3(texture(basetex,dispuv).rg,texture(basetex,uv).b);
	} else {
		col.r = texture(basetex,dispuv).r;
		if(shineprog > -.65761 && uv.x >= .58967) {
			float shine = texture(basetex,uv+vec2(shineprog,0)).g;
			col.b = shine*col.r;
			col.r *= (1-shine);
		}
	}

	fragColor = vec4(max(min((col-.5)*dpi+.5,1),0),1);
})");

	noneshader = add_shader(
//NONE VERT
R"(#version 150 core
in vec2 aPos;
uniform vec4 pos;
uniform float banner;
uniform float isjp;
uniform float time;
out vec2 uv;
out vec2 logouv;
void main(){
	gl_Position = vec4(vec2(aPos.x*pos.z+pos.x,(aPos.y*pos.w+pos.y)*(1-banner))*2-1,0,1);
	uv = aPos;
	float angle = -.0471238898;
	if(isjp > .5)
		angle = -.0844739358;
	logouv = vec2((aPos.x-.5)*1.4158415842,aPos.y-.5);
	logouv = vec2(
		(logouv.x*cos(angle)-logouv.y*sin(angle))/1.4158415842,
		logouv.x*sin(angle)+logouv.y*cos(angle))+.5;
	logouv.x -= time;
	if(isjp < .5)
		logouv -= vec2(.18,.09);
})",
//NONE FRAG
R"(#version 150 core
in vec2 uv;
in vec2 logouv;
uniform sampler2D tex;
uniform float isjp;
uniform float dpi;
out vec4 fragColor;
void main(){
	float border = (texture(tex,uv).b-.5)*max(1,dpi*.6)+.5;
	float logo = 0;
	if(isjp > .5)
		logo = texture(tex,mod(logouv,vec2(.2185314685,.3094059406))).g;
	else
		logo = texture(tex,mod(logouv,vec2(.2622377622,.5099009901))).r;
	fragColor = vec4(border,0,(logo-.5)*max(1,dpi*.6)+.5,1);
})");

	selectshader = add_shader(
//SELECT VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec4 pos;
uniform float ratio;
uniform vec3 uvoffset;
out vec2 texuv;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x*pos.z+pos.x,(aPos.y*pos.w+pos.y)*(1-banner))*2-1,0,1);
	texuv = vec2(aPos.x-.5,(aPos.y-.5)*ratio);
	texuv = vec2(
		texuv.x*cos(uvoffset.z)-texuv.y*sin(uvoffset.z),
		texuv.x*sin(uvoffset.z)+texuv.y*cos(uvoffset.z));
	texuv = vec2(texuv.x,texuv.y/ratio)+.5+uvoffset.xy;
	uv = aPos;
})",
//SELECT FRAG
R"(#version 150 core
in vec2 texuv;
in vec2 uv;
uniform sampler2D tex;
uniform float isjp;
uniform float dpi;
uniform float select;
uniform float ratio;
uniform float index;
out vec4 fragColor;
void main(){
	vec2 coords = vec2((max(min(texuv.x,1),0)+index)/8,texuv.y);
	if(index < .5)
		coords.y = coords.y*2-.5;

	float label = 0;
	if(isjp > .5)
		label = texture(tex,coords).g;
	else
		label = texture(tex,coords).r;
	if(dpi > 2)
		label = (label-.5)*max(1,dpi*.6)+.5;

	if(select < .5) {
		fragColor = vec4(0,0,0,label);
	} else if(select < 1.5) {
		fragColor = vec4(0,0,1,label);

	} else {
		coords = uv;
		if(uv.y > .5)
			coords.y = 1-coords.y;
		coords = coords*vec2(2,ratio*2)-1;
		coords.y = min(coords.y,0);
		float x = (1-sqrt(coords.x*coords.x+coords.y*coords.y))*10*dpi+.5;
		fragColor = vec4(1,1-label,0,x);
	}
})");

	visshader = add_shader(
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

	circleshader = add_shader(
//CIRCLE VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 knobscale;
uniform vec2 knobpos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos.x*knobscale.x+knobpos.x,(aPos.y*knobscale.y+knobpos.y)*(1-banner)-banner,0,1);
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

	panelshader = add_shader(
//PANEL VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec4 pos;
uniform vec2 offset;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x*pos.z+pos.x,(aPos.y*pos.w+pos.y)*(1-banner))*2-1,0,1);
	uv = aPos*(1/(1-offset))-vec2(0,offset.y);
})",
//PANEL FRAG
R"(#version 150 core
in vec2 uv;
uniform vec3 size;
out vec4 fragColor;
void main(){
	float x = 10;
	vec3 sizeclamp = size;
	sizeclamp.xy = max(size.xy,3);
	vec2 coord = uv;
	coord.y = 1-(1-coord.y)*min(size.y/3,1);
	if(coord.x > .6666) {
		if(coord.y > 1) {
			x = 0;
		} else if(coord.y > .3333) {
			coord = (1-coord)*sizeclamp.xy-vec2(1,0);
			coord.x = min(coord.x,0);
			x = 1-(1-sqrt(coord.x*coord.x+coord.y*coord.y))*sizeclamp.z;
		}
	} else if(uv.x >= 0) {
		if(coord.y > .3333) {
			coord = vec2(coord.x,1-coord.y)*sizeclamp.xy-2+(1/sizeclamp.z);
			coord.x = min(coord.x,0);
			coord.y = min(coord.y,0);
			x = (1-sqrt(coord.x*coord.x+coord.y*coord.y))*sizeclamp.z;
		} else {
			coord = coord*sizeclamp.xy-vec2(0,1);
			coord.y = min(coord.y,0);
			x = 1-(1-sqrt(coord.x*coord.x+coord.y*coord.y))*sizeclamp.z;
		}
	} else {
		x = 0;
	}
	fragColor = vec4(1,0,0,max(min(x-.5,1),0));
})");

	papershader = add_shader(
//PAPER VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec4 pos;
uniform vec4 texpos;
uniform vec4 texcoords;
uniform float rot;
out vec2 uv;
void main(){
	vec2 temppos = aPos*texpos.zw+texpos.xy-.5;
	gl_Position = vec4(vec2(
		(temppos.x*cos(rot)-temppos.y*sin(rot)+.5)*pos.z+pos.x,
		((temppos.x*sin(rot)+temppos.y*cos(rot)+.5)*pos.w+pos.y)*(1-banner))*2-1,0,1);
	uv = aPos*texcoords.zw+texcoords.xy;
})",
//PAPER FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform float hover;
uniform float dpi;
uniform float isshadow;
out vec4 fragColor;
void main(){
	vec3 col = texture(tex,uv).rgb;
	if(hover > .5) {
		if(round(col.b*8) == round(hover))
			fragColor = vec4(1,0,(col.gr-.5)*max(1,dpi*.6)+.5);
		else
			fragColor = vec4(0);
		return;
	}
	col = (col-.5)*max(1,dpi*.6)+.5;
	if(isshadow < .5) {
		float alpha = col.r+col.b;
		if(col.b < .5)
			col = vec3(1,0,0);
		col.b -= col.g;
		col.g = 0;
		fragColor = vec4(col,alpha);
	} else {
		if(col.b < .001)
			fragColor = vec4(0,0,1,col.g);
		else
			fragColor = vec4(0);
	}
})");

	ppshader = add_shader(
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

	//fragColor = vec4(texture(buffertex,uv).rgb*.5+.25,1);
	//fragColor = vec4(coldark.g,colgray.g,colwite.g,1);
})");

	add_texture(&baseentex, BinaryData::base_en_png, BinaryData::base_en_pngSize);
	if(jpmode)
		add_texture(&basejptex, BinaryData::base_jp_png, BinaryData::base_jp_pngSize);
	add_texture(&disptex, BinaryData::disp_png, BinaryData::disp_pngSize);
	add_texture(&nonetex, BinaryData::none_png, BinaryData::none_pngSize);
	add_texture(&selecttex, BinaryData::curvelabels_png, BinaryData::curvelabels_pngSize);
	if(jpmode)
		add_texture(&papertex, BinaryData::paper_jp_png, BinaryData::paper_jp_pngSize);
	else
		add_texture(&papertex, BinaryData::paper_en_png, BinaryData::paper_en_pngSize);
	add_texture(&handtex, BinaryData::hands_png, BinaryData::hands_pngSize);
	add_texture(&overlaytex, BinaryData::overlay_png, BinaryData::overlay_pngSize, GL_LINEAR, GL_LINEAR, GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);

	add_frame_buffer(&frame_buffer, width, height);

	draw_init();
}
void SunBurntAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	if(jpmode != prevjpmode) {
		prevjpmode = jpmode;
		remove_texture(&papertex);
		if(jpmode) {
			add_texture(&papertex, BinaryData::paper_jp_png, BinaryData::paper_jp_pngSize);
			add_texture(&basejptex, BinaryData::base_jp_png, BinaryData::base_jp_pngSize);
		} else {
			add_texture(&papertex, BinaryData::paper_en_png, BinaryData::paper_en_pngSize);
			remove_texture(&basejptex);
		}
	}

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	frame_buffer.makeCurrentRenderingTarget();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//base
	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	baseentex.bind();
	baseshader->setUniform("basetex",0);
	if(jpmode) {
		context.extensions.glActiveTexture(GL_TEXTURE1);
		basejptex.bind();
		baseshader->setUniform("basetexjp",1);
	}
	context.extensions.glActiveTexture(GL_TEXTURE2);
	disptex.bind();
	baseshader->setUniform("disptex",2);
	baseshader->setUniform("isjp",jpmode?1.f:0.f);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("shineprog",websiteht);
	baseshader->setUniform("hidecube",(float)hidecube[0],(float)hidecube[1]);
	baseshader->setUniform("randomid",(float)randomid[0],(float)randomid[1],(float)randomid[2],(float)randomid[3]);
	baseshader->setUniform("randomx",randomdir[0],randomdir[1],randomdir[2],randomdir[3]);
	baseshader->setUniform("randomy",randomdir[4],randomdir[5],randomdir[6],randomdir[7]);
	baseshader->setUniform("randomblend",randomblend[0],randomblend[1],randomblend[2],randomblend[3]);
	if(scaled_dpi > 2)
		baseshader->setUniform("dpi",(float)fmax(1,scaled_dpi*.6f));
	else
		baseshader->setUniform("dpi",1.f);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	//none selected
	int curveid = curveindex[curveselection];
	if(curveid == 0 && curveselection > 0) {
		noneshader->use();
		coord = context.extensions.glGetAttribLocation(noneshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glActiveTexture(GL_TEXTURE0);
		nonetex.bind();
		noneshader->setUniform("tex",0);
		noneshader->setUniform("dpi",scaled_dpi);
		noneshader->setUniform("banner",banner_offset);
		noneshader->setUniform("isjp",jpmode?1.f:0.f);
		noneshader->setUniform("pos",11.f/width,73.f/height,286.f/width,202.f/height);
		noneshader->setUniform("time",nonetime);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	//curve selection
	selectshader->use();
	coord = context.extensions.glGetAttribLocation(selectshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	selecttex.bind();
	selectshader->setUniform("tex",0);
	selectshader->setUniform("dpi",scaled_dpi);
	selectshader->setUniform("banner",banner_offset);
	selectshader->setUniform("isjp",jpmode?1.f:0.f);
	for(int i = 0; i < 5; ++i) {
		if(i != 0 && curveindex[i] == 0)
			selectshader->setUniform("index",-1.f);
		else
			selectshader->setUniform("index",(float)curveindex[i]);
		selectshader->setUniform("pos",(300.f+20*((i+1)/2))/width,(74.f+100.f*(i%2))/height,20.f/width,(i==0?200.f:100.f)/height);
		selectshader->setUniform("select",curveselection==i?2.f:(hover==29013?1.f:0.f));
		selectshader->setUniform("ratio",i==0?10.f:5.f);

		bool found = false;
		for(int ii = 0; ii < 4; ++ii) if(randomid[ii] == (i+36)) {
			selectshader->setUniform("uvoffset",
				(1-fabs(randomblend[ii]))*randomdir[ii]*.05f,
				(1-fabs(randomblend[ii]))*randomdir[ii+4]*(i==0?.005f:.01f),
				randomblend[ii]*.03);
			found = true;
			break;
		}
		if(!found) selectshader->setUniform("uvoffset",0.f,0.f,0.f);

		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}

	//vis line
	if(curveid > 0 || curveselection == 0) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		if(!delaymode) {
			visshader->use();
			coord = context.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
			context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3408, visline, GL_DYNAMIC_DRAW);
			visshader->setUniform("banner",banner_offset);
			visshader->setUniform("dpi",scaled_dpi);
			glDrawArrays(GL_TRIANGLE_STRIP,0,1136);
			context.extensions.glDisableVertexAttribArray(coord);
			context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
		}
	}

	//tension points
	circleshader->use();
	coord = context.extensions.glGetAttribLocation(circleshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	circleshader->setUniform("banner",banner_offset);
	circleshader->setUniform("colout",1.f,0.f,0.f);
	if(curveid > 0 || curveselection == 0) {
		if(!delaymode) {
			circleshader->setUniform("colin",1.f,0.f,0.f);
			circleshader->setUniform("knobscale",17.364f/width,17.364f/height);
			circleshader->setUniform("size",8.682f*scaled_dpi);
			circleshader->setUniform("fill",.3f);
			int nextpoint = 0;
			for(int i = 0; i < (curves[curveid].points.size()-1); ++i) {
				if(!curves[curveid].points[i].enabled) continue;
				++nextpoint;
				while(!curves[curveid].points[nextpoint].enabled) ++nextpoint;
				if(((curves[curveid].points[nextpoint].x-curves[curveid].points[i].x)*1.42f) <= .02 || fabs(curves[curveid].points[nextpoint].y-curves[curveid].points[i].y) <= .02) continue;
				double interp = curve::calctension(.5,curves[curveid].points[i].tension);
				float x = (curves[curveid].points[i].x+curves[curveid].points[nextpoint].x)*284.f+24.f-8.682f;
				float y = (curves[curveid].points[i].y*(1-interp)+curves[curveid].points[nextpoint].y*interp)*400.f+148.f-8.682f;
				circleshader->setUniform("knobpos",
					x/width-1,
					y/height-1);
				if(x >= 398 && y <= (66+panelheighttarget)*2 && panelvisible) {
					circleshader->setUniform("colin",1.f,1.f,0.f);
					glDrawArrays(GL_TRIANGLE_STRIP,0,4);
					circleshader->setUniform("colin",1.f,0.f,0.f);
				} else {
					glDrawArrays(GL_TRIANGLE_STRIP,0,4);
				}
			}
		}

		//control points
		circleshader->setUniform("colin",1.f,1.f,0.f);
		circleshader->setUniform("knobscale",25.364f/width,25.364f/height);
		circleshader->setUniform("size",12.682f*scaled_dpi);
		circleshader->setUniform("fill",1-9.f/12.682f);
		for(int i = 0; i < curves[curveid].points.size(); ++i) {
			if(!curves[curveid].points[i].enabled) continue;
			circleshader->setUniform("knobpos",
				(curves[curveid].points[i].x*568.f+24.f-12.682f)/width-1,
				(curves[curveid].points[i].y*400.f+148.f-12.682f)/height-1);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}

	//knob bg
	circleshader->setUniform("colin",1.f,1.f,0.f);
	circleshader->setUniform("knobscale",83.26f/width,83.26f/height);
	circleshader->setUniform("size",41.63f*scaled_dpi);
	circleshader->setUniform("fill",1-9.f/41.63f);
	for(int i = 0; i < knobcount; ++i) {
		float x = i*121.2f+23.38f;
		float y = 36.42f;
		for(int ii = 0; ii < 4; ++ii) if(randomid[ii] == (i+48)) {
			x += randomdir[ii]*2;
			y += randomdir[ii+4]*2;
			break;
		}
		circleshader->setUniform("knobpos",x/width-1,y/height-1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}

	//sliders panel
	if(panelheight > .2) {
		panelshader->use();
		coord = context.extensions.glGetAttribLocation(panelshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		panelshader->setUniform("banner",banner_offset);
		int radius = 5;
		panelshader->setUniform("pos",(200.f-radius)/width,73.f/height,(97.f+radius)/width,(panelheight+1.f+radius)/height);
		panelshader->setUniform("size",96.f/radius+1,panelheight/radius+1,radius*scaled_dpi);
		panelshader->setUniform("offset",1.f/(97.f+radius),1.f/(panelheight+1+radius));
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}

	//sliders
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if(panelvisible) {
		int y = slidersvisible.size();
		for(int i = 0; i < slidersvisible.size(); i++) {
			--y;
			bool hovered = hover == i-30;
			font.draw_string(0,hovered?0:1,hovered?1:0,1,0,hovered?0:1,hovered?1:0,0,sliderslabel[i],0,204.f/width,(261.f-y*font.line_height)/height,0,1);
			font.draw_string(0,hovered?0:1,hovered?1:0,1,0,hovered?0:1,hovered?1:0,0,slidersvalue[i],1,297.f/width,(261.f-y*font.line_height)/height,1,1);
		}
	}

	//knob dots
	circleshader->use();
	coord = context.extensions.glGetAttribLocation(circleshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	circleshader->setUniform("knobscale",25.f/width,25.f/height);
	circleshader->setUniform("size",12.5f*scaled_dpi);
	circleshader->setUniform("colin",1.f,0.f,0.f);
	circleshader->setUniform("colout",1.f,0.f,0.f);
	for(int i = 0; i < knobcount; i++) {
		float x = i*121.2f+52.51f+sin((knobs[i].value-.5f)*.8f*6.28318531f)*17.5f;
		float y = 65.55f+cos((knobs[i].value-.5f)*.8f*6.28318531f)*17.5f;
		for(int ii = 0; ii < 4; ++ii) if(randomid[ii] == (i+48)) {
			x += randomdir[ii]*2;
			y += randomdir[ii+4]*2;
			break;
		}
		circleshader->setUniform("knobpos",x/width-1,y/height-1);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}

	//menu open circle
	circleshader->setUniform("knobscale",33.f/width,33.f/height);
	circleshader->setUniform("size",16.5f*scaled_dpi);
	circleshader->setUniform("fill",1-5.f/16.5f);
	for(int i = 0; i < 4; i++) {
		float x = 643.5f;
		float y = 351.5f;
		if((i%2) > .5) y -= 200;
		if(i > 1.5) x += 40;
		for(int ii = 0; ii < 4; ++ii) if(randomid[ii] == (i+54)) {
			x += randomdir[ii]*2;
			y += randomdir[ii+4]*2;
			break;
		}
		circleshader->setUniform("knobpos",x/width-1,y/height-1);
		circleshader->setUniform("colin",1.f,menuindex==(i+1)?0.f:1.f,0.f);
		circleshader->setUniform("colout",1.f,menuindex==(i+1)?1.f:0.f,0.f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	//time text
	float x = 205.f/width;
	float y = 279.f/height;
	for(int ii = 0; ii < 4; ++ii) if(randomid[ii] == 51) {
		x += randomdir[ii]*2/width;
		y -= randomdir[ii+4]*2/height;
		break;
	}
	timefont.shader->use();
	timefont.shader->setUniform("time",timeopentime,timeclosetime);
	timefont.draw_string(1,0,0,1,1,0,0,0,timestring,0,x,y,.5,.5);
	timefont.draw_string(1,1,0,1,1,1,0,0,timestring,1,x,y,.5,.5);

	//paper
	papershader->use();
	coord = context.extensions.glGetAttribLocation(papershader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	papershader->setUniform("dpi",scaled_dpi);
	papershader->setUniform("banner",banner_offset);
	papershader->setUniform("hover",-1.f);
	if(paperindex < 17) {
		context.extensions.glActiveTexture(GL_TEXTURE0);
		papertex.bind();
		papershader->setUniform("tex",0);
		papershader->setUniform("rot",paperrot);
		papershader->setUniform("pos",-2.f/width,13.f/height,320.f/width,320.f/height);
		papershader->setUniform("texpos",
			papercoords[jpmode?1:0][paperindex*6+4]/640.f,
			1-(papercoords[jpmode?1:0][paperindex*6+5]+papercoords[jpmode?1:0][paperindex*6+3])/640.f,
			papercoords[jpmode?1:0][paperindex*6+2]/640.f,
			papercoords[jpmode?1:0][paperindex*6+3]/640.f);
		papershader->setUniform("texcoords",
			papercoords[jpmode?1:0][paperindex*6  ]/(jpmode?1714.f:1897.f),
			1-(papercoords[jpmode?1:0][paperindex*6+1]+papercoords[jpmode?1:0][paperindex*6+3])/(jpmode?1504.f:1355.f),
			papercoords[jpmode?1:0][paperindex*6+2]/(jpmode?1714.f:1897.f),
			papercoords[jpmode?1:0][paperindex*6+3]/(jpmode?1504.f:1355.f));
		for(int s = 1; s >= 0; --s) {
			papershader->setUniform("isshadow",(float)s);
			if(s == 0)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
		if(itemenabled && paperindex == 0 && hover > 0) {
			papershader->setUniform("hover",(float)hover);
			papershader->setUniform("texpos",
				papercoords[jpmode?1:0][17*6+4]/640.f,
				1-(papercoords[jpmode?1:0][17*6+5]+papercoords[jpmode?1:0][17*6+3])/640.f,
				papercoords[jpmode?1:0][17*6+2]/640.f,
				papercoords[jpmode?1:0][17*6+3]/640.f);
			papershader->setUniform("texcoords",
				papercoords[jpmode?1:0][17*6  ]/(jpmode?1714.f:1897.f),
				1-(papercoords[jpmode?1:0][17*6+1]+papercoords[jpmode?1:0][17*6+3])/(jpmode?1504.f:1355.f),
				papercoords[jpmode?1:0][17*6+2]/(jpmode?1714.f:1897.f),
				papercoords[jpmode?1:0][17*6+3]/(jpmode?1504.f:1355.f));
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			papershader->setUniform("hover",-1.f);
		}
	}

	//hands
	context.extensions.glActiveTexture(GL_TEXTURE0);
	handtex.bind();
	papershader->setUniform("tex",0);
	for(int s = 1; s >= 0; --s) {
		papershader->setUniform("isshadow",(float)s);
		if(s == 0)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		for(int h = 0; h < 2; ++h) {
			int index = (h*32+fmin(1,round(handposlerp[h*4+3]))*16+((int)round(handposlerp[h*4+2]))%16)*6;
			papershader->setUniform("pos",handposlerp[h*4]/width,1-(handposlerp[h*4+1]/height),128.f/width,128.f/height);
			papershader->setUniform("rot",(.5f-(float)fmod(.5f+handposlerp[h*4+2],1))*.5f);
			papershader->setUniform("texpos",
				handcoords[index+4]/256.f,
				1-(handcoords[index+5]+handcoords[index+3])/256.f,
				handcoords[index+2]/256.f,
				handcoords[index+3]/256.f);
			papershader->setUniform("texcoords",
				handcoords[index  ]/2048.f,
				1-(handcoords[index+1]+handcoords[index+3])/509.f,
				handcoords[index+2]/2048.f,
				handcoords[index+3]/509.f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}

	frame_buffer.releaseAsRenderingTarget();

	//post processing
	ppshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_buffer.getTextureID());
	ppshader->setUniform("buffertex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	overlaytex.bind();
	ppshader->setUniform("overlaytex",1);
	ppshader->setUniform("overlaypos",width/600.f,height/600.f,(1-width/600.f)*overlayposx,(1-height/600.f)*overlayposy);
	ppshader->setUniform("overlayorientation",(float)overlayorientation);
	ppshader->setUniform("unalignedoffset",unalignedx[0],unalignedy[0]);
	ppshader->setUniform("unalignedspeed",unalignedx[1],unalignedy[1]);
	ppshader->setUniform("unalignedamount",unalignedx[2],unalignedy[2]);
	ppshader->setUniform("chromatic",.3f/(getWidth()*dpi));
	ppshader->setUniform("banner",banner_offset);
	coord = context.extensions.glGetAttribLocation(ppshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void SunBurntAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void SunBurntAudioProcessorEditor::calcvis() {
	if(curveindex[curveselection] == 0 && curveselection != 0)
		return;
	curveiterator iterator;
	iterator.reset(curves[curveindex[curveselection]],568);
	double prevy = 74+iterator.next()*200;
	double currenty = prevy;
	double nexty = prevy;
	for(int i = 0; i < 568; i++) {
		nexty = 74+iterator.next()*200;
		double angle1 = std::atan2(currenty-prevy,.5)+1.5707963268;
		double angle2 = std::atan2(nexty-currenty,.5)+1.5707963268;
		double angle = (angle1+angle2)*.5;
		if(fabs(angle1-angle2)>1) {
			if(fabs(currenty-prevy) > fabs(nexty-currenty))
				angle = angle1;
			else
				angle = angle2;
		}
		visline[i*6  ] = (i*.5+12+cos(angle)*2.3f)/width;
		visline[i*6+3] = (i*.5+12-cos(angle)*2.3f)/width;
		visline[i*6+1] = (currenty+sin(angle)*2.3f)/height;
		visline[i*6+4] = (currenty-sin(angle)*2.3f)/height;
		visline[i*6+2] = 0.f;
		visline[i*6+5] = 1.f;
		prevy = currenty;
		currenty = nexty;
	}
}
void SunBurntAudioProcessorEditor::paint(Graphics& g) { }

void SunBurntAudioProcessorEditor::timerCallback() {
	if(websiteht >= -.65761f) websiteht -= .03f;

	if(menuindex > 0) {
		if(paperindex == 17) {
			paperrottarget = (random.nextFloat()-.5f)*.15f;
			paperrotoffset = (random.nextFloat()-.5f)*.25f;
		}
		if(updatetick || paperindex == 1)
			paperindex = fmax(0,paperindex-1);
	} else {
		if(paperindex == 0) {
			paperrotoffset = (random.nextFloat()-.5f)*.25f;
			for(int h = 0; h < 2; ++h) {
				handposrotated[h*3  ]  = -128*(1-h)+368*h;
				handposrotated[h*3+1]  = 191-random.nextFloat()*40;
				handposrotated[h*3+2] += (h*2-1)*12;
				handpos       [h*4+3]  = 3.5f;
			}
		}
		if((updatetick && paperindex > 0) || (!updatetick && paperindex == 0))
			paperindex = fmin(17,paperindex+1);
	}
	if(updatetick && paperindex != 0 && paperindex != 17)
		paperrot = paperrottarget+paperrotoffset*pow(paperindex/16.f,3)+(random.nextFloat()-.5)*.02f;
	if(updatetick) {
		for(int h = 0; h < 2; ++h) {
			handposlerp[h*4+3] = handposdamp[h*4+3].nextvalue(handpos[h*4+3],h);

			for(int i = 0; i < 3; ++i)
				handposdamp[h*4+i].v_smoothtime = .07+(1-pow(1-handposlerp[h*4+3],3))*.05;

			float rotationeuler = handposrotated[h*3+2]*6.28318530718f*.0625f;
			handposlerp[h*4  ] = handposinertiadamp[h*3  ].nextvalue(handposdamp[h*4  ].nextvalue(handposrotated[h*3  ],h),h);
			handposlerp[h*4+1] = handposinertiadamp[h*3+1].nextvalue(handposdamp[h*4+1].nextvalue(handposrotated[h*3+1],h),h);
			handposlerp[h*4+2] = handposinertiadamp[h*3+2].nextvalue(handposdamp[h*4+2].nextvalue(handposrotated[h*3+2]+(handposdamp[h*4].v_velocity[h]*cos(rotationeuler)+handposdamp[h*4+1].v_velocity[h]*sin(rotationeuler))*.003f,h),h);
		}
	}
	updatetick = !updatetick;

	if(hover > -1 && hover < knobcount && knobs[hover].id == "length" && menuindex == 0) {
		if(timeclosetime >= timestring.length()) {
			timeopentime = -1;
			timeclosetime = -1;
		}
		timeopentime = fmin(timeopentime+1,999);
		timeclosetime = fmax(timeclosetime-1,-1);
	} else {
		timeclosetime = fmin(timeclosetime+1,timestring.length());
	}

	panelvisible = (hover == -14 || hover >= knobcount || hover < -20) && menuindex == 0;
	if(panelvisible)
		panelheight = panelheight*.6+panelheighttarget*.4;
	else
		panelheight *= .6;

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

	if(audio_processor.updatevis.get()) {
		for(int i = 0; i < 8; i++)
			curves[i] = audio_processor.presets[audio_processor.currentpreset].curves[i];
		calcvis();
		randcubes(audio_processor.state.seed);
		audio_processor.updatevis = false;
	}

	if(++nonerate >= 3) {
		nonetime = fmod(nonetime+.0005f,10000.f);
		nonerate = 0;
	}

	update();
}

void SunBurntAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "sync") {
		sync = newValue;
		recalclabels();
		return;
	}
	if(parameterID == "length") {
		length = newValue;
		recalclabels();
		return;
	}
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		if(parameterID == "density")
			recalcsliders();
		return;
	}
	for(int i = 0; i < slidercount; i++) if(sliders[i].id == parameterID) {
		sliders[i].value = sliders[i].normalize(newValue);
		recalclabels();
		return;
	}
	for(int i = 1; i < 5; i++) if(parameterID == ("curve"+(String)i)) {
		curveindex[i] = newValue;
		if(i == curveselection) calcvis();
		recalcsliders();
		return;
	}
}
void SunBurntAudioProcessorEditor::recalclabels() {
	if(sync <= 0) {
		knobs[3].value = length;
		float seconds = pow(length,2)*(6-.1)+.1;
		if(seconds >= 1) {
			timestring = (String)(round(seconds*100)*.01f);
			if(timestring.length() == 1)
				timestring += ".00s";
			else if(timestring.length() == 3)
				timestring += "0s";
			else if(timestring.length() == 4)
				timestring += "s";
		} else {
			timestring = (String)round(seconds*1000)+"ms";
		}
	} else {
		knobs[3].value = (sync-1)/15.f;
		timestring = ((String)sync)+"q";
	}

	sliderslabel.clear();
	slidersvalue.clear();
	for(int i = 0; i < slidersvisible.size(); ++i) {
		sliderslabel.push_back(sliders[slidersvisible[i]].displayname+":");
		if(slidersvisible[i] == 0 || slidersvisible[i] == 2) {
			/*
			if((slidersvisible[i] == 0 && sliders[slidersvisible[i]].value <= 0) || (slidersvisible[i] == 2 && sliders[slidersvisible[i]].value >= 1)) {
				slidersvalue.push_back("Off");
				continue;
			}
			*/
			int pt = mapToLog10(sliders[slidersvisible[i]].inflate(sliders[slidersvisible[i]].value),20.0f,20000.0f);
			if(pt < 1000) {
				slidersvalue.push_back(((String)pt)+"hz");
			} else if(pt < 10000) {
				pt = round(pt*.01);
				slidersvalue.push_back((String)(pt*.1)+(pt%10==0?".0khz":"khz"));
			} else {
				slidersvalue.push_back((String)round(pt*.001)+"khz");
			}
		} else if(slidersvisible[i] == 1 || slidersvisible[i] == 3) {
			float pt = mapToLog10(sliders[slidersvisible[i]].inflate(sliders[slidersvisible[i]].value),0.1f,40.0f);
			if(pt < 10) {
				pt = round(pt*10);
				slidersvalue.push_back((String)(pt*.1)+(fmod(pt,10)==0?".0":""));
			} else {
				slidersvalue.push_back((String)round(pt));
			}
		} else if(slidersvisible[i] == 4) {
			slidersvalue.push_back((String)round(sliders[slidersvisible[i]].inflate(sliders[slidersvisible[i]].value))+"st");
		} else {
			slidersvalue.push_back((String)sliders[slidersvisible[i]].inflate(sliders[slidersvisible[i]].value));
		}
	}
}
void SunBurntAudioProcessorEditor::recalcsliders() {
	delaymode = knobs[2].value == 0 && curveselection == 0;
	for(int d = 1; d < 5; ++d)
		if(curveindex[d] == 6)
			delaymode = false;

	int curveid = curveindex[curveselection];
	slidersvisible.clear();
	if(curveid == 0 && curveselection != 0) {
		panelheighttarget = 0;
		sliderslabel.clear();
		slidersvalue.clear();
		return;
	}
	for(int i = 0; i < slidercount; ++i) {
		for(int s = 0; s < sliders[i].showon.size(); ++s) {
			if(sliders[i].showon[s] == curveid) {
				bool shown = true;
				for(int l = 0; l < sliders[i].dontshowif.size(); ++l) {
					for(int d = 1; d < 5; ++d) {
						if(d != curveselection && curveindex[d] == sliders[i].dontshowif[l]) {
							shown = false;
							break;
						}
					}
					if(!shown) break;
				}
				if(shown) slidersvisible.push_back(i);
				break;
			}
		}
	}
	panelheighttarget = font.line_height*slidersvisible.size();
	recalclabels();
}
void SunBurntAudioProcessorEditor::randcubes(int64 seed) {
	Random seed_random(seed);
	hidecube[0] = floor(seed_random.nextFloat()*9)+11;
	hidecube[1] = floor(seed_random.nextFloat()*9)+11;
	while(hidecube[0] == hidecube[1])
		hidecube[1] = floor(seed_random.nextFloat()*9)+11;
}
void SunBurntAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == -16 && prevhover != -16 && websiteht <= -.65761f) websiteht = 0;
	if(menuindex <= 0 || prevhover == hover)
		return;
	itemenabled = false;
	if(hoverid > 0)
		for(int i = 1; i < 5; ++i)
			if(curveindex[i] == hoverid && i != menuindex)
				itemenabled = true;
	for(int h = 0; h < 2; ++h) {
		if(hover == -1 || hover%2 != h) {
			if(prevhover%2 != h || prevhover == hover)
				continue;
			handpos[h*4  ] += (random.nextFloat()*2-1)*5+(h*2-1)*10;
			handpos[h*4+1] += (random.nextFloat()*2-1)*20;
			handpos[h*4+2]  = random.nextFloat()*(h*2-1)*2;
			handpos[h*4+3]  = 1;
		} else {
			handpos[h*4  ]  = (random.nextFloat()*2-1)*5+(jpmode?158:163)+(h*2-1)*130;
			handpos[h*4+1]  = (random.nextFloat()*2-1)*5+floor(hover*.5)*(jpmode?31:32)+(jpmode?133:122);
			handpos[h*4+2]  = (random.nextFloat()*2-1)*.3f+(h==0?4:-4);
			handpos[h*4+3]  = 0;
		}
		handposrotated[h*3  ] = (handpos[h*4]-160)*cos(-paperrot)-(handpos[h*4+1]-160)*sin(-paperrot)+158-64;
		handposrotated[h*3+1] = (handpos[h*4]-160)*sin(-paperrot)+(handpos[h*4+1]-160)*cos(-paperrot)+161+64;
		handposrotated[h*3+2] = 1024+handpos[h*4+2]-(paperrot/6.28318530718f)*16;
	}
}
void SunBurntAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void SunBurntAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(dpi < 0) return;

	if(event.mods.isRightButtonDown() && !event.mods.isLeftButtonDown()) {
		hover = recalc_hover(event.x,event.y);
		std::unique_ptr<PopupMenu> rightclickmenu(new PopupMenu());
		std::unique_ptr<PopupMenu> scalemenu(new PopupMenu());
		std::unique_ptr<PopupMenu> langmenu(new PopupMenu());
		int i = 20;
		while(++i < (ui_scales.size()+21))
			scalemenu->addItem(i,(String)round(ui_scales[i-21]*100)+"%",true,(i-21)==ui_scale_index);
		langmenu->addItem(11,"English",true,!jpmode);
		langmenu->addItem(12,String::fromUTF8("日本語"),true,jpmode);
		rightclickmenu->setLookAndFeel(&look_n_feel);
		if(hover == -14 || hover >= knobcount) {
			rightclickmenu->addItem(1,"'Copy curve",true);
			rightclickmenu->addItem(2,"'Paste curve",curve::isvalidcurvestring(SystemClipboard::getTextFromClipboard()));
			rightclickmenu->addItem(3,"'Reset curve",true);
			rightclickmenu->addSeparator();
		}
		rightclickmenu->addItem(4,"'Copy preset",true);
		rightclickmenu->addItem(5,"'Paste preset",curve::isvalidcurvestring(SystemClipboard::getTextFromClipboard()));
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu(jpmode?(String::fromUTF8("'スケール")):("'Scale"),*scalemenu);
		rightclickmenu->addSubMenu(jpmode?(String::fromUTF8("'言語")):("'Language"),*langmenu);

		String description = "";
		if(menuindex > 0 && hover > -1) {
			description = curvedescriptions[hoverid];
		} else if(hover < knobcount && hover != -1) {
			if(hover > -1)
				description = knobs[hover].description;
			else if(hover > -10)
				description = curvedescriptions[curveindex[(int)ceil(hover*.5)+5]];
			else if(hover < -20)
				description = sliders[slidersvisible[hover+30]].description;
			else {
				if(hover == -10)
					description = "Curve representing the shape of the reverb's tale, influencing its attack and release, and other characteristics.";
				else if(hover == -15)
					description = "Randomize seed.\nThis changes the reflections, and mostly has an effect in a lower density value.";
				else if(hover == -16)
					description = "Trust the process!";
				description = look_n_feel.add_line_breaks(description);
			}
		}
		if(description != "") {
			rightclickmenu->addSeparator();
			rightclickmenu->addItem(-1,"'"+description,false);
		}

		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20) { //size
				set_ui_scale(result-21);
			} else if(result >= 10) { //lang
				jpmode = result==12;
				audio_processor.setLang(result==12);
			} else if(result == 1) { //copy curve
				SystemClipboard::copyTextToClipboard(audio_processor.curvetostring());
			} else if(result == 2) { //paste curve
				audio_processor.curvefromstring(SystemClipboard::getTextFromClipboard());
			} else if(result == 3) { //reset curve
				audio_processor.resetcurve();
			} else if(result == 4) { //copy preset
				SystemClipboard::copyTextToClipboard(audio_processor.get_preset(audio_processor.currentpreset));
			} else if(result == 5) { //paste preset
				audio_processor.set_preset(SystemClipboard::getTextFromClipboard(), audio_processor.currentpreset);
			}
		});
		return;
	}

	initialdrag = hover;

	if(menuindex > 0) {
		if(hover == -1) {
			menuindex = 0;
			hover = recalc_hover(event.x,event.y);
			skipmouseup = true;
		}
		return;
	}

	if(hover > -1 || hover < -20) {
		valueoffset[0] = 0;
		audio_processor.undo_manager.beginNewTransaction();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
		if(hover >= knobcount) {
			int i = round(initialdrag-knobcount);
			if((i%2) == 0) {
				i /= 2;
				initialvalue[0] = curves[curveindex[curveselection]].points[i].x;
				initialvalue[1] = curves[curveindex[curveselection]].points[i].y;
				initialdotvalue[0] = initialvalue[0];
				initialdotvalue[1] = initialvalue[1];
				valueoffset[1] = 0;
			} else {
				i = (i-1)/2;
				initialvalue[0] = curves[curveindex[curveselection]].points[i].tension;
				initialdotvalue[0] = initialvalue[0];
			}
		} else if(hover > -1) {
			initialvalue[0] = knobs[hover].value;
			if(knobs[hover].id == "length" && event.mods.isCtrlDown()) {
				audio_processor.apvts.getParameter("sync")->beginChangeGesture();
				changegesturesync = true;
			} else {
				audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
			}
		} else {
			initialvalue[0] = sliders[slidersvisible[hover+30]].value;
			audio_processor.apvts.getParameter(sliders[slidersvisible[hover+30]].id)->beginChangeGesture();
		}
	} else if(hover >= -10 && hover <= -2) {
		if((hover+9)%2 == 0 || hover == -10) {
			curveselection = ceil(hover*.5)+5;
			audio_processor.params.curveselection = curveselection;
			calcvis();
			recalcsliders();
		} else {
			int id = (int)(hover*.5)+5;
			int val = 0;
			menuindex = id;
			initialdrag = -1;
			hover = recalc_hover(event.x,event.y);
		}
	} else if(hover == -15) {
		randcubes(audio_processor.reseed());
	}
}
void SunBurntAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(menuindex > 0) {
		hover = recalc_hover(event.x,event.y)==initialdrag?initialdrag:-1;
		return;
	}

	if(initialdrag >= knobcount) {
		float dragspeed = 1.f/(284*ui_scales[ui_scale_index]);
		int i = initialdrag-knobcount;
		if((i%2) == 0) { // dragging a dot
			i /= 2;
			if(!finemode && event.mods.isAltDown()) { //start of fine mode
				finemode = true;
				initialvalue[0] += event.getDistanceFromDragStartX()*dragspeed*.9f;
				initialvalue[1] -= event.getDistanceFromDragStartY()*dragspeed*.9f*1.42f;
			} else if(finemode && !event.mods.isAltDown()) { //end of fine mode
				finemode = false;
				initialvalue[0] -= event.getDistanceFromDragStartX()*dragspeed*.9f;
				initialvalue[1] += event.getDistanceFromDragStartY()*dragspeed*.9f*1.42f;
			}

			float valuey = initialvalue[1]-event.getDistanceFromDragStartY()*dragspeed*(finemode?.1f:1)*1.42f;
			float pointy = valuey-valueoffset[1];

			if(i > 0 && i < (curves[curveindex[curveselection]].points.size()-1)) {
				float valuex = initialvalue[0]+event.getDistanceFromDragStartX()*dragspeed*(finemode?.1f:1);
				float pointx = valuex-valueoffset[0];

				if(event.mods.isCtrlDown()) { // one axis
					if(axislock == -1) {
						initialaxispoint[0] = pointx;
						initialaxispoint[1] = pointy;
						axisvaluediff[0] = valuex;
						axisvaluediff[1] = valuey;
						axislock = 0;
					} else if(axislock == 0 && (fabs(valuex-axisvaluediff[0])+fabs(valuey-axisvaluediff[1])) > .1) {
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

				if(event.mods.isShiftDown() || delaymode) { // free mode
					if((i > 1 && pointx < curves[curveindex[curveselection]].points[i-1].x) || (i < (curves[curveindex[curveselection]].points.size()-2) && pointx > curves[curveindex[curveselection]].points[i+1].x)) {
						point pnt = curves[curveindex[curveselection]].points[i];
						int n = 1;
						for(n = 1; n < (curves[curveindex[curveselection]].points.size()-1); ++n) //finding new position
							if(pnt.x < curves[curveindex[curveselection]].points[n].x) break;
						if(n > i) { // move points back
							n--;
							for(int f = i; f <= n; f++)
								curves[curveindex[curveselection]].points[f] = curves[curveindex[curveselection]].points[f+1];
						} else { //move points forward
							for(int f = i; f >= n; f--)
								curves[curveindex[curveselection]].points[f] = curves[curveindex[curveselection]].points[f-1];
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
				if(!event.mods.isShiftDown() && !delaymode) { // no free mode
					if(i > 1) {
						xleft = curves[curveindex[curveselection]].points[i-1].x;
						if(pointx <= curves[curveindex[curveselection]].points[i-1].x) {
							pointx = curves[curveindex[curveselection]].points[i-1].x;
							clamppedx = true;
						}
					}
					if(i < (curves[curveindex[curveselection]].points.size()-2)) {
						xright = curves[curveindex[curveselection]].points[i+1].x;
						if(pointx >= curves[curveindex[curveselection]].points[i+1].x) {
							pointx = curves[curveindex[curveselection]].points[i+1].x;
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
				curves[curveindex[curveselection]].points[i].enabled = (fabs(amioutofbounds[0])+fabs(amioutofbounds[1])) < .8;

				curves[curveindex[curveselection]].points[i].x = pointx;
				if(axislock != 2)
					valueoffset[0] = fmax(fmin(valueoffset[0],valuex-xleft+.1f),valuex-xright-.1f);
			}
			curves[curveindex[curveselection]].points[i].y = fmax(fmin(pointy,1),0);
			if(axislock != 1)
				valueoffset[1] = fmax(fmin(valueoffset[1],valuey+.1f),valuey-1.1f);

		} else { //dragging tension
			i = (i-1)/2;
			int dir = curves[curveindex[curveselection]].points[i].y > curves[curveindex[curveselection]].points[i+1].y ? -1 : 1;
			if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
				finemode = true;
				initialvalue[0] -= dir*event.getDistanceFromDragStartY()*dragspeed*.9f*1.42f;
			} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
				finemode = false;
				initialvalue[0] += dir*event.getDistanceFromDragStartY()*dragspeed*.9f*1.42f;
			}

			float value = initialvalue[0]-dir*event.getDistanceFromDragStartY()*dragspeed*(finemode?.1f:1)*1.42f;
			curves[curveindex[curveselection]].points[i].tension = fmin(fmax(value-valueoffset[0],0),1);

			valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
		}
		calcvis();
	} else if(initialdrag == -16) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y)==-16?-16:-1;
		if(hover == -16 && prevhover != -16 && websiteht <= -.65761f) websiteht = 0;
	} else if(initialdrag > -1 || initialdrag < -20) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue[0] -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue[0] += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue[0]-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		if(initialdrag > -1) {
			if(knobs[hover].id == "length") {
				if(event.mods.isCtrlDown()) {
					if(!changegesturesync) {
						audio_processor.apvts.getParameter("sync")->beginChangeGesture();
						audio_processor.apvts.getParameter("length")->endChangeGesture();
						changegesturesync = true;
					}
					audio_processor.apvts.getParameter("sync")->setValueNotifyingHost((fmax(value-valueoffset[0],0)*15+1)*.0625);
				} else {
					if(sync > 0) audio_processor.apvts.getParameter("sync")->setValueNotifyingHost(0);
					if(changegesturesync) {
						audio_processor.apvts.getParameter("sync")->endChangeGesture();
						audio_processor.apvts.getParameter("length")->beginChangeGesture();
						changegesturesync = false;
					}
					audio_processor.apvts.getParameter("length")->setValueNotifyingHost(value-valueoffset[0]);
				}
			} else {
				audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset[0]);
			}
		} else {
			audio_processor.apvts.getParameter(sliders[slidersvisible[hover+30]].id)->setValueNotifyingHost(value-valueoffset[0]);
		}

		valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
	}
}
void SunBurntAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(skipmouseup) {
		skipmouseup = false;
		return;
	}
	if(menuindex > 0) {
		if(hover > -1 && initialdrag == hover && !itemenabled) {
			curveselection = menuindex;
			audio_processor.params.curveselection = curveselection;
			audio_processor.apvts.getParameter("curve"+(String)menuindex)->setValueNotifyingHost(hoverid/7.f);
			audio_processor.undo_manager.setCurrentTransactionName((String)"Changed curve "+(String)menuindex+" to "+audio_processor.params.curves[hoverid].name);
			audio_processor.undo_manager.beginNewTransaction();
			menuindex = 0;
			hover = recalc_hover(event.x,event.y);
		} else if(hover == -1) {
			itemenabled = false;
		}
		return;
	}

	if(hover > -1 || hover < -20) {
		if(hover >= knobcount) {
			int i = hover-knobcount;
			if((i%2) == 0) {
				i /= 2;
				axislock = -1;
				if((fabs(initialdotvalue[0]-curves[curveindex[curveselection]].points[i].x)+fabs(initialdotvalue[1]-curves[curveindex[curveselection]].points[i].y)) < .00001) {
					curves[curveindex[curveselection]].points[i].x = initialdotvalue[0];
					curves[curveindex[curveselection]].points[i].y = initialdotvalue[1];
					return;
				}
				dragpos.x += (curves[curveindex[curveselection]].points[i].x-initialdotvalue[0])*ui_scales[ui_scale_index]*284;
				dragpos.y += (initialdotvalue[1]-curves[curveindex[curveselection]].points[i].y)*ui_scales[ui_scale_index]*200;
				if(!curves[curveindex[curveselection]].points[i].enabled) {
					curves[curveindex[curveselection]].points.erase(curves[curveindex[curveselection]].points.begin()+i);
					audio_processor.deletepoint(i);
				} else audio_processor.movepoint(i,curves[curveindex[curveselection]].points[i].x,curves[curveindex[curveselection]].points[i].y);
			} else {
				i = (i-1)/2;
				if(fabs(initialdotvalue[0]-curves[curveindex[curveselection]].points[i].tension) < .00001) {
					curves[curveindex[curveselection]].points[i].tension = initialdotvalue[0];
					return;
				}
				float interp = curve::calctension(.5,curves[curveindex[curveselection]].points[i].tension)-curve::calctension(.5,initialdotvalue[0]);
				dragpos.y += (curves[curveindex[curveselection]].points[i].y-curves[curveindex[curveselection]].points[i+1].y)*interp*ui_scales[ui_scale_index]*200.f;
				audio_processor.movetension(i,curves[curveindex[curveselection]].points[i].tension);
			}
		}
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
		if(hover >= knobcount) {
			if(((initialdrag-knobcount)%2) == 0) audio_processor.undo_manager.setCurrentTransactionName("Moved point");
			else audio_processor.undo_manager.setCurrentTransactionName("Moved tension");
		} else if(hover > -1) {
			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((knobs[hover].value - initialvalue[0]) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
			if(changegesturesync) {
				audio_processor.apvts.getParameter("sync")->endChangeGesture();
				changegesturesync = false;
			} else {
				audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
			}
		} else {
			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((sliders[slidersvisible[hover+30]].value - initialvalue[0]) >= 0 ? "Increased " : "Decreased ") += sliders[slidersvisible[hover+30]].name);
			audio_processor.apvts.getParameter(sliders[slidersvisible[hover+30]].id)->endChangeGesture();
		}
		audio_processor.undo_manager.beginNewTransaction();
		axislock = -1;
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		if(hover == -16) {
			if(prevhover == -16) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht <= -.65761f) websiteht = 0;
		}
	}
}
void SunBurntAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(dpi < 0) return;
	if(menuindex > 0) return;

	if(hover == -14) {
		float x = fmin(fmax((((float)event.x)/ui_scales[ui_scale_index]-12)/284.f,0),1);
		float y = fmin(fmax(1-(((float)event.y)/ui_scales[ui_scale_index]-60)/200.f,0),1);
		int i = 1;
		for(i = 1; i < curves[curveindex[curveselection]].points.size(); ++i)
			if(x < curves[curveindex[curveselection]].points[i].x) break;
		curves[curveindex[curveselection]].points.insert(curves[curveindex[curveselection]].points.begin()+i,point(x,y,curves[curveindex[curveselection]].points[i-1].tension));
		audio_processor.addpoint(i,x,y);
		calcvis();
		hover = recalc_hover(event.x,event.y);
		return;
	}
	if(hover >= knobcount) {
		int i = initialdrag-knobcount;
		if((i%2) == 0) {
			i /= 2;
			if(i > 0 && i < (curves[curveindex[curveselection]].points.size()-1)) {
				curves[curveindex[curveselection]].points.erase(curves[curveindex[curveselection]].points.begin()+i);
				audio_processor.deletepoint(i);
			}
		} else {
			i = (i-1)/2;
			if(fabs(curves[curveindex[curveselection]].points[i].tension-.5) < .00001f) return;
			curves[curveindex[curveselection]].points[i].tension = .5f;
			audio_processor.movetension(i,.5f);
		}
		calcvis();
		hover = recalc_hover(event.x,event.y);
	} else if(hover > -1) {
		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
		audio_processor.undo_manager.beginNewTransaction();
	} else if(hover < -20) {
		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += sliders[slidersvisible[hover+30]].name);
		audio_processor.apvts.getParameter(sliders[slidersvisible[hover+30]].id)->setValueNotifyingHost(sliders[slidersvisible[hover+30]].defaultvalue);
		audio_processor.undo_manager.beginNewTransaction();
	}
}
void SunBurntAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(menuindex > 0) return;
	if(hover >= knobcount) return;
	if(hover > -1)
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	else if(hover < -20)
		audio_processor.apvts.getParameter(sliders[slidersvisible[hover+30]].id)->setValueNotifyingHost(
			sliders[slidersvisible[hover+30]].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int SunBurntAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if(menuindex > 0) {
		float rotx = (x-158)*cos(paperrot)-(y-161)*sin(paperrot)+160;
		float roty = (x-158)*sin(paperrot)+(y-161)*cos(paperrot)+160;
		int h = -1;
		if(jpmode) {
			if(rotx < 61 || roty < 117 || rotx >= 255 || roty >= 241)
				h = -1;
			else
				h = (rotx>=158?1:0)+floor((roty-117)/31)*2;
		} else {
			if(rotx < 60 || roty < 106 || rotx >= 266 || roty >= 234)
				h = -1;
			else
				h = (rotx>=163?1:0)+floor((roty-106)/32)*2;
		}
		switch(h) {
			case -1:
				hoverid = -1;
				break;
			case 0:
				hoverid = 0;
				break;
			case 1:
				hoverid = 5;
				break;
			case 2:
				hoverid = 7;
				break;
			case 3:
				hoverid = 6;
				break;
			case 4:
				hoverid = 1;
				break;
			case 5:
				hoverid = 3;
				break;
			case 6:
				hoverid = 2;
				break;
			case 7:
				hoverid = 4;
				break;
		}
		return h;
	}

	if(y < 7) return -1;
	if(y < 55) {
		//random seed
		if(x >= 169 && x < 217) return -15;
		//logo
		else if(y < 53 && x >= 217 && x < 365) return -16;
		return -1;
	}
	if(y <= 267) {
		//dots
		if(x < 300) {
			x -= 12;
			y -= 60;
			for(int i = 0; i < curves[curveindex[curveselection]].points.size(); ++i) {
				float xx = x-curves[curveindex[curveselection]].points[i].x*284;
				float yy = y-(1-curves[curveindex[curveselection]].points[i].y)*200;
				//dot
				if((xx*xx+yy*yy)<=37.1) return i*2+knobcount;

				if(i < (curves[curveindex[curveselection]].points.size()-1)) {
					float interp = curve::calctension(.5,curves[curveindex[curveselection]].points[i].tension);
					xx = x-(curves[curveindex[curveselection]].points[i].x+curves[curveindex[curveselection]].points[i+1].x)*.5f*284.f;
					yy = y-(1-(curves[curveindex[curveselection]].points[i].y*(1-interp)+curves[curveindex[curveselection]].points[i+1].y*interp))*200.f;
					//tension
					if((xx*xx+yy*yy)<=37.1) return i*2+1+knobcount;
				}
			}
			//sliders
			if(x >= 193 && x <= 285 && y <= 202)
				for(int i = slidersvisible.size()-1; i >= 0; --i)
					if(y <= (202-i*font.line_height) && y > (202-(i+1)*font.line_height))
						return slidersvisible.size()-i-31;
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
	CJKFont = find_font("Microsoft YaHei|Noto Sans CJK JP|Droid Sans Fallback|Lantinghei TC|MS PGothic|MS Gothic|GB18030 Bitmap");
	ENGFont = find_font("Consolas|Noto Mono|DejaVu Sans Mono|Menlo|Andale Mono|SF Mono|Lucida Console|Liberation Mono");
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	if(ENGFont == "None")
		return Font(14.f*scale,Font::plain);
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
		g.fillRoundedRectangle(area.withSizeKeepingCentre(area.getWidth()-10*scale,scale).toFloat(),scale*.5f);
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
		if(CJKFont == "None")
			return Font(20.f*scale,Font::plain);
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
int LookNFeel::getPopupMenuBorderSize() {
	return (int)floor(2*scale);
}
void LookNFeel::getIdealPopupMenuItemSize(const String& text, const bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight)
{
	if(isSeparator) {
		idealWidth = 50*scale;
		idealHeight = 10*scale;
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
