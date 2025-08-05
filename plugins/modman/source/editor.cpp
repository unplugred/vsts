#include "processor.h"
#include "editor.h"

ModManAudioProcessorEditor::ModManAudioProcessorEditor(ModManAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 660, 330) {
	for(int i = 0; i < paramcount; ++i) {
		float dir = random.nextFloat()*6.2831853072f;
		float amt = random.nextFloat()*.2f;
		knobs[i].centerx = sin(dir)*amt+.5f;
		knobs[i].centery = cos(dir)*amt+.5f;
		knobs[i].id = params.pots[i].id;
		knobs[i].name = params.pots[i].name;
		if(i == (paramcount-1)) {
			knobs[i].value[0] = params.pots[i].normalize(state.masterspeed);
			knobs[i].defaultvalue[0] = params.pots[i].normalize(.5f);
			add_listener(knobs[i].id);
		} else {
			for(int m = 0; m < MC; ++m) {
				if(i == 1 && m == 0) {
					knobs[i].value[m] = 0;
					knobs[i].defaultvalue[m] = 0;
					continue;
				}
				knobs[i].value[m] = params.pots[i].normalize(state.values[m][i]);
				knobs[i].defaultvalue[m] = params.pots[i].normalize(params.modulators[m].defaults[i]);
				add_listener("m"+((String)m)+knobs[i].id);
			}
		}
		knobs[i].minimumvalue = params.pots[i].minimumvalue;
		knobs[i].maximumvalue = params.pots[i].maximumvalue;
		knobcount++;
	}
	selectedmodulator = params.selectedmodulator.get();
	for(int i = 0; i < MC; i++)
		curves[i] = state.curves[i];

	knobs[3].x = 229.f;
	knobs[3].y = 286.25f;
	knobs[4].x = 274.f;
	knobs[4].y = 235.25f;
	knobs[5].x = 322.5f;
	knobs[5].y = 276.75f;
	flowers[0].x = 607;
	flowers[0].y = 42;
	flowers[0].s = 1137;
	flowers[0].lx1 = 599;
	flowers[0].ly1 = 74;
	flowers[0].lx2 = 626;
	flowers[0].ly2 = 86;
	flowers[1].x = 538;
	flowers[1].y = 90;
	flowers[1].s = 1468;
	flowers[1].lx1 = 507;
	flowers[1].ly1 = 125;
	flowers[1].lx2 = 562;
	flowers[1].ly2 = 139.5;
	flowers[2].x = 581.5;
	flowers[2].y = 162;
	flowers[2].s = 656;
	flowers[2].lx1 = 549.5;
	flowers[2].ly1 = 183.5;
	flowers[2].lx2 = 619;
	flowers[2].ly2 = 208.5;
	flowers[3].x = 614;
	flowers[3].y = 242;
	flowers[3].s = 858;
	flowers[3].lx1 = 587;
	flowers[3].ly1 = 270.5;
	flowers[3].lx2 = 652;
	flowers[3].ly2 = 283;
	flowers[4].x = 536;
	flowers[4].y = 274.5;
	flowers[4].s = 1450;
	flowers[4].lx1 = 502.5;
	flowers[4].ly1 = 310.5;
	flowers[4].lx2 = 567.5;
	flowers[4].ly2 = 326;

	int prevoff = offnum;
	int prevon = onnum;
	offnum = floor(random.nextFloat()*2);
	onnum = floor(random.nextFloat()*2);
	if(offnum >= prevoff) offnum += 1;
	if(onnum >= prevon) onnum += 1;

	calcvis();

	originpos[0] = -178.5f/width;
	targetpos[0] = (random.nextFloat()-.5f)*.02f+.01f;
	originpos[1] = (random.nextFloat()-.5f)*.2f +.38f;
	targetpos[1] = (random.nextFloat()-.5f)*.05f+.38f;
	originpos[2] = (random.nextFloat()-.5f)*1.f;
	targetpos[2] = (random.nextFloat()-.5f)*.1f;

	setResizable(false,false);
	init(&look_n_feel);
}
ModManAudioProcessorEditor::~ModManAudioProcessorEditor() {
	close();
}

void ModManAudioProcessorEditor::newOpenGLContextCreated() {
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 pos;
uniform vec2 margin;
uniform vec2 index;
out vec2 uv;
void main() {
	gl_Position = vec4(vec2(aPos.x*pos.y+pos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = aPos*(1-margin*2)+margin;
	uv.y = (uv.y+index.x)/index.y;
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D basetex;
out vec4 fragColor;
void main() {
	fragColor = texture(basetex,uv);
})");

	knobshader = add_shader(
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform float knobrot;
uniform vec2 knobscale;
uniform vec2 knobpos;
uniform vec2 basepos;
uniform vec2 center;
uniform float banner;
out vec2 uv;
out vec2 baseuv;
out vec4 knobuv;
void main() {
	gl_Position = vec4((aPos*knobscale+knobpos)*.5+.5,0,1);
	baseuv = vec2((gl_Position.x+basepos.x)*basepos.y,gl_Position.y);
	gl_Position.xy = vec2(gl_Position.x,gl_Position.y*(1-banner)+banner)*2-1;
	uv = aPos.xy*2-1;
	knobuv = vec4(
		    ((aPos.x-center.x)*cos(knobrot)-(aPos.y-center.y     )*sin(knobrot))*5.2+.5,
		.95-((aPos.x-center.x)*sin(knobrot)+(aPos.y-center.y     )*cos(knobrot))       ,
		    ((aPos.x-center.x)*cos(knobrot)-(aPos.y-center.y-.035)*sin(knobrot))*5.2+.5,
		.95-((aPos.x-center.x)*sin(knobrot)+(aPos.y-center.y-.035)*cos(knobrot))       );
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
in vec4 knobuv;
in vec2 baseuv;
uniform sampler2D basetex;
uniform sampler2D knobtex;
out vec4 fragColor;
void main() {
	if((uv.x*uv.x+uv.y*uv.y) > 1 || ((knobuv.x < 0 || knobuv.x > 1 || knobuv.y > 1) && (knobuv.z < 0 || knobuv.z > 1 || knobuv.w > 1))) {
		fragColor = vec4(0);
	} else {
		vec3 base = texture(basetex,baseuv).rgb;
		fragColor = vec4(min(texture(knobtex,knobuv.xy).rgb,min(base+max(base*.6-texture(knobtex,knobuv.zw).rgb,0)*.5,vec3(.839,.839,.859))),1);
	}
})");

	flowersshader = add_shader(
//FLOWERS VERT
R"(#version 150 core
in vec2 aPos;
uniform vec3 pos;
uniform float banner;
uniform vec4 rot;
uniform float rot2;
out vec2 uv;
out vec4 flower1;
out vec4 flower2;
out vec4 flower3;
out vec4 flower4;
out vec4 flower5;
void main() {
	gl_Position = vec4(vec2(1-(1-aPos.x)*pos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = vec2(1-(1-aPos.x*pos.z)*pos.y,aPos.y);

	vec2 flowerpos = vec2(
		(aPos.x*2-244/175.)*(175/118.),
		(aPos.y*2-576/330.)*(330/106.));
	flower1 = vec4(vec2(
		flowerpos.x*cos(rot.x)-flowerpos.y*sin(rot.x),
		flowerpos.x*sin(rot.x)+flowerpos.y*cos(rot.x))*.5+.5,
		(aPos-vec2(91.5/175.,229.5/330.))/vec2(73/175.,38.5/330.));

	flowerpos = vec2(
		(aPos.x*2-106/175.)*(175/118.),
		(aPos.y*2-480/330.)*(330/106.));
	flower2 = vec4(vec2(
		flowerpos.x*cos(rot.y)-flowerpos.y*sin(rot.y),
		flowerpos.x*sin(rot.y)+flowerpos.y*cos(rot.y))*.5+.5,
		(aPos-vec2(13/175.,179/330.))/vec2(73/175.,38.5/330.));

	flowerpos = vec2(
		(aPos.x*2-193/175.)*(175/118.),
		(aPos.y*2-336/330.)*(330/106.));
	flower3 = vec4(vec2(
		flowerpos.x*cos(rot.z)-flowerpos.y*sin(rot.z),
		flowerpos.x*sin(rot.z)+flowerpos.y*cos(rot.z))*.5+.5,
		(aPos-vec2(63/175.,110/330.))/vec2(73/175.,38.5/330.));

	flowerpos = vec2(
		(aPos.x*2-258/175.)*(175/118.),
		(aPos.y*2-176/330.)*(330/106.));
	flower4 = vec4(vec2(
		flowerpos.x*cos(rot.w)-flowerpos.y*sin(rot.w),
		flowerpos.x*sin(rot.w)+flowerpos.y*cos(rot.w))*.5+.5,
		(aPos-vec2(98.5/175.,32.5/330.))/vec2(73/175.,38.5/330.));

	flowerpos = vec2(
		(aPos.x*2-102/175.)*(175/118.),
		(aPos.y*2-111/330.)*(330/106.));
	flower5 = vec4(vec2(
		flowerpos.x*cos(rot2)-flowerpos.y*sin(rot2),
		flowerpos.x*sin(rot2)+flowerpos.y*cos(rot2))*.5+.5,
		(aPos-vec2(13/175.,-7.5/330.))/vec2(73/175.,38.5/330.));

})",
//FLOWERS FRAG
R"(#version 150 core
in vec2 uv;
in vec4 flower1;
in vec4 flower2;
in vec4 flower3;
in vec4 flower4;
in vec4 flower5;
uniform sampler2D basetex;
uniform sampler2D flowerstex;
uniform sampler2D labelstex;
uniform int selected;
out vec4 fragColor;
void main() {
	vec3 col = texture(basetex,uv).rgb;

	if(flower1.x >= 0 && flower1.x <= 1 && flower1.y >= 0 && flower1.y <= 1)
		col = min(col,texture(flowerstex,vec2(flower1.x,(flower1.y+4)/5)).rgb);
	if(flower2.x >= 0 && flower2.x <= 1 && flower2.y >= 0 && flower2.y <= 1)
		col = min(col,texture(flowerstex,vec2(flower2.x,(flower2.y+3)/5)).rgb);
	if(flower3.x >= 0 && flower3.x <= 1 && flower3.y >= 0 && flower3.y <= 1)
		col = min(col,texture(flowerstex,vec2(flower3.x,(flower3.y+2)/5)).rgb);
	if(flower4.x >= 0 && flower4.x <= 1 && flower4.y >= 0 && flower4.y <= 1)
		col = min(col,texture(flowerstex,vec2(flower4.x,(flower4.y+1)/5)).rgb);
	if(flower5.x >= 0 && flower5.x <= 1 && flower5.y >= 0 && flower5.y <= 1)
		col = min(col,texture(flowerstex,vec2(flower5.x,(flower5.y  )/5)).rgb);

	if(flower1.z >= 0 && flower1.z <= 1 && flower1.w >= 0 && flower1.w <= 1)
		col = min(col,texture(labelstex,vec2(flower1.z,(flower1.w+4)/10+(selected==0?0:.5))).rgb);
	if(flower2.z >= 0 && flower2.z <= 1 && flower2.w >= 0 && flower2.w <= 1)
		col = min(col,texture(labelstex,vec2(flower2.z,(flower2.w+3)/10+(selected==1?0:.5))).rgb);
	if(flower3.z >= 0 && flower3.z <= 1 && flower3.w >= 0 && flower3.w <= 1)
		col = min(col,texture(labelstex,vec2(flower3.z,(flower3.w+2)/10+(selected==2?0:.5))).rgb);
	if(flower4.z >= 0 && flower4.z <= 1 && flower4.w >= 0 && flower4.w <= 1)
		col = min(col,texture(labelstex,vec2(flower4.z,(flower4.w+1)/10+(selected==3?0:.5))).rgb);
	if(flower5.z >= 0 && flower5.z <= 1 && flower5.w >= 0 && flower5.w <= 1)
		col = min(col,texture(labelstex,vec2(flower5.z,(flower5.w  )/10+(selected==4?0:.5))).rgb);

	fragColor = vec4(col,1);
})");

	tackshader = add_shader(
//TACK VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 pos;
uniform vec2 size;
uniform vec2 basepos;
uniform vec4 letterpos;
uniform vec2 index;
out vec2 uv;
out vec2 baseuv;
out vec2 letteruv;
void main() {
	gl_Position = vec4(aPos*size.xy+pos.xy,0,1);
	baseuv = vec2((gl_Position.x+basepos.x)*basepos.y,gl_Position.y);
	gl_Position.xy = vec2(gl_Position.x,gl_Position.y*(1-banner)+banner)*2-1;
	uv = vec2(aPos.x,(aPos.y+index.x)/index.y);
	letteruv = (aPos.xy-letterpos.yw)/letterpos.xz;
})",
//TACK FRAG
R"(#version 150 core
in vec2 uv;
in vec2 baseuv;
in vec2 letteruv;
uniform sampler2D basetex;
uniform sampler2D tackstex;
uniform sampler2D numberstex;
uniform vec4 letters;
out vec4 fragColor;
void main() {
	vec3 numbers = vec3(1);
	vec3 numbershighlight = vec3(1);
	if(letteruv.x >= 0 && letteruv.x <= 4 && letteruv.y >= 0 && letteruv.y <= 1) {
		float letterindex = 0;
		if(letteruv.x < 1) letterindex = letters.x; else
		if(letteruv.x < 2) letterindex = letters.y; else
		if(letteruv.x < 3) letterindex = letters.z; else
		                   letterindex = letters.w;
		numbers              = texture(numberstex,vec2((mod(letteruv.x,1)+letterindex)/13,letteruv.y    )).rgb;
		if(letteruv.y >= .08)
			numbershighlight = texture(numberstex,vec2((mod(letteruv.x,1)+letterindex)/13,letteruv.y-.09)).rgb;
	}
	vec3 tack = texture(tackstex,uv).rgb;
	fragColor = vec4(min(numbers,min(texture(basetex,baseuv).rgb,min(tack+max(tack*.6-numbershighlight,0)*.5,vec3(.839,.839,.859)))),1);
})");

	cubershader = add_shader(
//CUBER VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec4 pos;
uniform vec2 modulation;
uniform vec2 basepos;
uniform float size;
out vec3 uv;
out vec2 baseuv;
void main() {
	gl_Position = vec4(aPos.x*pos.x+pos.y,aPos.y,0,1);
	baseuv = vec2((gl_Position.x+basepos.x)*basepos.y,gl_Position.y);
	gl_Position.xy = vec2(gl_Position.x,gl_Position.y*(1-banner)+banner)*2-1;
	uv = vec3(aPos.x*pos.w-size,(modulation+aPos.y)*pos.z);
})",
//CUBER FRAG
R"(#version 150 core
in vec3 uv;
in vec2 baseuv;
uniform sampler2D basetex;
uniform sampler2D cubertex;
uniform vec2 index;
out vec4 fragColor;
void main() {
	vec3 col = vec3(1);
	if(uv.y >= 0 && uv.y <= 1)
		col = texture(cubertex,vec2(uv.x,(uv.y+index.x)/index.y)).rgb;
	if(uv.z >= 0 && uv.z <= 1)
		col = min(col,texture(cubertex,vec2(uv.x,(uv.z+index.x)/index.y)).rgb);
	if(col.r >= .838 && col.g >= .838 && col.b >= .858)
		fragColor = vec4(.839,.839,.859,0);
	else
		fragColor = vec4(min(col,texture(basetex,baseuv).rgb),1);
})");

	onoffshader = add_shader(
//ON OFF VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 pos;
uniform vec2 size;
uniform vec2 basepos;
uniform vec2 index;
out vec2 uv;
out vec2 baseuv;
void main() {
	gl_Position = vec4(aPos*size.xy+pos.xy,0,1);
	baseuv = vec2((gl_Position.x+basepos.x)*basepos.y,gl_Position.y);
	gl_Position.xy = vec2(gl_Position.x,gl_Position.y*(1-banner)+banner)*2-1;
	uv = vec2(aPos.x,(aPos.y+index.x)/index.y);
})",
//ON OFF FRAG
R"(#version 150 core
in vec2 uv;
in vec2 baseuv;
uniform sampler2D basetex;
uniform sampler2D onofftex;
out vec4 fragColor;
void main() {
	fragColor = vec4(min(texture(basetex,baseuv).rgb,texture(onofftex,uv).rgb),1);
})");

	logoshader = add_shader(
//LOGO VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec4 pos;
uniform vec4 posuv;
uniform float rot;
uniform float ratio;
out vec2 uv;
out vec2 alphauv;
void main() {
	gl_Position = vec4(aPos.x-.5,(aPos.y-.5)*ratio,0,1);
	gl_Position.xy = (vec2(
		 gl_Position.x*cos(rot)-gl_Position.y*sin(rot),
		(gl_Position.x*sin(rot)+gl_Position.y*cos(rot))/ratio)+.5)*pos.xy+pos.zw;
	gl_Position.xy = vec2(gl_Position.x,gl_Position.y*(1-banner)+banner)*2-1;
	uv = aPos*posuv.xy+posuv.zw;
	alphauv = aPos;
})",
//LOGO FRAG
R"(#version 150 core
in vec2 uv;
in vec2 alphauv;
uniform sampler2D logotex;
uniform sampler2D logoalphatex;
uniform float index;
out vec4 fragColor;
void main() {
	fragColor = vec4(texture(logotex,(max(min(uv,1),0)+vec2(0,9-index))*vec2(1,.1)).rgb,texture(logoalphatex,alphauv).r);
})");

	lineshader = add_shader(
//LINE VERT
R"(#version 150 core
in vec4 aPos;
uniform float banner;
uniform float white;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,(aPos.y+white*.003)*(1-banner)+banner)*2-1,0,1);
	uv = aPos.zw;
})",
//LINE FRAG
R"(#version 150 core
in vec2 uv;
uniform float dpi;
uniform float white;
uniform sampler2D linetex;
out vec4 fragColor;
void main(){
	fragColor = texture(linetex,uv);
	if(white > .5)
		fragColor = vec4(.839,.839,.859,1.)*.8*fragColor.a;
})");

	dotsshader = add_shader(
//DOTS VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 pos;
uniform vec2 size;
uniform float index;
out vec2 uv;
void main() {
	gl_Position = vec4(vec2(aPos.x*size.x+pos.x,(aPos.y*size.y+pos.y)*(1-banner)+banner)*2-1,0,1);
	uv = vec2(aPos.x,(aPos.y+index)/6);
})",
//DOTS FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D dotstex;
out vec4 fragColor;
void main() {
	fragColor = texture(dotstex,uv);
})");

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize);
	add_texture(&bannertex, BinaryData::banner_png, BinaryData::banner_pngSize);
	add_texture(&flowerstex, BinaryData::flowers_png, BinaryData::flowers_pngSize);
	add_texture(&labelstex, BinaryData::labels_png, BinaryData::labels_pngSize);
	add_texture(&tackstex, BinaryData::tacks_png, BinaryData::tacks_pngSize);
	add_texture(&numberstex, BinaryData::numbers_png, BinaryData::numbers_pngSize);
	add_texture(&cubertex, BinaryData::cuber_png, BinaryData::cuber_pngSize);
	add_texture(&onofftex, BinaryData::onoff_png, BinaryData::onoff_pngSize);
	add_texture(&knobtex, BinaryData::knob_png, BinaryData::knob_pngSize);
	add_texture(&logotex, BinaryData::logo_png, BinaryData::logo_pngSize);
	add_texture(&logoalphatex, BinaryData::logoalpha_png, BinaryData::logoalpha_pngSize);
	add_texture(&linetex, BinaryData::line_png, BinaryData::line_pngSize);
	add_texture(&dotstex, BinaryData::dots_png, BinaryData::dots_pngSize);

	draw_init();
}
void ModManAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	OpenGLHelpers::clear(Colour::fromRGB(214,214,219));

	// banner
	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	bannertex.bind();
	baseshader->setUniform("basetex",0);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("pos",0.f,200.f/width);
	baseshader->setUniform("margin",.5f/width,.5f/height);
	baseshader->setUniform("index",4.f-selectedmodulator,5.f);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	// base
	basetex.bind();
	baseshader->setUniform("pos",200.f/width,295.f/width);
	baseshader->setUniform("margin",0.f,0.f);
	baseshader->setUniform("index",0.f,1.f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	// flowers
	flowersshader->use();
	flowersshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	flowerstex.bind();
	flowersshader->setUniform("flowerstex",1);
	context.extensions.glActiveTexture(GL_TEXTURE2);
	labelstex.bind();
	flowersshader->setUniform("labelstex",2);
	flowersshader->setUniform("banner",banner_offset);
	flowersshader->setUniform("pos",175.f/width,10.f/295.f,175.f/10.f);
	flowersshader->setUniform("rot",flowers[0].rot,flowers[1].rot,flowers[2].rot,flowers[3].rot);
	flowersshader->setUniform("rot2",flowers[4].rot);
	flowersshader->setUniform("selected",selectedmodulator);
	coord = context.extensions.glGetAttribLocation(flowersshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	// knobs
	knobshader->use();
	knobshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	knobtex.bind();
	knobshader->setUniform("knobtex",1);
	knobshader->setUniform("basepos",-200.f/width,width/295.f);
	knobshader->setUniform("knobscale",44.f*2.f/width,39.5f*2.f/height);
	knobshader->setUniform("banner",banner_offset);
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 3; i < knobcount; i++) {
		knobshader->setUniform("knobpos",((float)knobs[i].x*2-44.f)/width-1,1-((float)knobs[i].y*2+39.5f)/height);
		float angle = (knobs[i].value[i==(knobcount-1)?0:selectedmodulator]-.5f)*5.5f;
		knobshader->setUniform("knobrot",(float)std::atan2(sin(angle)-(knobs[i].centerx*2-1),cos(angle)-(knobs[i].centery*2-1)));
		knobshader->setUniform("center",knobs[i].centerx,knobs[i].centery);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	// tacks
	tackshader->use();
	tackshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	tackstex.bind();
	tackshader->setUniform("tackstex",1);
	context.extensions.glActiveTexture(GL_TEXTURE2);
	numberstex.bind();
	tackshader->setUniform("numberstex",2);
	tackshader->setUniform("banner",banner_offset);
	tackshader->setUniform("basepos",-200.f/width,width/295.f);
	tackshader->setUniform("letterpos",7/48.5f,5.5f/48.5f,17/33.f,8/33.f);
	tackshader->setUniform("size",48.5f/width,33.f/height);
	coord = context.extensions.glGetAttribLocation(tackshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < 2; ++i) {
		if(i == 0 && (selectedmodulator == 0 || (selectedmodulator == 2 && knobs[0].value[2] < .5)))
			continue;

		float val = knobs[i+1].value[selectedmodulator];
		bool ms = false;
		if(selectedmodulator == 0) {
			val = val*MAX_DRIFT;
			if(val < .1) {
				ms = true;
				val *= 1000;
			}
		} else if(selectedmodulator == 1) {
			val = mapToLog10(val,20.f,20000.f)/1000.f;
		} else if(selectedmodulator == 2) {
			val = mapToLog10(val,0.1f,40.f);
		} else {
			val *= 10;
		}

		float l1, l2, l3, l4;
		if(val < 1) {
			l1 =                10       ;
			l2 =      floor(val*10  )    ;
			l3 = fmod(floor(val*100 ),10);
			l4 = fmod(floor(val*1000),10);
		} else if(val < 10) {
			l1 =      floor(val     )    ;
			l2 =                10       ;
			l3 = fmod(floor(val*10  ),10);
			l4 = fmod(floor(val*100 ),10);
		} else if(val < 100) {
			l1 =      floor(val/10  )    ;
			l2 = fmod(floor(val     ),10);
			l3 =                10       ;
			l4 = fmod(floor(val*10  ),10);
		} else if(val < 1000) {
			l1 =      floor(val/100 )    ;
			l2 = fmod(floor(val/10  ),10);
			l3 = fmod(floor(val     ),10);
			l4 =                10       ;
		} else {
			l1 =      floor(val/1000)    ;
			l2 = fmod(floor(val/100 ),10);
			l3 = fmod(floor(val/10  ),10);
			l4 = fmod(floor(val     ),10);
		}
		if(selectedmodulator == 0) {
			if(ms) l3 = 11;
			l4 = 12;
		}

		tackshader->setUniform("pos",.597f,tackpos[i]*.895f-.015f);
		tackshader->setUniform("index",(float)i,2.f);
		tackshader->setUniform("letters",l1,l2,l3,l4);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	// cubers
	if(cubersize > 0) {
		cubershader->use();
		cubershader->setUniform("basetex",0);
		context.extensions.glActiveTexture(GL_TEXTURE1);
		cubertex.bind();
		cubershader->setUniform("cubertex",1);
		cubershader->setUniform("banner",banner_offset);
		cubershader->setUniform("basepos",-200.f/width,width/295.f);
		cubershader->setUniform("pos",14.f/width,.647f,height/12.5f,14.f/12.5f);
		cubershader->setUniform("size",1-cubersize);
		cubershader->setUniform("modulation",-cuberpos[0]*.867f,-cuberpos[1]*.867f);
		cubershader->setUniform("index",floor(cuberindex),6.f);
		coord = context.extensions.glGetAttribLocation(cubershader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	// on off
	onoffshader->use();
	onoffshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	onofftex.bind();
	onoffshader->setUniform("onofftex",1);
	onoffshader->setUniform("banner",banner_offset);
	onoffshader->setUniform("basepos",-200.f/width,width/295.f);
	onoffshader->setUniform("size",76.5f/width,47.5f/height);
	onoffshader->setUniform("pos",.48f,.23f);
	onoffshader->setUniform("index",(float)(knobs[0].value[selectedmodulator]>.5?onnum:(offnum+3)),6.f);
	coord = context.extensions.glGetAttribLocation(onoffshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	// logo
	logoshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	logotex.bind();
	logoshader->setUniform("logotex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	logoalphatex.bind();
	logoshader->setUniform("logoalphatex",1);
	logoshader->setUniform("banner",banner_offset);
	logoshader->setUniform("pos",178.5f/width,76.f/height,
			originpos[0]*(1-logoease)+targetpos[0]*logoease,
			originpos[1]*(1-logoease)+targetpos[1]*logoease);
	logoshader->setUniform("rot",
			originpos[2]*(1-logoease)+targetpos[2]*logoease);
	logoshader->setUniform("ratio",.5f);
	logoshader->setUniform("posuv",178.5f/148.5f,76.f/46.5f,-.106f,-.305f);
	logoshader->setUniform("index",(float)fmin(9,fmax(0,floor(websiteht))));
	coord = context.extensions.glGetAttribLocation(logoshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	// line
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	lineshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	linetex.bind();
	lineshader->setUniform("linetex",0);
	coord = context.extensions.glGetAttribLocation(lineshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,4,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2880, visline, GL_DYNAMIC_DRAW);
	lineshader->setUniform("banner",banner_offset);
	lineshader->setUniform("basepos",-200.f/width,width/295.f);
	lineshader->setUniform("dpi",scaled_dpi);
	lineshader->setUniform("white",1.f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,720);
	lineshader->setUniform("white",0.f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,720);
	context.extensions.glDisableVertexAttribArray(coord);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	// tension points
	dotsshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	dotstex.bind();
	dotsshader->setUniform("dotstex",0);
	coord = context.extensions.glGetAttribLocation(dotsshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	dotsshader->setUniform("banner",banner_offset);
	dotsshader->setUniform("size",11.f/width,10.f/height);
	int nextpoint = 0;
	for(int i = 0; i < (curves[selectedmodulator].points.size()-1); ++i) {
		if(!curves[selectedmodulator].points[i].enabled) continue;
		++nextpoint;
		while(!curves[selectedmodulator].points[nextpoint].enabled) ++nextpoint;
		if(((curves[selectedmodulator].points[nextpoint].x-curves[selectedmodulator].points[i].x)*1.42f) <= .02 || fabs(curves[selectedmodulator].points[nextpoint].y-curves[selectedmodulator].points[i].y) <= .02) continue;
		double interp = curve::calctension(.5,curves[selectedmodulator].points[i].tension);
		dotsshader->setUniform("pos",
				((curves[selectedmodulator].points[i].x           +curves[selectedmodulator].points[nextpoint].x       )*.5f*180.f+208.f-5.5f)/width ,
				((curves[selectedmodulator].points[i].y*(1-interp)+curves[selectedmodulator].points[nextpoint].y*interp)    *163.f+128.f-5.f )/height);
		dotsshader->setUniform("index",(float)fmod((float)i,3));
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}

	// control points
	for(int i = 0; i < curves[selectedmodulator].points.size(); ++i) {
		if(!curves[selectedmodulator].points[i].enabled) continue;
		dotsshader->setUniform("pos",
			(curves[selectedmodulator].points[i].x*180.f+208.f-5.5f)/width,
			(curves[selectedmodulator].points[i].y*163.f+128.f-5.f)/height);
		dotsshader->setUniform("index",(float)fmod((float)i,3)+3.f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}

	draw_end();
}
void ModManAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}

void ModManAudioProcessorEditor::gennoise(float distance, float amount, float tension) {
	int numpoints = ceil((.5f/distance)+.9f)*2-1;
	float startdist = (1-distance*(numpoints-3))*.5f;
	if(curves[selectedmodulator].points.size() != numpoints)
		curves[selectedmodulator].points.resize(numpoints);
	for(int i = 0; i < numpoints; ++i) {
		if(i == 0)
			curves[selectedmodulator].points[i].x = 0;
		else if(i == (curves[selectedmodulator].points.size()-1))
			curves[selectedmodulator].points[i].x = 1;
		else
			curves[selectedmodulator].points[i].x = (i-1)*distance+startdist;
		curves[selectedmodulator].points[i].y = noisetable[(21-numpoints)/2+i]*amount+curves[selectedmodulator].points[i].x*(1-amount);
		curves[selectedmodulator].points[i].tension = tension;
	}
}
void ModManAudioProcessorEditor::calcvis() {
	curveiterator iterator;
	iterator.reset(curves[selectedmodulator],360);
	double prevy = 128+iterator.next()*163;
	double currenty = prevy;
	double nexty = prevy;
	double dist = 0;
	for(int i = 0; i < 360; i++) {
		nexty = 128+iterator.next()*163;
		dist += (fabs(currenty-prevy)+1)/253.f;
		double angle1 = std::atan2(currenty-prevy,.5)+1.5707963268;
		double angle2 = std::atan2(nexty-currenty,.5)+1.5707963268;
		double angle = (angle1+angle2)*.5;
		if(fabs(angle1-angle2)>1) {
			if(fabs(currenty-prevy) > fabs(nexty-currenty))
				angle = angle1;
			else
				angle = angle2;
		}
		visline[i*8  ] = (i*.5+208+cos(angle)*2.25f)/width;
		visline[i*8+4] = (i*.5+208-cos(angle)*2.25f)/width;
		visline[i*8+1] = (currenty+sin(angle)*2.025f)/height;
		visline[i*8+5] = (currenty-sin(angle)*2.025f)/height;
		visline[i*8+2] = 0.f;
		visline[i*8+6] = 1.f;
		visline[i*8+3] = dist;
		visline[i*8+7] = dist;
		prevy = currenty;
		currenty = nexty;
	}
}
void ModManAudioProcessorEditor::paint(Graphics& g) { }

void ModManAudioProcessorEditor::timerCallback() {
	if(logolerp != ((hover<=-14&&hover>=-15)?1:0)) {
		logolerp = fmax(fmin(logolerp+((hover<=-14&&hover>=-15)?.13f:-.13f),1),0);
		logoease = 1-(1-logolerp)*(1-logolerp);
		if(logolerp <= 0) {
			originpos[0] = -178.5f/width;
			targetpos[0] = (random.nextFloat()-.5f)*.02f+.01f;
			originpos[1] = (random.nextFloat()-.5f)*.2f +.38f;
			targetpos[1] = (random.nextFloat()-.5f)*.05f+.38f;
			originpos[2] = (random.nextFloat()-.5f)*1.f;
			targetpos[2] = (random.nextFloat()-.5f)*.1f;
			websiteht = 0;
		}
	}
	if(websiteht > 0 && logolerp >= 1) {
		websiteht -= .5f;
		if(fmod(websiteht,1.f) < .1f) {
			targetpos[0] += (random.nextFloat()-.5f)*.001f;
			targetpos[1] += (random.nextFloat()-.5f)*.002f;
			targetpos[2] += (random.nextFloat()-.5f)*.01f;
		}
	}

	if(framecount++ >= 1) {
		float val = 0;
		framecount = 0;
		for(int i = 0; i < MC; ++i)
			if(knobs[0].value[i] > .5 && (i != 2 || knobs[0].value[1] > .5))
				flowers[i].rot = round(audio_processor.flower_rot[i].get()*10*10)/10;
		for(int i = 0; i < 2; ++i) {
			val = knobs[i+1].value[selectedmodulator];
			if(i == 0) val = fmin(val,knobs[2].value[selectedmodulator]);
			if(selectedmodulator != 0 && (selectedmodulator != 2 || knobs[0].value[2] > .5))
				val = val*.9f+i*.1f;
			else if(i == 1) val = val*.97f+.03f;
			else val = -.07f;
			tackpos[i] = tackpos[i]*.3f+val*.7f;

			val = knobs[i+1].value[selectedmodulator];
			if(selectedmodulator != 0 && (selectedmodulator != 2 || knobs[0].value[2] > .5))
				val = val*.93f+.07f;
			cuberposclamp[i] = cuberposclamp[i]*.3f+val*.7f;
		}
		bool ison = knobs[0].value[selectedmodulator] > .5 && (selectedmodulator != 2 || knobs[0].value[1] > .5);
		for(int i = 0; i < 2; ++i) {
			if(ison) {
				val = audio_processor.cuber_rot[i].get();
				if(selectedmodulator != 0) val = val*.93f+.07f;
			} else {
				val = cuberpos[i];
			}
			cuberpos[i] = fmin(fmax(cuberpos[i]*.3f+val*.7f,cuberposclamp[0]),cuberposclamp[1]);
		}
		cubersize = cubersize*.5f+(ison?1:0)*.5f;
	}
	cuberindex = fmod(cuberindex+.3333f,6);

	if(audio_processor.updatevis.get()) {
		for(int i = 0; i < MC; i++)
			curves[i] = audio_processor.presets[audio_processor.currentpreset].curves[i];
		calcvis();
		audio_processor.updatevis = false;
	} else if(updatetempvis) calcvis();
	updatetempvis = false;

	update();
}

void ModManAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == knobs[knobcount-1].id) {
		knobs[knobcount-1].value[0] = knobs[knobcount-1].normalize(newValue);
		return;
	}
	for(int m = 0; m < MC; ++m) {
		for(int i = 0; i < (knobcount-1); i++) {
			if(parameterID == ("m"+((String)m)+knobs[i].id)) {
				knobs[i].value[m] = knobs[i].normalize(newValue);
				return;
			}
		}
	}
}
void ModManAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == -15 && prevhover != -15 && websiteht <= 0) websiteht = 9.5f;
	else if(hover > -14 && prevhover <= -14) websiteht = 0.f;
}
void ModManAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void ModManAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(dpi < 0) return;
	if(event.mods.isRightButtonDown()) {
		hover = recalc_hover(event.x,event.y);
		std::unique_ptr<PopupMenu> rightclickmenu(new PopupMenu());
		std::unique_ptr<PopupMenu> scalemenu(new PopupMenu());

		int i = 20;
		while(++i < (ui_scales.size()+21))
			scalemenu->addItem(i,(String)round(ui_scales[i-21]*100)+"%",true,(i-21)==ui_scale_index);

		rightclickmenu->setLookAndFeel(&look_n_feel);
		if(hover == -13 || hover >= knobcount) {
			rightclickmenu->addItem(1,"'copy curve",true);
			rightclickmenu->addItem(2,"'paste curve",curve::isvalidcurvestring(SystemClipboard::getTextFromClipboard()));
			rightclickmenu->addItem(3,"'reset curve",true);
			rightclickmenu->addSeparator();
		}
		rightclickmenu->addItem(4,"'copy preset",true);
		rightclickmenu->addItem(5,"'paste preset",audio_processor.is_valid_preset_string(SystemClipboard::getTextFromClipboard()));
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu("'scale",*scalemenu);

		String description = "";
		if(hover != -1) {
			if(hover == -15)
				description = "made with love by mel :)";
			else if(hover == -14 || (hover >= -6 && hover <= -2)) {
				int modnum = hover==-14?selectedmodulator:(hover+6);
				     if(modnum == 0)
					description = "tape drift. results in changes in the pitch and speed of the incoming audio.";
				else if(modnum == 1)
					description = "low pass cutoff. results in changes in the amount of high frequencies of the incoming audio.";
				else if(modnum == 2)
					description = "low pass resonance. only works if the low pass modulator is on. when off, use the range selector to select a static resonance.";
				else if(modnum == 3)
					description = "saturation. results in changes in the amount of harmonics of the incoming audio.";
				else if(modnum == 4)
					description = "amplitude. results in changes in the loudness of the incoming audio. can also alter the panning when increasing the stereo knob.";
			} else if(hover == 0)
				description = "enables/disables the modulator";
			else if(hover == 1)
				description = "minimum range for the modulation";
			else if(hover == 2)
				description = "maximum range for the modulation";
			else if(hover == 3)
				description = "modulation speed";
			else if(hover == 4)
				description = "stereo separation between channels in modulator";
			else if(hover == 5)
				description = "global speed multiplier for all modulators. this parameter is shared across all modulators.";
			description = look_n_feel.add_line_breaks(description);
		}
		if(description != "") {
			rightclickmenu->addSeparator();
			rightclickmenu->addItem(-1,"'"+description,false);
		}

		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20)
				set_ui_scale(result-21);
			else if(result == 1) //copy curve
				SystemClipboard::copyTextToClipboard(audio_processor.curvetostring());
			else if(result == 2) //paste curve
				audio_processor.curvefromstring(SystemClipboard::getTextFromClipboard());
			else if(result == 3) //reset curve
				audio_processor.resetcurve();
			else if(result == 4) //copy preset
				SystemClipboard::copyTextToClipboard(audio_processor.get_preset(audio_processor.currentpreset));
			else if(result == 5) //paste preset
				audio_processor.set_preset(SystemClipboard::getTextFromClipboard(), audio_processor.currentpreset);
		});
		return;
	}

	initialdrag = hover;
	if(hover >= -12 && hover <= -7) {
		if(hover == -12) {
			audio_processor.curvefromstring("2,0,0,0.5,1,1,0.5");
			audio_processor.undo_manager.setCurrentTransactionName("Set preset");
			audio_processor.undo_manager.beginNewTransaction();
		} else {
			valueoffset[0] = 0;
			audio_processor.undo_manager.beginNewTransaction();
			dragpos = event.getScreenPosition();
			event.source.enableUnboundedMouseMovement(true);
			if(hover == -10 || hover == -7) {
				valueoffset[1] = 0;
				initialvalue[0] = .3f;
				initialvalue[1] = .3f;

				for(int i = 0; i < 21; ++i)
					noisetable[i] = random.nextFloat();
				gennoise(powf(.3f,2)*.45f+.05f,.3f,hover==-10?1.f:.5f);
			} else if(hover == -11) {
				initialvalue[0] = .15f;
				curves[selectedmodulator] = curve("3,0,0,0.15,0.5,1,0.85,1,0,0.5");
			} else if(hover == -9) {
				initialvalue[0] = .7f;
				curves[selectedmodulator] = curve("3,0,0,0.85,0.5,0.5,0.15,1,1,0.5");
			} else if(hover == -8) {
				initialvalue[0] = .7f;
				curves[selectedmodulator] = curve("3,0,0,0.15,0.5,0.5,0.85,1,1,0.5");
			}
			updatetempvis = true;
		}
	} else if(hover < -1 && hover >= (-1-MC)) {
		selectedmodulator = hover+MC+1;
		if(audio_processor.params.selectedmodulator.get() != selectedmodulator) {
			audio_processor.params.selectedmodulator = selectedmodulator;
			for(int i = 0; i < knobcount; ++i) {
				float dir = random.nextFloat()*6.2831853072f;
				float amt = random.nextFloat()*.2f;
				knobs[i].centerx = sin(dir)*amt+.5f;
				knobs[i].centery = cos(dir)*amt+.5f;
			}
			int prevoff = offnum;
			int prevon = onnum;
			offnum = floor(random.nextFloat()*2);
			onnum = floor(random.nextFloat()*2);
			if(offnum >= prevoff) offnum += 1;
			if(onnum >= prevon) onnum += 1;

			updatetempvis = true;
		}
	} else if(hover == 0) {
		audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(1-knobs[hover].value[selectedmodulator]);
		audio_processor.undo_manager.setCurrentTransactionName(knobs[hover].value[selectedmodulator]>.5f?"turned modulator off":"turned modulator on");
		audio_processor.undo_manager.beginNewTransaction();
	} else if(hover >= knobcount) {
		valueoffset[0] = 0;
		audio_processor.undo_manager.beginNewTransaction();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
		int i = round(initialdrag-knobcount);
		if((i%2) == 0) {
			i /= 2;
			initialvalue[0] = curves[selectedmodulator].points[i].x;
			initialvalue[1] = curves[selectedmodulator].points[i].y;
			initialdotvalue[0] = initialvalue[0];
			initialdotvalue[1] = initialvalue[1];
			valueoffset[1] = 0;
		} else {
			i = (i-1)/2;
			initialvalue[0] = curves[selectedmodulator].points[i].tension;
			initialdotvalue[0] = initialvalue[0];
		}
	} else if(hover > 0) {
		valueoffset[0] = 0;
		audio_processor.undo_manager.beginNewTransaction();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
		initialvalue[0] = knobs[hover].value[hover==(knobcount-1)?0:selectedmodulator];
		if(hover == (knobcount-1))
			audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		else
			audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->beginChangeGesture();
	}
}
void ModManAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(hover == -1) return;

	if(hover >= -11 && hover <= -7) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			if(hover == -10 || hover == -7) {
				initialvalue[0] -= event.getDistanceFromDragStartY()*.0045f;
				initialvalue[1] += event.getDistanceFromDragStartX()*.0045f;
			} else {
				initialvalue[0] -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
			}
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			if(hover == -10 || hover == -7) {
				initialvalue[0] += event.getDistanceFromDragStartY()*.0045f;
				initialvalue[1] -= event.getDistanceFromDragStartX()*.0045f;
			} else {
				initialvalue[0] += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
			}
		}

		float valuey = 0;
		float valuex = 0;
		if(hover == -10 || hover == -7) {
			valuey = initialvalue[0]-event.getDistanceFromDragStartY()*(finemode?.0005f:.005f);
			valuex = initialvalue[1]+event.getDistanceFromDragStartX()*(finemode?.0005f:.005f);
			gennoise(powf(fmin(fmax(valuex-valueoffset[1],0),1),2)*.45f+.05f,fmin(fmax(valuey-valueoffset[0],0),1),hover==-10?1.f:.5f);
			valueoffset[1] = fmax(fmin(valueoffset[1],valuex+.1f),valuex-1.1f);
		} else {
			valuey = initialvalue[0]-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
			if(hover == -11) {
				curves[selectedmodulator].points[0].tension = fmin(fmax(valuey-valueoffset[0],0),1);
				curves[selectedmodulator].points[1].tension = 1-curves[selectedmodulator].points[0].tension;
			} else if(hover == -9) {
				curves[selectedmodulator].points[0].tension = fmin(fmax(valuey-valueoffset[0],0),1)*.5+.5;
				curves[selectedmodulator].points[1].tension = 1-curves[selectedmodulator].points[0].tension;
			} else if(hover == -8) {
				curves[selectedmodulator].points[1].tension = fmin(fmax(valuey-valueoffset[0],0),1)*.5+.5;
				curves[selectedmodulator].points[0].tension = 1-curves[selectedmodulator].points[1].tension;
			}
		}
		valueoffset[0] = fmax(fmin(valueoffset[0],valuey+.1f),valuey-1.1f);
		updatetempvis = true;
	} else if(initialdrag >= knobcount) {
		float dragspeed = 1.f/(180*ui_scales[ui_scale_index]);
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

			if(i > 0 && i < (curves[selectedmodulator].points.size()-1)) {
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

				if(event.mods.isShiftDown()) { // free mode
					if((i > 1 && pointx < curves[selectedmodulator].points[i-1].x) || (i < (curves[selectedmodulator].points.size()-2) && pointx > curves[selectedmodulator].points[i+1].x)) {
						point pnt = curves[selectedmodulator].points[i];
						int n = 1;
						for(n = 1; n < (curves[selectedmodulator].points.size()-1); ++n) //finding new position
							if(pnt.x < curves[selectedmodulator].points[n].x) break;
						if(n > i) { // move points back
							n--;
							for(int f = i; f <= n; f++)
								curves[selectedmodulator].points[f] = curves[selectedmodulator].points[f+1];
						} else { //move points forward
							for(int f = i; f >= n; f--)
								curves[selectedmodulator].points[f] = curves[selectedmodulator].points[f-1];
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
				if(!event.mods.isShiftDown()) { // no free mode
					if(i > 1) {
						xleft = curves[selectedmodulator].points[i-1].x;
						if(pointx <= curves[selectedmodulator].points[i-1].x) {
							pointx = curves[selectedmodulator].points[i-1].x;
							clamppedx = true;
						}
					}
					if(i < (curves[selectedmodulator].points.size()-2)) {
						xright = curves[selectedmodulator].points[i+1].x;
						if(pointx >= curves[selectedmodulator].points[i+1].x) {
							pointx = curves[selectedmodulator].points[i+1].x;
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
				curves[selectedmodulator].points[i].enabled = (fabs(amioutofbounds[0])+fabs(amioutofbounds[1])) < .8;

				curves[selectedmodulator].points[i].x = pointx;
				if(axislock != 2)
					valueoffset[0] = fmax(fmin(valueoffset[0],valuex-xleft+.1f),valuex-xright-.1f);
			}
			curves[selectedmodulator].points[i].y = fmax(fmin(pointy,1),0);
			if(axislock != 1)
				valueoffset[1] = fmax(fmin(valueoffset[1],valuey+.1f),valuey-1.1f);

		} else { //dragging tension
			i = (i-1)/2;
			int dir = curves[selectedmodulator].points[i].y > curves[selectedmodulator].points[i+1].y ? -1 : 1;
			if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
				finemode = true;
				initialvalue[0] -= dir*event.getDistanceFromDragStartY()*dragspeed*.9f*1.42f;
			} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
				finemode = false;
				initialvalue[0] += dir*event.getDistanceFromDragStartY()*dragspeed*.9f*1.42f;
			}

			float value = initialvalue[0]-dir*event.getDistanceFromDragStartY()*dragspeed*(finemode?.1f:1)*1.42f;
			curves[selectedmodulator].points[i].tension = fmin(fmax(value-valueoffset[0],0),1);

			valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
		}
		updatetempvis = true;
	} else if(initialdrag > 0) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue[0] -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue[0] += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = 0;
		if(hover == 1 || hover == 2)
			value = initialvalue[0]-event.getDistanceFromDragStartY()*(finemode?.0005f:.005f)*.75f/ui_scales[ui_scale_index];
		else
			value = initialvalue[0]-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		if(hover == (knobcount-1)) {
			audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset[0]);
		} else {
			bool singletick = selectedmodulator == 2 && knobs[0].value[2] < .5;
			float val = value-valueoffset[0];
			if(selectedmodulator != 0 && !singletick) {
				if(hover == 1) val = fmin(val,knobs[2].value[selectedmodulator]); else
				if(hover == 2) val = fmax(val,knobs[1].value[selectedmodulator]);
			}
			if(singletick && hover == 2 && val < knobs[1].value[selectedmodulator])
				audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[1].id)->setValueNotifyingHost(val);
			audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(val);
		}

		valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
	} else if(initialdrag == -15) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y)==-15?-15:-14;
		if(hover == -15 && prevhover != -15 && websiteht <= 0) websiteht = 9.5f;
	}
}
void ModManAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(hover >= -11 && hover <= -7) {
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
		audio_processor.presets[audio_processor.currentpreset].curves[selectedmodulator].points = curves[selectedmodulator].points;
		audio_processor.updatedcurve = audio_processor.updatedcurve.get()|(1<<selectedmodulator);
		audio_processor.undo_manager.setCurrentTransactionName("Set preset");
		audio_processor.undo_manager.beginNewTransaction();
	} else if(hover >= knobcount) {
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
		axislock = -1;
		int i = hover-knobcount;
		if((i%2) == 0) {
			i /= 2;
			if((fabs(initialdotvalue[0]-curves[selectedmodulator].points[i].x)+fabs(initialdotvalue[1]-curves[selectedmodulator].points[i].y)) < .00001) {
				curves[selectedmodulator].points[i].x = initialdotvalue[0];
				curves[selectedmodulator].points[i].y = initialdotvalue[1];
				return;
			}
			dragpos.x += (curves[selectedmodulator].points[i].x-initialdotvalue[0])*ui_scales[ui_scale_index]*180;
			dragpos.y += (initialdotvalue[1]-curves[selectedmodulator].points[i].y)*ui_scales[ui_scale_index]*163;
			if(!curves[selectedmodulator].points[i].enabled) {
				curves[selectedmodulator].points.erase(curves[selectedmodulator].points.begin()+i);
				audio_processor.deletepoint(i);
			} else audio_processor.movepoint(i,curves[selectedmodulator].points[i].x,curves[selectedmodulator].points[i].y);
		} else {
			i = (i-1)/2;
			if(fabs(initialdotvalue[0]-curves[selectedmodulator].points[i].tension) < .00001) {
				curves[selectedmodulator].points[i].tension = initialdotvalue[0];
				return;
			}
			float interp = curve::calctension(.5,curves[selectedmodulator].points[i].tension)-curve::calctension(.5,initialdotvalue[0]);
			dragpos.y += (curves[selectedmodulator].points[i].y-curves[selectedmodulator].points[i+1].y)*interp*ui_scales[ui_scale_index]*200.f;
			audio_processor.movetension(i,curves[selectedmodulator].points[i].tension);
		}
		if(((initialdrag-knobcount)%2) == 0) audio_processor.undo_manager.setCurrentTransactionName("Moved point");
		else audio_processor.undo_manager.setCurrentTransactionName("Moved tension");
		audio_processor.undo_manager.beginNewTransaction();
	} else if(hover > 0) {
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((knobs[hover].value[hover==(knobcount-1)?0:selectedmodulator]-initialvalue[0])>=0?"increased ":"decreased ") += knobs[hover].name);
		if(hover == (knobcount-1))
			audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		else
			audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		if(hover == -15) {
			if(prevhover == -15) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht <= 0) websiteht = 9.5f;
		}
	}
}
void ModManAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover == -13) {
		float x = fmin(fmax((((float)event.x)/ui_scales[ui_scale_index]-208)/180.f,0),1);
		float y = fmin(fmax(1-(((float)event.y)/ui_scales[ui_scale_index]-39)/163.f,0),1);
		int i = 1;
		for(i = 1; i < curves[selectedmodulator].points.size(); ++i)
			if(x < curves[selectedmodulator].points[i].x) break;
		curves[selectedmodulator].points.insert(curves[selectedmodulator].points.begin()+i,point(x,y,curves[selectedmodulator].points[i-1].tension));
		audio_processor.addpoint(i,x,y);
		updatetempvis = true;
		hover = recalc_hover(event.x,event.y);
	} else if(hover <= 0) {
		return;
	} else if(hover >= knobcount) {
		int i = initialdrag-knobcount;
		if((i%2) == 0) {
			i /= 2;
			if(i > 0 && i < (curves[selectedmodulator].points.size()-1)) {
				curves[selectedmodulator].points.erase(curves[selectedmodulator].points.begin()+i);
				audio_processor.deletepoint(i);
			}
		} else {
			i = (i-1)/2;
			if(fabs(curves[selectedmodulator].points[i].tension-.5) < .00001f) return;
			curves[selectedmodulator].points[i].tension = .5f;
			audio_processor.movetension(i,.5f);
		}
		updatetempvis = true;
		hover = recalc_hover(event.x,event.y);
	} else {
		audio_processor.undo_manager.setCurrentTransactionName((String)"reset " += knobs[hover].name);
		if(hover == (knobcount-1)) {
			audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue[0]);
		} else {
			audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue[selectedmodulator]);
		}
		audio_processor.undo_manager.beginNewTransaction();
	}
}
void ModManAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= 0 || hover >= knobcount) return;
	if(hover == (knobcount-1)) {
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value[0]+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	} else {
		audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value[selectedmodulator]+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	}
}
int ModManAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	// website -15 - -14
	if(x < 197) {
		float logox =    (targetpos[0]) *width +14.5f;
		float logoy = (1-(targetpos[1]))*height-12.5f;
		if(x >= logox && x < (logox+152) && y >= (logoy-52) && y < logoy)
			return -15;
		else
			return -14;
	}

	// curve prestets -12 - -7
	if(x >= 207 && x <= 387 && y >= 10 && y <= 34) {
		for(int i = 0; i < 6; ++i) {
			if(x <= (207+30*(i+1)))
				return i-12;
		}
	}

	// flowers -6 - -2
	for(int i = 0; i < MC; ++i) {
		float xx = flowers[i].x-x;
		float yy = (flowers[i].y-y)*1.1139f;
		if((xx*xx+yy*yy) <= flowers[i].s) return i-MC-1;
		if(x >= flowers[i].lx1 && x <= flowers[i].lx2 && y >= flowers[i].ly1 && y <= flowers[i].ly2) return i-MC-1;
	}

	// on off 0
	if(x >= (.48*width) && x <= (.48*width+76.5) && y >= (.77f*height-47.5f) && y <= (.77f*height))
		return 0;

	// range selectors 1 - 2
	if(x >= 398 && x <= 438) {
		for(int i = 0; i < 2; ++i) {
			if(i == 0 && (selectedmodulator == 0 || (selectedmodulator == 2 && knobs[0].value[2] < .5)))
				continue;
			if(y >= (14+(1-tackpos[i])*295.5) && y <= (32+(1-tackpos[i])*295.5))
				return i+1;
		}
		return -1;
	}

	// knobs 3 - 5
	for(int i = 3; i < knobcount; ++i) {
		if(i == 1 || i == 2) continue;
		float xx = knobs[i].x-x;
		float yy = (knobs[i].y-y)*1.1139f;
		if((xx*xx+yy*yy) <= 484) return i;
	}

	// dots and tensions 6+
	if(x >= (208-6) && y >= (39-6) && x <= (388+6) && y <= (202+6)) {
		x -= 208;
		y -= 39;
		for(int i = 0; i < curves[selectedmodulator].points.size(); ++i) {
			float xx = x-curves[selectedmodulator].points[i].x*180;
			float yy = y-(1-curves[selectedmodulator].points[i].y)*163;
			//dot
			if((xx*xx+yy*yy)<=37.1) return i*2+knobcount;

			if(i < (curves[selectedmodulator].points.size()-1)) {
				float interp = curve::calctension(.5,curves[selectedmodulator].points[i].tension);
				xx = x-(curves[selectedmodulator].points[i].x+curves[selectedmodulator].points[i+1].x)*.5f*180.f;
				yy = y-(1-(curves[selectedmodulator].points[i].y*(1-interp)+curves[selectedmodulator].points[i+1].y*interp))*163.f;
				//tension
				if((xx*xx+yy*yy)<=37.1) return i*2+1+knobcount;
			}
		}

		// curve bg
		if(x >= 6 && y >= 6 && x <= (180-6) && y <= (163-6))
			return -13;
	}

	return -1;
}

LookNFeel::LookNFeel() {
	setColour(PopupMenu::backgroundColourId,Colour::fromFloatRGBA(0.f,0.f,0.f,0.f));
	font = find_font("Arial|Helvetica Neue|Helvetica|Roboto");
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	Font fontt;
	if(font == "None")
		fontt = Font(16.f*scale,Font::plain);
	else
		fontt = Font(font,"Regular",16.f*scale);
	fontt.setHorizontalScale(1.1f);
	fontt.setExtraKerningFactor(-.02f);
	return fontt;
}
void LookNFeel::drawPopupMenuBackground(Graphics &g, int width, int height) {
	g.setColour(fg);
	g.fillRect(0,0,width,height);
	g.setColour(bg1);
	g.fillRect(1.5f*scale,1.5f*scale,width-3*scale,height-3*scale);
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
		g.setColour(bg2);
		g.fillRect(1.5f*scale,0.f,area.getWidth()-3*scale,(float)area.getHeight());
		g.setColour(fg);
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
