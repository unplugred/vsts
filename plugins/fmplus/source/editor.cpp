#include "processor.h"
#include "editor.h"

FMPlusAudioProcessorEditor::FMPlusAudioProcessorEditor(FMPlusAudioProcessor& p, int pgeneralcount, int pparamcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 300, 300, 2.f, .5f) {

	tuningfile = params.tuningfile;
	themefile = params.themefile;
	presetunsaved = params.presetunsaved;
	generalcount = pgeneralcount+1;
	paramcount = pparamcount;
	for(int i = 0; i < pgeneralcount; ++i) {
		knobs[knobcount].id = params.general[i].id;
		knobs[knobcount].name = params.general[i].name;
		knobs[knobcount].value[0] = params.general[i].normalize(state.general[i]);
		knobs[knobcount].ttype = params.general[i].ttype;
		knobs[knobcount].minimumvalue = params.general[i].minimumvalue;
		knobs[knobcount].maximumvalue = params.general[i].maximumvalue;
		knobs[knobcount].defaultvalue = params.general[i].normalize(params.general[i].defaultvalue);
		add_listener(knobs[knobcount].id);
		++knobcount;
	}
	knobs[knobcount].id = "antialias";
	knobs[knobcount].name = "Anti-Alias";
	knobs[knobcount].value[0] = params.antialiasing;
	knobs[knobcount].minimumvalue = 0;
	knobs[knobcount].maximumvalue = 1;
	knobs[knobcount].defaultvalue = .7f;
	add_listener(knobs[knobcount].id);
	++knobcount;
	for(int i = 0; i < pparamcount; ++i) {
		knobs[knobcount].id = params.values[i].id;
		knobs[knobcount].name = params.values[i].name;
		knobs[knobcount].ttype = params.values[i].ttype;
		knobs[knobcount].minimumvalue = params.values[i].minimumvalue;
		knobs[knobcount].maximumvalue = params.values[i].maximumvalue;
		knobs[knobcount].defaultvalue = params.values[i].normalize(params.values[i].defaultvalue);
		for(int o = 0; o < MC; ++o) {
			knobs[knobcount].value[o] = params.values[i].normalize(state.values[o][i]);
			add_listener("o"+(String)o+knobs[knobcount].id);
		}
		++knobcount;
	}

	textlength = -1;
	addtext(160,31,"+",1);
	addtext(160,42,"-",1);
	addtext(5, 0,"Gen");
	addtext(5,11,"Vis");
	addtext(5,22,"FX");
	for(int o = 0; o < MC; ++o)
		addtext(5,11*(o+3),"OP"+(String)(o+1));
	for(int o = 0; o < (MC+1); ++o) {
		ops[o].connections = state.opconnections[o];
		ops[o].indicator = (o==0||o==5)?1.f:0.f;
		ops[o].pos[0] = state.oppos[o*2  ];
		ops[o].pos[1] = state.oppos[o*2+1];
		ops[o].textmesh = textlength;
		addtext(ops[o].pos[0]+162,ops[o].pos[1]+30,o==0?"Out":(knobs[generalcount].value[o-1]>.5f?("OP"+(String)o):"   "));
	}

	knobs[              5].name = "Arp";
	knobs[              6].name = "Direction";
	knobs[              7].name = "Length";
	knobs[              8].name = "Speed";
	knobs[             10].name = "Vibrato";
	knobs[             11].name = "Rate";
	knobs[             13].name = "Amount";
	knobs[             14].name = "Attack";
	knobs[generalcount   ].name = "OPERATOR_X";
	knobs[generalcount+ 2].name = "Amp";
	knobs[generalcount+13].name = "LFO";
	knobs[generalcount+14].name = "Target";
	knobs[generalcount+15].name = "Rate";
	knobs[generalcount+17].name = "Amount";
	knobs[generalcount+18].name = "Attack";

	for(int i = 0; i < 8; i++)
		curves[i] = state.curves[i];

	rebuildtab(params.selectedtab.get());

	setResizable(false,false);
	setWantsKeyboardFocus(true);
	init(&look_n_feel);
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
uniform vec3 col_bg_mod;
uniform vec3 col_conf;
uniform vec3 col_conf_mod;
uniform vec3 col_outline;
uniform vec3 col_vis;
uniform vec3 col_vis_mod;
uniform vec3 col_highlight;
out vec4 fragColor;
void main() {
	if(tabuv.x <= 1 && tabuv.y >= .03125 && ((tabuv.y <= .96875 && tabuv.x > .5) || tabuv.y <= .84375)) {
		vec3 col = max(min((texture(tabselecttex,tabuv).rgb-.5)*dpi+.5,1),0);
		fragColor = vec4(col,1);
		if(col.g > 0) {
			if(tabid < 3) col.b = 0;
			col = col_conf*(1-col.r-col.g)+col_outline*(col.r+col.b)+col_highlight*(col.g-col.b);
		} else if(tabid == 10 && tabuv.x < .95 && tabuv.y < .09375) {
			col = col_bg  *(1-col.r      )+col_outline* col.r                                   ;
		} else {
			col = col_conf*(1-col.r-col.b)+col_outline* col.r       +col_conf_mod *       col.b ;
		}
		fragColor = vec4(col,1);
	} else if(headeruv.x > 0 && headeruv.x < 1 && headeruv.y > 0 && headeruv.y < 1) {
		if(showheader > .5) {
			vec3 col = max(min((texture(headertex,vec2(headeruv.x,(headeruv.y+(10-tabid))/11)).rgb-.5)*dpi+.5,1),0);
			fragColor = vec4(col_conf*(1-col)+col_conf_mod*col,1);
		} else {
			fragColor = vec4(col_conf,1);
		}
	} else {
		vec3 col = max(min((texture(basetex,uv).rgb-.5)*dpi+.5,1),0);
		if(col.g > 0) {
		if(col.r > 0) {
			col = col_outline*(1-col.g)+col_vis *(col.g-col.b)+col_vis_mod *col.b;
		} else {
			col = col_outline*(1-col.g)+col_conf*(col.g-col.b)+col_conf_mod*col.b;
		}
		} else {
			col = col_outline*(1-col.r)+col_bg  *(col.r-col.b)+col_bg_mod  *col.b;
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
uniform vec3 col_conf;
uniform vec3 col_conf_mod;
uniform vec3 col_outline;
out vec4 fragColor;
void main() {
	float col = max(min((texture(creditstex,uv).r-.5)*dpi+.5,1),0);
	float shine = 0;
	if(xpos+shineprog < .65 && xpos+shineprog > 0)
		shine = max(min((texture(creditstex,uv+vec2(shineprog,0)).g-.5)*dpi+.5,1),0);
	fragColor = vec4(col_conf*(1-col)+(col_outline*(1-shine)+col_conf_mod*shine)*col,1);
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
uniform vec3 col_outline;
uniform vec3 col_outline_mod;
uniform vec3 col_highlight;
uniform vec3 col_conf_mod;
out vec4 fragColor;
void main() {
	if(col.x < -1) {
		fragColor = vec4(0,0,0,0);
		return;
	}
	fragColor = vec4(col_outline*max(0,1-col.x)+col_outline_mod*max(0,1-abs(col.x-1))+col_highlight*max(0,1-abs(col.x-2))+col_conf_mod*max(0,col.x-2),min(1,max(0,(col.y-2.5)*dpi+.5)));
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
out float linepos;
out float lineopacity;
out float anim;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	linepos = min(1,max(0,aPos.z));
	lineopacity = 1.5-abs(aPos.z-.5);
	float id = aPos.x>.51?-1:(aPos.y>.75?0:(aPos.y>.5?1:2));
	float xpos = id==-1?0:(aPos.x-(id==0?.39:.1033333))*(id==0?9.090909:2.5);
	anim = min(1,animation*2.5-xpos-id*.5+1);
})",
//LINE FRAG
R"(#version 150 core
in float anim;
in float linepos;
in float lineopacity;
uniform float dpi;
out vec4 fragColor;
void main(){
	fragColor = vec4(0,0,0,min((1-abs(linepos*2-1))*dpi,1)*lineopacity*floor(anim));
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
uniform vec3 col_conf;
uniform vec3 col_conf_mod;
uniform vec3 col_outline;
uniform float selected;
out vec4 fragColor;
void main() {
	vec4 col = vec4(max(min((texture(operatortex,uv).rgb-.5)*dpi+.5,1),0),1);
	if(col.g <= 0) {
		if(selected < .5 && (abs(uv.x-.5) >= 0.430556 || abs(uv.y-.5) >= 0.333333)) {
			col.a = 0;
		} else {
			col.a = col.r;
			col.r = 1;
		}
	} else if(col.g >= 1 && col.r > 0) {
		col.g -= col.r;
	}
	col.rgb = col_conf*(col.g-col.b)+col_conf_mod*col.b+col_outline*col.r;
	fragColor = col;
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
uniform vec3 col_outline_mod;
uniform vec3 col_highlight;
uniform float light;
out vec4 fragColor;
void main() {
	fragColor = vec4(col_outline_mod*(1-light)+col_highlight*light,1);
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
uniform vec3 col_outline;
uniform vec3 col_conf;
uniform vec3 col_highlight;
uniform vec3 col_conf_mod;
out vec4 fragColor;
void main() {
	if(col < -1)
		fragColor = vec4(0,0,0,0);
	else
		fragColor = vec4(col_outline*max(0,1-col)+col_conf*max(0,1-abs(col-1))+col_highlight*max(0,1-abs(col-2))+col_conf_mod*max(0,col-2),max(min((texture(texttex,uv).r-.5)*dpi+.5,1),0));
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
	baseshader->setUniform("tabid",(float)selectedtab);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("dpi",(float)fmax(scaled_dpi*.5f,1));
	baseshader->setUniform("col_bg"					,col_bg[0]			,col_bg[1]			,col_bg[2]			);
	baseshader->setUniform("col_bg_mod"				,col_bg_mod[0]		,col_bg_mod[1]		,col_bg_mod[2]		);
	baseshader->setUniform("col_conf"				,col_conf[0]		,col_conf[1]		,col_conf[2]		);
	baseshader->setUniform("col_conf_mod"			,col_conf_mod[0]	,col_conf_mod[1]	,col_conf_mod[2]	);
	baseshader->setUniform("col_outline"			,col_outline[0]		,col_outline[1]		,col_outline[2]		);
	baseshader->setUniform("col_vis"				,col_vis[0]			,col_vis[1]			,col_vis[2]			);
	baseshader->setUniform("col_vis_mod"			,col_vis_mod[0]		,col_vis_mod[1]		,col_vis_mod[2]		);
	baseshader->setUniform("col_highlight"			,col_highlight[0]	,col_highlight[1]	,col_highlight[2]	);
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
	creditsshader->setUniform("col_conf"			,col_conf[0]		,col_conf[1]		,col_conf[2]		);
	creditsshader->setUniform("col_conf_mod"		,col_conf_mod[0]	,col_conf_mod[1]	,col_conf_mod[2]	);
	creditsshader->setUniform("col_outline"			,col_outline[0]		,col_outline[1]		,col_outline[2]		);
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
	if(squarelength >= 0) {
		squareshader->use();
		squareshader->setUniform("banner",banner_offset);
		squareshader->setUniform("dpi",scaled_dpi);
		squareshader->setUniform("col_outline"		,col_outline[0]		,col_outline[1]		,col_outline[2]		);
		squareshader->setUniform("col_outline_mod"	,col_outline_mod[0]	,col_outline_mod[1]	,col_outline_mod[2]	);
		squareshader->setUniform("col_highlight"	,col_highlight[0]	,col_highlight[1]	,col_highlight[2]	);
		squareshader->setUniform("col_conf_mod"		,col_conf_mod[0]	,col_conf_mod[1]	,col_conf_mod[2]	);
		coord = context.extensions.glGetAttribLocation(squareshader->getProgramID(),"coords");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,4,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(squarelength+1)*4, squaremesh, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES,0,squarelength+1);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	// LINE
	lineshader->use();
	coord = context.extensions.glGetAttribLocation(lineshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(linelength+1)*3, visline, GL_DYNAMIC_DRAW);
	lineshader->setUniform("banner",banner_offset);
	lineshader->setUniform("dpi",scaled_dpi);
	lineshader->setUniform("animation",tabanimation/30.f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,linelength+1);
	context.extensions.glDisableVertexAttribArray(coord);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	// CIRCLE
	circleshader->use();
	circleshader->setUniform("banner",banner_offset);
	circleshader->setUniform("dpi",scaled_dpi);
	circleshader->setUniform("col"					,col_outline[0]		,col_outline[1]		,col_outline[2]		);
	coord = context.extensions.glGetAttribLocation(circleshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	circleshader->setUniform("size",9.f*scaled_dpi);
	float inverse_dpi = 1.f/scaled_dpi;
	for(int i = 0; i < 2; ++i) {
		if(!displayaddremove[i]) continue;
		circleshader->setUniform("pos",(159.f-.5f*inverse_dpi)/width,1.f-(41.f+11.f*i+.5f*inverse_dpi)/height,(9.f+inverse_dpi)/width,(9.f+inverse_dpi)/height);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	if(selectedtab >= 3) {
		circleshader->setUniform("size",2.5f*scaled_dpi);
		int nextpoint = 0;
		for(int i = 0; i < (curves[selectedtab-3].points.size()-1); ++i) {
			if(!curves[selectedtab-3].points[i].enabled) continue;
			++nextpoint;
			while(!curves[selectedtab-3].points[nextpoint].enabled) ++nextpoint;

			if(((curves[selectedtab-3].points[nextpoint].x-curves[selectedtab-3].points[i].x)*3.f) <= .02f || fabs(curves[selectedtab-3].points[nextpoint].y-curves[selectedtab-3].points[i].y) <= .02f) continue;
			float x = (curves[selectedtab-3].points[i].x+curves[selectedtab-3].points[nextpoint].x)*.5f;
			if((tabanimation*.0833333f-x-1) < 0) continue;

			double interp = curve::calctension(.5,curves[selectedtab-3].points[i].tension);
			      x = x                                                                                                  *120.f+31.f-1.25f-.5f*inverse_dpi;
			float y = (curves[selectedtab-3].points[i].y*(1-interp)+curves[selectedtab-3].points[nextpoint].y*interp)    *40.f +92.f-1.25f-.5f*inverse_dpi;
			circleshader->setUniform("pos",x/width,y/height,(2.5f+inverse_dpi)/width,(2.5f+inverse_dpi)/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
		circleshader->setUniform("size",4.f*scaled_dpi);
		for(int i = 0; i < curves[selectedtab-3].points.size(); ++i) {
			if(!curves[selectedtab-3].points[i].enabled) continue;
			if((tabanimation*.0833333f-curves[selectedtab-3].points[i].x-1) < 0) continue;
			circleshader->setUniform("pos",
				(curves[selectedtab-3].points[i].x*120.f+31.f-2.f-.5f*inverse_dpi)/width,
				(curves[selectedtab-3].points[i].y*40.0f+92.f-2.f-.5f*inverse_dpi)/height,(4.f+inverse_dpi)/width,(4.f+inverse_dpi)/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
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
	operatorshader->setUniform("col_outline"		,col_outline[0]		,col_outline[1]		,col_outline[2]		);
	operatorshader->setUniform("col_conf"			,col_conf[0]		,col_conf[1]		,col_conf[2]		);
	operatorshader->setUniform("col_conf_mod"		,col_conf_mod[0]	,col_conf_mod[1]	,col_conf_mod[2]	);
	operatorshader->setUniform("selected",0.f);
	for(int o = MC; o >= 0; --o) {
		if(o != 0 && knobs[generalcount].value[o==0?0:(o-1)] < .5) continue;
		operatorshader->setUniform("pos",(153.f+ops[o].pos[0])/width,(256.f-ops[o].pos[1])/height,36.f/width,18.f/height);
		if(selectedtab == (o==0?0:(o+2))) {
			operatorshader->setUniform("col_conf"		,col_highlight[0]	,col_highlight[1]	,col_highlight[2]	);
			operatorshader->setUniform("col_conf_mod"	,col_highlight[0]	,col_highlight[1]	,col_highlight[2]	);
			operatorshader->setUniform("selected",(opanimation<1||(opanimation>=3&&opanimation<4)||opanimation>=6)?1.f:0.f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			operatorshader->setUniform("col_conf"		,col_conf[0]		,col_conf[1]		,col_conf[2]		);
			operatorshader->setUniform("col_conf_mod"	,col_conf_mod[0]	,col_conf_mod[1]	,col_conf_mod[2]	);
			operatorshader->setUniform("selected",0.f);
		} else {
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	// INDICATOR
	indicatorshader->use();
	indicatorshader->setUniform("banner",banner_offset);
	indicatorshader->setUniform("col_outline_mod"	,col_outline_mod[0]	,col_outline_mod[1]	,col_outline_mod[2]	);
	indicatorshader->setUniform("col_highlight"		,col_highlight[0]	,col_highlight[1]	,col_highlight[2]	);
	coord = context.extensions.glGetAttribLocation(indicatorshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int o = MC; o >= 0; --o) {
		indicatorshader->setUniform("light",ops[o].indicator);
		if(o > 0) {
			indicatorshader->setUniform("pos",2.f/width,1.f-(30.f+11.f*o)/height,2.f/width,6.f/height);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
		if(o != 0 && knobs[generalcount].value[o==0?0:(o-1)] < .5) continue;
		indicatorshader->setUniform("pos",(153.f+6.f+ops[o].pos[0])/width,(256.f+6.f-ops[o].pos[1])/height,2.f/width,6.f/height);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	// TEXT
	// 0 - outline
	// 1 - conf
	// 2 - highlight
	// 3 - conf mod
	if(textlength >= 0) {
		textshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		texttex.bind();
		textshader->setUniform("texttex",0);
		textshader->setUniform("banner",banner_offset);
		textshader->setUniform("dpi",(float)fmax(scaled_dpi*.5f,1));
		textshader->setUniform("col_outline"		,col_outline[0]		,col_outline[1]		,col_outline[2]		);
		textshader->setUniform("col_conf"			,col_conf[0]		,col_conf[1]		,col_conf[2]		);
		textshader->setUniform("col_highlight"		,col_highlight[0]	,col_highlight[1]	,col_highlight[2]	);
		textshader->setUniform("col_conf_mod"		,col_conf_mod[0]	,col_conf_mod[1]	,col_conf_mod[2]	);
		coord = context.extensions.glGetAttribLocation(textshader->getProgramID(),"coords");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,4,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(textlength+1)*4, textmesh, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES,0,textlength+1);
		context.extensions.glDisableVertexAttribArray(coord);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	}

	draw_end();
}
void FMPlusAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}

void FMPlusAudioProcessorEditor::calcvis(int curveupdated) {
	if(curveupdated == 0) {
		linewritepos = -1;
		beginline(153.5f,277+.5f*20+.5f);
		for(int i = 0; i < 69; ++i)
			nextpoint(i+1+(i>=67?154.5f:154.f),277+.5f*20+.5f);
		endline();
		curvemesh[0] = linewritepos;
		return;
	}

	if(selectedtab < 3) return;

	if(curveupdated == 1) {
		int box = knobs[generalcount+3].box;
		int h = boxes[box].h;
		int x = boxes[box].x;
		int y = height-h-boxes[box].y;
		linewritepos = curvemesh[0];
		beginline(x,y+(osccalc(-1,knobs[generalcount+3].valuesmoothed)*-.5f+.5f)*h);
		for(int i = 0; i < (h+1); ++i)
			nextpoint(x+i+1,y+(osccalc((((float)i+1)/h)*2-1,knobs[generalcount+3].valuesmoothed)*.5f+.5f)*h);
		endline();
		curvemesh[1] = linewritepos;
		return;
	}

	if(curveupdated == 2) {
		int box = knobs[generalcount+9].box;
		int w = 120;
		int h = boxes[box].h;
		int x = boxes[box].x;
		int y = height-boxes[box].y;
		float currentx = adsr[0]+.5f;
		linewritepos = curvemesh[1];
		beginline(x,y-h+11);
		nextpoint(x+currentx,y);
		currentx += adsr[6]+1.f;
		nextpoint(x+currentx,y-(1-knobs[generalcount+11].valuesmoothed)*(h-11),true);
		currentx += adsr[12]+1.f;
		nextpoint(x+currentx,y-(1-knobs[generalcount+11].valuesmoothed)*(h-11),true);
		nextpoint(x+w,y-h+11,true);
		nextpoint(x+w,y-h+11);
		endline();
		curvemesh[2] = linewritepos;
		return;
	}

	if(curveupdated == 3) {
		int box = knobs[generalcount+14].box-1;
		int w = boxes[box].w;
		int h = boxes[box].h;
		int x = boxes[box].x;
		int y = height-h-boxes[box].y;
		linewritepos = curvemesh[2];
		curveiterator iterator;
		iterator.reset(curves[selectedtab-3],w);
		beginline(x,y+iterator.next()*h);
		for(int i = 0; i < (w+1); ++i)
			nextpoint(x+i+1,y+iterator.next()*h);
		endline();
		curvemesh[3] = linewritepos;
		return;
	}
}
void FMPlusAudioProcessorEditor::beginline(float x, float y) {
	lineprevx = x;
	lineprevy = y;
	linecurrentx = x;
	linecurrenty = y;
	linebegun = true;
}
void FMPlusAudioProcessorEditor::endline() {
	float inversedpi = .5f/scaled_dpi;
	linewritepos += 2;
	float angle = std::atan2(
		(visline[linewritepos*3-5]-visline[linewritepos*3-11])*height,
		(visline[linewritepos*3-6]-visline[linewritepos*3-12])*width )+1.5707963268f;
	visline[linewritepos*3-3] = visline[linewritepos*3-9]+ sin(angle)*inversedpi/width ;
	visline[linewritepos*3-2] = visline[linewritepos*3-8]- cos(angle)*inversedpi/height;
	visline[linewritepos*3-1] = -1.f;
	visline[linewritepos*3  ] = visline[linewritepos*3-6]+ sin(angle)*inversedpi/width ;
	visline[linewritepos*3+1] = visline[linewritepos*3-5]- cos(angle)*inversedpi/height;
	visline[linewritepos*3+2] =  2.f;
	visline[linewritepos*3-9]                            -=sin(angle)*inversedpi/width ;
	visline[linewritepos*3-8]                            +=cos(angle)*inversedpi/height;
	visline[linewritepos*3-7] =  0.f;
	visline[linewritepos*3-6]                            -=sin(angle)*inversedpi/width ;
	visline[linewritepos*3-5]                            +=cos(angle)*inversedpi/height;
	visline[linewritepos*3-4] =  1.f;
}
void FMPlusAudioProcessorEditor::nextpoint(float x, float y, bool knee) {
	float linewidth = .5f+(.5f/scaled_dpi);
	float angle1 = std::atan2(linecurrenty-   lineprevy,linecurrentx-   lineprevx)+1.5707963268f;
	float angle2 = std::atan2(           y-linecurrenty,           x-linecurrentx)+1.5707963268f;
	float angle = 0;
	for(int i = ((linebegun||!knee)?1:0); i < 2; ++i) {
		       if(knee) {
			angle = i==0?angle1:angle2;
		} else if(linebegun) {
			angle = angle2;
		} else if(fabs(angle1-angle2)<=1) {
			angle = (angle1+angle2)*.5f;
		} else if(fabs(linecurrenty-lineprevy) > fabs(y-linecurrenty)) {
			angle = angle1;
		} else {
			angle = angle2;
		}
		visline[++linewritepos*3] = (linecurrentx+cos(angle)*linewidth)/width ;
		visline[1+linewritepos*3] = (linecurrenty+sin(angle)*linewidth)/height;
		visline[2+linewritepos*3] = 0.f;
		visline[++linewritepos*3] = (linecurrentx-cos(angle)*linewidth)/width ;
		visline[1+linewritepos*3] = (linecurrenty-sin(angle)*linewidth)/height;
		visline[2+linewritepos*3] = 1.f;
	}
	if(linebegun) {
		float inversedpi = .5f/scaled_dpi;
		linewritepos += 2;
		visline[linewritepos*3-3] = visline[linewritepos*3-9]+ sin(angle)*inversedpi/width ;
		visline[linewritepos*3-2] = visline[linewritepos*3-8]- cos(angle)*inversedpi/height;
		visline[linewritepos*3-1] =  0.f;
		visline[linewritepos*3  ] = visline[linewritepos*3-6]+ sin(angle)*inversedpi/width ;
		visline[linewritepos*3+1] = visline[linewritepos*3-5]- cos(angle)*inversedpi/height;
		visline[linewritepos*3+2] =  1.f;
		visline[linewritepos*3-9]                            -=sin(angle)*inversedpi/width ;
		visline[linewritepos*3-8]                            +=cos(angle)*inversedpi/height;
		visline[linewritepos*3-7] = -1.f;
		visline[linewritepos*3-6]                            -=sin(angle)*inversedpi/width ;
		visline[linewritepos*3-5]                            +=cos(angle)*inversedpi/height;
		visline[linewritepos*3-4] =  2.f;
		linebegun = false;
	}
	lineprevx = linecurrentx;
	lineprevy = linecurrenty;
	linecurrentx = x;
	linecurrenty = y;
}
void FMPlusAudioProcessorEditor::rebuildtab(int tab) {
	if(selectedtab == tab) return;

	displayaddremove[0] = false;
	for(int o = 0; o < MC; ++o) if(knobs[generalcount].value[o] < .5f) {
		displayaddremove[0] = true;
		break;
	}
	displayaddremove[1] = tab>=3&&knobs[generalcount].value[tab-3]>.5f;
	for(int n = 0; n < 2; ++n) for(int i = 0; i < 6; ++i)
		textmesh[(n*6+i)*4+2] = displayaddremove[n]?((hover+25)==n?2:1):-10;

	opanimation = 0;
	if(selectedtab >= 3 && tab >= 3) {
		selectedtab = tab;
		replacetext(boxes[knobs[generalcount].box].textmesh-6,(String)(selectedtab-2));
		for(int i = 0; i < paramcount  ; ++i) {
			if((i >= 10 && i <= 12) || i == 16) continue;
			updatevalue(i+generalcount);
		}
		updatehighlight(true);

		calcvis(3);

		return;
	}
	tabanimation = 0;
	selectedtab = tab;

	boxnum = 0;
	if(selectedtab == 0) {
		boxes[boxnum++] = box(             -1, 31,  2,120, 10,2,15,true);

		boxes[boxnum++] = box(              0,114, 22, 37, 11,0, 4);
		boxes[boxnum++] = box(              1,114, 34, 37, 11,0, 5);
		boxes[boxnum++] = box(              2,114, 46, 37, 11,0, 3);
		boxes[boxnum++] = box(              3,114, 58, 37, 11,0, 4);
		boxes[boxnum++] = box(              4,114, 70, 37, 11,0, 4,true);

		boxes[boxnum++] = box(              5, 31, 91,  8,  8,3);
		boxes[boxnum++] = box(              6,107,101, 44, 11,0, 6);
		boxes[boxnum++] = box(              7,107,113, 44, 11,0, 4);
		boxes[boxnum++] = box(              8,107,125, 44, 11,0, 6,true);
		knobs[ 9].box = boxnum-1;

		boxes[boxnum++] = box(             10, 31,146,  8,  8,3);
		boxes[boxnum++] = box(             11, 86,156, 65, 11,0, 6);
		knobs[12].box = boxnum-1;
		boxes[boxnum++] = box(             13, 86,168, 65, 11,0, 4);
		boxes[boxnum++] = box(             14, 86,180, 65, 11,0, 5,true);

		boxes[boxnum++] = box(             15,114,201, 37, 11,0, 3);
		boxes[boxnum++] = box(             -1, 31,224,111, 10,2,15);
		boxes[boxnum++] = box(             -1,143,224,  8, 10,2);
		boxes[boxnum++] = box(             -1, 31,246,111, 10,2,15);
		boxes[boxnum++] = box(             -1,143,246,  8, 10,2);

	} else if(selectedtab >= 3) {
		boxes[boxnum++] = box(generalcount   , 31,  2,  8,  8,3);
		boxes[boxnum++] = box(generalcount+ 1, 65, 12, 51, 11,1, 5);
		boxes[boxnum++] = box(generalcount+ 2, 65, 24, 51, 11,0, 4);
		boxes[boxnum++] = box(generalcount+ 3,117,  2, 33, 33,2);
		boxes[boxnum++] = box(generalcount+ 4,121, 36, 29, 11,0, 4);
		boxes[boxnum++] = box(generalcount+ 5,121, 48, 29, 11,0, 4,true);

		boxes[boxnum++] = box(generalcount+ 6, 60, 76,  8, 10,2, 1);
		boxes[boxnum++] = box(             -1, 69, 69, 29,  6,2);
		boxes[boxnum++] = box(generalcount+ 7, 69, 76, 29, 10,2, 4);
		boxes[boxnum++] = box(             -1, 69, 87, 29,  6,2);
		boxes[boxnum++] = box(             -1, 99, 76,  8, 10,2, 1);
		boxes[boxnum++] = box(             -1,108, 69, 29,  6,2);
		boxes[boxnum++] = box(generalcount+ 8,108, 76, 29, 10,2, 4);
		boxes[boxnum++] = box(             -1,108, 87, 29,  6,2);

		boxes[boxnum++] = box(generalcount+ 9, 31,103,120, 43,4);
		boxes[boxnum++] = box(generalcount+10, 31,103,120, 43,4);
		boxes[boxnum++] = box(generalcount+11, 31,103,120, 43,4);
		boxes[boxnum++] = box(generalcount+12, 31,103,120, 43,4, 5,true);

		boxes[boxnum++] = box(generalcount+13, 31,159,  8,  8,3);
		boxes[boxnum++] = box(             -1, 31,168,120, 40,2);
		boxes[boxnum++] = box(generalcount+14, 86,209, 65, 11,0, 5);
		boxes[boxnum++] = box(generalcount+15, 86,221, 65, 11,0, 5);
		knobs[generalcount+16].box = boxnum-1;
		boxes[boxnum++] = box(generalcount+17, 86,233, 65, 11,0, 4);
		boxes[boxnum++] = box(generalcount+18, 86,245, 65, 11,0, 5,true);
	}

	squarelength = -1;
	textlength = 365;
	for(int i = 0; i < boxnum; ++i) {
		if(boxes[i].knob != -1) knobs[boxes[i].knob].box = i;
		if(boxes[i].type == -1) continue;

		boxes[i].mesh = squarelength+1;
		if(boxes[i].type == 0 || boxes[i].type == 1) {
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h,3,boxes[i].corner);
			addsquare(boxes[i].x+round(boxes[i].w*boxes[i].type*.5f),boxes[i].y+boxes[i].h-1,1-boxes[i].type,1,0);
			addtext(30,boxes[i].y,knobs[boxes[i].knob].name+":");
		} else if(boxes[i].type == 2) {
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h,3,boxes[i].corner);
			if(selectedtab == 0) {
				if(i ==  0) {
					addtext(boxes[i].x+109,boxes[i].y,">");
				}
				if(i == 15) {
					addtext(boxes[i].x-1,boxes[i].y-11,"Tuning File:");
				}
				if(i == 16) addtext(boxes[i].x,boxes[i].y,"X");
				if(i == 17) {
					addtext(boxes[i].x-1,boxes[i].y-11,"Theme:");
				}
				if(i == 18) addtext(boxes[i].x,boxes[i].y,"X");
			} else if(selectedtab >= 3) {
				if(i ==  6) addtext(30,boxes[i].y,"Freq");
				if(i ==  7) addtext(boxes[i].x+11,boxes[i].y-2,"<");
				if(i ==  9) addtext(boxes[i].x+11,boxes[i].y-2,">");
				if(i == 11) addtext(boxes[i].x+11,boxes[i].y-2,"<");
				if(i == 12) addsquare(boxes[i].x,boxes[i].y,8,boxes[i].h,0);
				if(i == 13) addtext(boxes[i].x+11,boxes[i].y-2,">");
				if(i == 12) addtext(137,boxes[i].y,"Hz");
			}
			if((selectedtab >= 3 && (i == 7 || i == 9 || i == 11 || i == 13)) || (selectedtab == 0 && i == 0)) {
				textmesh[   (textlength-5)*4] +=  9.f/width ; // top    left
				textmesh[ 1+(textlength-5)*4] -=  1.f/height;
				textmesh[ 4+(textlength-5)*4] += -1.f/width ; // bottom left
				textmesh[ 5+(textlength-5)*4] -= -9.f/height;
				textmesh[ 8+(textlength-5)*4] +=  1.f/width ; // top    right
				textmesh[ 9+(textlength-5)*4] -=  9.f/height;
				textmesh[12+(textlength-5)*4] += -1.f/width ; // bottom left
				textmesh[13+(textlength-5)*4] -= -9.f/height;
				textmesh[16+(textlength-5)*4] +=  1.f/width ; // top    right
				textmesh[17+(textlength-5)*4] -=  9.f/height; textmesh[20+(textlength-5)*4] += -9.f/width ; // bottom right
				textmesh[21+(textlength-5)*4] -= -1.f/height;
			}
		} else if(boxes[i].type == 3) {
			addtext(boxes[i].x+boxes[i].w,boxes[i].y-1,knobs[boxes[i].knob].name);
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h,0,boxes[i].corner);
			addsquare(boxes[i].x+1,boxes[i].y+1,boxes[i].w-2,boxes[i].h-2,1,boxes[i].corner);
		} else if(boxes[i].type == 4) {
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h-11,3);
			addsquare(boxes[i].x,boxes[i].y+boxes[i].h-10,boxes[i].w,10,3,boxes[i].corner);
			addtext(boxes[i].x,boxes[i].y+boxes[i].h-10,knobs[boxes[i].knob].id.substring(0,1));
		}
		boxes[i].textmesh = textlength;
		if(boxes[i].textamount > 0) {
			if(boxes[i].type == 4) {
				addtext(boxes[i].x+boxes[i].w-7*boxes[i].textamount,boxes[i].y+boxes[i].h,String("-----------------",boxes[i].textamount));
			} else {
				addtext(boxes[i].x,boxes[i].y,String("-----------------",boxes[i].textamount));
			}
		}
	}

	if(selectedtab >= 3) { // adsr mesh indicies
		boxes[knobs[ 9+generalcount].box].textmesh = boxes[knobs[12+generalcount].box].textmesh;
		boxes[knobs[10+generalcount].box].textmesh = boxes[knobs[12+generalcount].box].textmesh;
		boxes[knobs[11+generalcount].box].textmesh = boxes[knobs[12+generalcount].box].textmesh;
	}
	if(selectedtab == 0 || selectedtab >= 3) {
		for(int i = (selectedtab>=3?generalcount:-3); i < (selectedtab>=3?(paramcount+generalcount):generalcount); ++i) {
			if(i >= (10+generalcount) && i <= (12+generalcount)) continue;
			if(i == 9 || i == 12 || i == (16+generalcount)) continue;
			if(i == (3+generalcount)) continue;
			if(i >= 0) {
				knobs[i].valuesmoothed = boxes[knobs[i].box].type==1?.5f:0;
				knobs[i].velocity = 0;
			}
			updatevalue(i);
		}
	}
	prevtextindex[0] = 365; // animation sections
	if(selectedtab == 0) {
		prevtextindex[1] = boxes[knobs[              5].box-1].textmesh+boxes[knobs[              5].box-1].textamount*6;
		prevtextindex[2] = boxes[knobs[             10].box-1].textmesh+boxes[knobs[             10].box-1].textamount*6;
		prevtextindex[3] = boxes[knobs[             15].box-1].textmesh+boxes[knobs[             15].box-1].textamount*6;
	} else if(selectedtab >= 3) {
		prevtextindex[1] = boxes[knobs[generalcount+ 6].box-1].textmesh+boxes[knobs[generalcount+ 6].box-1].textamount*6;
		prevtextindex[2] = boxes[knobs[generalcount+ 9].box-1].textmesh+boxes[knobs[generalcount+ 9].box-1].textamount*6;
		prevtextindex[3] = boxes[knobs[generalcount+13].box-1].textmesh+boxes[knobs[generalcount+13].box-1].textamount*6;

		// operator title
		replacetext(boxes[knobs[generalcount].box].textmesh-6,(String)(selectedtab-2));
		for(int p = 0; p < 8; ++p) { // reset adsr animation
			adsr[p*3] = adsr[p*3+1]+.0001f;
			adsr[p*3+2] = 0;
		}
		updatehighlight(true);
	}
	// make invisible pre animation
	for(int i =   0; i < (1+squarelength); ++i) if(squaremesh[2+i*4] >= 0) squaremesh[2+i*4] -= 10;
	for(int i = 366; i < (1+textlength  ); ++i) if(textmesh  [2+i*4] >= 0) textmesh  [2+i*4] -= 10;

	if(selectedtab >= 3) for(int i = 0; i < 4; ++i) calcvis(i);
	else calcvis(0);
	linelength = curvemesh[selectedtab>=3?3:0];
}
void FMPlusAudioProcessorEditor::addsquare(float x, float y, float w, float h, float color, bool corner) {
	squaremesh[++squarelength*4] =     x    /width  ; // top    left
	squaremesh[1+squarelength*4] = 1-( y    /height);
	squaremesh[2+squarelength*4] = color;
	squaremesh[3+squarelength*4] =          100;

	for(int i = 0; i < 2; ++i) {
	squaremesh[++squarelength*4] =     x    /width  ; // bottom left
	squaremesh[1+squarelength*4] = 1-((y+ h)/height);
	squaremesh[2+squarelength*4] = color;
	squaremesh[3+squarelength*4] = corner?w:100;

	squaremesh[++squarelength*4] =    (x+ w)/width  ; // top    right
	squaremesh[1+squarelength*4] = 1-( y    /height);
	squaremesh[2+squarelength*4] = color;
	squaremesh[3+squarelength*4] = corner?h:100;
	}

	squaremesh[++squarelength*4] =    (x+ w)/width  ; // bottom right
	squaremesh[1+squarelength*4] = 1-((y+ h)/height);
	squaremesh[2+squarelength*4] = color;
	squaremesh[3+squarelength*4] = corner?0:100;
}
void FMPlusAudioProcessorEditor::addtext(float x, float y, String txt, float color) {
	int initpos = textlength;
	float s = x;
	for(int n = 0; n < txt.length(); ++n) {

	textmesh  [++textlength  *4] =     s    /width  ; // top    left
	textmesh  [1+textlength  *4] = 1-( y    /height);
	textmesh  [2+textlength  *4] = color;

	for(int i = 0; i < 2; ++i) {
	textmesh  [++textlength  *4] =     s    /width  ; // bottom left
	textmesh  [1+textlength  *4] = 1-((y+10)/height);
	textmesh  [2+textlength  *4] = color;

	textmesh  [++textlength  *4] =    (s+ 8)/width  ; // top    right
	textmesh  [1+textlength  *4] = 1-( y    /height);
	textmesh  [2+textlength  *4] = color;
	}

	textmesh  [++textlength  *4] =    (s+ 8)/width  ; // bottom right
	textmesh  [1+textlength  *4] = 1-((y+10)/height);
	textmesh  [2+textlength  *4] = color;

	s += 7;
	}

	replacetext(initpos,txt);
}
void FMPlusAudioProcessorEditor::replacetext(int id, String txt, int length) {
	const char* text = txt.toUTF8();
	int n = -1;
	bool ended = false;
	while(++n < length || (!ended && text[n] != '\0')) {
	if(text[n] == '\0') ended = true;
	int c = ' ';
	if(!ended) c = text[n]+floor(text[n]/16.f);
	textmesh  [3+(1+n*6+id)  *4] = c   ;
	textmesh  [3+(2+n*6+id)  *4] = c+17;
	textmesh  [3+(3+n*6+id)  *4] = c+ 1;
	textmesh  [3+(4+n*6+id)  *4] = c+17;
	textmesh  [3+(5+n*6+id)  *4] = c+ 1;
	textmesh  [3+(6+n*6+id)  *4] = c+18;
	}
}
void FMPlusAudioProcessorEditor::updatevalue(int param) {
	if(param == -3) { // preset name
		String text = audio_processor.presets[audio_processor.currentpreset].name;
		int textlength = fmin(boxes[0].textamount,text.length());
		if(textlength == (boxes[0].textamount-1)) textlength += 1;
		text = text.substring(0,boxes[0].textamount-(presetunsaved?1:0));
		float x = boxes[0].x+floor((boxes[0].w-14)*.5f-textlength*3.5f);
		for(int i = 0; i < fmin(boxes[0].textamount,textlength+1); ++i) for(int t = 0; t < 6; ++t) {
			if(fmod(fmax(1,fmin(4,t)),2) == 1)
				textmesh[(boxes[0].textmesh+i*6+t+1)*4] = (x+i*7  )/width;
			else
				textmesh[(boxes[0].textmesh+i*6+t+1)*4] = (x+i*7+8)/width;
		}
		if(presetunsaved) text += "*";
		replacetext(boxes[0].textmesh,text,boxes[0].textamount);
		return;
	} else if(param == -2 || param == -1) { // tuning/theme
		int box = knobs[generalcount-1].box+(param==-2?1:3);
		String text = (param==-2?tuningfile:themefile).substring(0,boxes[box].textamount);
		float x = boxes[box].x+floor(boxes[box].w*.5f-text.length()*3.5f);
		for(int i = 0; i < boxes[box].textamount; ++i) for(int t = 0; t < 6; ++t) {
			if(fmod(fmax(1,fmin(4,t)),2) == 1)
				textmesh[(boxes[box].textmesh+i*6+t+1)*4] = (x+i*7  )/width;
			else
				textmesh[(boxes[box].textmesh+i*6+t+1)*4] = (x+i*7+8)/width;
		}
		replacetext(boxes[box].textmesh,text,boxes[box].textamount);
		return;
	}

	if(boxes[knobs[param].box].textamount > 0 && (param < (9+generalcount) || param > (12+generalcount))) {
		int i = param;
		if(i == 8 || i == 11 || i == (15+generalcount))
			if(knobs[i+1].value[selectedtab<3?0:(selectedtab-3)] > 0)
				++i;
		float value = knobs[i].inflate(knobs[i].value[selectedtab<3?0:(selectedtab-3)]);
		String s = get_string(selectedtab==0?i:(i-generalcount),value,selectedtab);
		if(i == (generalcount+7) || i == (generalcount+8)) {
			if(i == (generalcount+8)) {
				if(s.startsWith("-")) s = s.substring(1);
				replacetext(boxes[knobs[i].box].textmesh-6*4,value>=.5?"+":"-");
			}
			if(freqdigit >= -10 && (i==(generalcount+8)) == (freqdigit>=10)) {
				int newdotpos = s.indexOfChar('.');
				if(newdotpos == -1) newdotpos = 4;
				int dotpos = 4;
				for(int n = 0; n < 4; ++n) {
					int c = textmesh[3+(1+n*6+boxes[knobs[i].box].textmesh)*4]-51;
					if(c < 0) {
						dotpos = n;
						break;
					}
				}
				if(newdotpos != dotpos) freqselect(freqdigit+(newdotpos-dotpos),false);
			}
		}
		replacetext(boxes[knobs[i].box].textmesh,s,boxes[knobs[i].box].textamount);
	}

	if(param >= (generalcount+ 9) && param <= (generalcount+12)) {
		float padsr[4] = {
			powf(knobs[generalcount+ 9].value[selectedtab-3],2)*MAXA,
			powf(knobs[generalcount+10].value[selectedtab-3],2)*MAXD,
			1,
			powf(knobs[generalcount+12].value[selectedtab-3],2)*MAXR
		};
		float scaling = (120.f-3)/(padsr[0]+padsr[1]+padsr[2]+padsr[3]);
		for(int p = 0; p < 4; ++p) padsr[p] *= scaling;

		for(int s = 0; s < 2; ++s) {
			float prevovershoot = 0;
			int prevnonclampedsections = 4;
			for(int i = 0; i < 3; ++i) {
				float overshoot = -120.f+3;
				int nonclampedsections = 4;
				for(int p = 0; p < 4; ++p) {
					padsr[p] -= (prevovershoot/prevnonclampedsections);
					if(padsr[p] <= (s==0?1:8)) {
						--nonclampedsections;
						padsr[p] = s==0?1:8;
					}
					overshoot += padsr[p];
				}
				if(overshoot < .5f) break;
				prevnonclampedsections = nonclampedsections;
				prevovershoot = overshoot;
			}

			for(int p = 0; p < 4; ++p) adsr[p*6+s*3+1] = padsr[p];
		}
		float currentx = boxes[knobs[generalcount+9].box].x;
		for(int p = 0; p < 4; ++p) {
			boxes[knobs[generalcount+9+p].box].x = round(              currentx);
			boxes[knobs[generalcount+9+p].box].w = round(adsr[p*6+3+1]+currentx)-round(currentx);
			currentx += adsr[p*6+3+1]+1;
		}
	}
}
void FMPlusAudioProcessorEditor::paint(Graphics& g) { }

void FMPlusAudioProcessorEditor::timerCallback() {
	// website animation
	if(websiteht > -1) websiteht -= .0815;

	// tab animation
	if(tabanimation < 30) {
		if(boxnum > 0) {
			for(int q = 0; q < 2; ++q) {
				int boxbegin = 0;
				int boxend = 0;
				for(int s = 0; s < 4; ++s) {
					     if(s == 0 && selectedtab == 0) boxend = knobs[              5].box-1;
					else if(s == 1 && selectedtab == 0) boxend = knobs[             10].box-1;
					else if(s == 2 && selectedtab == 0) boxend = knobs[             15].box-1;
					else if(s == 0 && selectedtab >= 3) boxend = knobs[generalcount+ 6].box-1;
					else if(s == 1 && selectedtab >= 3) boxend = knobs[generalcount+ 9].box-1;
					else if(s == 2 && selectedtab >= 3) boxend = knobs[generalcount+13].box-1;
					else if(s == 3                    ) boxend = boxnum-1;
					int current = tabanimation+s*3-4;
					if(current >= boxbegin && current <= boxend) {
						if(squaremesh[2+(boxes[current].mesh)*4] < 0)
							for(int i = 0; i < 6; ++i)
								squaremesh[2+(boxes[current].mesh+i)*4] += 10;
					}
					current -= 6;
					if(current >= boxbegin && current <= boxend) {
						if(boxes[current].type != 2)
							if(squaremesh[2+(boxes[current].mesh+6)*4] < 0)
								for(int i = 0; i < 6; ++i)
									squaremesh[2+(boxes[current].mesh+6+i)*4] += 10;
						for(int l = prevtextindex[s]; l < (boxes[current].textmesh+boxes[current].textamount*6); ++l)
							if(textmesh[2+(l+1)*4] < 0)
								textmesh[2+(l+1)*4] += 10;
						prevtextindex[s] = boxes[current].textmesh+boxes[current].textamount*6;
					}
					boxbegin = boxend+1;
				}
				++tabanimation;
			}
		} else {
			tabanimation += 2;
		}
	}
	if(opanimation < 30) ++opanimation;

	// smooth value
	bool updatedadsr = false;
	for(int i = (selectedtab>=3?generalcount:0); i < (selectedtab>=3?(paramcount+generalcount):generalcount); ++i) {
		if(i == 9 || i == 12 || i == (16+generalcount) || i == (10+generalcount) || i == (12+generalcount)) continue; // skip tempo sync, update adsr once
		if(knobs[i].box == -1 || squaremesh[2+(boxes[knobs[i].box].mesh+6)*4] < 0) continue; // box doesnt exist or invisible, skip
		float current = knobs[i].valuesmoothed;
		float target = knobs[i].value[selectedtab<3?0:(selectedtab-3)];
		if(i == 8 || i == 11 || i == (15+generalcount)) // tempo sync is active, use that value instead
			if(knobs[i+1].value[selectedtab<3?0:(selectedtab-3)] > 0)
				target = (knobs[i+1].value[selectedtab<3?0:(selectedtab-3)]*knobs[i+1].maximumvalue-1)/(knobs[i+1].maximumvalue-1);
		if(i == 15 && target >= .5f) target = fmin(6,floor(target*8-1))/6.f; // round second half of anti aliasing knob
		if(i >= (9+generalcount) && i <= (12+generalcount) && i != (11+generalcount)) { // interpolate adsr
			bool updated = false;
			for(int p = 0; p < 8; ++p) {
				if(adsr[p*3] == adsr[p*3+1]) continue;
				updated = true;
				if(fabs(adsr[p*3]-adsr[p*3+1]) <= .0005f) {
					adsr[p*3] = adsr[p*3+1];
					adsr[p*3+2] = 0;
				} else adsr[p*3] = functions::smoothdamp(adsr[p*3],adsr[p*3+1],&adsr[p*3+2],.05f,-1,30);
			}
			if(!updated) continue;
		} else { // clamp to 0
			if(current == target) continue;
			if(fabs(current-target) <= .0005f) {
				current = target;
				knobs[i].velocity = 0;
			}
		}
		knobs[i].valuesmoothed = functions::smoothdamp(current,target,&knobs[i].velocity,boxes[knobs[i].box].type==3?.05f:.05f,-1,30); // interpolate

		if(knobs[i].box != -1 && (boxes[knobs[i].box].type == 0 || boxes[knobs[i].box].type == 1)) {
			int roundedvalue = 0;
			if(boxes[knobs[i].box].type == 0) // regular
				roundedvalue = floor(current*(boxes[knobs[i].box].w-1.99f)+1.99f);
			else // pan
				roundedvalue = floor(current*(boxes[knobs[i].box].w-.99f)+.99f+(.5-fabs(current-.5f)));
			for(int t = 0; t < 6; ++t)
				if(fmod(fmax(1,fmin(4,t)),2) == 0)
					squaremesh[(boxes[knobs[i].box].mesh+t+6)*4] = (boxes[knobs[i].box].x+(float)roundedvalue)/width;
		} else if(boxes[knobs[i].box].type == 3) { // switch
			for(int t = 0; t < 6; ++t)
				squaremesh[(boxes[knobs[i].box].mesh+t+6)*4+2] = 1+current;
		} else if(boxes[knobs[i].box].type == 4) { // adsr
			if(i != (11+generalcount)) for(int s = 0; s < 2; ++s) {
				float currentx = boxes[knobs[generalcount+9].box].x;
				for(int p = 0; p < 4; ++p) {
					int box = knobs[generalcount+9+p].box;
					for(int t = 0; t < 6; ++t) {
						if(fmod(fmax(1,fmin(4,t)),2) == 1)
							squaremesh[(boxes[box].mesh+s*6          +t)*4] = ((float)round(currentx                  )  )/width;
						else
							squaremesh[(boxes[box].mesh+s*6          +t)*4] = ((float)round(currentx+adsr[p*6+s*3]    )  )/width;
					}
					if(s == 1) for(int t = 0; t < 6; ++t) {
						if(fmod(fmax(1,fmin(4,t)),2) == 1)
							textmesh  [(boxes[box].textmesh-(4-p)*6+1+t)*4] = ((float)round(currentx+adsr[p*6+  3]*.5f)-4)/width;
						else
							textmesh  [(boxes[box].textmesh-(4-p)*6+1+t)*4] = ((float)round(currentx+adsr[p*6+  3]*.5f)+4)/width;
					}
					if(p == 3 && s == 1)
						squaremesh[boxes[box].mesh*4+39] = round(currentx+adsr[21])-currentx;
					currentx += adsr[p*6+s*3]+1;
				}
			}
			updatedadsr = true;
		} else if(i == (generalcount+3)) {
			calcvis(1);
		}
	}

	if(audio_processor.updatevis.get()) {
		for(int i = 0; i < MC; i++)
			curves[i] = audio_processor.presets[audio_processor.currentpreset].curves[i];
		calcvis(3);
		audio_processor.updatevis = false;
	}

	// update lines on dpi change
	if(prevdpi != scaled_dpi) {
		prevdpi = scaled_dpi;
		for(int i = 0; i < 4; ++i) calcvis(i);
		updatedadsr = false;
	}

	if(updatedadsr) calcvis(2);

	update();
}

void FMPlusAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; ++i)
		for(int o = 0; o < (i<generalcount?1:MC); ++o)
			if(parameterID == (i<generalcount?knobs[i].id:("o"+(String)o+knobs[i].id))) {

		knobs[i].value[o] = knobs[i].normalize(newValue);

		if(selectedtab == (i<generalcount?0:(o+3))) {
			if(i == 9 || i == 12 || i == (16+generalcount))
				updatevalue(i-1);
			else
				updatevalue(i  );
			if(i >= (9+generalcount) && i <= (12+generalcount))
				updatehighlight(true);
			if(i == generalcount) {
				if(knobs[i].value[o] > .5f) opanimation = 0;
				displayaddremove[1] = knobs[generalcount].value[selectedtab-3]>.5f;
				for(int i = 0; i < 6; ++i)
					textmesh[(6+i)*4+2] = displayaddremove[1]?(hover==-24?2:1):-10;
			}
		}

		if(i == generalcount) {
			replacetext(ops[o+1].textmesh,knobs[i].value[o]>.5f?("OP"+(String)(o+1)):"   ");
			displayaddremove[0] = knobs[i].value[o]<.5f;
			if(!displayaddremove[0]) for(int o = 0; o < MC; ++o) if(knobs[generalcount].value[o] < .5f) {
				displayaddremove[0] = true;
				break;
			}
			for(int i = 0; i < 6; ++i)
				textmesh[i*4+2] = displayaddremove[0]?(hover==-25?2:1):-10;
		}

		if(parameterID != "antialias" && !presetunsaved) {
			presetunsaved = true;
			if(selectedtab == 0) updatevalue(-3);
		}

		return;
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
	if(hover >= 0 && hover < boxnum)
		if(boxes[hover].type != -1 && boxes[hover].type != 3)
			for(int t = 0; t < (boxes[hover].type==4?12:6); ++t)
				squaremesh[(boxes[hover].mesh+t)*4+2] = 3;
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
		if(hover == 19) {
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

	if(boxes[initialdrag].knob == (7+generalcount) || boxes[initialdrag].knob == (8+generalcount)) {
		freqselect(fmin(floor((((float)event.x)/ui_scales[ui_scale_index]-boxes[initialdrag].x)/7.f),3)+(boxes[initialdrag].knob-generalcount-7)*20);

		int dotpos = 4;
		for(int n = 1; n < 4; ++n) {
			int c = textmesh[3+(1+n*6+boxes[initialdrag].textmesh)*4]-51;
			if(c < 0) {
				dotpos = n;
				break;
			}
		}
		freqoffset = knobs[boxes[initialdrag].knob].value[selectedtab-3];
		if(boxes[initialdrag].knob == (7+generalcount))
			freqoffset = knobs[7+generalcount].inflate(freqoffset);
		else
			freqoffset = freqaddinflate(freqoffset);
		     if(dotpos >= 3) freqoffset = round(freqoffset      )     ;
		else if(dotpos == 2) freqoffset = round(freqoffset*10.f )*.1f ;
		else                 freqoffset = round(freqoffset*100.f)*.01f;
		bool flipped = freqoffset<-.005f;
		freqoffset = round(fmod(fabs(freqoffset)+.00001f,freqstep)*100.f)*.01f;
		if(flipped) freqoffset = freqstep-fabs(freqoffset);

	} else freqselect(-20);

	if(initialdrag == -1) return;
	if(initialdrag > -1) {
		if(boxes[initialdrag].knob == -1) {
			if(selectedtab == 0) {
				       if(initialdrag ==  0) { // preset
					// TODO preset select
				} else if(initialdrag == 15 || initialdrag == 17) { // file select
					// TODO file select
				} else if(initialdrag == 16) { // default tuning
					tuningfile = "Standard";
					updatevalue(-2);
				} else if(initialdrag == 18) { // default theme
					themefile = "Default";
					updatevalue(-1);
				}
			} else if(selectedtab >= 3) {
				       if(initialdrag ==  7 || initialdrag ==  9) { // up down freq mult
					float v = knobs[generalcount+7].inflate(knobs[generalcount+7].value[selectedtab-3]);
					float multiplier = 1;
					     if(v < (initialdrag==7?.375f: .75f)) multiplier = .25f;
					else if(v < (initialdrag==7?.75f :1.25f)) multiplier = .5f ;
					audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount+7].id)->setValueNotifyingHost(knobs[generalcount+7].normalize(v+(8-initialdrag)*multiplier));
				} else if(initialdrag == 10) { // sign freq offset
					audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount+8].id)->setValueNotifyingHost(1-knobs[generalcount+8].value[selectedtab-3]);
				} else if(initialdrag == 11 || initialdrag == 13) { // up down freq offset
					float v = freqaddinflate(knobs[generalcount+8].value[selectedtab-3]);
					float s = fabs(get_string(8,knobs[generalcount+8].value[selectedtab-3],selectedtab).getFloatValue());
					int m = initialdrag==11?1:-1;
					if((initialdrag==13) == (v>=0)) s -= .001f;
					     if(s >= 1000) v += 1000*m;
					else if(s >=  100) v +=  100*m;
					else if(s >=   10) v +=   10*m;
					else if(s >=    1) v +=      m;
					else if(s >=   .1) v +=  .1f*m;
					else               v += .01f*m;
					if(fabs(v) <= .005f) v = 0;
					audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount+8].id)->setValueNotifyingHost(freqaddnormalize(v));
				} else if(lfohover >= 0) { // lfo
					valueoffset[0] = 0;
					audio_processor.undo_manager.beginNewTransaction();
					dragpos = event.getScreenPosition();
					event.source.enableUnboundedMouseMovement(true);
					int i = lfohover;
					if((i%2) == 0) {
						i /= 2;
						initialvalue[0] = curves[selectedtab-3].points[i].x;
						initialvalue[1] = curves[selectedtab-3].points[i].y;
						initialdotvalue[0] = initialvalue[0];
						initialdotvalue[1] = initialvalue[1];
						valueoffset[1] = 0;
					} else {
						i = (i-1)/2;
						initialvalue[0] = curves[selectedtab-3].points[i].tension;
						initialdotvalue[0] = initialvalue[0];
					}
				}
			}
		} else {
			String id;
			if(boxes[initialdrag].knob >= generalcount) id = "o"+(String)(selectedtab-3)+knobs[boxes[initialdrag].knob].id;
			else id = knobs[boxes[initialdrag].knob].id;

			initialvalue[0] = knobs[boxes[initialdrag].knob].value[selectedtab==0?0:(selectedtab-3)];

			if(boxes[initialdrag].knob == 8 || boxes[initialdrag].knob == 11 || boxes[initialdrag].knob == (15+generalcount)) {
				if(knobs[boxes[initialdrag].knob+1].value[selectedtab==0?0:(selectedtab-3)] > 0) {
					initialvalue[0] = knobs[boxes[initialdrag].knob+1].value[selectedtab==0?0:(selectedtab-3)];
					initialvalue[0] = 1-(1-fmax(0,initialvalue[0]))/(1-1.f/knobs[boxes[initialdrag].knob+1].maximumvalue);
				}
			}
			     if(boxes[initialdrag].knob == (7+generalcount))
				initialvalue[0] =                  initialvalue[0]              /freqstep*24  /5;
			else if(boxes[initialdrag].knob == (8+generalcount))
				initialvalue[0] = ((freqaddinflate(initialvalue[0])/9999+1)*.5f)/freqstep*9999/5;

			valueoffset[0] = 0;
			audio_processor.undo_manager.beginNewTransaction();
			audio_processor.apvts.getParameter(id)->beginChangeGesture();

			if(knobs[boxes[initialdrag].knob].ttype == potentiometer::ptype::booltype) {
				audio_processor.apvts.getParameter(id)->setValueNotifyingHost(1-initialvalue[0]);
			} else {
				dragpos = event.getScreenPosition();
				event.source.enableUnboundedMouseMovement(true);
			}
		}
	} else if(initialdrag >= -12) {
		rebuildtab(initialdrag+12);
		audio_processor.params.selectedtab = selectedtab;
	} else if(initialdrag >= -23) {
		rebuildtab(initialdrag+23);
		audio_processor.params.selectedtab = selectedtab;
		initialvalue[0] = ops[selectedtab==0?0:(selectedtab-2)].pos[0];
		initialvalue[1] = ops[selectedtab==0?0:(selectedtab-2)].pos[1];
	} else if(initialdrag == -24) {
		if(selectedtab >= 3 &&                knobs[generalcount].value[selectedtab -3] > .5) {
			int prevop = selectedtab-3;
			   for(int o = 0; o < MC; ++o) if(knobs[generalcount].value[(prevop+o+1)%8] > .5) {
				rebuildtab((prevop+o+1)%8+3);
				audio_processor.params.selectedtab = selectedtab;
				break;
			}
			audio_processor.apvts.getParameter("o"+(String)prevop+knobs[generalcount].id)->setValueNotifyingHost(0.f);
		}
	} else if(initialdrag == -25) {
		if(selectedtab >= 3 &&                knobs[generalcount].value[selectedtab -3] < .5) {
			audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount].id)->setValueNotifyingHost(1.f);
		} else for(int o = 0; o < MC; ++o) if(knobs[generalcount].value[        o     ] < .5) {
			rebuildtab(o+3);
			audio_processor.params.selectedtab = selectedtab;
			audio_processor.apvts.getParameter("o"+(String)o              +knobs[generalcount].id)->setValueNotifyingHost(1.f);
			break;
		}
	}
}
void FMPlusAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(initialdrag == -1) return;
	if(initialdrag > -1) {
		if(boxes[initialdrag].knob == -1) {
			if(lfohover < 0) {
				int prevhover = hover;
				hover = initialdrag==recalc_hover(event.x,event.y)?initialdrag:-1;
				updatehighlight();
				if(prevhover == -1 && hover != -1 && boxes[initialdrag].type != -1 && boxes[initialdrag].type != 3)
					for(int t = 0; t < (boxes[initialdrag].type==4?12:6); ++t)
						squaremesh[(boxes[initialdrag].mesh+t)*4+2] = 2;
				return;
			}
			float dragspeed = 1.f/(boxes[initialdrag].w*ui_scales[ui_scale_index]);
			int i = lfohover;
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

				if(i > 0 && i < (curves[selectedtab-3].points.size()-1)) {
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
						if((i > 1 && pointx < curves[selectedtab-3].points[i-1].x) || (i < (curves[selectedtab-3].points.size()-2) && pointx > curves[selectedtab-3].points[i+1].x)) {
							point pnt = curves[selectedtab-3].points[i];
							int n = 1;
							for(n = 1; n < (curves[selectedtab-3].points.size()-1); ++n) //finding new position
								if(pnt.x < curves[selectedtab-3].points[n].x) break;
							if(n > i) { // move points back
								n--;
								for(int f = i; f <= n; f++)
									curves[selectedtab-3].points[f] = curves[selectedtab-3].points[f+1];
							} else { //move points forward
								for(int f = i; f >= n; f--)
									curves[selectedtab-3].points[f] = curves[selectedtab-3].points[f-1];
							}
							i = n;
							lfohover = i*2;
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
							xleft = curves[selectedtab-3].points[i-1].x;
							if(pointx <= curves[selectedtab-3].points[i-1].x) {
								pointx = curves[selectedtab-3].points[i-1].x;
								clamppedx = true;
							}
						}
						if(i < (curves[selectedtab-3].points.size()-2)) {
							xright = curves[selectedtab-3].points[i+1].x;
							if(pointx >= curves[selectedtab-3].points[i+1].x) {
								pointx = curves[selectedtab-3].points[i+1].x;
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
					curves[selectedtab-3].points[i].enabled = (fabs(amioutofbounds[0])+fabs(amioutofbounds[1])) < .8;

					curves[selectedtab-3].points[i].x = pointx;
					if(axislock != 2)
						valueoffset[0] = fmax(fmin(valueoffset[0],valuex-xleft+.1f),valuex-xright-.1f);
				}
				curves[selectedtab-3].points[i].y = fmax(fmin(pointy,1),0);
				if(axislock != 1)
					valueoffset[1] = fmax(fmin(valueoffset[1],valuey+.1f),valuey-1.1f);
			} else { //dragging tension
				i = (i-1)/2;
				int dir = curves[selectedtab-3].points[i].y > curves[selectedtab-3].points[i+1].y ? -1 : 1;
				if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
					finemode = true;
					initialvalue[0] -= dir*event.getDistanceFromDragStartY()*dragspeed*.9f*3.f;
				} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
					finemode = false;
					initialvalue[0] += dir*event.getDistanceFromDragStartY()*dragspeed*.9f*3.f;
				}

				float value = initialvalue[0]-dir*event.getDistanceFromDragStartY()*dragspeed*(finemode?.1f:1)*3.f;
				curves[selectedtab-3].points[i].tension = fmin(fmax(value-valueoffset[0],0),1);

				valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
			}
			calcvis(3);
		} else {
			String id = knobs[boxes[initialdrag].knob].id;
			int bpmmax = -1;
			if(boxes[initialdrag].knob == 8 || boxes[initialdrag].knob == 11 || boxes[initialdrag].knob == (15+generalcount)) {
				if(event.mods.isCtrlDown() != (boxes[initialdrag].knob == 8)) {
					id = knobs[boxes[initialdrag].knob+1].id;
					bpmmax = knobs[boxes[initialdrag].knob+1].maximumvalue;
				} else if(knobs[boxes[initialdrag].knob].value[selectedtab==0?0:(selectedtab-3)] > 0) {
					String bpmid = knobs[boxes[initialdrag].knob+1].id;
					if(boxes[initialdrag].knob >= generalcount) bpmid = "o"+(String)(selectedtab-3)+bpmid;
					audio_processor.apvts.getParameter(bpmid)->setValueNotifyingHost(0);
				}
			}
			if(boxes[initialdrag].knob >= generalcount) id = "o"+(String)(selectedtab-3)+id;

			if(knobs[boxes[initialdrag].knob].ttype == potentiometer::ptype::booltype) {
				int prevhover = hover;
				hover = initialdrag==recalc_hover(event.x,event.y)?initialdrag:-1;
				updatehighlight();
				if(prevhover == -1 && hover != -1 && boxes[initialdrag].type != -1 && boxes[initialdrag].type != 3)
					for(int t = 0; t < (boxes[initialdrag].type==4?12:6); ++t)
						squaremesh[(boxes[initialdrag].mesh+t)*4+2] = 2;
				if(knobs[boxes[initialdrag].knob].value[selectedtab==0?0:(selectedtab-3)] != (initialdrag==hover?(1-initialvalue[0]):initialvalue[0]))
					audio_processor.apvts.getParameter(id)->setValueNotifyingHost(initialdrag==hover?(1-initialvalue[0]):initialvalue[0]);
			} else {
				       if(!finemode &&  (event.mods.isShiftDown() || event.mods.isAltDown()) && boxes[initialdrag].knob != (7+generalcount) && boxes[initialdrag].knob != (8+generalcount)) {
					finemode = true;
					initialvalue[0] -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
				} else if( finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
					finemode = false;
					initialvalue[0] += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
				}

				float value = initialvalue[0]-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
				float updatevalue = value;
				float clampres = .1f;

				       if(boxes[initialdrag].knob == (7+generalcount)) {
					clampres = .1f*freqstep/24  *5;
					value = value *freqstep/24  *5;
					updatevalue = knobs[7+generalcount].inflate(value-valueoffset[0]);
					if(!(event.mods.isShiftDown() || event.mods.isAltDown()))
						updatevalue = floor(fmax(fmin(updatevalue,  24-freqoffset),    0)/freqstep+.00001f)*freqstep;
					updatevalue = knobs[7+generalcount].normalize(updatevalue+freqoffset);
				} else if(boxes[initialdrag].knob == (8+generalcount)) {
					clampres = .1f*freqstep/9999*5;
					value = value *freqstep/9999*5;
					updatevalue = (9999*(2*                    (value-valueoffset[0])-1));
					if(!(event.mods.isShiftDown() || event.mods.isAltDown()))
						updatevalue = floor(fmax(fmin(updatevalue,9999-freqoffset),-9999.1f)/freqstep+.00001f)*freqstep;
					updatevalue = freqaddnormalize               (updatevalue+freqoffset);

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
	} else if(initialdrag >= -23) {
		int o = selectedtab==0?0:(selectedtab-2);
		ops[o].pos[0] = fmin(112,fmax(0,initialvalue[0]+event.getDistanceFromDragStartX()/ui_scales[ui_scale_index]));
		ops[o].pos[1] = fmin(257,fmax(0,initialvalue[1]+event.getDistanceFromDragStartY()/ui_scales[ui_scale_index]));
		audio_processor.presets[audio_processor.currentpreset].oppos[o*2  ] = ops[o].pos[0];
		audio_processor.presets[audio_processor.currentpreset].oppos[o*2+1] = ops[o].pos[1];

		float x = ops[o].pos[0]+162;
		float y = ops[o].pos[1]+30;
		int s = ops[o].textmesh+1;
		for(int n = 0; n < 3; ++n) {
			textmesh[   (s+n*6)*4] =     x    /width  ; // top    left
			textmesh[ 1+(s+n*6)*4] = 1-( y    /height);

			textmesh[ 4+(s+n*6)*4] =     x    /width  ; // bottom left
			textmesh[ 5+(s+n*6)*4] = 1-((y+10)/height);

			textmesh[ 8+(s+n*6)*4] =    (x+ 8)/width  ; // top    right
			textmesh[ 9+(s+n*6)*4] = 1-( y    /height);

			textmesh[12+(s+n*6)*4] =     x    /width  ; // bottom left
			textmesh[13+(s+n*6)*4] = 1-((y+10)/height);

			textmesh[16+(s+n*6)*4] =    (x+ 8)/width  ; // top    right
			textmesh[17+(s+n*6)*4] = 1-( y    /height);

			textmesh[20+(s+n*6)*4] =    (x+ 8)/width  ; // bottom right
			textmesh[21+(s+n*6)*4] = 1-((y+10)/height);
			x += 7;
		}
	} else if(initialdrag == -26) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y)==-26?-26:-1;
		if(hover == -26 && prevhover != -26 && websiteht < -1) websiteht = .65f;
	}
}
void FMPlusAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(initialdrag > -1) {
		if(boxes[initialdrag].knob == -1) {
			if(lfohover < 0) {
				hover = recalc_hover(event.x,event.y);
				updatehighlight();
				return;
			}
			event.source.enableUnboundedMouseMovement(false);
			Desktop::setMousePosition(dragpos);
			axislock = -1;
			int i = lfohover;
			if((i%2) == 0) {
				i /= 2;
				if((fabs(initialdotvalue[0]-curves[selectedtab-3].points[i].x)+fabs(initialdotvalue[1]-curves[selectedtab-3].points[i].y)) < .00001) {
					curves[selectedtab-3].points[i].x = initialdotvalue[0];
					curves[selectedtab-3].points[i].y = initialdotvalue[1];
					return;
				}
				dragpos.x += (curves[selectedtab-3].points[i].x-initialdotvalue[0])*ui_scales[ui_scale_index]*boxes[initialdrag].w;
				dragpos.y += (initialdotvalue[1]-curves[selectedtab-3].points[i].y)*ui_scales[ui_scale_index]*boxes[initialdrag].h;
				if(!curves[selectedtab-3].points[i].enabled) {
					curves[selectedtab-3].points.erase(curves[selectedtab-3].points.begin()+i);
					audio_processor.deletepoint(i);
				} else audio_processor.movepoint(i,curves[selectedtab-3].points[i].x,curves[selectedtab-3].points[i].y);
			} else {
				i = (i-1)/2;
				if(fabs(initialdotvalue[0]-curves[selectedtab-3].points[i].tension) < .00001) {
					curves[selectedtab-3].points[i].tension = initialdotvalue[0];
					return;
				}
				float interp = curve::calctension(.5,curves[selectedtab-3].points[i].tension)-curve::calctension(.5,initialdotvalue[0]);
				dragpos.y += (curves[selectedtab-3].points[i].y-curves[selectedtab-3].points[i+1].y)*interp*ui_scales[ui_scale_index]*200.f;
				audio_processor.movetension(i,curves[selectedtab-3].points[i].tension);
			}
			if((lfohover%2) == 0) audio_processor.undo_manager.setCurrentTransactionName("Moved point");
			else audio_processor.undo_manager.setCurrentTransactionName("Moved tension");
			audio_processor.undo_manager.beginNewTransaction();
		} else {
			String id;
			if(boxes[initialdrag].knob >= generalcount) id = "o"+(String)(selectedtab-3)+knobs[boxes[initialdrag].knob].id;
			else id = knobs[boxes[initialdrag].knob].id;

			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((knobs[boxes[initialdrag].knob].value[selectedtab==0?0:(selectedtab-3)]-initialvalue[0])>=0?"Increased ":"Decreased ") += knobs[boxes[initialdrag].knob].name);
			audio_processor.apvts.getParameter(id)->endChangeGesture();
			audio_processor.undo_manager.beginNewTransaction();

			if(knobs[boxes[initialdrag].knob].ttype != potentiometer::ptype::booltype) {
				event.source.enableUnboundedMouseMovement(false);
				Desktop::setMousePosition(dragpos);
			}

			hover = recalc_hover(event.x,event.y);
			updatehighlight();
		}
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		updatehighlight();
		if(hover == -26) {
			if(prevhover == -26) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht < -1) websiteht = .65f;
		}
	}
	initialdrag = -1;
}
void FMPlusAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	if(boxes[hover].knob == -1) {
		if(selectedtab < 3 || hover != 19) return;
		if(lfohover < 0) {
			float x = fmin(fmax((((float)event.x)/ui_scales[ui_scale_index]-boxes[hover].x)/boxes[hover].w,0),1);
			float y = fmin(fmax(1-(((float)event.y)/ui_scales[ui_scale_index]-boxes[hover].y)/boxes[hover].h,0),1);
			int i = 1;
			for(i = 1; i < curves[selectedtab-3].points.size(); ++i)
				if(x < curves[selectedtab-3].points[i].x) break;
			curves[selectedtab-3].points.insert(curves[selectedtab-3].points.begin()+i,point(x,y,curves[selectedtab-3].points[i-1].tension));
			audio_processor.addpoint(i,x,y);
		} else {
			int i = lfohover;
			if((i%2) == 0) {
				i /= 2;
				if(i > 0 && i < (curves[selectedtab-3].points.size()-1)) {
					curves[selectedtab-3].points.erase(curves[selectedtab-3].points.begin()+i);
					audio_processor.deletepoint(i);
				}
			} else {
				i = (i-1)/2;
				if(fabs(curves[selectedtab-3].points[i].tension-.5) < .00001f) return;
				curves[selectedtab-3].points[i].tension = .5f;
				audio_processor.movetension(i,.5f);
			}
		}
		hover = recalc_hover(event.x,event.y);
		calcvis(3);
	} else {
		if(knobs[boxes[hover].knob].ttype == potentiometer::ptype::booltype) return;

		String id;
		if(boxes[hover].knob >= generalcount) id = "o"+(String)(selectedtab-3)+knobs[boxes[hover].knob].id;
		else id = knobs[boxes[hover].knob].id;

		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[boxes[hover].knob].name);
		audio_processor.apvts.getParameter(id)->setValueNotifyingHost(knobs[boxes[hover].knob].defaultvalue);
		if(boxes[hover].knob == 8 || boxes[hover].knob == 11 || boxes[hover].knob == (15+generalcount)) {
			String id = knobs[boxes[hover].knob+1].id;
			if(boxes[hover].knob >= generalcount) id = "o"+(String)(selectedtab-3)+id;
			audio_processor.apvts.getParameter(id)->setValueNotifyingHost(knobs[boxes[hover].knob+1].defaultvalue);
		}
		audio_processor.undo_manager.beginNewTransaction();
	}
}
void FMPlusAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(initialdrag != -1 || hover <= -1) return;
	if(boxes[hover].knob == -1) return;
	if(knobs[boxes[hover].knob].ttype == potentiometer::ptype::booltype) return;

	String id = knobs[boxes[hover].knob].id;
	float value = knobs[boxes[hover].knob].value[selectedtab==0?0:(selectedtab-3)];
	float range = knobs[boxes[hover].knob].ttype==potentiometer::ptype::floattype?-1:knobs[boxes[hover].knob].maximumvalue-knobs[boxes[hover].knob].minimumvalue;
	int bpmmax = -1;
	if(boxes[hover].knob == 8 || boxes[hover].knob == 11 || boxes[hover].knob == (15+generalcount)) { // bpm sync
		if(knobs[boxes[hover].knob+1].value[selectedtab==0?0:(selectedtab-3)] > 0) {
			value = knobs[boxes[hover].knob+1].value[selectedtab==0?0:(selectedtab-3)];
			value = 1-(1-fmax(0,value))/(1-1.f/knobs[boxes[hover].knob+1].maximumvalue);
		}
		if(event.mods.isCtrlDown() != (boxes[hover].knob == 8)) {
			id = knobs[boxes[hover].knob+1].id;
			bpmmax = knobs[boxes[hover].knob+1].maximumvalue;
			range = knobs[boxes[hover].knob+1].ttype==potentiometer::ptype::floattype?-1:knobs[boxes[hover].knob+1].maximumvalue-knobs[boxes[hover].knob+1].minimumvalue;
		} else if(knobs[boxes[hover].knob].value[selectedtab==0?0:(selectedtab-3)] > 0) {
			String bpmid = knobs[boxes[hover].knob+1].id;
			if(boxes[hover].knob >= generalcount) bpmid = "o"+(String)(selectedtab-3)+bpmid;
			audio_processor.apvts.getParameter(bpmid)->setValueNotifyingHost(0);
		}
	}
	if(boxes[hover].knob >= generalcount) id = "o"+(String)(selectedtab-3)+id;
	if(boxes[hover].knob == (7+generalcount) || boxes[hover].knob == (8+generalcount)) { // freq
		int a = fmin(floor((((float)event.x)/ui_scales[ui_scale_index]-boxes[hover].x)/7.f),3)+(boxes[hover].knob-generalcount-7)*20;
		if(scrolldigit != a) {
			freqselect(a);
			scrolldigit = a;
		}

		       if(boxes[hover].knob == (7+generalcount)) {
			value = knobs[7+generalcount].inflate  (value);
			if(event.mods.isShiftDown() || event.mods.isAltDown())
				value +=  wheel.deltaY*2.f        *freqstep;
			else if((value+(wheel.deltaY>0?1.f:-1.f)*freqstep) >=     0 && (value+(wheel.deltaY>0?1.f:-1.f)*freqstep) <=   24)
				value += (wheel.deltaY>0?1.f:-1.f)*freqstep;
			value = knobs[7+generalcount].normalize(value);
		} else if(boxes[hover].knob == (8+generalcount)) {
			value = freqaddinflate                 (value);
			if(event.mods.isShiftDown() || event.mods.isAltDown())
				value +=  wheel.deltaY*2.f        *freqstep;
			else if((value+(wheel.deltaY>0?1.f:-1.f)*freqstep) >= -9999 && (value+(wheel.deltaY>0?1.f:-1.f)*freqstep) <= 9999)
				value += (wheel.deltaY>0?1.f:-1.f)*freqstep;
			value = freqaddnormalize               (value);
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
		if((code==45) == (knobs[generalcount+8].value[selectedtab-3]>=.5f))
			audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount+8].id)->setValueNotifyingHost(1-knobs[generalcount+8].value[selectedtab-3]);
		return true;
	}

	// slash or backslash or asterisk
	if((code == 47 || code == 92 || code == 42) && freqdigit <= 10) {
		// change mult mode
		audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount+6].id)->setValueNotifyingHost(code==42?1:0);
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
			audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount+8].id)->setValueNotifyingHost(               freqaddnormalize(               freqaddinflate(knobs[generalcount+8].value[selectedtab-3])+(up?1:-1)*freqstep));
		else
			audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount+7].id)->setValueNotifyingHost(knobs[generalcount+7].normalize(knobs[generalcount+7].inflate(knobs[generalcount+7].value[selectedtab-3])+(up?1:-1)*freqstep));
		return true;
	}

	if(code != 46 && (code < 48 || code > 57)) return false;

	int current = fmin(3,fmax(0,freqdigit>10?(freqdigit-20):freqdigit));
	int dotpos = 4;
	int nums[4];
	for(int n = 0; n < 4; ++n) {
		nums[n] = textmesh[3+(1+n*6+boxes[knobs[generalcount+(freqdigit>10?8:7)].box].textmesh)*4]-51;
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
		audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount+7].id)->setValueNotifyingHost(knobs[7+generalcount].normalize(v));
	} else {
		if(knobs[generalcount+8].value[selectedtab-3] < .5f) v *= -1;
		audio_processor.apvts.getParameter("o"+(String)(selectedtab-3)+knobs[generalcount+8].id)->setValueNotifyingHost(               freqaddnormalize(v));
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

	lfohover = -1;

	// -26 logo
	if(x > 224.5f && x < 298.5f && y > 1.5f && y < 24.5f)
		return -26;

	// -25 - -24 +-
	float xx = x-163.5f;
	float yy = y-36.5f;
	if((xx*xx+yy*yy) <= 30.25f && displayaddremove[0]) return -25;
	yy -= 11;
	if((xx*xx+yy*yy) <= 30.25f && displayaddremove[1]) return -24;

	// -23 - -13 operators
	for(int o = 0; o < (MC+1); ++o) {
		if(o != 0 && knobs[generalcount].value[o==0?0:(o-1)] < .5) continue;
		if(x >= (ops[o].pos[0]+157) && x < (ops[o].pos[0]+187) && y >= (ops[o].pos[1]+30) && y < (ops[o].pos[1]+42))
			return o==0?-23:(o-21);
	}

	// -12 - -2 tabs
	if(x <= 29) {
		int i = floor(y/11.f);
		if(i >= 11) return -1;
		return i-12;
	}

	// 0 - 23 boxes
	for(int i = 0; i < boxnum; ++i) {
		if(i == 19 && selectedtab >= 3 && x >= (boxes[i].x-4) && x < (boxes[i].x+boxes[i].w+4) && y >= (boxes[i].y-4) && y < (boxes[i].y+boxes[i].h+4)) {
			x -= boxes[i].x;
			y -= boxes[i].y;
			for(int p = 0; p < curves[selectedtab-3].points.size(); ++p) {
				float xx = x-curves[selectedtab-3].points[p].x*boxes[i].w;
				float yy = y-(1-curves[selectedtab-3].points[p].y)*boxes[i].h;
				// dot
				if((xx*xx+yy*yy) <= 8) {
					lfohover = p*2;
					return 19;
				}

				if(p < (curves[selectedtab-3].points.size()-1)) {
					float interp = curve::calctension(.5,curves[selectedtab-3].points[p].tension);
					xx = x-(curves[selectedtab-3].points[p].x+curves[selectedtab-3].points[p+1].x)*.5f*boxes[i].w;
					yy = y-(1-(curves[selectedtab-3].points[p].y*(1-interp)+curves[selectedtab-3].points[p+1].y*interp))*boxes[i].h;
					// tension
					if((xx*xx+yy*yy) <= 8) {
						lfohover = p*2+1;
						return 19;
					}
				}
			}

			// curve bg
			if(x >= 0 && y >= 0 && x <= boxes[i].w && y <= boxes[i].h) {
				lfohover = -1;
				return 19;
			}
		} else if(boxes[i].type != -1 && x >= boxes[i].x && x < (boxes[i].x+boxes[i].w) && y >= boxes[i].y && y < (boxes[i].y+boxes[i].h)) {
			return i;
		}
	}

	// 24+ node connections

	return -1;
}
void FMPlusAudioProcessorEditor::updatehighlight(bool update_adsr) {
	if(highlight == hover && !update_adsr) return;
	bool adsrlen = update_adsr;
	if(highlight >= 0 && highlight < boxnum) {
		if(boxes[highlight].type != -1 && boxes[highlight].type != 3)
			for(int t = 0; t < (boxes[highlight].type==4?12:6); ++t)
				squaremesh[(boxes[highlight].mesh+t)*4+2] = 3;

		if((boxes[highlight].knob == (7+generalcount) && freqdigit >= -10 && freqdigit < 10) || (boxes[highlight].knob == (8+generalcount) && freqdigit >= 10))
			for(int t = 0; t < 6; ++t)
				textmesh[(boxes[highlight].textmesh+6*(int)fmin(3,fmax(0,freqdigit>=10?(freqdigit-20):freqdigit))+t+1)*4+2] = 3;

		if(boxes[highlight].knob >= (9+generalcount) && boxes[highlight].knob <= (12+generalcount))
			adsrlen = true;
	} else if((highlight == -25 || highlight == -24) && displayaddremove[highlight+25]) {
		for(int t = 0; t < 6; ++t)
			textmesh[((highlight+25)*6+t)*4+2] = 1;
	}
	if(hover     >= 0 && hover     < boxnum) {
		if(boxes[hover    ].type != -1 && boxes[hover    ].type != 3)
			for(int t = 0; t < (boxes[hover    ].type==4?12:6); ++t)
				squaremesh[(boxes[hover    ].mesh+t)*4+2] = 2;

		if((boxes[hover    ].knob == (7+generalcount) && freqdigit >= -10 && freqdigit < 10) || (boxes[hover    ].knob == (8+generalcount) && freqdigit >= 10))
			for(int t = 0; t < 6; ++t)
				textmesh[(boxes[hover    ].textmesh+6*(int)fmin(3,fmax(0,freqdigit>=10?(freqdigit-20):freqdigit))+t+1)*4+2] = 2;

		if(boxes[hover    ].knob >= (9+generalcount) && boxes[hover    ].knob <= (12+generalcount)) {
			adsrlen = false;
			String s = get_string(boxes[hover].knob-generalcount,knobs[boxes[hover].knob].value[selectedtab-3],selectedtab);
			replacetext(boxes[hover].textmesh,s.paddedLeft(' ',5));
		}
	} else if((hover     == -25 || hover     == -24) && displayaddremove[hover    +25]) {
		for(int t = 0; t < 6; ++t)
			textmesh[((hover    +25)*6+t)*4+2] = 2;
	}
	if(adsrlen) {
		String s = format_time(powf(knobs[9+generalcount].value[selectedtab-3],2)*MAXA+powf(knobs[10+generalcount].value[selectedtab-3],2)*MAXD+powf(knobs[12+generalcount].value[selectedtab-3],2)*MAXR);
		replacetext(boxes[knobs[9+generalcount].box].textmesh,s.paddedLeft(' ',5));
	}
	highlight = hover;
}
void FMPlusAudioProcessorEditor::freqselect(int digit, bool calcfreqstep) {
	int square = boxes[knobs[generalcount+8].box].mesh;
	int nbox = knobs[generalcount+(    digit>10?8:7)].box;
	int pbox = knobs[generalcount+(freqdigit>10?8:7)].box;
	if((digit<-10) != (freqdigit<-10)) {
		if(digit >= -10)
			grabKeyboardFocus();
		else
			giveAwayKeyboardFocus();
		for(int t = 0; t < 6; ++t)
				squaremesh[(square             +6              +t  )*4+2] = digit<-10?-10:0;
	}
	if(freqdigit >= -10) {
		for(int t = 0; t < 6; ++t)
				textmesh  [(boxes[pbox].textmesh+6*(int)fmin(3,fmax(0,freqdigit>=10?(freqdigit-20):freqdigit))+t+1)*4+2] = 0;
	}
	if(digit >= -10) {
		int clampeddigit =                         (int)fmin(3,fmax(0,    digit>=10?(    digit-20):    digit));
		for(int t = 0; t < 6; ++t)
				textmesh  [(boxes[nbox].textmesh+6*clampeddigit                                               +t+1)*4+2] = hover==nbox?2:3;
		for(int t = 0; t < 6; ++t) {
			if(fmod(fmax(1,fmin(4,t)),2) == 1)
				squaremesh[(square              +6              +t  )*4  ] = (boxes[nbox].x+clampeddigit*7.f  )/width;
			else
				squaremesh[(square              +6              +t  )*4  ] = (boxes[nbox].x+clampeddigit*7.f+8)/width;
		}
	}
	freqdigit = digit;

	if(freqdigit >= -10 && calcfreqstep) {
		int dotpos = 4;
		for(int n = 1; n < 4; ++n) {
			int c = textmesh[3+(1+n*6+boxes[nbox].textmesh)*4]-51;
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
