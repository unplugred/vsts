// TODO tabular save file
// TODO testing
//
// TODO only downsample
// TODO param groupings
// TODO string to param

#include "processor.h"
#include "editor.h"

FMPlusAudioProcessorEditor::FMPlusAudioProcessorEditor(FMPlusAudioProcessor& p) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 300, 300, 2.f, .5f) {

	for(int p = 0; p < PC; ++p) {
		audio_processor.pages[p]->mesh = &mesh;

		audio_processor.pages[p]->knobsmooth.reset(.05f,30,audio_processor.pages[p]->params.size());

		for(int i = 0; i < audio_processor.pages[p]->params.size(); ++i) {
			audio_processor.pages[p]->params[i].defaultvaluenorm = audio_processor.pages[p]->params[i].normalize(audio_processor.pages[p]->params[i].defaultvalue);

			String id = audio_processor.pages[p]->params[i].id;
			for(int o = 0; o < audio_processor.pages[p]->instances; ++o) {
				add_listener(p==1?("o"+(String)o+id):id);

				audio_processor.pages[p]->valuenorm[i][o] = audio_processor.pages[p]->params[i].normalize(audio_processor.pages[p]->state.value[i][o]);

			}
		}

		for(int i = 0; i < audio_processor.pages[p]->curvesnum; ++i)
			for(int o = 0; o < audio_processor.pages[p]->instances; ++o)
				audio_processor.pages[p]->curves[i].cui[o] = audio_processor.pages[p]->curves[i].c[o];
	}

	mesh.width = width;
	mesh.height = height;
	mesh.scaled_dpi = scaled_dpi;
	mesh.textlength = -1;
	mesh.addtext(160,31,"+",1);
	mesh.addtext(160,42,"-",1);
	for(int i = 0; i < TABC; ++i)
		mesh.addtext(5,11*i,i==2?"FX":audio_processor.pages[audio_processor.tabp[i]]->tabname.replace("#",(String)(audio_processor.tabo[i]+1)));
	for(int o = 0; o < (MC+1); ++o) {
		ops[0][o].connections = audio_processor.presets[audio_processor.currentpreset].opconnections[o];
		ops[0][o].pos[0] = audio_processor.presets[audio_processor.currentpreset].oppos[    o *2  ];
		ops[0][o].pos[1] = audio_processor.presets[audio_processor.currentpreset].oppos[    o *2+1];
		ops[0][o].textmesh = mesh.textlength;
		mesh.addtext(ops[0][o].pos[0]+162,ops[0][o].pos[1]+30,o==0?"Out":"OP"+(String)o,(o==0||audio_processor.page_op.valuenorm[0][o-1]>.5f)?0:-10);
	}
	ops[1][0] = ops[0][0];

	fxdamp.reset(.08f,30,(FXC+1)*2);
	updatefxop();
	for(int o = 1; o < (FXC+1); ++o) {
		ops[1][o].pos[0] = audio_processor.presets[audio_processor.currentpreset].oppos[(MC+o)*2  ];
		ops[1][o].pos[1] = audio_processor.presets[audio_processor.currentpreset].oppos[(MC+o)*2+1];
		if(audio_processor.pages[o+2]->valuenorm[0][0] < .5) {
			fx_target[o][0] = ops[1][o].pos[0];
			fx_target[o][1] = ops[1][o].pos[1];
		}
		fx_smooth[o][0] = fx_target[o][0];
		fx_smooth[o][1] = fx_target[o][1];
		fxdamp.setto(fx_target[o][0],o*2  );
		fxdamp.setto(fx_target[o][1],o*2+1);
		ops[1][o].textmesh = mesh.textlength;
		mesh.addtext(ops[1][o].pos[0]+162,ops[1][o].pos[1]+30,audio_processor.pages[o+2]->tabname,-10);
	}
	int selectedtab = audio_processor.params.selectedtab.get();
	if(selectedtab != 2) {
		fx_target[0][0] = ops[0][0].pos[0];
		fx_target[0][1] = ops[0][0].pos[1];
	}
	fx_smooth[0][0] = fx_target[0][0];
	fx_smooth[0][1] = fx_target[0][1];
	fxdamp.setto(fx_target[0][0],0);
	fxdamp.setto(fx_target[0][1],1);
	resortop();
	pagestart = mesh.textlength;
	audio_processor.getframe = true;

	calcvis(0);
	audio_processor.tabp[2] = audio_processor.params.fxtab.get();
	rebuildtab(selectedtab);

	setResizable(false,false);
	setWantsKeyboardFocus(true);
	init(&look_n_feel);
	updatetheme();
}
FMPlusAudioProcessorEditor::~FMPlusAudioProcessorEditor() {
	close();
}

void FMPlusAudioProcessorEditor::newOpenGLContextCreated() {
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 coords;
uniform float banner;
uniform float tabid;
out vec2 uv;
out vec2 tabuv;
out vec2 headeruv;
void main(){
	gl_Position = vec4(vec2(coords.x,coords.y*(1-banner)+banner)*2-1,0,1);
	uv = coords;
	tabuv = (coords-vec2(0,.96-.036667*tabid))*vec2(10,18.75  );
	headeruv = (coords-vec2(.1,.043333))      *vec2(2.83019,10);
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
in vec2 tabuv;
in vec2 headeruv;
uniform sampler2D basetex;
uniform sampler2D tabselecttex;
uniform sampler2D headertex;
uniform float dpi;
uniform float showheader;
uniform float tabid;
uniform vec3 col_bg;
uniform vec3 col_ht;
uniform vec3 col_fg;
uniform vec3 col_on;
out vec4 fragColor;
void main() {
	if(tabuv.x <= 1 && tabuv.y >= .03125 && ((tabuv.y <= .96875 && tabuv.x > .5) || tabuv.y <= .84375)) {
		vec3 col = max(min((texture(tabselecttex,tabuv).rgb-.5)*dpi+.5,1),0);
		fragColor = vec4(col,1);
		if(col.g > 0) {
			if(tabid < 3) col.b = 0;
			col = col_bg*(1-col.r-col.g)+col_fg*(col.r+col.b)+ col_on        *(col.g-col.b);
		} else if(tabid == 10 && tabuv.x < .95 && tabuv.y < .09375) {
			col = col_ht*(1-col.r      )+col_fg* col.r                                     ;
		} else {
			col = col_bg*(1-col.r-col.b)+col_fg* col.r       +        col_ht * col.b       ;
		}
		fragColor = vec4(col,1);
	} else if(headeruv.x > 0 && headeruv.x < 1 && headeruv.y > 0 && headeruv.y < 1) {
		if(showheader > .5) {
			vec3 col = max(min((texture(headertex,vec2(headeruv.x,(headeruv.y+(10-tabid))/11)).rgb-.5)*dpi+.5,1),0);
			fragColor = vec4(col_bg*(1-col)+col_ht*col,1);
		} else {
			fragColor = vec4(col_bg,1);
		}
	} else {
		vec3 col = max(min((texture(basetex,uv).rgb-.5)*dpi+.5,1),0);
		if(col.g > 0) {
		if(col.r > 0) {
			col = col_fg*(1-      col.g)+col_ht*(col.g-col.b)+(col_bg+col_ht)* col.b*.5    ;
		} else {
			col = col_fg*(1-      col.g)+col_bg*(col.g-col.b)+        col_ht * col.b       ;
		}
		} else {
			col = col_fg*(1-col.r      )+col_ht*(col.r-col.b)+ col_bg        * col.b       ;
		}
		fragColor = vec4(col,1);
	}
})");

	creditsshader = add_shader(
//CREDITS VERT
R"(#version 150 core
in vec2 coords;
uniform vec4 pos;
uniform float banner;
out vec2 uv;
out float xpos;
void main() {
	gl_Position = vec4(coords.x*pos.z+pos.x,(coords.y*pos.w+pos.y)*(1-banner)+banner,0,1);
	uv = coords;
	xpos = coords.x;
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 uv;
in float xpos;
uniform sampler2D creditstex;
uniform float shineprog;
uniform float dpi;
uniform vec3 col_bg;
uniform vec3 col_ht;
uniform vec3 col_fg;
out vec4 fragColor;
void main() {
	float col = max(min((texture(creditstex,uv).r-.5)*dpi+.5,1),0);
	float shine = 0;
	if(xpos+shineprog < .65 && xpos+shineprog > 0)
		shine = max(min((texture(creditstex,uv+vec2(shineprog,0)).g-.5)*dpi+.5,1),0);
	fragColor = vec4(col_bg*(1-col)+(col_fg*(1-shine)+col_ht*shine)*col,1);
})");

	squareshader = add_shader(
//SQUARE VERT
R"(#version 150 core
in vec4 coords;
uniform vec4 pos;
uniform float banner;
out vec2 col;
void main() {
	gl_Position = vec4(vec2(coords.x,coords.y*(1-banner)+banner)*2-1,0,1);
	col = coords.zw;
})",
//SQUARE FRAG
R"(#version 150 core
in vec2 col;
uniform float dpi;
uniform vec3 col_fg;
uniform vec3 col_off;
uniform vec3 col_on;
uniform vec3 col_ht;
out vec4 fragColor;
void main() {
	if(col.x < -1) {
		fragColor = vec4(0,0,0,0);
		return;
	}
	fragColor = vec4(col_fg*max(0,1-col.x)+col_off*max(0,1-abs(col.x-1))+col_on*max(0,1-abs(col.x-2))+col_ht*max(0,col.x-2),min(1,max(0,(col.y-2.5)*dpi+.5)));
})");

	circleshader = add_shader(
//CIRCLE VERT
R"(#version 150 core
in vec2 coords;
uniform vec4 pos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(coords.x*pos.z+pos.x,(coords.y*pos.w+pos.y)*(1-banner)+banner)*2-1,0,1);
	uv = coords*2-1;
})",
//CIRCLE FRAG
R"(#version 150 core
in vec2 uv;
uniform float size;
uniform vec3 col;
out vec4 fragColor;
void main(){
	float x = sqrt(uv.x*uv.x+uv.y*uv.y);
	fragColor = vec4(col,min(max((1-x)*size*.5f,0),1));
})");

	lineshader = add_shader(
//LINE VERT
R"(#version 150 core
in vec3 aPos;
uniform float banner;
uniform float animation;
uniform vec4 dividerx;
uniform vec3 dividery;
uniform vec4 htpos;
out float linepos;
out float lineopacity;
out float anim;
out float highlight;
void main() {
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	linepos = min(1,max(0,aPos.z));
	lineopacity = 1.5-abs(aPos.z-.5);
	if(aPos.x < .51) {
		int id = -1;
		float left = .1033333;
		float right = .5033333;
		float htxpos = -1;
		     if(aPos.y > dividery.x) { id = 0; right = dividerx.x; htxpos = htpos.x; }
		else if(aPos.y > dividery.y) { id = 2; right = dividerx.y; htxpos = htpos.y; }
		else if(aPos.y > dividery.z) { id = 4; right = dividerx.z; htxpos = htpos.z; }
		else                         { id = 6; right = dividerx.w; htxpos = htpos.w; }
		if(aPos.x > right) {
			left = right;
			right = .5033333;
			id += 1;
		} else right = min(.5033333,right);
		float xpos = (aPos.x-left)/(right-left);

		anim = animation-xpos-id*.25+1;
		highlight = (xpos-htxpos)*120;
	} else {
		anim = 10;
		highlight = 10;
	}
})",
//LINE FRAG
R"(#version 150 core
in float highlight;
in float anim;
in float linepos;
in float lineopacity;
uniform float dpi;
uniform vec3 col_fg;
uniform vec3 col_on;
out vec4 fragColor;
void main() {
	float mix = min(1,max(0,(1-abs(highlight))*dpi+.5));
	fragColor = vec4(col_fg*(1-mix)+col_on*mix,min((1-abs(linepos*2-1))*dpi,1)*lineopacity*floor(min(1,anim)));
})");

	operatorshader = add_shader(
//OPERATOR VERT
R"(#version 150 core
in vec2 coords;
uniform vec4 pos;
uniform float banner;
out vec2 uv;
void main() {
	gl_Position = vec4(vec2(coords.x*pos.z+pos.x,(coords.y*pos.w+pos.y)*(1-banner)+banner)*2-1,0,1);
	uv = coords;
})",
//OPERATOR FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D operatortex;
uniform float dpi;
uniform float stretch;
uniform float stage;
uniform vec3 col_bg;
uniform vec3 col_ht;
uniform vec3 col_fg;
uniform float selected;
out vec4 fragColor;
void main() {
	vec2 muv = vec2(min(uv.x*stretch,.5)+max(.5-(1-uv.x)*stretch,0),uv.y);
	vec4 col = vec4(max(min((texture(operatortex,muv).rgb-.5)*dpi+.5,1),0),1);
	if(stage < .5) {
		if(col.g <= 0) {
			if(selected < .5 && (abs(muv.x-.5) >= 0.430556 || abs(muv.y-.5) >= 0.333333))
				col.a = 0;
			else
				col.a = col.r;
		}
		fragColor = vec4(col_fg,col.a);
	} else if(stage <= 1.5) {
		col.a = col.g;
		col.rgb = col_bg*(1-col.b)+col_ht*col.b;
		fragColor = col;
	} else {
		if(col.g < 1)
			col.a = 0;
		else
			col.a = col.r;
		fragColor = vec4(col_fg,col.a);
	}
})");

	indicatorshader = add_shader(
//INDICATOR VERT
R"(#version 150 core
in vec2 coords;
uniform vec4 pos;
uniform float banner;
void main() {
	gl_Position = vec4(vec2(coords.x*pos.z+pos.x,(coords.y*pos.w+pos.y)*(1-banner)+banner)*2-1,0,1);
})",
//INDICATOR FRAG
R"(#version 150 core
uniform vec3 col_off;
uniform vec3 col_on;
uniform float light;
out vec4 fragColor;
void main() {
	fragColor = vec4(col_off*(1-light)+col_on*light,1);
})");

	textshader = add_shader(
//TEXT VERT
R"(#version 150 core
in vec4 coords;
uniform vec4 pos;
uniform float banner;
out vec2 uv;
out float col;
void main() {
	gl_Position = vec4(vec2(coords.x,coords.y*(1-banner)+banner)*2-1,0,1);
	uv = vec2((mod(coords.w+.5,17)-.5)/16,1-floor((coords.w+.5)/17)/16);
	col = coords.z;
})",
//TEXT FRAG
R"(#version 150 core
in vec2 uv;
in float col;
uniform float dpi;
uniform sampler2D texttex;
uniform vec3 col_fg;
uniform vec3 col_bg;
uniform vec3 col_on;
uniform vec3 col_ht;
out vec4 fragColor;
void main() {
	if(col < -1)
		fragColor = vec4(0,0,0,0);
	else
		fragColor = vec4(col_fg*max(0,1-col)+col_bg*max(0,1-abs(col-1))+col_on*max(0,1-abs(col-2))+col_ht*max(0,col-2),max(min((texture(texttex,uv).r-.5)*dpi+.5,1),0));
})");

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize);
	add_texture(&tabselecttex, BinaryData::tabselect_png, BinaryData::tabselect_pngSize);
	add_texture(&headertex, BinaryData::header_png, BinaryData::header_pngSize);
	add_texture(&creditstex, BinaryData::credits_png, BinaryData::credits_pngSize);
	add_texture(&operatortex, BinaryData::operator_png, BinaryData::operator_pngSize);
	add_texture(&texttex, BinaryData::text_png, BinaryData::text_pngSize);

	draw_init();
}
void FMPlusAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	// BASE
	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"coords");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	tabselecttex.bind();
	baseshader->setUniform("tabselecttex",1);
	context.extensions.glActiveTexture(GL_TEXTURE2);
	headertex.bind();
	baseshader->setUniform("headertex",2);
	baseshader->setUniform("showheader",((tabanimation>=20&&tabanimation<=21)||tabanimation>=25)?1.f:0.f);
	baseshader->setUniform("tabid",(float)mesh.tab);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("dpi",(float)fmax(scaled_dpi*.5f,1));
	baseshader->setUniform("col_bg",col_bg[0],col_bg[1],col_bg[2]);
	baseshader->setUniform("col_ht",col_ht[0],col_ht[1],col_ht[2]);
	baseshader->setUniform("col_fg",col_fg[0],col_fg[1],col_fg[2]);
	baseshader->setUniform("col_on",col_on[0],col_on[1],col_on[2]);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	// CREDITS
	creditsshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	creditstex.bind();
	creditsshader->setUniform("creditstex",0);
	creditsshader->setUniform("banner",banner_offset);
	creditsshader->setUniform("dpi",(float)fmax(scaled_dpi*.5f,1));
	creditsshader->setUniform("shineprog",websiteht);
	creditsshader->setUniform("pos",149.f/width,251.f/height,148.f/width,46.f/height);
	creditsshader->setUniform("col_bg",col_bg[0],col_bg[1],col_bg[2]);
	creditsshader->setUniform("col_ht",col_ht[0],col_ht[1],col_ht[2]);
	creditsshader->setUniform("col_fg",col_fg[0],col_fg[1],col_fg[2]);
	coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	// SQUARE
	// 0 - outline
	// 1 - outline mod
	// 2 - highlight
	// 3 - conf mod
	if(mesh.squarelength >= 0) {
		squareshader->use();
		squareshader->setUniform("banner",banner_offset);
		squareshader->setUniform("dpi",scaled_dpi);
		squareshader->setUniform("col_fg" ,col_fg [0],col_fg [1],col_fg [2]);
		squareshader->setUniform("col_off",col_off[0],col_off[1],col_off[2]);
		squareshader->setUniform("col_on" ,col_on [0],col_on [1],col_on [2]);
		squareshader->setUniform("col_ht" ,col_ht [0],col_ht [1],col_ht [2]);
		coord = context.extensions.glGetAttribLocation(squareshader->getProgramID(),"coords");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,4,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(mesh.squarelength+1)*4, mesh.square, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES,0,mesh.squarelength+1);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	// LINE
	lineshader->use();
	coord = context.extensions.glGetAttribLocation(lineshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(mesh.linelength+mesh.connectorlength+1)*3, mesh.line, GL_DYNAMIC_DRAW);
	lineshader->setUniform("banner",banner_offset);
	lineshader->setUniform("dpi",scaled_dpi);
	lineshader->setUniform("animation",tabanimation/30.f*2.5f);
	lineshader->setUniform("col_fg",col_fg[0],col_fg[1],col_fg[2]);
	lineshader->setUniform("col_on",col_on[0],col_on[1],col_on[2]);
	lineshader->setUniform("htpos",mesh.lineht[0],mesh.lineht[1],mesh.lineht[2],mesh.lineht[3]);
	lineshader->setUniform("dividerx",selectedpage->dividerx[0],selectedpage->dividerx[1],selectedpage->dividerx[2],selectedpage->dividerx[3]);
	lineshader->setUniform("dividery",1-selectedpage->dividery[0],1-selectedpage->dividery[1],1-selectedpage->dividery[2]);
	glDrawArrays(GL_TRIANGLE_STRIP,0,mesh.linelength+(fxanim>0?0:mesh.connectorlength)+1);
	context.extensions.glDisableVertexAttribArray(coord);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	// CIRCLE
	circleshader->use();
	circleshader->setUniform("banner",banner_offset);
	circleshader->setUniform("dpi",scaled_dpi);
	circleshader->setUniform("col",col_fg[0],col_fg[1],col_fg[2]);
	coord = context.extensions.glGetAttribLocation(circleshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	float inverse_dpi = 1.f/scaled_dpi;
	if(mesh.tab == 2) {
		if(fxanim <= 1) {
			float x = 160.f+66.5f-.5f*inverse_dpi;
			float y = round(fx_smooth[0][1])+4.5f-.5f*inverse_dpi;

			for(int o = 1; o < (FXC+1); ++o) {
				if(audio_processor.pages[o+2]->valuenorm[0][0] > .5) {

					if(fx_smooth[o][1] > fx_smooth[0][1]) continue;
					if(fx_smooth[o][1] < (255-fx_smooth[0][1])) continue;
					float scale = sqrt(fmax(0,16.f-fabs(fx_smooth[o][0]-round(66.5f-audio_processor.pages[o+2]->tabname.length()*3.5f)))/16.f);
					if(scaleop == o) scale *= scaleoplerp;
					circleshader->setUniform("size",5.f*scale*scaled_dpi);

					if(fx_smooth[o][1] > (266-fx_smooth[0][1])) {
						circleshader->setUniform("pos",(x-2.5f*scale)/width,(256+14.5f-round(fx_smooth[o][1])-2.5f*scale-.5f*inverse_dpi)/height,(5.f*scale+inverse_dpi)/width,(5.f*scale+inverse_dpi)/height);
						glDrawArrays(GL_TRIANGLE_STRIP,0,4);
					}

					if(fx_smooth[o][1] < (fx_smooth[0][1]-6)) {
						circleshader->setUniform("pos",(x-2.5f*scale)/width,(256+ 3.5f-round(fx_smooth[o][1])-2.5f*scale-.5f*inverse_dpi)/height,(5.f*scale+inverse_dpi)/width,(5.f*scale+inverse_dpi)/height);
						glDrawArrays(GL_TRIANGLE_STRIP,0,4);
					}
				}
			}
			circleshader->setUniform("size",5.f*scaled_dpi);
			circleshader->setUniform("pos",(x-2.5f)/width,(256+14.5f-round(fx_smooth[0][1])-2.5f-.5*inverse_dpi)/height,(5.f+inverse_dpi)/width,(5.f+inverse_dpi)/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);

			circleshader->setUniform("size",9.f*scaled_dpi);
			circleshader->setUniform("pos",(x-4.5f)/width,(y-4.5f)/height,(9.f+inverse_dpi)/width,(9.f+inverse_dpi)/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			circleshader->setUniform("size",7.f*scaled_dpi);
			circleshader->setUniform("pos",(x-3.5f)/width,(y-3.5f)/height,(7.f+inverse_dpi)/width,(7.f+inverse_dpi)/height);
			circleshader->setUniform("col",col_ht[0],col_ht[1],col_ht[2]);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			circleshader->setUniform("size",3.f*scaled_dpi);
			circleshader->setUniform("pos",(x-1.5f)/width,(y-1.5f)/height,(3.f+inverse_dpi)/width,(3.f+inverse_dpi)/height);
			circleshader->setUniform("col",col_fg[0],col_fg[1],col_fg[2]);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	} else for(int i = 0; i < 2; ++i) {
		if(!displayaddremove[i]) continue;
		circleshader->setUniform("size",9.f*scaled_dpi);
		circleshader->setUniform("pos",(159.f-.5f*inverse_dpi)/width,1.f-(41.f+11.f*i+.5f*inverse_dpi)/height,(9.f+inverse_dpi)/width,(9.f+inverse_dpi)/height);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	if(selectedpage->curvesnum > 0) {
		for(int c = 0; c < selectedpage->curvesnum; ++c) {
			circleshader->setUniform("size",2.5f*scaled_dpi);
			int nextpoint = 0;

			curve* currentcurve = &selectedpage->curves[c].cui[mesh.o];
			float animoffset = selectedpage->curves[c].animid*.25f;
			for(int i = 0; i < (currentcurve->points.size()-1); ++i) {
				if(!currentcurve->points[i].enabled) continue;
				++nextpoint;
				while(!currentcurve->points[nextpoint].enabled) ++nextpoint;

				if(((currentcurve->points[nextpoint].x-currentcurve->points[i].x)*3.f) <= .02f || fabs(currentcurve->points[nextpoint].y-currentcurve->points[i].y) <= .02f) continue;
				float x = (currentcurve->points[i].x+currentcurve->points[nextpoint].x)*.5f;
				if((tabanimation/30.f*2.5-                        x-animoffset) < 0) continue;

				float interp = curve::calctension(.5,currentcurve->points[i].tension);
				      x = x                                                                                  *120.f+31.f-1.25f-.5f*inverse_dpi;
				float y = (currentcurve->points[i].y*(1-interp)+currentcurve->points[nextpoint].y*interp)    *40.f +92.f-1.25f-.5f*inverse_dpi;
				circleshader->setUniform("pos",x/width,y/height,(2.5f+inverse_dpi)/width,(2.5f+inverse_dpi)/height);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			}
			circleshader->setUniform("size",4.f*scaled_dpi);
			for(int i = 0; i < currentcurve->points.size(); ++i) {
				if(!currentcurve->points[i].enabled) continue;
				if((tabanimation/30.f*2.5-currentcurve->points[i].x-animoffset) < 0) continue;
				circleshader->setUniform("pos",
					(currentcurve->points[i].x*120.f+31.f-2.f-.5f*inverse_dpi)/width,
					(currentcurve->points[i].y*40.0f+92.f-2.f-.5f*inverse_dpi)/height,(4.f+inverse_dpi)/width,(4.f+inverse_dpi)/height);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			}
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	// OPERATOR
	operatorshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	operatortex.bind();
	operatorshader->setUniform("operatortex",0);
	operatorshader->setUniform("banner",banner_offset);
	operatorshader->setUniform("dpi",(float)fmax(scaled_dpi*.5f,1));
	coord = context.extensions.glGetAttribLocation(operatorshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	operatorshader->setUniform("col_fg",col_fg[0],col_fg[1],col_fg[2]);
	operatorshader->setUniform("col_bg",col_bg[0],col_bg[1],col_bg[2]);
	operatorshader->setUniform("col_ht",col_ht[0],col_ht[1],col_ht[2]);
	operatorshader->setUniform("selected",0.f);
	for(int stage = 0; stage < 3; ++stage) {
		operatorshader->setUniform("stage",(float)stage);
		for(int n = 0; n < (mesh.tab==2?(FXC+1):(MC+1)); ++n) {
			int o = opz[n];
			if(fxanim > (stage==2?3:4) && o != 0) continue;
			if(mesh.tab == 2 || o == 0) {
				float sl = (audio_processor.pages[o+2]->tabname.length()*7+15.f);
				operatorshader->setUniform("stretch",sl/36.f);
				operatorshader->setUniform("pos",(153.f+round(fx_smooth[o][0]))/width,(256.f-round(fx_smooth[o][1]))/height,sl/width,18.f/height);
				if((mesh.tab == 0 && o == 0) || audio_processor.tabp[2] == (o+2)) {
					operatorshader->setUniform("col_bg",col_on[0],col_on[1],col_on[2]);
					operatorshader->setUniform("col_ht",col_on[0],col_on[1],col_on[2]);
					operatorshader->setUniform("selected",(opanimation<1||(opanimation>=3&&opanimation<4)||opanimation>=6)?1.f:0.f);
					glDrawArrays(GL_TRIANGLE_STRIP,0,4);
					operatorshader->setUniform("col_bg",col_bg[0],col_bg[1],col_bg[2]);
					operatorshader->setUniform("col_ht",col_ht[0],col_ht[1],col_ht[2]);
					operatorshader->setUniform("selected",0.f);
				} else {
					glDrawArrays(GL_TRIANGLE_STRIP,0,4);
				}
			} else {
				if(audio_processor.page_op.valuenorm[0][o==0?0:(o-1)] < .5 || (dragoutop && o == (initialdrag+21))) continue;
				operatorshader->setUniform("stretch",1.f);
				operatorshader->setUniform("pos",(153.f+ops[0][o].pos[0])/width,(256.f-ops[0][o].pos[1])/height,36.f/width,18.f/height);
				if(mesh.tab == (o+2)) {
					operatorshader->setUniform("col_bg",col_on[0],col_on[1],col_on[2]);
					operatorshader->setUniform("col_ht",col_on[0],col_on[1],col_on[2]);
					operatorshader->setUniform("selected",(opanimation<1||(opanimation>=3&&opanimation<4)||opanimation>=6)?1.f:0.f);
					glDrawArrays(GL_TRIANGLE_STRIP,0,4);
					operatorshader->setUniform("col_bg",col_bg[0],col_bg[1],col_bg[2]);
					operatorshader->setUniform("col_ht",col_ht[0],col_ht[1],col_ht[2]);
					operatorshader->setUniform("selected",0.f);
				} else {
					glDrawArrays(GL_TRIANGLE_STRIP,0,4);
				}
			}
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	// TEXT
	// 0 - outline
	// 1 - conf
	// 2 - highlight
	// 3 - conf mod
	if(mesh.textlength >= 0) {
		textshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		texttex.bind();
		textshader->setUniform("texttex",0);
		textshader->setUniform("banner",banner_offset);
		textshader->setUniform("dpi",(float)fmax(scaled_dpi*.5f,1));
		textshader->setUniform("col_fg",col_fg[0],col_fg[1],col_fg[2]);
		textshader->setUniform("col_bg",col_bg[0],col_bg[1],col_bg[2]);
		textshader->setUniform("col_on",col_on[0],col_on[1],col_on[2]);
		textshader->setUniform("col_ht",col_ht[0],col_ht[1],col_ht[2]);
		coord = context.extensions.glGetAttribLocation(textshader->getProgramID(),"coords");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,4,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(mesh.textlength+1)*4, mesh.text, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES,0,mesh.textlength+1);
		context.extensions.glDisableVertexAttribArray(coord);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	}

	// INDICATOR
	indicatorshader->use();
	indicatorshader->setUniform("banner",banner_offset);
	indicatorshader->setUniform("col_off",col_off[0],col_off[1],col_off[2]);
	indicatorshader->setUniform("col_on" ,col_on [0],col_on [1],col_on [2]);
	coord = context.extensions.glGetAttribLocation(indicatorshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	if(mesh.tab == 2 || fxanim > 3) {
		for(int o = 1; o < (MC+1); ++o) {
			indicatorshader->setUniform("light",ops[0][o].indicator);
			indicatorshader->setUniform("pos",2.f/width,1.f-(30.f+11.f*o)/height,2.f/width,6.f/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
		for(int n = 0; n < (FXC+1); ++n) {
			int o = opz[n];
			if(fxanim > 3 && o != 0) continue;
			indicatorshader->setUniform("light",ops[1][o].indicator);
			indicatorshader->setUniform("pos",(153.f+6.f+round(fx_smooth[o][0]))/width,(256.f+6.f-round(fx_smooth[o][1]))/height,2.f/width,6.f/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	} else {
		indicatorshader->setUniform("light",ops[0][0].indicator);
		indicatorshader->setUniform("pos",(153.f+6.f+round(fx_smooth[0][0]))/width,(256.f+6.f-round(fx_smooth[0][1]))/height,2.f/width,6.f/height);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		for(int n = 0; n < (MC+1); ++n) {
			int o = opz[n];
			if(o == 0) continue;
			indicatorshader->setUniform("light",ops[0][o].indicator);
			indicatorshader->setUniform("pos",2.f/width,1.f-(30.f+11.f*o)/height,2.f/width,6.f/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			if(audio_processor.page_op.valuenorm[0][o-1] < .5 || (dragoutop && o == (initialdrag+21))) continue;
			indicatorshader->setUniform("pos",(153.f+6.f+ops[0][o].pos[0])/width,(256.f+6.f-ops[0][o].pos[1])/height,2.f/width,6.f/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void FMPlusAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}

void FMPlusAudioProcessorEditor::updatetheme() {
	opacity_ht = audio_processor.params.theme[0]/255.f;
	for(int i = 0; i < 3; ++i) {
		col_bg [i] = audio_processor.params.theme[i+ 1]/255.f;
		col_fg [i] = audio_processor.params.theme[i+ 4]/255.f;
		col_off[i] = audio_processor.params.theme[i+ 7]/255.f;
		col_on [i] = audio_processor.params.theme[i+10]/255.f;
		col_ht [i] = col_fg[i]*(1-opacity_ht)+col_bg[i]*opacity_ht;
	}
	look_n_feel.bg = Colour::fromFloatRGBA(col_bg[0],col_bg[1],col_bg[2],1.f);
	look_n_feel.fg = Colour::fromFloatRGBA(col_fg[0],col_fg[1],col_fg[2],1.f);
	look_n_feel.on = Colour::fromFloatRGBA(col_on[0],col_on[1],col_on[2],1.f);
}
void FMPlusAudioProcessorEditor::removeop() {
	if(mesh.p != 0 && audio_processor.page_op.valuenorm[0][mesh.o] > .5) {
		int prevop = mesh.o;
		   for(int o = 0; o < MC; ++o) if(audio_processor.page_op.valuenorm[0][(prevop+o+1)%8] > .5) {
			rebuildtab((prevop+o+1)%8+3);
			audio_processor.params.selectedtab = mesh.tab;
			break;
		}
		audio_processor.apvts.getParameter("o"+(String)prevop+audio_processor.page_op.params[0].id)->setValueNotifyingHost(0.f);
	}
}
void FMPlusAudioProcessorEditor::addop() {
	if(mesh.p != 0 && audio_processor.page_op.valuenorm[0][mesh.o] < .5) {
		audio_processor.apvts.getParameter("o"+(String)mesh.o+audio_processor.page_op.params[0].id)->setValueNotifyingHost(1.f);
	} else for(int o = 0; o < MC; ++o) if(audio_processor.page_op.valuenorm[0][        o     ] < .5) {
		rebuildtab(o+3);
		audio_processor.params.selectedtab = mesh.tab;
		audio_processor.apvts.getParameter("o"+(String)o     +audio_processor.page_op.params[0].id)->setValueNotifyingHost(1.f);
		break;
	}
}
void FMPlusAudioProcessorEditor::updatefxop() {
	if(mesh.tab == 2) {
		if(initialdrag >= -20 && initialdrag < -12)
			opdrag = initialdrag+23;
		else
			opdrag = -10;

		int opnum = 0;
		for(int o = 1; o < (FXC+1); ++o)
			if(audio_processor.pages[o+2]->valuenorm[0][0] > .5 && opdrag != (o+2))
				++opnum;

		float fxy = 136.5f+10.f-8.f-11.f*opnum;

		if(opdrag >= 0) {
			if(fabs((66.5f-audio_processor.pages[opdrag]->tabname.length()*3.5f)-ops[1][opdrag-2].pos[0]) >= 30) {
				opdragpos = -10;
			} else {
				float dragpos = (ops[1][opdrag-2].pos[1]-fxy)/22.f+1;
				if(dragpos < -.5f || dragpos > (opnum+2.f)) opdragpos = -10;
				else {
					fxy -= 11;
					opdragpos = fmin(opnum,fmax(0,floor(dragpos)));
				}
			}
		} else opdragpos = -10;

		opnum = 0;
		if(opdragpos == 0) {
			fxy += 22;
			++opnum;
		}
		for(int o = 0; o < FXC; ++o) {
			int p = audio_processor.presets[audio_processor.currentpreset].fxorder[o];
			if(audio_processor.pages[p]->valuenorm[0][0] > .5 && opdrag != p) {
				fx_target[p-2][0] = round(66.5f-audio_processor.pages[p]->tabname.length()*3.5f);
				fx_target[p-2][1] = round(fxy);
				fxy += 22;
				++opnum;
				if(opdragpos == opnum) {
					fxy += 22;
					++opnum;
				}
			}
		}
		fx_target[0][0] = round(66.5f-audio_processor.pages[0]->tabname.length()*3.5f);
		fx_target[0][1] = round(fxy);

		fxy = 136.5f-11.f*opnum-22-7;
		for(int o = 1; o < (FXC+1); ++o) {
			if(audio_processor.pages[o+2]->valuenorm[0][0] < .5 && opdrag != (o+2)) {
				if(fabs((66.5f-audio_processor.pages[o+2]->tabname.length()*3.5f)-ops[1][o].pos[0]) < 30) {
					if(ops[1][o].pos[1] > fxy && ops[1][o].pos[1] < (fxy+(opnum+3)*22-17)) {
						if(ops[1][o].pos[1] <= 136.5f-8)
							fx_target[o][1] = ops[1][o].pos[1]-11*fmax(0,ceil ((ops[1][o].pos[1]- fxy                 )/11.f));
						else
							fx_target[o][1] = ops[1][o].pos[1]-11*fmin(0,floor((ops[1][o].pos[1]-(fxy+(opnum+3)*22-17))/11.f));
					} else  fx_target[o][1] = ops[1][o].pos[1];
				}
			}
		}
	} else {
		opdrag = -10;
		opdragpos = -10;
	}
}
void FMPlusAudioProcessorEditor::resortop() {
	float opx[MC+1];
	if(mesh.tab == 2) {
		for(char o = 0; o < (FXC+1); ++o) {
			if(opz[o] == (audio_processor.tabp[2]-2))
				opx[o] = 300;
			else
				opx[o] = fx_smooth[opz[o]][0]+fx_smooth[opz[o]][1]/300.f;
			opx[o] += audio_processor.pages[opz[o]+2]->tabname.length()*7;
		}
	} else {
		for(char o = 0; o < (MC+1); ++o) {
			if(opz[o] == 0) {
				if(mesh.tab == 0)
					opx[o] = 300;
				else
					opx[o] = fx_smooth[opz[o]][0]+fx_smooth[opz[o]][1]/300.f;
			} else if(opz[o] == (mesh.tab-2)) {
				opx[o] = 300;
			} else {
				opx[o] = ops[0][opz[o]].pos[0]+ops[0][opz[o]].pos[1]/300.f;
			}
		}
	}

	for(char i = 1; i < (mesh.tab==2?(FXC+1):(MC+1)); i++) {
		char j = i;
		while(j > 0 && opx[j] < opx[j-1]) {
			signed short temp = opx[j];
			opx[j] = opx[j-1];
			opx[j-1] = temp;
			             temp = opz[j];
			opz[j] = opz[j-1];
			opz[j-1] = temp;
			j--;
		}
	}
}
void FMPlusAudioProcessorEditor::calcline() {
	mesh.linewritepos = mesh.linelength;
	if(mesh.tab == 2) {
		mesh.beginline(160.f+66.5f,round(256.f+13.f-fx_smooth[0][1]));
		mesh.nextpoint(160.f+66.5f,round(  2.f     +fx_smooth[0][1]));
		mesh.nextpoint(160.f+66.5f,round(  2.f     +fx_smooth[0][1]));
		mesh.endline();
	} else {
		// TODO connectors
	}
	mesh.connectorlength = mesh.linewritepos-mesh.linelength;
}
void FMPlusAudioProcessorEditor::calcvis(int lineupdated) {
	if(lineupdated == 0) {
		mesh.linewritepos = -1;
		mesh.beginline(153.5f,277+.5f*20+.5f);
		for(int i = 0; i < 69; ++i)
			mesh.nextpoint(i+1+(i>=67?154.5f:154.f),277+.5f*20+.5f);
		mesh.endline();
		mesh.linesegments[0] = mesh.linewritepos;
		return;
	}

	selectedpage->calcvis(lineupdated);
}
void FMPlusAudioProcessorEditor::rebuildtab(int tab) {
	if(mesh.p == audio_processor.tabp[tab] && mesh.o == audio_processor.tabo[tab]) return;

	// update + - buttons
	displayaddremove[0] = false;
	for(int o = 0; o < MC; ++o) if(audio_processor.page_op.valuenorm[0][o] < .5f) {
		displayaddremove[0] = true;
		break;
	}
	displayaddremove[1] = audio_processor.tabp[tab]==1&&audio_processor.page_op.valuenorm[0][audio_processor.tabo[tab]]>.5f;
	if(tab != 2) for(int n = 0; n < 2; ++n) for(int i = 0; i < 6; ++i)
		mesh.text[2+(n*6+i)*4] = displayaddremove[n]?((hover+25)==n?2:1):-10;

	opanimation = 0;
	if(mesh.p == audio_processor.tabp[tab]) {
		mesh.tab = tab;
		mesh.p = audio_processor.tabp[tab];
		mesh.o = audio_processor.tabo[tab];
		for(int i = selectedpage->paramstart; i < ((int)selectedpage->params.size()); ++i)
			updatevalue(i);
		updatehighlight(false);
		updateadsr();
		for(int i = 0; i < selectedpage->curvesnum; ++i)
			calcvis(selectedpage->curves[i].linenum);
		resortop();

		return;
	}

	bool rebuildnodes = false;
	if((mesh.tab==2) != (tab==2)) {
		rebuildnodes = true;
		fxanim = 12;
		ops[tab==2?1:0][0].pos[0] = ops[mesh.tab==2?1:0][0].pos[0];
		ops[tab==2?1:0][0].pos[1] = ops[mesh.tab==2?1:0][0].pos[1];
		fx_target[0][0] = ops[0][0].pos[0];
		fx_target[0][1] = ops[0][0].pos[1];
		if(tab == 2) {
			for(int o = 1; o < (FXC+1); ++o) {
				if(audio_processor.pages[o+2]->valuenorm[0][0] < .5) {
					fx_target[o][0] = ops[1][o].pos[0];
					fx_target[o][1] = ops[1][o].pos[1];
				}
				fx_smooth[o][0] = fx_target[o][0];
				fx_smooth[o][1] = fx_target[o][1];
				fxdamp.setto(fx_target[o][0],o*2  );
				fxdamp.setto(fx_target[o][1],o*2+1);
			}
			for(int n = 0; n < 12; ++n) // remove + - buttons
				mesh.text[2+n*4] = -10;
		}
		// hide/unhide text
		if(tab == 2) {
			for(int o = 0; o < MC; ++o)
				if(audio_processor.page_op.valuenorm[0][o] > .5f)
					for(int n = (ops[0][o+1].textmesh+1); n < (ops[0][o+1].textmesh+3*6+1); ++n)
						mesh.text[2+n*4] = -10;
		} else {
			for(int n = (ops[1][1].textmesh+1); n < (prevtextindex[0]+1); ++n)
				mesh.text[2+n*4] = -10;
		}
	}

	tabanimation = 0;
	for(int i = 0; i < 4; ++i) mesh.lineht[i] = 10;
	mesh.tab = tab;
	mesh.p = audio_processor.tabp[tab];
	mesh.o = audio_processor.tabo[tab];
	selectedpage = audio_processor.pages[mesh.p];

	mesh.boxnum = 0;
	mesh.squarelength = -1;
	mesh.textlength = pagestart;
	selectedpage->rebuildtab();

	for(int i = 0; i < mesh.boxnum; ++i) {
		if(mesh.boxes[i].knob != -1) selectedpage->knobbox[mesh.boxes[i].knob] = i;
		if(mesh.boxes[i].type == -1) continue;

		mesh.boxes[i].mesh = mesh.squarelength+1;
		if(mesh.boxes[i].type == 0 || mesh.boxes[i].type == 1) { // slider, polar slider
			mesh.addsquare(mesh.boxes[i].x,mesh.boxes[i].y,mesh.boxes[i].w,mesh.boxes[i].h,3,mesh.boxes[i].corner);
			mesh.addsquare(mesh.boxes[i].x+round(mesh.boxes[i].w*mesh.boxes[i].type*.5f),mesh.boxes[i].y+mesh.boxes[i].h-1,1-mesh.boxes[i].type,1,0);
			mesh.addtext(30,mesh.boxes[i].y,selectedpage->params[mesh.boxes[i].knob].nameui+":");
		} else if(mesh.boxes[i].type == 2) { // no line
			mesh.addsquare(mesh.boxes[i].x,mesh.boxes[i].y,mesh.boxes[i].w,mesh.boxes[i].h,3,mesh.boxes[i].corner);
		} else if(mesh.boxes[i].type == 3) { // tick
			mesh.addtext(mesh.boxes[i].x+mesh.boxes[i].w,mesh.boxes[i].y-1,selectedpage->params[mesh.boxes[i].knob].nameui);
			mesh.addsquare(mesh.boxes[i].x,mesh.boxes[i].y,mesh.boxes[i].w,mesh.boxes[i].h,0,mesh.boxes[i].corner);
			mesh.addsquare(mesh.boxes[i].x+1,mesh.boxes[i].y+1,mesh.boxes[i].w-2,mesh.boxes[i].h-2,1,mesh.boxes[i].corner);
		} else if(mesh.boxes[i].type == 4) { // burger
			mesh.addsquare(mesh.boxes[i].x,mesh.boxes[i].y,mesh.boxes[i].w,mesh.boxes[i].h-11,3);
			mesh.addsquare(mesh.boxes[i].x,mesh.boxes[i].y+mesh.boxes[i].h-10,mesh.boxes[i].w,10,3,mesh.boxes[i].corner);
			mesh.addtext(mesh.boxes[i].x,mesh.boxes[i].y+mesh.boxes[i].h-10,selectedpage->params[mesh.boxes[i].knob].id.substring(0,1));
		}
		selectedpage->addtext(i);
		mesh.boxes[i].textmesh = mesh.textlength;
		if(mesh.boxes[i].textamount > 0) {
			if(mesh.boxes[i].type == 4) {
				mesh.addtext(mesh.boxes[i].x+mesh.boxes[i].w-7*mesh.boxes[i].textamount,mesh.boxes[i].y+mesh.boxes[i].h,String("-----------------",mesh.boxes[i].textamount));
			} else {
				mesh.addtext(mesh.boxes[i].x,mesh.boxes[i].y,String("-----------------",mesh.boxes[i].textamount));
			}
		}
	}

	audio_processor.updatepreset = false;
	for(int i = selectedpage->paramstart; i < ((int)selectedpage->params.size()); ++i) {
		if(i >= 0) {
			if(selectedpage->params[i].adsrstage > 0 || selectedpage->params[i].bpmsync == 2) continue; // skip adsr, bpm sync
			if(selectedpage->params[i].adsrstage == 0) { // set textmesh for adsr params
				int stages = selectedpage->adsr[selectedpage->params[i].adsr].size()/4;
				for(int s = 0; s < (stages-1); ++s) {
					mesh.boxes[selectedpage->knobbox[i+s]].textmesh = mesh.boxes[selectedpage->knobbox[i+stages-1]].textmesh;
					selectedpage->params[i+s].uismooth = selectedpage->knobsmooth.setto(selectedpage->valuenorm[i+s][mesh.o],i+s);
				}
			} else if(mesh.boxes[selectedpage->knobbox[i]].type == 2) { // no transition if no line
				selectedpage->params[i].uismooth = selectedpage->knobsmooth.setto(selectedpage->valuenorm[i][mesh.o],i);
			} else { // start position
				selectedpage->params[i].uismooth = selectedpage->knobsmooth.setto(mesh.boxes[selectedpage->knobbox[i]].type==1?.5f:0,i);
			}
		}
		updatevalue(i);
	}
	for(int a = 0; a < selectedpage->adsrnum; ++a) // reset adsr
		for(int s = 0; s < (selectedpage->adsr[a].size()/2); ++s)
			selectedpage->adsr[a][s*2] = selectedpage->dampadsr.setto(selectedpage->adsr[a][s*2+1]+.0001f,a*selectedpage->adsr[a].size()/2+s);
	updatehighlight(false);
	updateadsr();

	prevtextindex[0] = pagestart; // animation sections
	for(int i = 0; i < 3; ++i) if(selectedpage->sections[i] != -1)
		prevtextindex[i+1] = mesh.boxes[selectedpage->knobbox[selectedpage->sections[i]]-1].textmesh+mesh.boxes[selectedpage->knobbox[selectedpage->sections[i]]-1].textamount*6;

	// make invisible pre animation
	for(int i =           0; i < (1+mesh.squarelength); ++i) if(mesh.square[2+i*4] >= 0) mesh.square[2+i*4] -= 10;
	for(int i = pagestart+1; i < (1+mesh.textlength  ); ++i) if(mesh.text  [2+i*4] >= 0) mesh.text  [2+i*4] -= 10;

	for(int i = 1; i < (selectedpage->linesnum+1); ++i) calcvis(i);
	mesh.linelength = mesh.linesegments[selectedpage->linesnum];
	if(rebuildnodes) {
		updatefxop();
		calcline();
	}
	resortop();
}
void FMPlusAudioProcessorEditor::updatevalue(int param) {
	if(selectedpage->updatevalue(param)) return;
	if(selectedpage->params[param].adsrstage > 0 || selectedpage->params[param].bpmsync == 2) return;
	if(mesh.boxes[selectedpage->knobbox[param]].textamount > 0) {
		int i = param;
		if(selectedpage->params[i].bpmsync == 1) // bpm sync
			if(selectedpage->valuenorm[i+1][mesh.o] > 0)
				++i;
		float value = selectedpage->params[i].inflate(selectedpage->valuenorm[i][mesh.o]);
		String s = get_string(i,value,mesh.p);

		if(mesh.p == 1 && (i == 7 || i == 8)) { // frequency selector
			if(i == 8) {
				if(s.startsWith("-")) s = s.substring(1);
				mesh.replacetext(mesh.boxes[selectedpage->knobbox[i]].textmesh-6*4,value>=.5?"+":"-");
			}
			if(freqdigit >= -10 && (i==8) == (freqdigit>=10)) {
				int newdotpos = s.indexOfChar('.');
				if(newdotpos == -1) newdotpos = 4;
				int dotpos = 4;
				for(int n = 0; n < 4; ++n) {
					int c = mesh.text[3+(1+n*6+mesh.boxes[selectedpage->knobbox[i]].textmesh)*4]-51;
					if(c < 0) {
						dotpos = n;
						break;
					}
				}
				if(newdotpos != dotpos) freqselect(freqdigit+(newdotpos-dotpos),false);
			}
		}

		mesh.replacetext(mesh.boxes[selectedpage->knobbox[i]].textmesh,s,mesh.boxes[selectedpage->knobbox[i]].textamount);
	}
}
void FMPlusAudioProcessorEditor::paint(Graphics& g) { }

void FMPlusAudioProcessorEditor::timerCallback() {
	// website animation
	if(websiteht > -1) websiteht -= .0815;

	// tab animation
	if(tabanimation < 30) for(int q = 0; q < 2; ++q) { // double the speed
		int boxbegin = 0;
		int boxend = 0;
		for(int s = 0; s < 4; ++s) {
			if(selectedpage->sections[s] == -1) boxend = mesh.boxnum-1;
			else boxend = selectedpage->knobbox[selectedpage->sections[s]]-1;

			int current = tabanimation+s*3-4;
			if(current >= boxbegin && current <= boxend) {
				if(mesh.square[2+(mesh.boxes[current].mesh)*4] < 0)
					for(int i = 0; i < 6; ++i)
						mesh.square[2+(mesh.boxes[current].mesh+i)*4] += 10;
			}
			current -= 6;
			if(current <= boxend) {
				if(current >= boxbegin) {
					if(mesh.boxes[current].type != 2)
						if(mesh.square[2+(mesh.boxes[current].mesh+6)*4] < 0)
							for(int i = 0; i < 6; ++i)
								mesh.square[2+(mesh.boxes[current].mesh+6+i)*4] += 10;
					for(int l = prevtextindex[s]; l < (mesh.boxes[current].textmesh+mesh.boxes[current].textamount*6); ++l)
						if(mesh.text[2+(l+1)*4] < 0)
							mesh.text[2+(l+1)*4] += 10;
					prevtextindex[s] = mesh.boxes[current].textmesh+mesh.boxes[current].textamount*6;
				}
			} else if(boxend == (mesh.boxnum-1) && prevtextindex[s] < mesh.textlength) {
				for(int l = prevtextindex[s]; l < mesh.textlength; ++l)
					if(mesh.text[2+(l+1)*4] < 0)
						mesh.text[2+(l+1)*4] += 10;
				prevtextindex[s] = mesh.textlength;
			}
			boxbegin = boxend+1;
		}
		++tabanimation;
	}
	if(fxanim > 0) {
		--fxanim;
		if(fxanim == 2) {
			if(mesh.tab == 2) {
				for(int n = (ops[1][1].textmesh+1); n < (prevtextindex[0]+1); ++n)
					mesh.text[2+n*4] = 0;
			} else {
				for(int o = 0; o < MC; ++o)
					if(audio_processor.page_op.valuenorm[0][o] > .5f)
						for(int n = (ops[0][o+1].textmesh+1); n < (ops[0][o+1].textmesh+3*6+1); ++n)
							mesh.text[2+n*4] = 0;
			}
		}
	}
	if(fxanim <= 4 && opanimation < 30) ++opanimation;

	// smooth value
	bool updateline[MC+1];
	for(int i = 0; i < (selectedpage->linesnum+1); ++i)
		updateline[i] = false;
	for(int i = 0; i < selectedpage->params.size(); ++i) {
		if(selectedpage->params[i].bpmsync == 2 || (selectedpage->params[i].adsrstage > 0 && selectedpage->params[i].adsrstage < 10)) continue; // skip tempo sync, update adsr once
		if(selectedpage->knobbox[i] == -1 || (mesh.square[2+(mesh.boxes[selectedpage->knobbox[i]].mesh+6)*4] < 0 && selectedpage->params[i].adsr < 0)) continue; // box doesnt exist or invisible, skip
		float current = selectedpage->params[i].uismooth;
		float target = selectedpage->valuenorm[i][mesh.o];
		if(selectedpage->params[i].bpmsync == 1) // tempo sync is active, use that value instead
			if(selectedpage->valuenorm[i+1][mesh.o] > 0)
				target = (selectedpage->valuenorm[i+1][mesh.o]*selectedpage->params[i+1].maximumvalue-1)/(selectedpage->params[i+1].maximumvalue-1);
		if(mesh.p == 0 && i == 15 && target >= .5f) target = fmin(6,floor(target*8-1))/6.f; // round second half of anti aliasing knob
		if(selectedpage->params[i].adsrstage == 0) { // interpolate adsr
			bool updated = false;
			int adsrid = selectedpage->params[i].adsr;
			for(int s = 0; s < (selectedpage->adsr[adsrid].size()/2); ++s) {
				if(selectedpage->adsr[adsrid][s*2] == selectedpage->adsr[adsrid][s*2+1]) continue;
				updated = true;
				if(fabs(selectedpage->adsr[adsrid][s*2]-selectedpage->adsr[adsrid][s*2+1]) <= .0005f)
					selectedpage->adsr[adsrid][s*2] = selectedpage->dampadsr.setto(selectedpage->adsr[adsrid][s*2+1],adsrid*selectedpage->adsr[adsrid].size()/2+s);
				else
					selectedpage->adsr[adsrid][s*2] = selectedpage->dampadsr.nextvalue(selectedpage->adsr[adsrid][s*2+1],adsrid*selectedpage->adsr[adsrid].size()/2+s);
			}
			if(!updated) continue;
		} else { // clamp to 0
			if(current == target) continue;
			if(fabs(current-target) <= .0005f)
				current = selectedpage->knobsmooth.setto(target,i);
		}
		selectedpage->params[i].uismooth = selectedpage->knobsmooth.nextvalue(target,i); // interpolate

		if(selectedpage->knobbox[i] != -1 && (mesh.boxes[selectedpage->knobbox[i]].type == 0 || mesh.boxes[selectedpage->knobbox[i]].type == 1)) {
			int roundedvalue = 0;
			if(mesh.boxes[selectedpage->knobbox[i]].type == 0) // regular
				roundedvalue = floor(current*(mesh.boxes[selectedpage->knobbox[i]].w-1.99f)+1.99f);
			else // pan
				roundedvalue = floor(current*(mesh.boxes[selectedpage->knobbox[i]].w-.99f)+.99f+(.5-fabs(current-.5f)));
			for(int t = 0; t < 6; ++t)
				if(fmod(fmax(1,fmin(4,t)),2) == 0)
					mesh.square[(mesh.boxes[selectedpage->knobbox[i]].mesh+t+6)*4] = (mesh.boxes[selectedpage->knobbox[i]].x+(float)roundedvalue)/width;
		} else if(mesh.boxes[selectedpage->knobbox[i]].type == 3) { // switch
			for(int t = 0; t < 6; ++t)
				mesh.square[(mesh.boxes[selectedpage->knobbox[i]].mesh+t+6)*4+2] = 1+current;
		} else if(mesh.boxes[selectedpage->knobbox[i]].type == 4) { // update adsr mesh
			int adsrid = selectedpage->params[i].adsr;
			if(selectedpage->params[i].adsrstage == 0) for(int s = 0; s < 2; ++s) {
				float currentx = mesh.boxes[selectedpage->knobbox[i]].x;
				for(int p = 0; p < 4; ++p) {
					int box = selectedpage->knobbox[i+p];
					for(int t = 0; t < 6; ++t) {
						if(fmod(fmax(1,fmin(4,t)),2) == 1)
							mesh.square[(mesh.boxes[box].mesh+s*6          +t)*4] = ((float)round(currentx                                        )  )/width;
						else
							mesh.square[(mesh.boxes[box].mesh+s*6          +t)*4] = ((float)round(currentx+selectedpage->adsr[adsrid][p*4+s*2]    )  )/width;
					}
					if(s == 1) for(int t = 0; t < 6; ++t) {
						if(fmod(fmax(1,fmin(4,t)),2) == 1)
							mesh.text  [(mesh.boxes[box].textmesh-(4-p)*6+1+t)*4] = ((float)round(currentx+selectedpage->adsr[adsrid][p*4+  2]*.5f)-4)/width;
						else
							mesh.text  [(mesh.boxes[box].textmesh-(4-p)*6+1+t)*4] = ((float)round(currentx+selectedpage->adsr[adsrid][p*4+  2]*.5f)+4)/width;
					}
					if(p == 3 && s == 1)
						mesh.square[mesh.boxes[box].mesh*4+39] = round(currentx+selectedpage->adsr[adsrid][14])-currentx;
					currentx += selectedpage->adsr[adsrid][p*4+s*2]+1;
				}
			}
		}
		if(selectedpage->params[i].updateline >= 0)
			updateline[selectedpage->params[i].updateline] = true;
	}

	if(audio_processor.updatevis.get()) {
		for(int p = 0; p < PC; ++p)
			for(int i = 0; i < audio_processor.pages[p]->curvesnum; ++i)
				for(int o = 0; o < audio_processor.pages[p]->instances; ++o)
					audio_processor.pages[p]->curves[i].cui[o] = audio_processor.pages[p]->curves[i].c[o];
		for(int i = 0; i < selectedpage->curvesnum; ++i)
			updateline[selectedpage->curves[i].linenum] = true;
		audio_processor.updatevis = false;
	}

	// update lines on dpi change
	if(prevdpi != scaled_dpi) {
		prevdpi = scaled_dpi;
		mesh.scaled_dpi = scaled_dpi;
		for(int i = 0; i < (selectedpage->linesnum+1); ++i) calcvis(i);
		calcline();
	} else for(int i = 0; i < (selectedpage->linesnum+1); ++i) if(updateline[i]) calcvis(i);

	if(!audio_processor.getframe.get()) {
		for(int o = 0; o < (MC+1); ++o)
			ops[0][o].indicator = fmin(1,audio_processor.indicators[o]);
		for(int i = 0; i < selectedpage->framedatavals; ++i)
			mesh.framedata[i] = audio_processor.framedata[i].get();
		// TODO get vis
		selectedpage->getframe();
		audio_processor.getframe = true;
	}

	if(mesh.p == 0 && audio_processor.updatepreset.get()) {
		audio_processor.updatepreset = false;
		updatevalue(-3); // update preset name
	}

	// fx ops
	bool rebuildnodes = false;
	for(int o = 0; o < (mesh.tab==2?(FXC+1):1); ++o) {
		float currentx = fxdamp.nextvalue(mesh.tab==2?fx_target[o][0]:ops[0][0].pos[0],o*2  );
		float currenty = fxdamp.nextvalue(mesh.tab==2?fx_target[o][1]:ops[0][0].pos[1],o*2+1);
		if(round(currentx) != round(fx_smooth[o][0]) || round(currenty) != round(fx_smooth[o][1])) {
			rebuildnodes = true;
			float x = round(currentx)+162;
			float y = round(currenty)+30;
			int s = ops[1][o].textmesh+1;
			int sl = audio_processor.pages[o+2]->tabname.length();
			for(int n = 0; n < sl; ++n) {
				mesh.text[   (s+n*6)*4] =     x    /width  ; // top    left
				mesh.text[ 1+(s+n*6)*4] = 1-( y    /height);

				mesh.text[ 4+(s+n*6)*4] =     x    /width  ; // bottom left
				mesh.text[ 5+(s+n*6)*4] = 1-((y+10)/height);

				mesh.text[ 8+(s+n*6)*4] =    (x+ 8)/width  ; // top    right
				mesh.text[ 9+(s+n*6)*4] = 1-( y    /height);

				mesh.text[12+(s+n*6)*4] =     x    /width  ; // bottom left
				mesh.text[13+(s+n*6)*4] = 1-((y+10)/height);

				mesh.text[16+(s+n*6)*4] =    (x+ 8)/width  ; // top    right
				mesh.text[17+(s+n*6)*4] = 1-( y    /height);

				mesh.text[20+(s+n*6)*4] =    (x+ 8)/width  ; // bottom right
				mesh.text[21+(s+n*6)*4] = 1-((y+10)/height);
				x += 7;
			}
			if(mesh.tab != 2 || o == 0) {
				fx_smooth[o][0] = currentx;
				fx_smooth[o][1] = currenty;
			}
		}
		fx_smooth[o][0] = currentx;
		fx_smooth[o][1] = currenty;
	}
	scaleoplerp = scaleoplerp*.4f+.6f;
	if(rebuildnodes) {
		calcline();
		resortop();
	}

	//debug(mesh.boxnum);
	//debug(mesh.squarelength);
	//debug(mesh.linelength);
	//debug(mesh.textlength);

	update();
}

void FMPlusAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int p = 0; p < PC; ++p) {
		for(int o = 0; o < audio_processor.pages[p]->instances; ++o) {
			if(p == 1 && !parameterID.startsWith("o"+(String)o)) continue;
			for(int i = 0; i < audio_processor.pages[p]->params.size(); ++i) {
				if(parameterID != (p==1?("o"+(String)o+audio_processor.pages[p]->params[i].id):audio_processor.pages[p]->params[i].id)) continue;

				audio_processor.pages[p]->valuenorm[i][o] = audio_processor.pages[p]->params[i].normalize(newValue);

				if(mesh.p == p && mesh.o == o) { // update in gui
					if(audio_processor.pages[p]->params[i].bpmsync == 2)
						updatevalue(i-1);
					else
						updatevalue(i  );
					if(audio_processor.pages[p]->params[i].adsr >= 0)
						updateadsr(audio_processor.pages[p]->params[i].adsr);

					if(p == 1 && i == 0) {
						// reset node select animation
						if(audio_processor.pages[p]->valuenorm[i][o] > .5f) opanimation = 0;
						// display minus sign
						displayaddremove[1] = audio_processor.page_op.valuenorm[0][mesh.o]>.5f;
						if(mesh.tab != 2) for(int n = 0; n < 6; ++n)
							mesh.text[2+(6+n)*4] = displayaddremove[1]?(hover==-24?2:1):-10;
					}
				}

				if(i == 0) {
					if(p == 1) { // update plus and minus buttons, remove the operator node
						if(mesh.tab != 2) for(int n = (ops[0][o+1].textmesh+1); n < (ops[0][o+1].textmesh+3*6+1); ++n)
							mesh.text[2+n*4] = audio_processor.pages[p]->valuenorm[i][o]>.5f?0:-10;
						displayaddremove[0] = audio_processor.pages[p]->valuenorm[i][o]<.5f;
						if(!displayaddremove[0]) for(int n = 0; n < MC; ++n) if(audio_processor.page_op.valuenorm[0][n] < .5f) {
							displayaddremove[0] = true;
							break;
						}
						if(mesh.tab != 2) for(int n = 0; n < 6; ++n)
							mesh.text[2+n*4] = displayaddremove[0]?(hover==-25?2:1):-10;
						return;
					}

					if(p >= 3) { // fx node
						scaleop = p-2;
						scaleoplerp = 0;
						if(newValue < .5f) {
							fx_target[p-2][0] = ops[1][p-2].pos[0];
							fx_target[p-2][1] = ops[1][p-2].pos[1];
						}
						updatefxop();
					}
				}

				return;
			}
		}
	}
}
void FMPlusAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	if(prevx == event.x && prevy == event.y) return;
	prevx = event.x;
	prevy = event.y;
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	updatehighlight();
	scrolldigit = -20;
	if(hover == -26 && prevhover != -26 && websiteht <= -1) websiteht = .65f;
}
void FMPlusAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	if(hover >= 0 && hover < mesh.boxnum)
		if(mesh.boxes[hover].type != -1 && mesh.boxes[hover].type != 3)
			for(int t = 0; t < (mesh.boxes[hover].type==4?12:6); ++t)
				mesh.square[(mesh.boxes[hover].mesh+t)*4+2] = 3;
	hover = -1;
}
void FMPlusAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(dpi < 0) return;
	if(event.mods.isRightButtonDown()) {
		hover = recalc_hover(event.x,event.y);
		updatehighlight();
		std::unique_ptr<PopupMenu> rightclickmenu(new PopupMenu());
		std::unique_ptr<PopupMenu> scalemenu(new PopupMenu());

		int i = 20;
		while(++i < (ui_scales.size()+21))
			scalemenu->addItem(i,(String)round(ui_scales[i-21]*100)+"%",true,(i-21)==ui_scale_index);

		rightclickmenu->setLookAndFeel(&look_n_feel);
		if(mesh.boxes[hover].curve >= 0) {
			rightclickmenu->addItem(1,"'Copy curve",true);
			rightclickmenu->addItem(2,"'Paste curve",curve::isvalidcurvestring(SystemClipboard::getTextFromClipboard()));
			rightclickmenu->addItem(3,"'Reset curve",true);
			rightclickmenu->addSeparator();
		}
		rightclickmenu->addItem(4,"'Copy preset",true);
		rightclickmenu->addItem(5,"'Paste preset",audio_processor.is_valid_preset_string(SystemClipboard::getTextFromClipboard()));
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu("'Scale",*scalemenu);
		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20)
				set_ui_scale(result-21);
			else if(result == 1) //copy curve
				SystemClipboard::copyTextToClipboard(audio_processor.curvetostring(mesh.p,mesh.boxes[hover].curve,mesh.o));
			else if(result == 2) //paste curve
				audio_processor.curvefromstring(mesh.p,mesh.boxes[hover].curve,mesh.o,SystemClipboard::getTextFromClipboard());
			else if(result == 3) //reset curve
				audio_processor.resetcurve(mesh.p,mesh.boxes[hover].curve,mesh.o);
			else if(result == 4) //copy preset
				SystemClipboard::copyTextToClipboard(audio_processor.get_preset(audio_processor.currentpreset));
			else if(result == 5) //paste preset
				audio_processor.set_preset(SystemClipboard::getTextFromClipboard(), audio_processor.currentpreset);
		});
		return;
	}

	initialdrag = hover;

	if(mesh.p == 1) {
		if(mesh.boxes[initialdrag].knob == 7 || mesh.boxes[initialdrag].knob == 8) {
			freqselect(fmin(floor((((float)event.x)/ui_scales[ui_scale_index]-mesh.boxes[initialdrag].x)/7.f),3)+(mesh.boxes[initialdrag].knob-7)*20);

			int dotpos = 4;
			for(int n = 1; n < 4; ++n) {
				int c = mesh.text[3+(1+n*6+mesh.boxes[initialdrag].textmesh)*4]-51;
				if(c < 0) {
					dotpos = n;
					break;
				}
			}
			freqoffset = selectedpage->valuenorm[mesh.boxes[initialdrag].knob][mesh.o];
			if(mesh.boxes[initialdrag].knob == 7)
				freqoffset = selectedpage->params[7].inflate(freqoffset);
			else
				freqoffset = freqaddinflate(freqoffset);
			     if(dotpos >= 3) freqoffset = round(freqoffset      )     ;
			else if(dotpos == 2) freqoffset = round(freqoffset*10.f )*.1f ;
			else                 freqoffset = round(freqoffset*100.f)*.01f;
			bool flipped = freqoffset<-.005f;
			freqoffset = round(fmod(fabs(freqoffset)+.00001f,freqstep)*100.f)*.01f;
			if(flipped) freqoffset = freqstep-fabs(freqoffset);

		} else freqselect(-20);
	}

	if(initialdrag == -1) return;
	if(initialdrag > -1) {
		if(mesh.boxes[initialdrag].knob == -1) {
			if(curvehover >= 0) { // lfo
				curve* currentcurve = &selectedpage->curves[mesh.boxes[initialdrag].curve].cui[mesh.o];
				valueoffset[0] = 0;
				audio_processor.undo_manager.beginNewTransaction();
				dragpos = event.getScreenPosition();
				event.source.enableUnboundedMouseMovement(true);
				int i = curvehover;
				if((i%2) == 0) {
					i /= 2;
					initialvalue[0] = currentcurve->points[i].x;
					initialvalue[1] = currentcurve->points[i].y;
					initialdotvalue[0] = initialvalue[0];
					initialdotvalue[1] = initialvalue[1];
					valueoffset[1] = 0;
				} else {
					i = (i-1)/2;
					initialvalue[0] = currentcurve->points[i].tension;
					initialdotvalue[0] = initialvalue[0];
				}
			} else if(mesh.p == 0) {
				       if(initialdrag ==  0) { // preset
					// TODO preset select
				} else if(initialdrag == 15) { // tuning select
					File tuningdir = audio_processor.params.tuningdir;
					if(!tuningdir.exists()) tuningdir = File();
					tuningchooser = std::make_unique<FileChooser>("Open tuning...",tuningdir,"*.scl;*.kbm");
					int folderChooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
					tuningchooser->launchAsync(folderChooserFlags,[this](const FileChooser& chooser) {
						audio_processor.updatetuning(chooser.getResult());
						updatevalue(-2);
					});
				} else if(initialdrag == 17) { // theme select
					File themedir = audio_processor.params.themedir;
					if(!themedir.exists()) themedir = File();
					themechooser = std::make_unique<FileChooser>("Open theme...",themedir,"*.txt;*.ini");
					int folderChooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
					themechooser->launchAsync(folderChooserFlags,[this](const FileChooser& chooser) {
						audio_processor.updatetheme(chooser.getResult());
						updatevalue(-1);
						updatetheme();
					});
				} else if(initialdrag == 16) { // default tuning
					audio_processor.resettuning();
					updatevalue(-2);
				} else if(initialdrag == 18) { // default theme
					audio_processor.resettheme();
					updatevalue(-1);
					updatetheme();
				}
			} else if(mesh.p == 1) {
				       if(initialdrag ==  7 || initialdrag ==  9) { // up down freq mult
					float v = selectedpage->params[7].inflate(selectedpage->valuenorm[7][mesh.o]);
					float multiplier = 1;
						 if(v < (initialdrag==7?.375f: .75f)) multiplier = .25f;
					else if(v < (initialdrag==7?.75f :1.25f)) multiplier = .5f ;
					audio_processor.apvts.getParameter("o"+(String)(mesh.o)+selectedpage->params[7].id)->setValueNotifyingHost(selectedpage->params[7].normalize(v+(8-initialdrag)*multiplier));
				} else if(initialdrag == 10) { // sign freq offset
					audio_processor.apvts.getParameter("o"+(String)(mesh.o)+selectedpage->params[8].id)->setValueNotifyingHost(1-selectedpage->valuenorm[8][mesh.o]);
				} else if(initialdrag == 11 || initialdrag == 13) { // up down freq offset
					float v = freqaddinflate(selectedpage->valuenorm[8][mesh.o]);
					float s = fabs(get_string(8,selectedpage->valuenorm[8][mesh.o],mesh.p).getFloatValue());
					int m = initialdrag==11?1:-1;
					if((initialdrag==13) == (v>=0)) s -= .001f;
						 if(s >= 1000) v += 1000*m;
					else if(s >=  100) v +=  100*m;
					else if(s >=   10) v +=   10*m;
					else if(s >=    1) v +=      m;
					else if(s >=   .1) v +=  .1f*m;
					else               v += .01f*m;
					if(fabs(v) <= .005f) v = 0;
					audio_processor.apvts.getParameter("o"+(String)(mesh.o)+selectedpage->params[8].id)->setValueNotifyingHost(freqaddnormalize(v));
				}
			}
		} else {
			String id = selectedpage->params[mesh.boxes[initialdrag].knob].id;
			if(mesh.p == 1) id = "o"+(String)mesh.o+id;

			initialvalue[0] = selectedpage->valuenorm[mesh.boxes[initialdrag].knob][mesh.o];

			if(selectedpage->params[mesh.boxes[initialdrag].knob].bpmsync == 1) {
				if(selectedpage->valuenorm[mesh.boxes[initialdrag].knob+1][mesh.o] > 0) {
					initialvalue[0] = selectedpage->valuenorm[mesh.boxes[initialdrag].knob+1][mesh.o];
					initialvalue[0] = 1-(1-fmax(0,initialvalue[0]))/(1-1.f/selectedpage->params[mesh.boxes[initialdrag].knob+1].maximumvalue);
				}
			}
			if(mesh.p == 1) {
				     if(mesh.boxes[initialdrag].knob == 7)
					initialvalue[0] =                  initialvalue[0]              /freqstep*24  /5;
				else if(mesh.boxes[initialdrag].knob == 8)
					initialvalue[0] = ((freqaddinflate(initialvalue[0])/9999+1)*.5f)/freqstep*9999/5;
			}

			valueoffset[0] = 0;
			audio_processor.undo_manager.beginNewTransaction();
			audio_processor.apvts.getParameter(id)->beginChangeGesture();

			if(selectedpage->params[mesh.boxes[initialdrag].knob].ttype == potentiometer::ptype::booltype) {
				audio_processor.apvts.getParameter(id)->setValueNotifyingHost(1-initialvalue[0]);
			} else {
				dragpos = event.getScreenPosition();
				event.source.enableUnboundedMouseMovement(true);
			}
		}
	} else if(initialdrag >= -12) { // click tab
		rebuildtab(initialdrag+12);
		audio_processor.params.selectedtab = mesh.tab;
	} else if(initialdrag >= -21) { // click op
		if(mesh.tab == 2) {
			if(initialdrag == -21) {
				initialdrag = -1;
				rebuildtab(0);
			} else {
				audio_processor.tabp[2] = initialdrag+23;
				audio_processor.params.fxtab = audio_processor.tabp[2];
				rebuildtab(2);
				initialvalue[0] = fx_smooth[audio_processor.tabp[2]-2][0];
				initialvalue[1] = fx_smooth[audio_processor.tabp[2]-2][1];
			}
		} else {
			rebuildtab(initialdrag==-21?0:initialdrag+23);
			initialvalue[0] = ops[0][(int)fmax(0,mesh.tab-2)].pos[0];
			initialvalue[1] = ops[0][(int)fmax(0,mesh.tab-2)].pos[1];
		}
		audio_processor.params.selectedtab = mesh.tab;
	} else if(initialdrag == -24) { // minus op
		removeop();
	} else if(initialdrag == -25) { // plus op
		addop();
	}
}
void FMPlusAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(initialdrag == -1) return;
	if(initialdrag > -1) {
		if(mesh.boxes[initialdrag].knob == -1) {
			if(curvehover == -1) {
				int prevhover = hover;
				hover = initialdrag==recalc_hover(event.x,event.y)?initialdrag:-1;
				updatehighlight();
				if(prevhover == -1 && hover != -1 && mesh.boxes[initialdrag].type != -1 && mesh.boxes[initialdrag].type != 3)
					for(int t = 0; t < (mesh.boxes[initialdrag].type==4?12:6); ++t)
						mesh.square[(mesh.boxes[initialdrag].mesh+t)*4+2] = 2;
				return;
			}
			// CURVE
			curve* currentcurve = &selectedpage->curves[mesh.boxes[initialdrag].curve].cui[mesh.o];
			float dragspeed = 1.f/(mesh.boxes[initialdrag].w*ui_scales[ui_scale_index]);
			int i = curvehover;
			if((i%2) == 0) { // dragging a dot
				i /= 2;
				if(!finemode && event.mods.isAltDown()) { //start of fine mode
					finemode = true;
					initialvalue[0] += event.getDistanceFromDragStartX()*dragspeed*.9f;
					initialvalue[1] -= event.getDistanceFromDragStartY()*dragspeed*.9f*3.f;
				} else if(finemode && !event.mods.isAltDown()) { //end of fine mode
					finemode = false;
					initialvalue[0] -= event.getDistanceFromDragStartX()*dragspeed*.9f;
					initialvalue[1] += event.getDistanceFromDragStartY()*dragspeed*.9f*3.f;
				}

				float valuey = initialvalue[1]-event.getDistanceFromDragStartY()*dragspeed*(finemode?.1f:1)*3.f;
				float pointy = valuey-valueoffset[1];

				if(i > 0 && i < (currentcurve->points.size()-1)) {
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
						if((i > 1 && pointx < currentcurve->points[i-1].x) || (i < (currentcurve->points.size()-2) && pointx > currentcurve->points[i+1].x)) {
							point pnt = currentcurve->points[i];
							int n = 1;
							for(n = 1; n < (currentcurve->points.size()-1); ++n) //finding new position
								if(pnt.x < currentcurve->points[n].x) break;
							if(n > i) { // move points back
								n--;
								for(int f = i; f <= n; f++)
									currentcurve->points[f] = currentcurve->points[f+1];
							} else { //move points forward
								for(int f = i; f >= n; f--)
									currentcurve->points[f] = currentcurve->points[f-1];
							}
							i = n;
							curvehover = i*2;
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
							xleft = currentcurve->points[i-1].x;
							if(pointx <= currentcurve->points[i-1].x) {
								pointx = currentcurve->points[i-1].x;
								clamppedx = true;
							}
						}
						if(i < (currentcurve->points.size()-2)) {
							xright = currentcurve->points[i+1].x;
							if(pointx >= currentcurve->points[i+1].x) {
								pointx = currentcurve->points[i+1].x;
								clamppedx = true;
							}
						}
					}
					if(pointx < 0) { pointx = 0; clamppedx = true; }
					if(pointx > 1) { pointx = 1; clamppedx = true; }
					if(pointy < 0) { pointy = 0; clamppedy = true; }
					if(pointy > 1) { pointy = 1; clamppedy = true; }
					if(clamppedx) // remove if out of bounds
						amioutofbounds[0] += preclampx-pointx-fmin(fmax(amioutofbounds[0],-.1f),.1f);
					else amioutofbounds[0] = 0;
					if(clamppedy)
						amioutofbounds[1] += preclampy-pointy-fmin(fmax(amioutofbounds[1],-.1f),.1f);
					else amioutofbounds[1] = 0;
					currentcurve->points[i].enabled = (fabs(amioutofbounds[0])+fabs(amioutofbounds[1])) < .8;

					currentcurve->points[i].x = pointx;
					if(axislock != 2)
						valueoffset[0] = fmax(fmin(valueoffset[0],valuex-xleft+.1f),valuex-xright-.1f);
				}
				currentcurve->points[i].y = fmax(fmin(pointy,1),0);
				if(axislock != 1)
					valueoffset[1] = fmax(fmin(valueoffset[1],valuey+.1f),valuey-1.1f);
			} else { //dragging tension
				i = (i-1)/2;
				int dir = currentcurve->points[i].y > currentcurve->points[i+1].y ? -1 : 1;
				if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
					finemode = true;
					initialvalue[0] -= dir*event.getDistanceFromDragStartY()*dragspeed*.9f*3.f;
				} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
					finemode = false;
					initialvalue[0] += dir*event.getDistanceFromDragStartY()*dragspeed*.9f*3.f;
				}

				float value = initialvalue[0]-dir*event.getDistanceFromDragStartY()*dragspeed*(finemode?.1f:1)*3.f;
				currentcurve->points[i].tension = fmin(fmax(value-valueoffset[0],0),1);

				valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
			}
			calcvis(selectedpage->curves[mesh.boxes[initialdrag].curve].linenum);
		} else {
			String id = selectedpage->params[mesh.boxes[initialdrag].knob].id;
			int bpmmax = -1;
			if(selectedpage->params[mesh.boxes[initialdrag].knob].bpmsync == 1) {
				if(event.mods.isCtrlDown() != selectedpage->params[mesh.boxes[initialdrag].knob].defaultbpm) {
					id = selectedpage->params[mesh.boxes[initialdrag].knob+1].id;
					bpmmax = selectedpage->params[mesh.boxes[initialdrag].knob+1].maximumvalue;
				} else if(selectedpage->valuenorm[mesh.boxes[initialdrag].knob][mesh.o] > 0) {
					String bpmid = selectedpage->params[mesh.boxes[initialdrag].knob+1].id;
					if(mesh.p == 1) bpmid = "o"+(String)(mesh.o)+bpmid;
					audio_processor.apvts.getParameter(bpmid)->setValueNotifyingHost(0);
				}
			}
			if(mesh.p == 1) id = "o"+(String)mesh.o+id;

			if(selectedpage->params[mesh.boxes[initialdrag].knob].ttype == potentiometer::ptype::booltype) {
				int prevhover = hover;
				hover = initialdrag==recalc_hover(event.x,event.y)?initialdrag:-1;
				updatehighlight();
				if(prevhover == -1 && hover != -1 && mesh.boxes[initialdrag].type != -1 && mesh.boxes[initialdrag].type != 3)
					for(int t = 0; t < (mesh.boxes[initialdrag].type==4?12:6); ++t)
						mesh.square[(mesh.boxes[initialdrag].mesh+t)*4+2] = 2;
				if(selectedpage->valuenorm[mesh.boxes[initialdrag].knob][mesh.o] != (initialdrag==hover?(1-initialvalue[0]):initialvalue[0]))
					audio_processor.apvts.getParameter(id)->setValueNotifyingHost(initialdrag==hover?(1-initialvalue[0]):initialvalue[0]);
			} else {
				       if(!finemode &&  (event.mods.isShiftDown() || event.mods.isAltDown()) && (mesh.p != 1 || (mesh.boxes[initialdrag].knob != 7 && mesh.boxes[initialdrag].knob != 8))) {
					finemode = true;
					initialvalue[0] -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
				} else if( finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
					finemode = false;
					initialvalue[0] += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
				}

				float value = initialvalue[0]-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
				float updatevalue = value;
				float clampres = .1f;

				if(mesh.p == 1 && (mesh.boxes[initialdrag].knob == 7 || mesh.boxes[initialdrag].knob == 8)) {
					if(mesh.boxes[initialdrag].knob == 7) {
						clampres = .1f*freqstep/24  *5;
						value = value *freqstep/24  *5;
						updatevalue = selectedpage->params[7].inflate(value-valueoffset[0]);
						if(!(event.mods.isShiftDown() || event.mods.isAltDown()))
							updatevalue = floor(fmax(fmin(updatevalue,  24-freqoffset),       0)/freqstep+.00001f)*freqstep;
						updatevalue = selectedpage->params[7].normalize(updatevalue+freqoffset);
					} else {
						clampres = .1f*freqstep/9999*5;
						value = value *freqstep/9999*5;
						updatevalue = (9999*(2*                      (value-valueoffset[0])-1));
						if(!(event.mods.isShiftDown() || event.mods.isAltDown()))
							updatevalue = floor(fmax(fmin(updatevalue,9999-freqoffset),-9999.1f)/freqstep+.00001f)*freqstep;
						updatevalue = freqaddnormalize               (updatevalue+freqoffset);

					}
				} else {
					updatevalue = value-valueoffset[0];
					if(bpmmax > 0) updatevalue = 1-(1-fmax(0,updatevalue))*(1-1.f/bpmmax);
				}
				audio_processor.apvts.getParameter(id)->setValueNotifyingHost(updatevalue);

				valueoffset[0] = fmax(fmin(valueoffset[0],value+clampres),value-1-clampres);
			}
		}
	} else if(initialdrag >= -12) {
		return;
	} else if(initialdrag >= -21) { // drag operator
		int o = initialdrag+21;
		int sl = 3;
		int t = mesh.tab==2?1:0;
		if(mesh.tab == 2 && initialdrag != -21)
			sl = selectedpage->tabname.length();
		signed short x = initialvalue[0]+event.getDistanceFromDragStartX()/ui_scales[ui_scale_index];
		signed short y = initialvalue[1]+event.getDistanceFromDragStartY()/ui_scales[ui_scale_index];
		if(mesh.tab != 2 && o != 0) {
			if((x < -100 || y < -100 || x > (133-3*7+100) || y > (257+100))) {
				if(!dragoutop) {
					dragoutop = true;
					for(int n = (ops[0][o].textmesh+1); n < (ops[0][o].textmesh+3*6+1); ++n)
						mesh.text[2+n*4] = -10;
				}
			} else if(dragoutop) {
				dragoutop = false;
				for(int n = (ops[0][o].textmesh+1); n < (ops[0][o].textmesh+3*6+1); ++n)
					mesh.text[2+n*4] = 0;
			}
		}
		if(dragoutop) return;

		x = fmin(133-sl*7,fmax(0,x));
		y = fmin(257     ,fmax(0,y));
		if(x < 14 && y < 25) { // + - collision
			if(ops[t][o].pos[0] < 14) y = 25;
			else                      x = 14;
		}
		if(ops[t][o].pos[0] == x && ops[t][o].pos[1] == y) return;
		ops[t][o].pos[0] = x;
		ops[t][o].pos[1] = y;
		updatefxop();
		if(mesh.tab == 0 || (mesh.tab == 2 && initialdrag != -21)) {
			for(int n = 0; n < 2; ++n) {
				fx_target[o][n] = ops[mesh.tab==2?1:0][o].pos[n];
				fx_smooth[o][n] = fx_target[o][n];
				fxdamp.setto(fx_target[o][n],o*2+n);
			}
		}

		// update text
		x = ops[t][o].pos[0]+162;
		y = ops[t][o].pos[1]+30;
		int s = ops[t][o].textmesh+1;
		for(int n = 0; n < sl; ++n) {
			mesh.text[   (s+n*6)*4] =     x    /((float)width  ); // top    left
			mesh.text[ 1+(s+n*6)*4] = 1-( y    /((float)height));

			mesh.text[ 4+(s+n*6)*4] =     x    /((float)width  ); // bottom left
			mesh.text[ 5+(s+n*6)*4] = 1-((y+10)/((float)height));

			mesh.text[ 8+(s+n*6)*4] =    (x+ 8)/((float)width  ); // top    right
			mesh.text[ 9+(s+n*6)*4] = 1-( y    /((float)height));

			mesh.text[12+(s+n*6)*4] =     x    /((float)width  ); // bottom left
			mesh.text[13+(s+n*6)*4] = 1-((y+10)/((float)height));

			mesh.text[16+(s+n*6)*4] =    (x+ 8)/((float)width  ); // top    right
			mesh.text[17+(s+n*6)*4] = 1-( y    /((float)height));

			mesh.text[20+(s+n*6)*4] =    (x+ 8)/((float)width  ); // bottom right
			mesh.text[21+(s+n*6)*4] = 1-((y+10)/((float)height));
			x += 7;
		}
		resortop();
	} else if(initialdrag == -26) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y)==-26?-26:-1;
		if(hover == -26 && prevhover != -26 && websiteht < -1) websiteht = .65f;
	}
}
void FMPlusAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) {
		initialdrag = -1;
		return;
	}
	if(initialdrag > -1) {
		if(mesh.boxes[initialdrag].knob == -1) {
			if(curvehover == -1) {
				hover = recalc_hover(event.x,event.y);
				updatehighlight();
				initialdrag = -1;
				return;
			}
			curve* currentcurve = &selectedpage->curves[mesh.boxes[initialdrag].curve].cui[mesh.o];
			event.source.enableUnboundedMouseMovement(false);
			Desktop::setMousePosition(dragpos);
			axislock = -1;
			int i = curvehover;
			if((i%2) == 0) {
				i /= 2;
				if((fabs(initialdotvalue[0]-currentcurve->points[i].x)+fabs(initialdotvalue[1]-currentcurve->points[i].y)) < .00001) {
					currentcurve->points[i].x = initialdotvalue[0];
					currentcurve->points[i].y = initialdotvalue[1];
					initialdrag = -1;
					return;
				}
				dragpos.x += (currentcurve->points[i].x-initialdotvalue[0])*ui_scales[ui_scale_index]*mesh.boxes[initialdrag].w;
				dragpos.y += (initialdotvalue[1]-currentcurve->points[i].y)*ui_scales[ui_scale_index]*mesh.boxes[initialdrag].h;
				if(!currentcurve->points[i].enabled) {
					currentcurve->points.erase(currentcurve->points.begin()+i);
					audio_processor.deletepoint(mesh.p,mesh.boxes[initialdrag].curve,mesh.o,i);
				} else audio_processor.movepoint(mesh.p,mesh.boxes[initialdrag].curve,mesh.o,i,currentcurve->points[i].x,currentcurve->points[i].y);
			} else {
				i = (i-1)/2;
				if(fabs(initialdotvalue[0]-currentcurve->points[i].tension) < .00001) {
					currentcurve->points[i].tension = initialdotvalue[0];
					initialdrag = -1;
					return;
				}
				float interp = curve::calctension(.5,currentcurve->points[i].tension)-curve::calctension(.5,initialdotvalue[0]);
				dragpos.y += (currentcurve->points[i].y-currentcurve->points[i+1].y)*interp*ui_scales[ui_scale_index]*200.f;
				audio_processor.movetension(mesh.p,mesh.boxes[initialdrag].curve,mesh.o,i,currentcurve->points[i].tension);
			}
			if((curvehover%2) == 0) audio_processor.undo_manager.setCurrentTransactionName("Moved point");
			else audio_processor.undo_manager.setCurrentTransactionName("Moved tension");
			audio_processor.undo_manager.beginNewTransaction();
		} else {
			String id;
			if(mesh.p == 1) id = "o"+(String)mesh.o+selectedpage->params[mesh.boxes[initialdrag].knob].id;
			else id = selectedpage->params[mesh.boxes[initialdrag].knob].id;

			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((selectedpage->valuenorm[mesh.boxes[initialdrag].knob][mesh.o]-initialvalue[0])>=0?"Increased ":"Decreased ") += selectedpage->params[mesh.boxes[initialdrag].knob].name);
			audio_processor.apvts.getParameter(id)->endChangeGesture();
			audio_processor.undo_manager.beginNewTransaction();

			if(selectedpage->params[mesh.boxes[initialdrag].knob].ttype != potentiometer::ptype::booltype) {
				event.source.enableUnboundedMouseMovement(false);
				Desktop::setMousePosition(dragpos);
			}
		}
	} else if(initialdrag >= -21 && initialdrag < -12) { // release operator
		if(opdragpos >= 0 || dragoutop) {
			ops[mesh.tab==2?1:0][initialdrag+21].pos[0] = audio_processor.presets[audio_processor.currentpreset].oppos[((mesh.tab==2?MC:0)+initialdrag+21)*2  ];
			ops[mesh.tab==2?1:0][initialdrag+21].pos[1] = audio_processor.presets[audio_processor.currentpreset].oppos[((mesh.tab==2?MC:0)+initialdrag+21)*2+1];
			if(dragoutop) {
				float x = ops[0][initialdrag+21].pos[0]+162;
				float y = ops[0][initialdrag+21].pos[1]+30;
				int   s = ops[0][initialdrag+21].textmesh+1;
				for(int n = 0; n < 3; ++n) {
					mesh.text[   (s+n*6)*4] =     x    /width  ; // top    left
					mesh.text[ 1+(s+n*6)*4] = 1-( y    /height);

					mesh.text[ 4+(s+n*6)*4] =     x    /width  ; // bottom left
					mesh.text[ 5+(s+n*6)*4] = 1-((y+10)/height);

					mesh.text[ 8+(s+n*6)*4] =    (x+ 8)/width  ; // top    right
					mesh.text[ 9+(s+n*6)*4] = 1-( y    /height);

					mesh.text[12+(s+n*6)*4] =     x    /width  ; // bottom left
					mesh.text[13+(s+n*6)*4] = 1-((y+10)/height);

					mesh.text[16+(s+n*6)*4] =    (x+ 8)/width  ; // top    right
					mesh.text[17+(s+n*6)*4] = 1-( y    /height);

					mesh.text[20+(s+n*6)*4] =    (x+ 8)/width  ; // bottom right
					mesh.text[21+(s+n*6)*4] = 1-((y+10)/height);
					x += 7;
				}
				resortop();
			}
		} else {
			audio_processor.presets[audio_processor.currentpreset].oppos[((mesh.tab==2?MC:0)+initialdrag+21)*2  ] = ops[mesh.tab==2?1:0][initialdrag+21].pos[0];
			audio_processor.presets[audio_processor.currentpreset].oppos[((mesh.tab==2?MC:0)+initialdrag+21)*2+1] = ops[mesh.tab==2?1:0][initialdrag+21].pos[1];
		}
		if(dragoutop) {
			removeop();
			dragoutop = false;
		} else if(opdrag >= 0) {
			if(opdragpos >= 0) {
				//reorder operator
				int currentposition = -10;
				int desiredposition = -10;
				int opnum = 0;
				if(opdragpos == 0) {
					desiredposition = 0;
					++opnum;
				}
				audio_processor.updatefxorder = false;
				char* fxorder = audio_processor.presets[audio_processor.currentpreset].fxorder;
				for(int o = 0; o < FXC; ++o) {
					int p = fxorder[o];
					if(opdragpos == opnum) {
						desiredposition = o;
						++opnum;
					}
					if(opdrag == p) {
						currentposition = o;
						--opdragpos;
					} else if(audio_processor.pages[p]->valuenorm[0][0] > .5) {
						++opnum;
					}
				}
				if(currentposition < desiredposition) {
					for(int o = currentposition; o < desiredposition; ++o)
						fxorder[o] = fxorder[o+1];
				} else if(currentposition > desiredposition) {
					for(int o = currentposition; o > desiredposition; --o)
						fxorder[o] = fxorder[o-1];
				}
				fxorder[desiredposition] = opdrag;
				audio_processor.updatefxorder = true;

				// turn on operator
				if(selectedpage->valuenorm[0][0] < .5) {
					audio_processor.undo_manager.setCurrentTransactionName((String)"Turn "+selectedpage->tabname+" on");
					audio_processor.apvts.getParameter(selectedpage->params[0].id)->setValueNotifyingHost(1.f);
					selectedpage->valuenorm[0][0] = 1.f;
				}
			} else if(selectedpage->valuenorm[0][0] > .5) {
				// turn off operator
				audio_processor.undo_manager.setCurrentTransactionName((String)"Turn "+selectedpage->tabname+" off");
				audio_processor.apvts.getParameter(selectedpage->params[0].id)->setValueNotifyingHost(0.f);
				selectedpage->valuenorm[0][0] = 0.f;
			}
		}
		initialdrag = -1;
		updatefxop();
	}
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	updatehighlight();
	if(hover == -26) {
		if(prevhover == -26) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		else if(websiteht < -1) websiteht = .65f;
	}
	initialdrag = -1;
}
void FMPlusAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(mesh.boxes[hover].knob == -1) { // curves
		if(mesh.boxes[hover].curve == -1) return;
		curve* currentcurve = &selectedpage->curves[mesh.boxes[hover].curve].cui[mesh.o];
		if(curvehover < 0) { // double click add point
			float x = fmin(fmax((((float)event.x)/ui_scales[ui_scale_index]-mesh.boxes[hover].x)/mesh.boxes[hover].w,0),1);
			float y = fmin(fmax(1-(((float)event.y)/ui_scales[ui_scale_index]-mesh.boxes[hover].y)/mesh.boxes[hover].h,0),1);
			int i = 1;
			for(i = 1; i < currentcurve->points.size(); ++i)
				if(x < currentcurve->points[i].x) break;
			currentcurve->points.insert(currentcurve->points.begin()+i,point(x,y,currentcurve->points[i-1].tension));
			audio_processor.addpoint(mesh.p,mesh.boxes[hover].curve,mesh.o,i,x,y);
		} else {
			int i = curvehover;
			if((i%2) == 0) { // double click delete point
				i /= 2;
				if(i > 0 && i < (currentcurve->points.size()-1)) {
					currentcurve->points.erase(currentcurve->points.begin()+i);
					audio_processor.deletepoint(mesh.p,mesh.boxes[hover].curve,mesh.o,i);
				}
			} else { // double click reset tension
				i = (i-1)/2;
				if(fabs(currentcurve->points[i].tension-.5) < .00001f) return;
				currentcurve->points[i].tension = .5f;
				audio_processor.movetension(mesh.p,mesh.boxes[hover].curve,mesh.o,i,.5f);
			}
		}
		hover = recalc_hover(event.x,event.y);
		calcvis(selectedpage->curves[mesh.boxes[hover].curve].linenum);
	} else if(hover >= 0) { // set to default
		if(selectedpage->params[mesh.boxes[hover].knob].ttype == potentiometer::ptype::booltype) return;

		String id = selectedpage->params[mesh.boxes[hover].knob].id;
		if(mesh.p == 1) id = "o"+(String)mesh.o+id;
		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += selectedpage->params[mesh.boxes[hover].knob].name);
		audio_processor.apvts.getParameter(id)->setValueNotifyingHost(selectedpage->params[mesh.boxes[hover].knob].defaultvaluenorm);

		if(selectedpage->params[mesh.boxes[hover].knob].bpmsync == 1) { // reset bpm sync
			String id = selectedpage->params[mesh.boxes[hover].knob+1].id;
			if(mesh.p == 1) id = "o"+(String)mesh.o+id;
			audio_processor.apvts.getParameter(id)->setValueNotifyingHost(selectedpage->params[mesh.boxes[hover].knob+1].defaultvaluenorm);
		}

		audio_processor.undo_manager.beginNewTransaction();
	} else if(mesh.tab != 2) {
		if(displayaddremove[1] && hover >= -20 && hover < -12) { // double click operator
			removeop();
		} else if(displayaddremove[0] && hover == -22) { // double click bg
			addop();
			short xx = fmin(112,fmax(0,round(((float)event.x)/ui_scales[ui_scale_index])-154-18));
			short yy = fmin(257,fmax(0,round(((float)event.y)/ui_scales[ui_scale_index])- 25- 9));
			if(xx < 14 && yy < 25) { // + - collision
				if((xx-14) < (yy-25)) xx = 14;
				else                  yy = 25;
			}
			ops[0][mesh.o+1].pos[0] = xx;
			ops[0][mesh.o+1].pos[1] = yy;
			audio_processor.presets[audio_processor.currentpreset].oppos[(mesh.o+1)*2  ] = ops[0][mesh.o+1].pos[0];
			audio_processor.presets[audio_processor.currentpreset].oppos[(mesh.o+1)*2+1] = ops[0][mesh.o+1].pos[1];
			float x = ops[0][mesh.o+1].pos[0]+162;
			float y = ops[0][mesh.o+1].pos[1]+30;
			int   s = ops[0][mesh.o+1].textmesh+1;
			for(int n = 0; n < 3; ++n) {
				mesh.text[   (s+n*6)*4] =     x    /width  ; // top    left
				mesh.text[ 1+(s+n*6)*4] = 1-( y    /height);

				mesh.text[ 4+(s+n*6)*4] =     x    /width  ; // bottom left
				mesh.text[ 5+(s+n*6)*4] = 1-((y+10)/height);

				mesh.text[ 8+(s+n*6)*4] =    (x+ 8)/width  ; // top    right
				mesh.text[ 9+(s+n*6)*4] = 1-( y    /height);

				mesh.text[12+(s+n*6)*4] =     x    /width  ; // bottom left
				mesh.text[13+(s+n*6)*4] = 1-((y+10)/height);

				mesh.text[16+(s+n*6)*4] =    (x+ 8)/width  ; // top    right
				mesh.text[17+(s+n*6)*4] = 1-( y    /height);

				mesh.text[20+(s+n*6)*4] =    (x+ 8)/width  ; // bottom right
				mesh.text[21+(s+n*6)*4] = 1-((y+10)/height);
				x += 7;
			}
			resortop();
		}
	}
}
void FMPlusAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(initialdrag != -1 || hover <= -1) return;
	if(mesh.boxes[hover].knob == -1) return;
	if(selectedpage->params[mesh.boxes[hover].knob].ttype == potentiometer::ptype::booltype) return;

	String id = selectedpage->params[mesh.boxes[hover].knob].id;
	float value = selectedpage->valuenorm[mesh.boxes[hover].knob][mesh.o];
	float range = selectedpage->params[mesh.boxes[hover].knob].ttype==potentiometer::ptype::floattype?-1:selectedpage->params[mesh.boxes[hover].knob].maximumvalue-selectedpage->params[mesh.boxes[hover].knob].minimumvalue;
	int bpmmax = -1;
	if(selectedpage->params[mesh.boxes[hover].knob].bpmsync == 1) {
		if(selectedpage->valuenorm[mesh.boxes[hover].knob+1][mesh.o] > 0) {
			value = selectedpage->valuenorm[mesh.boxes[hover].knob+1][mesh.o];
			value = 1-(1-fmax(0,value))/(1-1.f/selectedpage->params[mesh.boxes[hover].knob+1].maximumvalue);
		}
		if(event.mods.isCtrlDown() != selectedpage->params[mesh.boxes[hover].knob].defaultbpm) {
			id = selectedpage->params[mesh.boxes[hover].knob+1].id;
			bpmmax = selectedpage->params[mesh.boxes[hover].knob+1].maximumvalue;
			range = selectedpage->params[mesh.boxes[hover].knob+1].ttype==potentiometer::ptype::floattype?-1:selectedpage->params[mesh.boxes[hover].knob+1].maximumvalue-selectedpage->params[mesh.boxes[hover].knob+1].minimumvalue;
		} else if(selectedpage->valuenorm[mesh.boxes[hover].knob][mesh.o] > 0) {
			String bpmid = selectedpage->params[mesh.boxes[hover].knob+1].id;
			if(mesh.p == 1) bpmid = "o"+(String)mesh.o+bpmid;
			audio_processor.apvts.getParameter(bpmid)->setValueNotifyingHost(0);
		}
	}
	if(mesh.p == 1) id = "o"+(String)mesh.o+id;
	if(mesh.p == 1 && (mesh.boxes[hover].knob == 7 || mesh.boxes[hover].knob == 8)) { // freq
		int a = fmin(floor((((float)event.x)/ui_scales[ui_scale_index]-mesh.boxes[hover].x)/7.f),3)+(mesh.boxes[hover].knob-7)*20;
		if(scrolldigit != a) {
			freqselect(a);
			scrolldigit = a;
		}

		if(mesh.boxes[hover].knob == 7) {
			value = selectedpage->params[7].inflate  (value);
			if(event.mods.isShiftDown() || event.mods.isAltDown())
				value +=  wheel.deltaY*2.f        *freqstep;
			else if((value+(wheel.deltaY>0?1.f:-1.f)*freqstep) >=     0 && (value+(wheel.deltaY>0?1.f:-1.f)*freqstep) <=   24)
				value += (wheel.deltaY>0?1.f:-1.f)*freqstep;
			value = selectedpage->params[7].normalize(value);
		} else {
			value = freqaddinflate                   (value);
			if(event.mods.isShiftDown() || event.mods.isAltDown())
				value +=  wheel.deltaY*2.f        *freqstep;
			else if((value+(wheel.deltaY>0?1.f:-1.f)*freqstep) >= -9999 && (value+(wheel.deltaY>0?1.f:-1.f)*freqstep) <= 9999)
				value += (wheel.deltaY>0?1.f:-1.f)*freqstep;
			value = freqaddnormalize                 (value);
		}

	} else { // regular
		if(range < 0)
			value += wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f);
		else
			value += (wheel.deltaY>0?1.f:-1.f)*fmax(1.f/range,fabs(wheel.deltaY)*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
		if(bpmmax > 0) value = 1-(1-fmax(0,value))*(1-1.f/bpmmax);
	}

	audio_processor.apvts.getParameter(id)->setValueNotifyingHost(value);
}
bool FMPlusAudioProcessorEditor::keyPressed(const KeyPress& key) {
	if(freqdigit < -10) return false;
	int code = key.getKeyCode();

	// tab
	if(code == 9) {
		// switch field
		freqselect(freqdigit>10?0:20);
		return true;
	}

	// escape or enter
	if(code == 27 || code == 13) {
		// release focus
		freqselect(-20);
		return true;
	}

	// minus or plus
	if((code == 45 || code == 43) && freqdigit > 10) {
		// change sign
		if((code==45) == (selectedpage->valuenorm[8][mesh.o]>=.5f))
			audio_processor.apvts.getParameter("o"+(String)mesh.o+selectedpage->params[8].id)->setValueNotifyingHost(1-selectedpage->valuenorm[8][mesh.o]);
		return true;
	}

	// slash or backslash or asterisk
	if((code == 47 || code == 92 || code == 42) && freqdigit <= 10) {
		// change mult mode
		audio_processor.apvts.getParameter("o"+(String)mesh.o+selectedpage->params[6].id)->setValueNotifyingHost(code==42?1:0);
		return true;
	}

	// left or right or h or l or a or d or backspace or space
	if(code == 268435537 || code == 104 || code == 97 || code == 8 || code == 268435539 || code == 108 || code == 100 || code == 32) {
		bool right = code == 268435537 || code == 104 || code == 97 || code == 8;
		// move cursor
		int min = freqdigit>10?20:0;
		freqselect(fmin(min+3,fmax(min,(right?-1:1)+fmin(min+3,fmax(min,freqdigit)))));
		return true;
	}

	// up or down or k or j or w or s
	if(code == 268435538 || code == 107 || code == 119 || code == 268435540 || code == 106 || code == 115) {
		bool up = code == 268435538 || code == 107 || code == 119;
		// increment digit
		if(freqdigit > 10)
			audio_processor.apvts.getParameter("o"+(String)mesh.o+selectedpage->params[8].id)->setValueNotifyingHost(                 freqaddnormalize(                 freqaddinflate(selectedpage->valuenorm[8][mesh.o])+(up?1:-1)*freqstep));
		else
			audio_processor.apvts.getParameter("o"+(String)mesh.o+selectedpage->params[7].id)->setValueNotifyingHost(selectedpage->params[7].normalize(selectedpage->params[7].inflate(selectedpage->valuenorm[7][mesh.o])+(up?1:-1)*freqstep));
		return true;
	}

	if(code != 46 && (code < 48 || code > 57)) return false;

	int current = fmin(3,fmax(0,freqdigit>10?(freqdigit-20):freqdigit));
	int dotpos = 4;
	int nums[4];
	for(int n = 0; n < 4; ++n) {
		nums[n] = mesh.text[3+(1+n*6+mesh.boxes[selectedpage->knobbox[freqdigit>10?8:7]].textmesh)*4]-51;
		if(nums[n] < 0) dotpos = n;
	}

	// dot
	if(code == 46) {
		if(current == 0) {
			//if first digit, set start to 0., skip to third digit
			nums[0] = 0;
			nums[1] = -3;
			if(dotpos >= 2) {
				for(int i = 2; i < 4; ++i) {
					if((i+dotpos-1) >= 4)
						nums[i] = 0;
					else
						nums[i] = nums[i+dotpos-1];
				}
			}
			dotpos = 1;
			current = 1;
		} else {
			//if dot is ahead, truncate number
			if(dotpos > current) {
				for(int i = current+1; i < 4; ++i) {
					if((i+dotpos-current) >= 4)
						nums[i] = 0;
					else
						nums[i] = nums[i+dotpos-current];
				}
				nums[current] = -3;
				dotpos = current;
			//if dot is before, do nothing
			} else if(dotpos < current) {
				return true;
			}
			//if dot is current, ignore
		}

	// digit
	} else if(nums[current] >= 0 || current <= 1 || freqdigit >= 10) {
		//if dot is current, inflate.
		if(freqdigit < 10 && current == 0 && dotpos == 2 && (code-48) > 2) {
			nums[0] = code-48;
			nums[1] = -3;
			for(int i = 2; i < 3; ++i) nums[i] = nums[i+1];
			nums[3] = 0;
			--dotpos;
			++current;
		} else {
			if(nums[current] < 0) {
				for(int i = 3; i >= (current+1); --i) nums[i] = nums[i-1];
				++dotpos;
			}
			//replace digit
			nums[current] = code-48;
			if(nums[current] == 0 && current == 0 && dotpos > 1) --current;
			else if(freqdigit < 10 && ((current == 1 && dotpos == 2) || (current == 0 && nums[current] > 2))) ++current;
		}
	}

	float v = 0;
	for(int n = 0; n < 4; ++n) {
		if(dotpos == n) continue;
		v += nums[n]*pow(10,dotpos-n-(dotpos>n?1:0));
	}
	if(freqdigit <= 10) {
		audio_processor.apvts.getParameter("o"+(String)mesh.o+selectedpage->params[7].id)->setValueNotifyingHost(selectedpage->params[7].normalize(v));
	} else {
		if(selectedpage->valuenorm[8][mesh.o] < .5f) v *= -1;
		audio_processor.apvts.getParameter("o"+(String)mesh.o+selectedpage->params[8].id)->setValueNotifyingHost(                 freqaddnormalize(v));
	}

	if(current == 3)
		freqselect(-20);
	else
		freqselect(current+(freqdigit>10?21:1));

	return true;
}
int FMPlusAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	curvehover = -1;

	// -26 logo
	if(x > 224.5f && x < 298.5f && y > 1.5f && y < 24.5f)
		return -26;

	if(x >= 154 && y >= 25) {
		// -21 - -13 operators
		if(mesh.tab == 2) {
			for(int o = 0; o < (FXC+1); ++o) {
				if(x >= (fx_smooth[o][0]+156) && x < (fx_smooth[o][0]+165+audio_processor.pages[o+2]->tabname.length()*7) && y >= (fx_smooth[o] [1]+29) && y < (fx_smooth[o] [1]+41))
					return o-21;
			}
		} else {
			for(int o = 0; o < (MC+1); ++o) {
				if(o != 0 && audio_processor.page_op.valuenorm[0][o==0?0:(o-1)] < .5) continue;
				if(x >= (ops[0][o].pos[0]+156) && x < (ops[0][o].pos[0]+186                                             ) && y >= (ops[0][o].pos[1]+29) && y < (ops[0][o].pos[1]+41))
					return o-21;
			}


		// -25 - -24 +-
			float xx = x-163.5f;
			float yy = y-36.5f;
			if((xx*xx+yy*yy) <= 30.25f && displayaddremove[0]) return -25;
			yy -= 11;
			if((xx*xx+yy*yy) <= 30.25f && displayaddremove[1]) return -24;
		}

		// -22 op bg
		if(x >= 170 || y >= 54)
			return -22;

		// -23 ???
	}

	// -12 - -2 tabs
	if(x <= 29) {
		int i = floor(y/11.f);
		if(i >= TABC) return -1;
		return i-12;
	}

	// 0 - 23 boxes
	for(int i = 0; i < mesh.boxnum; ++i) {
		if(mesh.boxes[i].curve >= 0 && x >= (mesh.boxes[i].x-4) && x < (mesh.boxes[i].x+mesh.boxes[i].w+4) && y >= (mesh.boxes[i].y-4) && y < (mesh.boxes[i].y+mesh.boxes[i].h+4)) {
			x -= mesh.boxes[i].x;
			y -= mesh.boxes[i].y;
			curve* currentcurve = &selectedpage->curves[mesh.boxes[i].curve].cui[mesh.o];
			for(int p = 0; p < currentcurve->points.size(); ++p) {
				float xx = x-currentcurve->points[p].x*mesh.boxes[i].w;
				float yy = y-(1-currentcurve->points[p].y)*mesh.boxes[i].h;
				// dot
				if((xx*xx+yy*yy) <= 8) {
					curvehover = p*2;
					return i;
				}

				if(p < (currentcurve->points.size()-1)) {
					float interp = curve::calctension(.5,currentcurve->points[p].tension);
					xx = x-(currentcurve->points[p].x+currentcurve->points[p+1].x)*.5f*mesh.boxes[i].w;
					yy = y-(1-(currentcurve->points[p].y*(1-interp)+currentcurve->points[p+1].y*interp))*mesh.boxes[i].h;
					// tension
					if((xx*xx+yy*yy) <= 8) {
						curvehover = p*2+1;
						return i;
					}
				}
			}

			// curve bg
			if(x >= 0 && y >= 0 && x <= mesh.boxes[i].w && y <= mesh.boxes[i].h) {
				curvehover = -1;
				return i;
			}
		} else if(mesh.boxes[i].type != -1 && x >= mesh.boxes[i].x && x < (mesh.boxes[i].x+mesh.boxes[i].w) && y >= mesh.boxes[i].y && y < (mesh.boxes[i].y+mesh.boxes[i].h)) {
			return i;
		}
	}

	return -1;
}
void FMPlusAudioProcessorEditor::updatehighlight(bool update_adsr) {
	if(highlight == hover && !update_adsr) return;
	// prev highlight
	bool validhighlight = false;
	if(highlight >= 0 && highlight < mesh.boxnum) {
		validhighlight = mesh.boxes[highlight].knob >= 0;

		if(mesh.boxes[highlight].type != -1 && mesh.boxes[highlight].type != 3)
			for(int t = 0; t < (mesh.boxes[highlight].type==4?12:6); ++t)
				mesh.square[(mesh.boxes[highlight].mesh+t)*4+2] = 3;

		if(update_adsr && mesh.boxes[highlight].knob >= 0 && selectedpage->params[mesh.boxes[highlight].knob].adsr >= 0)
			updateadsr(selectedpage->params[mesh.boxes[highlight].knob].adsr);

		if(mesh.p == 1 && ((mesh.boxes[highlight].knob == 7 && freqdigit >= -10 && freqdigit < 10) || (mesh.boxes[highlight].knob == 8 && freqdigit >= 10)))
			for(int t = 0; t < 6; ++t)
				mesh.text[(mesh.boxes[highlight].textmesh+6*(int)fmin(3,fmax(0,freqdigit>=10?(freqdigit-20):freqdigit))+t+1)*4+2] = 3;

	} else if(mesh.tab != 2 && (highlight == -25 || highlight == -24) && displayaddremove[highlight+25]) {
		for(int t = 0; t < 6; ++t)
			mesh.text[2+((highlight+25)*6+t)*4] = 1;
	}
	// current highligt
	if(hover     >= 0 && hover     < mesh.boxnum) {
		if(mesh.boxes[hover    ].type != -1 && mesh.boxes[hover    ].type != 3)
			for(int t = 0; t < (mesh.boxes[hover    ].type==4?12:6); ++t)
				mesh.square[(mesh.boxes[hover    ].mesh+t)*4+2] = 2;

		if(update_adsr && mesh.boxes[hover    ].knob >= 0 && selectedpage->params[mesh.boxes[hover    ].knob].adsr >= 0)
			if(!validhighlight || selectedpage->params[mesh.boxes[highlight].knob].adsr != selectedpage->params[mesh.boxes[hover    ].knob].adsr)
				updateadsr(selectedpage->params[mesh.boxes[hover    ].knob].adsr);

		if(mesh.p == 1 && ((mesh.boxes[hover    ].knob == 7 && freqdigit >= -10 && freqdigit < 10) || (mesh.boxes[hover    ].knob == 8 && freqdigit >= 10)))
			for(int t = 0; t < 6; ++t)
				mesh.text[(mesh.boxes[hover    ].textmesh+6*(int)fmin(3,fmax(0,freqdigit>=10?(freqdigit-20):freqdigit))+t+1)*4+2] = 2;
	} else if(mesh.tab != 2 && (hover     == -25 || hover     == -24) && displayaddremove[hover    +25]) {
		for(int t = 0; t < 6; ++t)
			mesh.text[2+((hover    +25)*6+t)*4] = 2;
	}
	highlight = hover;
}
void FMPlusAudioProcessorEditor::updateadsr(int adsr) {
	int currentadsr = -1;
	int hoverknob = -1;
	int hoveradsr = -1;
	if(hover >= 0 && hover < mesh.boxnum && mesh.boxes[hover].knob >= 0 && selectedpage->params[mesh.boxes[hover].knob].adsr >= 0 && (adsr == -1 || adsr == selectedpage->params[mesh.boxes[hover].knob].adsr)) {
		hoverknob = mesh.boxes[hover].knob;
		hoveradsr = selectedpage->params[hoverknob].adsr;
	}
	for(int i = 0; i < selectedpage->params.size(); ++i) {
		if(hoverknob == i) {
			String s = get_string(mesh.boxes[hover].knob,selectedpage->valuenorm[mesh.boxes[hover].knob][mesh.o],mesh.p);
			mesh.replacetext(mesh.boxes[hover].textmesh,s.paddedLeft(' ',5));
		}

		if(selectedpage->params[i].adsr == currentadsr) continue;
		currentadsr = selectedpage->params[i].adsr;
		if(currentadsr == hoveradsr || currentadsr < 0 || (adsr != currentadsr && adsr != -1)) continue;

		String s = format_time( // does not support multistage
			powf(selectedpage->valuenorm[i  ][mesh.o],2)*(MAXA-MINA)+MINA+
			powf(selectedpage->valuenorm[i+1][mesh.o],2)*(MAXD-MIND)+MIND+
			powf(selectedpage->valuenorm[i+3][mesh.o],2)*(MAXR-MINR)+MINR);
		mesh.replacetext(mesh.boxes[selectedpage->knobbox[i]].textmesh,s.paddedLeft(' ',5));

		if(adsr >= 0) return;
	}
}
void FMPlusAudioProcessorEditor::freqselect(int digit, bool calcfreqstep) {
	int square = mesh.boxes[selectedpage->knobbox[8]].mesh;
	int nbox = selectedpage->knobbox[    digit>10?8:7];
	int pbox = selectedpage->knobbox[freqdigit>10?8:7];
	if((digit<-10) != (freqdigit<-10)) {
		if(digit >= -10)
			grabKeyboardFocus();
		else
			giveAwayKeyboardFocus();
		for(int t = 0; t < 6; ++t)
				mesh.square[(square                   +6              +t  )*4+2] = digit<-10?-10:0;
	}
	if(freqdigit >= -10) {
		for(int t = 0; t < 6; ++t)
				mesh.text  [(mesh.boxes[pbox].textmesh+6*(int)fmin(3,fmax(0,freqdigit>=10?(freqdigit-20):freqdigit))+t+1)*4+2] = 0;
	}
	if(digit >= -10) {
		int clampeddigit =                               (int)fmin(3,fmax(0,    digit>=10?(    digit-20):    digit));
		for(int t = 0; t < 6; ++t)
				mesh.text  [(mesh.boxes[nbox].textmesh+6*clampeddigit                                               +t+1)*4+2] = hover==nbox?2:3;
		for(int t = 0; t < 6; ++t) {
			if(fmod(fmax(1,fmin(4,t)),2) == 1)
				mesh.square[(square                   +6              +t  )*4  ] = (mesh.boxes[nbox].x+clampeddigit*7.f  )/width;
			else
				mesh.square[(square                   +6              +t  )*4  ] = (mesh.boxes[nbox].x+clampeddigit*7.f+8)/width;
		}
	}
	freqdigit = digit;

	if(freqdigit >= -10 && calcfreqstep) {
		int dotpos = 4;
		for(int n = 1; n < 4; ++n) {
			int c = mesh.text[3+(1+n*6+mesh.boxes[nbox].textmesh)*4]-51;
			if(c < 0) {
				dotpos = n;
				break;
			}
		}
		freqstep = dotpos-fmin(3,fmax(0,freqdigit>10?(freqdigit-20):freqdigit));
		if(freqstep > 0) --freqstep;
		freqstep = powf(10,freqstep);
	}
}

LookNFeel::LookNFeel() {
	setColour(PopupMenu::backgroundColourId,Colour::fromFloatRGBA(0.f,0.f,0.f,0.f));
	font = find_font("Consolas|Noto Mono|DejaVu Sans Mono|Menlo|Andale Mono|SF Mono|Lucida Console|Liberation Mono");
}
LookNFeel::~LookNFeel() { }
Font LookNFeel::getPopupMenuFont() {
	if(font == "None")
		return Font(18.f*scale,Font::plain);
	return Font(font,"Regular",14.f*scale);
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
		g.setColour(on);
		g.fillRect(scale,0.f,area.getWidth()-2*scale,(float)area.getHeight());
	}
	g.setColour(fg);
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
void LookNFeel::getIdealPopupMenuItemSize(const String& text, const bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) {
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
