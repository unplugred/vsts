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
		    ((aPos.x-center.x)*cos(knobrot)-(aPos.y-center.y     )*sin(knobrot))*5.5+.5,
		.95-((aPos.x-center.x)*sin(knobrot)+(aPos.y-center.y     )*cos(knobrot))       ,
		    ((aPos.x-center.x)*cos(knobrot)-(aPos.y-center.y-.035)*sin(knobrot))*5.5+.5,
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

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize);
	add_texture(&bannertex, BinaryData::banner_png, BinaryData::banner_pngSize);
	add_texture(&flowerstex, BinaryData::flowers_png, BinaryData::flowers_pngSize);
	add_texture(&labelstex, BinaryData::labels_png, BinaryData::labels_pngSize);
	add_texture(&tackstex, BinaryData::tacks_png, BinaryData::tacks_pngSize);
	add_texture(&numberstex, BinaryData::numbers_png, BinaryData::numbers_pngSize);
	add_texture(&cubertex, BinaryData::cuber_png, BinaryData::cuber_pngSize);
	add_texture(&onofftex, BinaryData::onoff_png, BinaryData::onoff_pngSize);
	add_texture(&knobtex, BinaryData::knob_png, BinaryData::knob_pngSize);

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
	basetex.bind();
	baseshader->setUniform("pos",200.f/width,295.f/width);
	baseshader->setUniform("margin",0.f,0.f);
	baseshader->setUniform("index",0.f,1.f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

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
		if(i == 0 && selectedmodulator == 0) continue;

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

	draw_end();
}
void ModManAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void ModManAudioProcessorEditor::paint(Graphics& g) { }

void ModManAudioProcessorEditor::timerCallback() {
	//if(creditsalpha != ((hover<=-2&&hover>=-3)?1:0)) { //TODO
	//	creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.17f:-.17f),1),0);
	//}
	//if(creditsalpha <= 0) websiteht = -1;
	//if(websiteht > -1 && creditsalpha >= 1) {
	//	websiteht -= .0815;
	//}

	if(framecount++ >= 1) {
		float val = 0;
		framecount = 0;
		for(int i = 0; i < MC; ++i) if(knobs[0].value[i] > .5)
			flowers[i].rot = round(audio_processor.flower_rot[i].get()*10*10)/10;
		for(int i = 0; i < 2; ++i) {
			val = knobs[i+1].value[selectedmodulator];
			if(i == 0) val = fmin(val,knobs[2].value[selectedmodulator]);
			if(selectedmodulator != 0) val = val*.9f+i*.1f;
			else if(i == 1) val = val*.97f+.03f;
			else val = -.07f;
			tackpos[i] = tackpos[i]*.3f+val*.7f;

			val = knobs[i+1].value[selectedmodulator];
			if(selectedmodulator != 0) val = val*.93f+.07f;
			cuberposclamp[i] = cuberposclamp[i]*.3f+val*.7f;
		}
		for(int i = 0; i < 2; ++i) {
			if(knobs[0].value[selectedmodulator] > .5) {
				val = audio_processor.cuber_rot[i].get();
				if(selectedmodulator != 0) val = val*.93f+.07f;
			} else {
				val = cuberpos[i];
			}
			cuberpos[i] = fmin(fmax(cuberpos[i]*.3f+val*.7f,cuberposclamp[0]),cuberposclamp[1]);
		}
		cubersize = cubersize*.5f+knobs[0].value[selectedmodulator]*.5f;
	}
	cuberindex = fmod(cuberindex+.3333f,6);

	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;

	time = fmod(time+.0002f,1.f);

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
	//if(hover == -3 && prevhover != -3 && websiteht <= -1) websiteht = .65f; TODO
	//else if(hover > -2 && prevhover <= -2) websiteht = -2;
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
		rightclickmenu->addItem(1,"'Copy preset",true);
		rightclickmenu->addItem(2,"'Paste preset",audio_processor.is_valid_preset_string(SystemClipboard::getTextFromClipboard()));
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu("'Scale",*scalemenu);
		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20) {
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
	if(hover < -1 && hover >= (-1-MC)) {
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
		}
	} else if(hover == 0) {
		audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(1-knobs[hover].value[selectedmodulator]);
		audio_processor.undo_manager.setCurrentTransactionName(knobs[hover].value[selectedmodulator]>.5f?"Turned modulator off":"Turned modulator on");
		audio_processor.undo_manager.beginNewTransaction();
	} else if(hover > 0 && hover < knobcount) {
		initialvalue = knobs[hover].value[hover==(knobcount-1)?0:selectedmodulator];
		valueoffset = 0;
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
		audio_processor.undo_manager.beginNewTransaction();
		if(hover == (knobcount-1))
			audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		else
			audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->beginChangeGesture();
	}
}
void ModManAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(hover == -1) return;
	if(initialdrag > 0 && initialdrag < knobcount) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = 0;
		if(hover == 1 || hover == 2)
			value = initialvalue-event.getDistanceFromDragStartY()*(finemode?.0005f:.005f)*.75f/ui_scales[ui_scale_index];
		else
			value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		if(hover == (knobcount-1)) {
			audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset);
		} else {
			float val = value-valueoffset;
			if(selectedmodulator != 0) {
				if(hover == 1) val = fmin(val,knobs[2].value[selectedmodulator]); else
				if(hover == 2) val = fmax(val,knobs[1].value[selectedmodulator]);
			}
			audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(val);
		}

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	//} else if(initialdrag == -3) { TODO
	//	int prevhover = hover;
	//	hover = recalc_hover(event.x,event.y)==-3?-3:-2;
	//	if(hover == -3 && prevhover != -3 && websiteht < -1) websiteht = .65f;
	}
}
void ModManAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(hover > 0 && hover < knobcount) {
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((knobs[hover].value[hover==(knobcount-1)?0:selectedmodulator]-initialvalue)>=0?"Increased ":"Decreased ") += knobs[hover].name);
		if(hover == (knobcount-1))
			audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		else
			audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		//if(hover == -3) { TODO
		//	if(prevhover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		//	else if(websiteht < -1) websiteht = .65f;
		//}
	}
}
void ModManAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= 0 || hover >= knobcount) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	if(hover == (knobcount-1)) {
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue[0]);
	} else {
		audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue[selectedmodulator]);
	}
	audio_processor.undo_manager.beginNewTransaction();
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

	// curve prestets -14 - -9

	// website -8 - -7

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

	// knobs 1 - 2
	if(x >= 398 && x <= 438) {
		for(int i = 0; i < 2; ++i) {
			if(i == 0 && selectedmodulator == 0) continue;
			if(y >= (14+(1-tackpos[i])*295.5) && y <= (32+(1-tackpos[i])*295.5)) {
				return i+1;
			}
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
	g.setColour(fg1);
	g.fillRect(0,0,width,height);
	g.setColour(bg1);
	g.fillRect(scale,scale,width-2*scale,height-2*scale);
}
void LookNFeel::drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) {
	if(isSeparator) {
		g.setColour(fg1);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		return;
	}

	bool removeleft = text.startsWith("'");
	if(isHighlighted && isActive) {
		g.setColour(fg2);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		g.setColour(bg2);
		g.fillRect(scale,0.f,area.getWidth()-2*scale,(float)area.getHeight());
		g.setColour(fg2);
	} else {
		g.setColour(fg1);
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
