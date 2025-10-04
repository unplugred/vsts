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

	rebuildtab(params.selectedtab.get());

	setResizable(false,false);
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
out vec4 fragColor;
void main() {
	if(col < -1)
		fragColor = vec4(0,0,0,0);
	else
		fragColor = vec4(col_outline*max(0,1-col)+col_conf*(1-abs(col-1))+col_highlight*max(0,col-1),max(min((texture(texttex,uv).r-.5)*dpi+.5,1),0));
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
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	}

	// LINE

	// CIRCLE

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

void FMPlusAudioProcessorEditor::calcvis() {
	for(int c = 0; c < 1; c++) {
		for(int i = 0; i < 226; i++) {
			visline[c][i*2] = (i/112.5f)*(1-(16.f/width))+(16.f/width)-1;
			visline[c][i*2+1] = 1-(62+sin(i/35.8098621957f)*.8f*38)/(height*.5f);
		}
	}
}
void FMPlusAudioProcessorEditor::rebuildtab(int tab) {
	if(selectedtab == tab) return;
	if(selectedtab >= 3 && tab >= 3) {
		opanimation = 0;
		selectedtab = tab;
		replacetext(boxes[knobs[generalcount].box].textmesh-6,(String)(selectedtab-2));
		for(int i = 0; i < paramcount  ; ++i) {
			if((i >= 10 && i <= 12) || i == 16) continue;
			updatevalue(i+generalcount);
		}
		updatehighlight(true);
		return;
	}
	tabanimation = 0;
	opanimation = 0;
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
		boxes[boxnum++] = box(              6,107,101, 44, 11,0, 4);
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
	textlength = 353;
	for(int i = 0; i < boxnum; ++i) {
		if(boxes[i].knob != -1) knobs[boxes[i].knob].box = i;
		if(boxes[i].type == -1) continue;

		boxes[i].mesh = squarelength+1;
		if(boxes[i].type == 0 || boxes[i].type == 1) {
			addtext(30,boxes[i].y,knobs[boxes[i].knob].name+":");
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h,3,boxes[i].corner);
			addsquare(boxes[i].x+round(boxes[i].w*boxes[i].type*.5f),boxes[i].y+boxes[i].h-1,1-boxes[i].type,1,0);
		} else if(boxes[i].type == 2) {
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
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h,3,boxes[i].corner);
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

	if(selectedtab >= 3) {
		boxes[knobs[ 9+generalcount].box].textmesh = boxes[knobs[12+generalcount].box].textmesh;
		boxes[knobs[10+generalcount].box].textmesh = boxes[knobs[12+generalcount].box].textmesh;
		boxes[knobs[11+generalcount].box].textmesh = boxes[knobs[12+generalcount].box].textmesh;
	}
	for(int i = (selectedtab>=3?generalcount:-3); i < (selectedtab>=3?(paramcount+generalcount):generalcount); ++i) {
		if(i >= (10+generalcount) && i <= (12+generalcount)) continue;
		if(i == 9 || i == 12 || i == (16+generalcount)) continue;
		if(i >= 0) {
			knobs[i].valuesmoothed = boxes[knobs[i].box].type==1?.5f:0;
			knobs[i].velocity = 0;
		}
		updatevalue(i);
	}
	prevtextindex[0] = 353;
	if(selectedtab == 0) {
		prevtextindex[1] = boxes[knobs[              5].box-1].textmesh+boxes[knobs[              5].box-1].textamount*6;
		prevtextindex[2] = boxes[knobs[             10].box-1].textmesh+boxes[knobs[             10].box-1].textamount*6;
		prevtextindex[3] = boxes[knobs[             15].box-1].textmesh+boxes[knobs[             15].box-1].textamount*6;
	} else if(selectedtab >= 3) {
		prevtextindex[1] = boxes[knobs[generalcount+ 6].box-1].textmesh+boxes[knobs[generalcount+ 6].box-1].textamount*6;
		prevtextindex[2] = boxes[knobs[generalcount+ 9].box-1].textmesh+boxes[knobs[generalcount+ 9].box-1].textamount*6;
		prevtextindex[3] = boxes[knobs[generalcount+13].box-1].textmesh+boxes[knobs[generalcount+13].box-1].textamount*6;

		replacetext(boxes[knobs[generalcount].box].textmesh-6,(String)(selectedtab-2));
		for(int p = 0; p < 8; ++p) {
			adsr[p*3] = adsr[p*3+1]+.0001f;
			adsr[p*3+2] = 0;
		}
		updatehighlight(true);
	}
	for(int i =   0; i < (1+squarelength); ++i) if(squaremesh[2+i*4] >= 0) squaremesh[2+i*4] -= 10;
	for(int i = 354; i < (1+textlength  ); ++i) if(textmesh  [2+i*4] >= 0) textmesh  [2+i*4] -= 10;
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
	if(param == -3) {
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
	} else if(param == -2) {
		int box = knobs[generalcount-1].box+1;
		String text = tuningfile.substring(0,boxes[box].textamount);
		float x = boxes[box].x+floor(boxes[box].w*.5f-text.length()*3.5f);
		for(int i = 0; i < boxes[box].textamount; ++i) for(int t = 0; t < 6; ++t) {
			if(fmod(fmax(1,fmin(4,t)),2) == 1)
				textmesh[(boxes[box].textmesh+i*6+t+1)*4] = (x+i*7  )/width;
			else
				textmesh[(boxes[box].textmesh+i*6+t+1)*4] = (x+i*7+8)/width;
		}
		replacetext(boxes[box].textmesh,text,boxes[box].textamount);
		return;
	} else if(param == -1) {
		int box = knobs[generalcount-1].box+3;
		String text = themefile.substring(0,boxes[box].textamount);
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
		if(i == (generalcount+8)) {
			if(s.startsWith("-")) s = s.substring(1);
			replacetext(boxes[knobs[i].box].textmesh-6*4,value>=.5?"+":"-");
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
		if(i == 9 || i == 12 || i == (16+generalcount) || (updatedadsr && i >= (9+generalcount) && i <= (12+generalcount))) continue; // skip tempo sync, update adsr once
		if(knobs[i].box == -1 || squaremesh[2+(boxes[knobs[i].box].mesh+6)*4] < 0) continue; // box doesnt exist or invisible, skip
		float current = knobs[i].valuesmoothed;
		float target = knobs[i].value[selectedtab<3?0:(selectedtab-3)];
		if(i == 8 || i == 11 || i == (15+generalcount)) // tempo sync is active, use that value instead
			if(knobs[i+1].value[selectedtab<3?0:(selectedtab-3)] > 0)
				target = (knobs[i+1].value[selectedtab<3?0:(selectedtab-3)]*knobs[i+1].maximumvalue-1)/(knobs[i+1].maximumvalue-1);
		if(i == 15 && target >= .5f) target = fmin(6,floor(target*8-1))/6.f; // round second half of anti aliasing knob
		if(i >= (9+generalcount) && i <= (12+generalcount)) { // interpolate adsr
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

		if(knobs[i].box != -1 && boxes[knobs[i].box].type == 0 || boxes[knobs[i].box].type == 1) {
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
			updatedadsr = true;
			for(int s = 0; s < 2; ++s) {
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
		}
	}

	// calculate rms
	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;

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
		}

		if(i == generalcount)
			replacetext(ops[o+1].textmesh,knobs[i].value[o]>.5f?("OP"+(String)(o+1)):"   ");

		if(parameterID != "antialias" && !presetunsaved) {
			presetunsaved = true;
			if(selectedtab == 0) updatevalue(-3);
		}

		return;
	}
}
void FMPlusAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	updatehighlight();
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
	if(initialdrag == -1) return;
	if(initialdrag > -1) {
		if(boxes[initialdrag].knob == -1) {
			// TODO special buttons
			if(selectedtab == 0) {
				       if(initialdrag ==  0) { // preset
				} else if(initialdrag == 15 || initialdrag == 17) { // file select
				} else if(initialdrag == 16) { // default tuning
					tuningfile = "Standard";
					updatevalue(-2);
				} else if(initialdrag == 18) { // default theme
					themefile = "Default";
					updatevalue(-1);
				}
			} else if(selectedtab >= 3) {
				       if(initialdrag ==  7) { // up freq mult
				} else if(initialdrag ==  9) { // down freq mult
				} else if(initialdrag == 10) { // sign freq offset
				} else if(initialdrag == 11) { // up freq offset
				} else if(initialdrag == 13) { // down freq offset
				} else if(initialdrag == 19) { // lfo
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
	}
}
void FMPlusAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(initialdrag == -1) return;
	if(initialdrag > -1) {
		if(boxes[initialdrag].knob == -1) {
			if(selectedtab < 3 || initialdrag != 19) {
				int prevhover = hover;
				hover = initialdrag==recalc_hover(event.x,event.y)?initialdrag:-1;
				updatehighlight();
				if(prevhover == -1 && hover != -1 && boxes[initialdrag].type != -1 && boxes[initialdrag].type != 3)
					for(int t = 0; t < (boxes[initialdrag].type==4?12:6); ++t)
						squaremesh[(boxes[initialdrag].mesh+t)*4+2] = 2;
				return;
			}
			// TODO lfo
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
				if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
					finemode = true;
					initialvalue[0] -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
				} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
					finemode = false;
					initialvalue[0] += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
				}

				float value = initialvalue[0]-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
				float updatevalue = value-valueoffset[0];
				if(bpmmax > 0) updatevalue = 1-(1-fmax(0,updatevalue))*(1-1.f/bpmmax);
				audio_processor.apvts.getParameter(id)->setValueNotifyingHost(updatevalue);

				valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
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
			if(selectedtab < 3 || initialdrag != 19) {
				hover = recalc_hover(event.x,event.y);
				updatehighlight();
				return;
			}
			// TODO lfo
		} else {
			String id;
			if(boxes[initialdrag].knob >= generalcount) id = "o"+(String)(selectedtab-3)+knobs[boxes[initialdrag].knob].id;
			else id = knobs[boxes[initialdrag].knob].id;

			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((knobs[boxes[initialdrag].knob].value[selectedtab==0?0:(selectedtab-3)]-initialvalue[0])>=0?"Increased ":"Decreased ") += knobs[boxes[initialdrag].knob].name);
			audio_processor.apvts.getParameter(id)->endChangeGesture();
			audio_processor.undo_manager.beginNewTransaction();

			if(knobs[boxes[initialdrag].knob].ttype == potentiometer::ptype::booltype) {
				hover = recalc_hover(event.x,event.y);
				updatehighlight();
			} else {
				event.source.enableUnboundedMouseMovement(false);
				Desktop::setMousePosition(dragpos);
			}
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
}
void FMPlusAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	if(boxes[hover].knob == -1) {
		if(selectedtab < 3 || hover != 19) return;
		// TODO lfo
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
	if(hover <= -1) return;
	if(boxes[hover].knob == -1) return;
	if(knobs[boxes[hover].knob].ttype == potentiometer::ptype::booltype) return;

	String id = knobs[boxes[hover].knob].id;
	float value = knobs[boxes[hover].knob].value[selectedtab==0?0:(selectedtab-3)];
	float range = knobs[boxes[hover].knob].ttype==potentiometer::ptype::floattype?-1:knobs[boxes[hover].knob].maximumvalue-knobs[boxes[hover].knob].minimumvalue;
	int bpmmax = -1;
	if(boxes[hover].knob == 8 || boxes[hover].knob == 11 || boxes[hover].knob == (15+generalcount)) {
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
	if(range < 0)
		value += wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f);
	else
		value += (wheel.deltaY>0?1.f:-1.f)*fmax(1.f/range,fabs(wheel.deltaY)*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	if(bpmmax > 0) value = 1-(1-fmax(0,value))*(1-1.f/bpmmax);
	audio_processor.apvts.getParameter(id)->setValueNotifyingHost(value);
}
int FMPlusAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	// -26 logo
	if(x > 224.5f && x < 298.5f && y > 1.5f && y < 24.5f)
		return -26;

	// -25 - -24 +-

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
		if(boxes[i].type != -1 && x >= boxes[i].x && x < (boxes[i].x+boxes[i].w) && y >= boxes[i].y && y < (boxes[i].y+boxes[i].h)) {
			if(selectedtab >= 3 && i == 19) {
				// TODO lfo
			}
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

		if(boxes[highlight].knob >= (9+generalcount) && boxes[highlight].knob <= (12+generalcount))
			adsrlen = true;
	}
	if(hover     >= 0 && hover     < boxnum) {
		if(boxes[hover    ].type != -1 && boxes[hover    ].type != 3)
			for(int t = 0; t < (boxes[hover    ].type==4?12:6); ++t)
				squaremesh[(boxes[hover    ].mesh+t)*4+2] = 2;

		if(boxes[hover    ].knob >= (9+generalcount) && boxes[hover    ].knob <= (12+generalcount)               ) {
			adsrlen = false;
			String s = get_string(boxes[hover].knob-generalcount,knobs[boxes[hover].knob].value[selectedtab-3],selectedtab);
			replacetext(boxes[hover].textmesh,s.paddedLeft(' ',5));
		}
	}
	if(adsrlen) {
		String s = format_time(powf(knobs[9+generalcount].value[selectedtab-3],2)*MAXA+powf(knobs[10+generalcount].value[selectedtab-3],2)*MAXD+powf(knobs[12+generalcount].value[selectedtab-3],2)*MAXR);
		replacetext(boxes[knobs[9+generalcount].box].textmesh,s.paddedLeft(' ',5));
	}
	highlight = hover;
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
