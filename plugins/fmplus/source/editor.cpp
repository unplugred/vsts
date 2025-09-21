#include "processor.h"
#include "editor.h"

FMPlusAudioProcessorEditor::FMPlusAudioProcessorEditor(FMPlusAudioProcessor& p, int pgeneralcount, int pparamcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 300, 300, 2.f, .5f) {
	generalcount = pgeneralcount+1;
	paramcount = pparamcount;
	selectedtab = params.selectedtab.get();
	for(int i = 0; i < pgeneralcount; ++i) {
		knobs[knobcount].id = params.general[i].id;
		knobs[knobcount].name = params.general[i].name;
		knobs[knobcount].value[0] = params.general[i].normalize(state.general[i]);
		knobs[knobcount].isbool = params.general[i].ttype == potentiometer::ptype::booltype;
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
		knobs[knobcount].isbool = params.values[i].ttype == potentiometer::ptype::booltype;
		knobs[knobcount].minimumvalue = params.values[i].minimumvalue;
		knobs[knobcount].maximumvalue = params.values[i].maximumvalue;
		knobs[knobcount].defaultvalue = params.values[i].normalize(params.values[i].defaultvalue);
		for(int o = 0; o < MC; ++o) {
			knobs[knobcount].value[o] = params.values[i].normalize(state.values[o][i]);
			add_listener("o"+(String)o+knobs[knobcount].id);
		}
		++knobcount;
	}
	for(int o = 0; o < (MC+1); ++o) {
		ops[o].connections = state.opconnections[o];
		ops[o].indicator = (o==0||o==5)?1.f:0.f;
		ops[o].pos[0] = state.oppos[o*2  ];
		ops[o].pos[1] = state.oppos[o*2+1];
	}

	rebuildtab();

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
		vec3 col = max(min((texture(headertex,vec2(headeruv.x,(headeruv.y+(10-tabid))/11)).rgb-.5)*dpi+.5,1),0);
		fragColor = vec4(col_conf*(1-col)+col_conf_mod*col,1);
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
uniform vec3 col_conf_mod;
uniform vec3 col_highlight;
out vec4 fragColor;
void main() {
	if(col.x < 2.5)
		fragColor = vec4(col_outline*min(1,2-col.x)+col_outline_mod*max(0,col.x-1),min(1,col.x));
	else
		fragColor = vec4(col_conf_mod*(4-col.x)+col_highlight*(col.x-3),min(1,max(0,(col.y-2.5)*dpi+.5)));
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
uniform vec3 col_highlight;
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
	col.rgb = (col_conf*(col.g-col.b)+col_conf_mod*col.b)*(1-selected)+col_highlight*col.g*selected+col_outline*col.r;
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

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize);
	add_texture(&tabselecttex, BinaryData::tabselect_png, BinaryData::tabselect_pngSize);
	add_texture(&headertex, BinaryData::header_png, BinaryData::header_pngSize);
	add_texture(&creditstex, BinaryData::credits_png, BinaryData::credits_pngSize);
	add_texture(&operatortex, BinaryData::operator_png, BinaryData::operator_pngSize);

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
	// colors:
	// 0 - transparent
	// 1 - outline
	// 2 - outline mod
	// 3 - conf mod
	// 4 - highlight
	if(currentpoint >= 0) {
		squareshader->use();
		squareshader->setUniform("banner",banner_offset);
		squareshader->setUniform("dpi",scaled_dpi);
		squareshader->setUniform("col_outline"		,col_outline[0]		,col_outline[1]		,col_outline[2]		);
		squareshader->setUniform("col_outline_mod"	,col_outline_mod[0]	,col_outline_mod[1]	,col_outline_mod[2]	);
		squareshader->setUniform("col_conf_mod"		,col_conf_mod[0]	,col_conf_mod[1]	,col_conf_mod[2]	);
		squareshader->setUniform("col_highlight"	,col_highlight[0]	,col_highlight[1]	,col_highlight[2]	);
		coord = context.extensions.glGetAttribLocation(squareshader->getProgramID(),"coords");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,4,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(currentpoint+1)*4, squaremesh, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES,0,currentpoint+1);
		context.extensions.glDisableVertexAttribArray(coord);
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
	operatorshader->setUniform("col_conf"			,col_conf[0]		,col_conf[1]		,col_conf[2]		);
	operatorshader->setUniform("col_conf_mod"		,col_conf_mod[0]	,col_conf_mod[1]	,col_conf_mod[2]	);
	operatorshader->setUniform("col_outline"		,col_outline[0]		,col_outline[1]		,col_outline[2]		);
	operatorshader->setUniform("col_highlight"		,col_highlight[0]	,col_highlight[1]	,col_highlight[2]	);
	coord = context.extensions.glGetAttribLocation(operatorshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	for(int o = MC; o >= 0; --o) {
		if(o != 0 && knobs[generalcount].value[o==0?0:(o-1)] < .5) continue;
		operatorshader->setUniform("selected",(float)(selectedtab==(o==0?0:(o+2))?1.f:0.f));
		operatorshader->setUniform("pos",(153.f+ops[o].pos[0])/width,(256.f-ops[o].pos[1])/height,36.f/width,18.f/height);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
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
void FMPlusAudioProcessorEditor::rebuildtab() {
	boxnum = 0;
	if(selectedtab == 0) {
		boxes[boxnum++] = box(             -1, 31,  2,120, 10,2,true);

		boxes[boxnum++] = box(              0,114, 22, 37, 11,0);
		boxes[boxnum++] = box(              1,114, 34, 37, 11,0);
		boxes[boxnum++] = box(              2,114, 46, 37, 11,0);
		boxes[boxnum++] = box(              3,114, 58, 37, 11,0);
		boxes[boxnum++] = box(              4,114, 70, 37, 11,0,true);

		boxes[boxnum++] = box(              5, 31, 91,  8,  8,3);
		boxes[boxnum++] = box(              6,107,101, 44, 11,0);
		knobs[ 7].box = boxnum-1;
		boxes[boxnum++] = box(              8,107,113, 44, 11,0);
		boxes[boxnum++] = box(              9,107,125, 44, 11,0,true);

		boxes[boxnum++] = box(             10, 31,146,  8,  8,3);
		boxes[boxnum++] = box(             11, 86,156, 65, 11,0);
		knobs[12].box = boxnum-1;
		boxes[boxnum++] = box(             13, 86,168, 65, 11,0);
		boxes[boxnum++] = box(             14, 86,180, 65, 11,0,true);

		boxes[boxnum++] = box(             15,114,201, 37, 11,0);
		boxes[boxnum++] = box(             -1, 31,224,111, 10,2);
		boxes[boxnum++] = box(             -1,143,224,  8, 10,2);
		boxes[boxnum++] = box(             -1, 31,246,111, 10,2);
		boxes[boxnum++] = box(             -1,143,246,  8, 10,2);

	} else if(selectedtab >= 3) {
		boxes[boxnum++] = box(generalcount   , 31,  2,  8,  8,3);
		boxes[boxnum++] = box(generalcount+ 1, 65, 12, 51, 11,1);
		boxes[boxnum++] = box(generalcount+ 2, 65, 24, 51, 11,0);
		boxes[boxnum++] = box(generalcount+ 3,117,  2, 33, 33,2);
		boxes[boxnum++] = box(generalcount+ 4,121, 36, 29, 11,0);
		boxes[boxnum++] = box(generalcount+ 5,121, 48, 29, 11,0,true);

		boxes[boxnum++] = box(generalcount+ 6, 60, 76,  8, 10,2);
		boxes[boxnum++] = box(             -1, 69, 69, 29,  6,2);
		boxes[boxnum++] = box(generalcount+ 7, 69, 76, 29, 10,2);
		boxes[boxnum++] = box(             -1, 69, 87, 29,  6,2);
		boxes[boxnum++] = box(             -1, 99, 76,  8, 10,2);
		boxes[boxnum++] = box(             -1,108, 69, 29,  6,2);
		boxes[boxnum++] = box(generalcount+ 8,108, 76, 29, 10,2);
		boxes[boxnum++] = box(             -1,108, 87, 29,  6,2);

		boxes[boxnum++] = box(generalcount+ 9, 31,103, 24, 43,4);
		boxes[boxnum++] = box(generalcount+10, 56,103, 17, 43,4);
		boxes[boxnum++] = box(generalcount+11, 74,103, 41, 43,4);
		boxes[boxnum++] = box(generalcount+12,116,103, 35, 43,4,true);

		boxes[boxnum++] = box(generalcount+13, 31,159,  8,  8,3);
		boxes[boxnum++] = box(             -1, 31,168,120, 40,2);
		boxes[boxnum++] = box(generalcount+14, 86,209, 65, 11,0);
		boxes[boxnum++] = box(generalcount+15, 86,221, 65, 11,0);
		knobs[16].box = boxnum-1;
		boxes[boxnum++] = box(generalcount+17, 86,233, 65, 11,0);
		boxes[boxnum++] = box(generalcount+18, 86,245, 65, 11,0,true);
	}

	currentpoint = -1;
	for(int i = 0; i < boxnum; ++i) {
		if(boxes[i].knob != -1) knobs[boxes[i].knob].box = i;
		if(boxes[i].type == -1) continue;

		float value = 0;
		if(boxes[i].knob >= 0) value = knobs[boxes[i].knob].value[selectedtab==0?0:(selectedtab-3)];

		boxes[i].mesh = currentpoint+1;
		if(boxes[i].type == 0 || boxes[i].type == 1) {
			int roundedvalue = 0;
			if(boxes[i].knob == 15 && value >= .5f)
				value = floor(value*8)/8.f;
			if(boxes[i].type == 0)
				roundedvalue = floor(value*(boxes[i].w-1.99f)+1.99f);
			else
				roundedvalue = floor(value*(boxes[i].w-.99f)+.99f+(.5-fabs(value-.5f)));
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h,3,boxes[i].corner);
			addsquare(boxes[i].x+round(boxes[i].w*boxes[i].type*.5f),boxes[i].y+boxes[i].h-1,roundedvalue-round(boxes[i].w*boxes[i].type*.5f),1,1);
		} else if(boxes[i].type == 2) {
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h,3,boxes[i].corner);
		} else if(boxes[i].type == 3) {
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h,1,boxes[i].corner);
			addsquare(boxes[i].x+1,boxes[i].y+1,boxes[i].w-2,boxes[i].h-2,value>.5?4:2,boxes[i].corner);
		} else if(boxes[i].type == 4) {
			addsquare(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h-11,3);
			addsquare(boxes[i].x,boxes[i].y+boxes[i].h-10,boxes[i].w,10,3,boxes[i].corner);
		}
	}

	if(selectedtab >= 3) updateburger();
}
void FMPlusAudioProcessorEditor::addsquare(float x, float y, float w, float h, float color, bool corner) {
	squaremesh[++currentpoint*4] =     x   /width  ; // top    left
	squaremesh[1+currentpoint*4] = 1-( y   /height);
	squaremesh[2+currentpoint*4] = color;
	squaremesh[3+currentpoint*4] =          100;

	for(int i = 0; i < 2; ++i) {
	squaremesh[++currentpoint*4] =     x   /width  ; // bottom left
	squaremesh[1+currentpoint*4] = 1-((y+h)/height);
	squaremesh[2+currentpoint*4] = color;
	squaremesh[3+currentpoint*4] = corner?w:100;

	squaremesh[++currentpoint*4] =    (x+w)/width  ; // top    right
	squaremesh[1+currentpoint*4] = 1-( y   /height);
	squaremesh[2+currentpoint*4] = color;
	squaremesh[3+currentpoint*4] = corner?h:100;
	}

	squaremesh[++currentpoint*4] =    (x+w)/width  ; // bottom right
	squaremesh[1+currentpoint*4] = 1-((y+h)/height);
	squaremesh[2+currentpoint*4] = color;
	squaremesh[3+currentpoint*4] = corner?0:100;
}
void FMPlusAudioProcessorEditor::updateburger() {
	float adsr[4] = {
		knobs[generalcount+ 9].value[selectedtab-3]*MAXA,
		knobs[generalcount+10].value[selectedtab-3]*MAXD,
		1,
		knobs[generalcount+12].value[selectedtab-3]*MAXR };
	float scaling = (120.f-3)/(adsr[0]+adsr[1]+adsr[2]+adsr[3]);
	for(int p = 0; p < 4; ++p) adsr[p] *= scaling;

	for(int s = 0; s < 2; ++s) {
		float prevovershoot = 0;
		int prevnonclampedsections = 4;
		for(int i = 0; i < 3; ++i) {
			float overshoot = -120.f+3;
			int nonclampedsections = 4;
			for(int p = 0; p < 4; ++p) {
				adsr[p] -= (prevovershoot/prevnonclampedsections);
				if(adsr[p] <= (s==0?1:8)) {
					--nonclampedsections;
					adsr[p] = s==0?1:8;
				}
				overshoot += adsr[p];
			}
			if(overshoot < .5f) break;
			prevnonclampedsections = nonclampedsections;
			prevovershoot = overshoot;
		}

		float currentx = boxes[knobs[generalcount+9].box].x;
		for(int p = 0; p < 4; ++p) {
			for(int t = 0; t < 6; ++t) {
				if(fmod(fmax(1,fmin(4,t)),2) == 1)
					squaremesh[(boxes[knobs[generalcount+9+p].box].mesh+s*6+t)*4] = ((float)round(currentx        ))/width;
				else
					squaremesh[(boxes[knobs[generalcount+9+p].box].mesh+s*6+t)*4] = ((float)round(currentx+adsr[p]))/width;
			}
			currentx += adsr[p]+1;
		}
	}
	squaremesh[(boxes[knobs[generalcount+12].box].mesh+6+3)*4+3] = adsr[3];

	float currentx = boxes[knobs[generalcount+9].box].x;
	for(int p = 0; p < 4; ++p) {
		boxes[knobs[generalcount+9+p].box].x = round(        currentx);
		boxes[knobs[generalcount+9+p].box].w = round(adsr[p]+currentx)-round(currentx);
		currentx += adsr[p]+1;
	}
}
void FMPlusAudioProcessorEditor::paint(Graphics& g) { }

void FMPlusAudioProcessorEditor::timerCallback() {
	if(websiteht > -1) websiteht -= .0815;

	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;

	time = fmod(time+.0002f,1.f);

	update();
}

void FMPlusAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; ++i)
		for(int o = 0; o < (i<generalcount?1:MC); ++o)
			if(parameterID == (i<generalcount?knobs[i].id:("o"+(String)o+knobs[i].id))) {

		knobs[i].value[o] = knobs[i].normalize(newValue);

		if(selectedtab == (i<generalcount?0:(o+3))) {
			float value = knobs[i].value[o];
			if(boxes[knobs[i].box].type == 0 || boxes[knobs[i].box].type == 1) {
				int roundedvalue = 0;
				if(i == 15 && value >= .5f)
					value = fmin(6,floor(value*8-1))/6.f;
				if(boxes[knobs[i].box].type == 0)
					roundedvalue = floor(value*(boxes[knobs[i].box].w-1.99f)+1.99f);
				else
					roundedvalue = floor(value*(boxes[knobs[i].box].w-.99f)+.99f+(.5-fabs(value-.5f)));
				for(int t = 0; t < 6; ++t)
					if(fmod(fmax(1,fmin(4,t)),2) == 0)
						squaremesh[(boxes[knobs[i].box].mesh+t+6)*4] = (boxes[knobs[i].box].x+(float)roundedvalue)/width;
			} else if(boxes[knobs[i].box].type == 3) {
				for(int t = 0; t < 6; ++t)
					squaremesh[(boxes[knobs[i].box].mesh+t+6)*4+2] = value>.5?4:2;
			} else if(boxes[knobs[i].box].type == 4) {
				updateburger();
			}
		}
		return;
	}
}
void FMPlusAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
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
				if(initialdrag == 0) { // preset
				} else if(initialdrag == 15 || initialdrag == 17) { // file select
				} else if(initialdrag == 16) { // default theme
				} else if(initialdrag == 18) { // default tuning
				}
			} else if(selectedtab >= 3) {
				if(initialdrag == 7) { // up freq mult
				} else if(initialdrag == 9) { // down freq mult
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
			valueoffset[0] = 0;
			audio_processor.undo_manager.beginNewTransaction();
			audio_processor.apvts.getParameter(id)->beginChangeGesture();

			if(knobs[boxes[initialdrag].knob].isbool) {
				audio_processor.apvts.getParameter(id)->setValueNotifyingHost(1-initialvalue[0]);
			} else {
				dragpos = event.getScreenPosition();
				event.source.enableUnboundedMouseMovement(true);
			}
		}
	} else if(initialdrag >= -12) {
		selectedtab = initialdrag+12;
		audio_processor.params.selectedtab = selectedtab;
		rebuildtab();
	} else if(initialdrag >= -23) {
		selectedtab = initialdrag+23;
		audio_processor.params.selectedtab = selectedtab;
		rebuildtab();
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
				hover = initialdrag==recalc_hover(event.x,event.y,false)?initialdrag:-1;
				if(prevhover == -1 && hover != -1 && boxes[initialdrag].type != -1 && boxes[initialdrag].type != 3)
					for(int t = 0; t < (boxes[initialdrag].type==4?12:6); ++t)
						squaremesh[(boxes[initialdrag].mesh+t)*4+2] = 4;
				return;
			}
			// TODO lfo
		} else {
			String id;
			if(boxes[initialdrag].knob >= generalcount) id = "o"+(String)(selectedtab-3)+knobs[boxes[initialdrag].knob].id;
			else id = knobs[boxes[initialdrag].knob].id;

			if(knobs[boxes[initialdrag].knob].isbool) {
				int prevhover = hover;
				hover = initialdrag==recalc_hover(event.x,event.y,false)?initialdrag:-1;
				if(prevhover == -1 && hover != -1 && boxes[initialdrag].type != -1 && boxes[initialdrag].type != 3)
					for(int t = 0; t < (boxes[initialdrag].type==4?12:6); ++t)
						squaremesh[(boxes[initialdrag].mesh+t)*4+2] = 4;
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
				audio_processor.apvts.getParameter(id)->setValueNotifyingHost(value-valueoffset[0]);

				valueoffset[0] = fmax(fmin(valueoffset[0],value+.1f),value-1.1f);
			}
		}
	} else if(initialdrag >= -12) {
		return;
	} else if(initialdrag >= -23) {
		ops[selectedtab==0?0:(selectedtab-2)].pos[0] = fmin(112,fmax(0,initialvalue[0]+event.getDistanceFromDragStartX()/ui_scales[ui_scale_index]));
		ops[selectedtab==0?0:(selectedtab-2)].pos[1] = fmin(257,fmax(0,initialvalue[1]+event.getDistanceFromDragStartY()/ui_scales[ui_scale_index]));
		audio_processor.presets[audio_processor.currentpreset].oppos[(selectedtab==0?0:(selectedtab-2))*2  ] = ops[selectedtab==0?0:(selectedtab-2)].pos[0];
		audio_processor.presets[audio_processor.currentpreset].oppos[(selectedtab==0?0:(selectedtab-2))*2+1] = ops[selectedtab==0?0:(selectedtab-2)].pos[1];
	} else if(initialdrag == -26) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y,false)==-26?-26:-1;
		if(hover == -26 && prevhover != -26 && websiteht < -1) websiteht = .65f;
	}
}
void FMPlusAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(initialdrag > -1) {
		if(boxes[initialdrag].knob == -1) {
			if(selectedtab < 3 || initialdrag != 19) {
				hover = recalc_hover(event.x,event.y);
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

			if(knobs[boxes[initialdrag].knob].isbool) {
				hover = recalc_hover(event.x,event.y);
			} else {
				event.source.enableUnboundedMouseMovement(false);
				Desktop::setMousePosition(dragpos);
			}
		}
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		if(hover == -26) {
			if(prevhover == -26) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht < -1) websiteht = .65f;
		}
	}
}
void FMPlusAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	if(boxes[hover].knob == -1) {
		if(selectedtab < 3 || initialdrag != 19) return;
		// TODO lfo
	} else {
		if(knobs[boxes[hover].knob].isbool) return;

		String id;
		if(boxes[hover].knob >= generalcount) id = "o"+(String)(selectedtab-3)+knobs[boxes[hover].knob].id;
		else id = knobs[boxes[hover].knob].id;

		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[boxes[hover].knob].name);
		audio_processor.apvts.getParameter(id)->setValueNotifyingHost(knobs[boxes[hover].knob].defaultvalue);
		audio_processor.undo_manager.beginNewTransaction();
	}
}
void FMPlusAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	if(boxes[hover].knob == -1) return;
	if(knobs[boxes[hover].knob].isbool) return;

	String id;
	if(boxes[hover].knob >= generalcount) id = "o"+(String)(selectedtab-3)+knobs[boxes[hover].knob].id;
	else id = knobs[boxes[hover].knob].id;

	audio_processor.apvts.getParameter(id)->setValueNotifyingHost(
		knobs[boxes[hover].knob].value[selectedtab==0?0:(selectedtab-3)]+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));

}
int FMPlusAudioProcessorEditor::recalc_hover(float x, float y, bool update_highlights) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	// 0+ knobs
	for(int i = 0; i < boxnum; ++i) {
		if(boxes[i].type != -1 && x >= boxes[i].x && x < (boxes[i].x+boxes[i].w) && y >= boxes[i].y && y < (boxes[i].y+boxes[i].h)) {
			if(hover != i) {
				if(hover >= 0 && hover < boxnum)
					if(boxes[hover].type != -1 && boxes[hover].type != 3)
						for(int t = 0; t < (boxes[hover].type==4?12:6); ++t)
							squaremesh[(boxes[hover].mesh+t)*4+2] = 3;
				if(update_highlights && boxes[i].type != -1 && boxes[i].type != 3)
					for(int t = 0; t < (boxes[i].type==4?12:6); ++t)
						squaremesh[(boxes[i].mesh+t)*4+2] = 4;
			}
			return i;
		}
	}
	if(hover >= 0 && hover < boxnum)
		if(boxes[hover].type != -1 && boxes[hover].type != 3)
			for(int t = 0; t < (boxes[hover].type==4?12:6); ++t)
				squaremesh[(boxes[hover].mesh+t)*4+2] = 3;

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

	// maxboxnum+ node connections

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
