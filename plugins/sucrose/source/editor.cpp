#include "processor.h"
#include "editor.h"

SucroseAudioProcessorEditor::SucroseAudioProcessorEditor(SucroseAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 356, 382, 1.5f) {
	for(int i = 0; i < paramcount; i++) {
		knobs[i].id = params.pots[i].id;
		knobs[i].name = params.pots[i].name;
		knobs[i].value = params.pots[i].normalize(state.values[i]);
		knobs[i].minimumvalue = params.pots[i].minimumvalue;
		knobs[i].maximumvalue = params.pots[i].maximumvalue;
		knobs[i].defaultvalue = params.pots[i].normalize(params.pots[i].defaultvalue);
		knobcount++;
		add_listener(knobs[i].id);
	}

	knobs[0].x[0] = 26;
	knobs[0].x[1] = 158;
	knobs[0].y[0] = 210;
	knobs[0].y[1] = 212;
	knobs[0].y[2] = 211;
	knobs[0].y[3] = 209;
	knobs[0].y[4] = 212;
	knobs[0].y[5] = 211;
	knobs[0].y[6] = 208;
	knobs[0].y[7] = 210;

	knobs[1].x[0] = 199;
	knobs[1].x[1] = 331;
	knobs[1].y[0] = 211;
	knobs[1].y[1] = 212;
	knobs[1].y[2] = 212;
	knobs[1].y[3] = 208;
	knobs[1].y[4] = 213;
	knobs[1].y[5] = 212;
	knobs[1].y[6] = 211;
	knobs[1].y[7] = 208;

	knobs[2].x[0] = 26;
	knobs[2].x[1] = 158;
	knobs[2].y[0] = 109;
	knobs[2].y[1] = 110;
	knobs[2].y[2] = 108;
	knobs[2].y[3] = 110;
	knobs[2].y[4] = 110;
	knobs[2].y[5] = 111;
	knobs[2].y[6] = 110;
	knobs[2].y[7] = 108;

	knobs[3].x[0] = 193;
	knobs[3].x[1] = 322;
	knobs[3].y[0] = 113;
	knobs[3].y[1] = 114;
	knobs[3].y[2] = 115;
	knobs[3].y[3] = 112;
	knobs[3].y[4] = 112;
	knobs[3].y[5] = 112;
	knobs[3].y[6] = 111;
	knobs[3].y[7] = 105;

	knobs[4].x[0] = 30;
	knobs[4].x[1] = 107;
	knobs[4].y[0] = 45;
	knobs[4].y[1] = 47;
	knobs[4].y[2] = 58;
	knobs[4].y[3] = 62;
	knobs[4].y[4] = 64;
	knobs[4].y[5] = 72;
	knobs[4].y[6] = 79;
	knobs[4].y[7] = 84;

	knobs[5].x[0] = 138;
	knobs[5].x[1] = 218;
	knobs[5].y[0] = 83;
	knobs[5].y[1] = 76;
	knobs[5].y[2] = 72;
	knobs[5].y[3] = 69;
	knobs[5].y[4] = 64;
	knobs[5].y[5] = 55;
	knobs[5].y[6] = 49;
	knobs[5].y[7] = 46;

	for(int i = 0; i < 6; ++i) {
		knobs[i].a[0] = std::atan2(knobs[i].y[3]-knobs[i].y[0],(knobs[i].x[1]-knobs[i].x[0])*.5f);
		knobs[i].a[1] = std::atan2(knobs[i].y[7]-knobs[i].y[4],(knobs[i].x[1]-knobs[i].x[0])*.5f);
	}

	for(int i = 0; i < 2; ++i) {
		logopos[i*3  ] = random.nextFloat()*(i==0?.2f:.28f)+(i==0?.16f:.4f);
		logopos[i*3+1] = random.nextFloat()*.09f;
		logopos[i*3+2] = (random.nextFloat()+.2f)*(i==0?.08f:.06f)*(random.nextFloat()>.5f?1:-1);
		offset[i] = .003f+random.nextFloat()*.003f;
	}

	angledamp.reset(0,.5f,-1,30,4);
	noisegen.init();
	for(int i = 0; i < 4; ++i) {
		scribble[SPEED*2*i+(SPEED-1)*2  ] = 160+171*fmod(i,2);
		scribble[SPEED*2*i+(SPEED-1)*2+1] = 263-moveup[i]*.5f-100*floor(i*.5f);
	}
	for(int i = 0; i < SPEED*1.5f; ++i) calcnext();
	calcvis();

	setResizable(false,false);
	init(&look_n_feel);
}
SucroseAudioProcessorEditor::~SucroseAudioProcessorEditor() {
	close();
}

void SucroseAudioProcessorEditor::newOpenGLContextCreated() {
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec3 rota;
uniform vec3 rotb;
uniform vec2 algoprog;
out vec2 uv;
out vec4 webuv;
out vec4 cuv;
void main() {
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner))*2-1,0,1);
	uv = aPos;
	webuv = vec4(
		 (uv.x-rota.x)*cos(rota.z)-(uv.y-rota.y)*.9319371728*sin(rota.z)             +rota.x,
		((uv.x-rota.x)*sin(rota.z)+(uv.y-rota.y)*.9319371728*cos(rota.z))/.9319371728+rota.y,
		 (uv.x-rotb.x)*cos(rotb.z)-(uv.y-rotb.y)*.9319371728*sin(rotb.z)             +rotb.x,
		((uv.x-rotb.x)*sin(rotb.z)+(uv.y-rotb.y)*.9319371728*cos(rotb.z))/.9319371728+rotb.y);
	cuv = vec4(
		(uv.x-.873)*cos(algoprog.y-1)-(uv.y-.155)*sin(algoprog.y-1)+.873,
		(uv.x-.873)*sin(algoprog.y-1)+(uv.y-.155)*cos(algoprog.y-1)+.155,
		(uv.x-.873)*cos(algoprog.y  )-(uv.y-.155)*sin(algoprog.y  )+.873,
		(uv.x-.873)*sin(algoprog.y  )+(uv.y-.155)*cos(algoprog.y  )+.155);
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
in vec4 webuv;
in vec4 cuv;
uniform vec4 amttop;
uniform vec3 amtbot;
uniform sampler2D fgtex;
uniform float algo;
uniform vec2 algoprog;
out vec4 fragColor;
void main() {
	vec2 auv = uv;
	vec4 acuv = cuv;
	if(uv.y < .68) {
		float sect = -9;
		if(uv.y >= .25)
			sect = (uv.x>.49?1:0)+(uv.y<.45?2:0);
		else if(uv.y >= .09)
			sect = floor(uv.x*3)+4;

		float amt = 0;
		if(sect == 0) amt = amttop.x; else
		if(sect == 1) amt = amttop.y; else
		if(sect == 2) amt = amttop.z; else
		if(sect == 3) amt = amttop.w; else
		if(sect == 4) amt = amtbot.x; else
		if(sect == 5) amt = amtbot.y; else
		if(sect == 6) amt = amtbot.z;

		if(abs(amt) > .0001) {
			vec4 origin = vec4(-1);
			if(uv.y >= .25)
				origin = vec4(.26+.48*mod(sect,2),.64-.26*floor(sect*.5),.2,.3);
			else
				origin = vec4(.19+.31*(sect-4),.16,.12,.9);
			float dist = 1+min((sqrt(pow(uv.x-origin.x,2.)+pow(uv.y-origin.y,2.))-origin.z),.1)*origin.w*amt;
			 auv    = ( auv   -origin.xy)*dist+origin.xy;
			acuv.xy = (acuv.xy-origin.xy)*dist+origin.xy;
			acuv.zw = (acuv.zw-origin.xy)*dist+origin.xy;
		}
	}

	fragColor = texture(fgtex,auv);
	if(uv.y > .1 && uv.y < .21) {
		if(uv.x > .816 && uv.x < .93 && fragColor.r <= .0001) {
			if(algoprog.y >= .9999) {
				if(algo > 1.5) fragColor.r += fragColor                    .g; else
				if(algo > 0.5) fragColor.r += texture(fgtex,auv-vec2(.5,0)).g; else
				               fragColor.r += fragColor                    .b;
			} else {
				vec2 algocrest = vec2(0);
				if(algo > 1.5) {
					algocrest = vec2(texture(fgtex,acuv.xy           ).g,
					                 texture(fgtex,acuv.zw-vec2(.5,0)).g);
				} else if(algo > 0.5) {
					algocrest = vec2(texture(fgtex,acuv.xy-vec2(.5,0)).g,
					                 texture(fgtex,acuv.zw           ).b);
				} else {
					algocrest = vec2(texture(fgtex,acuv.xy           ).b,
					                 texture(fgtex,acuv.zw           ).g);
				}
				fragColor.r += sqrt(pow(algoprog.x*algocrest.r,2)+pow((1-algoprog.x)*algocrest.g,2));
			}
		}
		fragColor.gb = vec2(0);
	} else if(uv.y < .09) {
		if(uv.x > .16 && uv.x < .36)
			fragColor = texture(fgtex,webuv.xy);
		else if(uv.x > .4 && uv.x < .68)
			fragColor = texture(fgtex,webuv.zw);
	}
})");

	lineshader = add_shader(
//LINE VERT
R"(#version 150 core
in vec4 aPos;
uniform float banner;
uniform vec4 amttop;
uniform vec2 amtbot;
out float opacity;
out float channel;
out vec2 uv;
void main() {
	opacity = 1.5-abs(aPos.z-.5);
	uv = vec2(mod(aPos.w,1.),min(1,max(0,aPos.z))*.5+mod(floor(aPos.w/3.),2.)*.5);
	channel = mod(floor(aPos.w),3.);

	float sect = mod(floor(aPos.w),9.);
	float amt = 0;
	if(sect == 0) amt = amttop.x; else
	if(sect == 1) amt = amttop.y; else
	if(sect == 2) amt = amttop.z; else
	if(sect == 3) amt = amttop.w; else
	if(sect == 4) amt = amtbot.x; else
	if(sect == 5) amt = amtbot.y;

	vec2 pos = aPos.xy;
	if(abs(amt) > .0001) {
		vec4 origin = vec4(-1);
		if(sect < 4)
			origin = vec4(.26+.48*mod(sect,2),.64-.26*floor(sect*.5),.2,.3);
		else
			origin = vec4(.19+.31*(sect-4),.16,.12,.9);
		float dist = 1+min((sqrt(pow(pos.x-origin.x,2.)+pow(pos.y-origin.y,2.))-origin.z),.1)*origin.w*amt;
		pos = (pos-origin.xy)/dist+origin.xy;
	}

	gl_Position = vec4(vec2(pos.x,pos.y*(1-banner))*2-1,0,1);
})",
//LINE FRAG
R"(#version 150 core
in float opacity;
in float channel;
in vec2 uv;
uniform float dpi;
uniform sampler2D linetex;
out vec4 fragColor;
void main() {
	vec3 col = texture(linetex,uv).rgb;
	if(channel > 1.5) col.r = col.b; else
	if(channel >  .5) col.r = col.g;
	fragColor = vec4(1,0,0,pow(max(0,sqrt(col.r)-1+opacity),2));
})");

	ppshader = add_shader(
//PP VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = aPos;
})",
//PP FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D buffertex;
uniform sampler2D bgtex;
uniform vec2 misalignment;
uniform float chromatic;
uniform float algo;
out vec4 fragColor;
void main(){
	vec3 col = vec3(0);
	vec3 alpha = vec3(0);

	vec3 texl = texture(buffertex,uv-vec2(chromatic,0)).rgb;
	vec3 texc = texture(buffertex,uv                  ).rgb;
	vec3 texr = texture(buffertex,uv+vec2(chromatic,0)).rgb;
	vec3 fgcolr = vec3(texl.r,texc.r,texr.r);
	vec3 fgcolg = vec3(texl.g,texc.g,texr.g);
	vec3 fgcolb = vec3(texl.b,texc.b,texr.b);
	if(uv.y > .73) {
		if(uv.x > .25 && (fgcolg.r+fgcolg.g+fgcolg.b) > .00001 && (fgcolb.r+fgcolb.g+fgcolb.b) > .00001) {
			col += vec3(.5,.7890625,.5078125)*fgcolg;
		} else if(uv.x > .4) {
			if(uv.x > .6) {
				col += vec3(.97265625,.48828125,.5078125)*fgcolg;
			} else {
				col += vec3(.984375,.52734375,.7578125)*fgcolg;
			}
			col += vec3(.47265625,.57421875,.84375)*fgcolb;
		} else {
			col += vec3(.97265625,.48828125,.5078125)*fgcolg;
			col += vec3(.9921875,.8046875,.28515625)*fgcolb;
		}
	} else if(uv.y < .09) {
		if((fgcolg.r+fgcolg.g+fgcolg.b) > .00001 && (fgcolb.r+fgcolb.g+fgcolb.b) > .00001) {
			col += vec3(.2421875,.375,.7734375)*fgcolg;
		} else {
			col += vec3(.95703125,.34765625,.53515625)*fgcolg;
			col += vec3(.22265625,.62109375,.52734375)*fgcolb;
		}
	}
	col += vec3(.23828125,.1953125,.22265625)*fgcolr;
	alpha = min(vec3(1),fgcolr+max(fgcolg,fgcolb));

	texl = texture(bgtex,uv-vec2(chromatic,0)).rgb;
	texc = texture(bgtex,uv                  ).rgb;
	texr = texture(bgtex,uv+vec2(chromatic,0)).rgb;
	vec3 bgcol = vec3(0);
	     if(algo > 1.5) bgcol = vec3(texl.b,texc.b,texr.b);
	else if(algo > 0.5) bgcol = vec3(texl.r,texc.r,texr.r);
	else                bgcol = vec3(texl.g,texc.g,texr.g);
	bgcol *= 1-alpha;

	texl = texture(buffertex,uv+misalignment-vec2(chromatic,0)).rgb;
	texc = texture(buffertex,uv+misalignment                  ).rgb;
	texr = texture(buffertex,uv+misalignment+vec2(chromatic,0)).rgb;
	bgcol *= 1-min(vec3(1),vec3(texl.r,texc.r,texr.r)+max(vec3(texl.g,texc.g,texr.g),vec3(texl.b,texc.b,texr.b)));

	col += vec3(.8671875,.8984375,.99609375)*bgcol;
	alpha += bgcol;

	fragColor = vec4(col.rgb+(1-alpha)*.984375,1);
	//fragColor = texture(buffertex,uv);
})");

	add_texture(&bgtex  ,BinaryData::bg_png  ,BinaryData::bg_pngSize  );
	add_texture(&fgtex  ,BinaryData::fg_png  ,BinaryData::fg_pngSize  );
	add_texture(&linetex,BinaryData::line_png,BinaryData::line_pngSize);

	add_frame_buffer(&frame_buffer,width,height);

	draw_init();
}
void SucroseAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	frame_buffer.makeCurrentRenderingTarget();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	OpenGLHelpers::clear(Colours::black);

	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	fgtex.bind();
	baseshader->setUniform("fgtex",0);
	baseshader->setUniform("amttop",
		knobs[0].lerpedvalue[3],
		knobs[1].lerpedvalue[3],
		knobs[2].lerpedvalue[3],
		knobs[3].lerpedvalue[3]);
	baseshader->setUniform("amtbot",
		knobs[4].lerpedvalue[3],
		knobs[5].lerpedvalue[3],
		knobs[6].lerpedvalue[3]);
	baseshader->setUniform("algo",knobs[6].value*3.f);
	baseshader->setUniform("algoprog",knobs[6].lerpedvalue[0],knobs[6].lerpedvalue[1]*.4f+.6f-knobs[6].lerpedvalue[2]*.2f);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("rota",logopos[0],logopos[1],logopos[2]*(1-powf(1-websiteht[0],2)));
	baseshader->setUniform("rotb",logopos[3],logopos[4],logopos[5]*(1-powf(1-websiteht[1],2)));
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	// LINE
	lineshader->use();
	coord = context.extensions.glGetAttribLocation(lineshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,4,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(linelength+1)*4, visline, GL_DYNAMIC_DRAW);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	linetex.bind();
	lineshader->setUniform("linetex",0);
	lineshader->setUniform("banner",banner_offset);
	lineshader->setUniform("dpi",scaled_dpi);
	lineshader->setUniform("amttop",
		knobs[0].lerpedvalue[3],
		knobs[1].lerpedvalue[3],
		knobs[2].lerpedvalue[3],
		knobs[3].lerpedvalue[3]);
	lineshader->setUniform("amtbot",
		knobs[4].lerpedvalue[3],
		knobs[5].lerpedvalue[3]);
	glDrawArrays(GL_TRIANGLE_STRIP,0,linelength+1);
	context.extensions.glDisableVertexAttribArray(coord);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	frame_buffer.releaseAsRenderingTarget();

	ppshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_buffer.getTextureID());
	ppshader->setUniform("buffertex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	bgtex.bind();
	ppshader->setUniform("bgtex",1);
	ppshader->setUniform("misalignment",offset[0],offset[1]);
	ppshader->setUniform("chromatic",.5f/(getWidth()*dpi));
	ppshader->setUniform("algo",knobs[6].value*3.f);
	ppshader->setUniform("banner",banner_offset);
	coord = context.extensions.glGetAttribLocation(ppshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void SucroseAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void SucroseAudioProcessorEditor::calcnext() {
	for(int i = 0; i < 4; ++i) {
		float lastposx = scribble[SPEED*2*i+(int)fmod(writepos-1+SPEED,SPEED)*2  ];
		float lastposy = scribble[SPEED*2*i+(int)fmod(writepos-1+SPEED,SPEED)*2+1];
		float targposx = noisegen.noise(i*8,time*(knobs[i].value*3+1))*(sqrt(knobs[i].value)*35.f+2.f)+160+171*fmod(i,2);
		float targposy = noisegen.noise(time*(knobs[i].value*3+1),i*8)*(sqrt(knobs[i].value)*35.f+2.f)+263-moveup[i]*.5f-100*floor(i*.5f);
		float angle = std::atan2(targposy-lastposy,targposx-lastposx);
		while((angledamp.v_current[i]-angle) < -3.1415926535f) angledamp.v_current[i] += 3.1415926535f*2;
		while((angledamp.v_current[i]-angle) >  3.1415926535f) angledamp.v_current[i] -= 3.1415926535f*2;
		angle = angledamp.nextvalue(angle,i);
		scribble[SPEED*2*i+writepos*2  ] = (cos(angle)*(.4f+knobs[i].value*knobs[i].value*1.6f)+lastposx)*.988f+targposx*.012f;
		scribble[SPEED*2*i+writepos*2+1] = (sin(angle)*(.4f+knobs[i].value*knobs[i].value*1.6f)+lastposy)*.988f+targposy*.012f;
	}
	time += .006f;
	writepos = fmod(writepos+1,SPEED);
	for(int i = 0; i < 4; ++i) {
		for(int x = 0; x < SPEED; ++x) {
			int index = SPEED*2*i+x*2;
			scribble[index  ] -=     130.f/SPEED;
			scribble[index+1] += moveup[i]/SPEED;
		}
	}
}
void SucroseAudioProcessorEditor::calcvis() {
	linelength = -1;

	for(int i = 0; i < 6; ++i) {
		int yindex = floor(knobs[i].lerpedvalue[0]*6.999f);
		float ylerp = knobs[i].lerpedvalue[0]*7-yindex;
		float originy = knobs[i].y[yindex]*(1-ylerp                  )+knobs[i].y[yindex+1]*ylerp;
		float originx = knobs[i].x[0     ]*(1-knobs[i].lerpedvalue[0])+knobs[i].x[       1]*knobs[i].lerpedvalue[0];
		float angle   = knobs[i].a[0     ]*(1-knobs[i].lerpedvalue[0])+knobs[i].a[       1]*knobs[i].lerpedvalue[0];
		beginline(i,.1f*(knobs[i].lerpedvalue[1]*2.1f+knobs[i].lerpedvalue[2]*.3f+i*8));
		for(int k = 0; k <= 9; ++k) {
			float valuex = knobs[i].lerpedvalue[1]*2.1f+k*.06f;
			float valuey = knobs[i].lerpedvalue[2]* .3f+i*8   ;
			float perlinx = noisegen.noise(valuex,valuey)*2.f;
			float perliny = noisegen.noise(valuey,valuex);
			float m = (k/8.f-.5f)*7+perliny;
			nextpoint(originx-m*sin(angle)+perlinx*cos(angle),originy+m*cos(angle)+perlinx*sin(angle));
		}
		endline();
	}

	for(int i = 0; i < 4; ++i) {
		beginline(i+6+3,lastdist[i]);
		for(int x = 0; x < SPEED; ++x) {
			int index = SPEED*2*i+fmod(x+writepos,SPEED)*2;
			nextpoint(
				noisegen.noise(i*8,x*.003f)*1.6f+scribble[index  ],
				noisegen.noise(x*.003f,i*8)*1.6f+scribble[index+1]);
			if(x == 2) lastdist[i] = linedist;
		}
		endline();
	}
}
void SucroseAudioProcessorEditor::beginline(int channel, float dist) {
	linedist = dist;
	linechannel = channel;
	linebegun = 0;
}
void SucroseAudioProcessorEditor::endline() {
	float tipsize = 1.8f;
	linelength += 2;
	linedist = fmod(linedist+tipsize/177.f,1);
	float angle = std::atan2(
		(visline[linelength*4-7]-visline[linelength*4-15])*height,
		(visline[linelength*4-8]-visline[linelength*4-16])*width )+1.5707963268f;
	visline[linelength*4-4] = visline[linelength*4-12]+sin(angle)*tipsize/width ;
	visline[linelength*4-3] = visline[linelength*4-11]-cos(angle)*tipsize/height;
	visline[linelength*4-2] = -1.f;
	visline[linelength*4-1] = linedist+linechannel;
	visline[linelength*4  ] = visline[linelength*4- 8]+sin(angle)*tipsize/width ;
	visline[linelength*4+1] = visline[linelength*4- 7]-cos(angle)*tipsize/height;
	visline[linelength*4+2] =  2.f;
	visline[linelength*4+3] = linedist+linechannel;
}
void SucroseAudioProcessorEditor::nextpoint(float x, float y) {
	if(linebegun == 0) {
		lineprevx = x;
		lineprevy = y;
		linecurrentx = x;
		linecurrenty = y;
		++linebegun;
		return;
	}
	float linewidth = 3.f;
	linedist = fmod(linedist+sqrt(powf(linecurrenty-lineprevy,2)+powf(linecurrentx-lineprevx,2))/177.f,1);
	float angle1 = std::atan2(linecurrenty-   lineprevy,linecurrentx-   lineprevx)+1.5707963268f;
	float angle2 = std::atan2(           y-linecurrenty,           x-linecurrentx)+1.5707963268f;
	float angle = 0;
	     if(linebegun == 1)
		angle = angle2;
	else if(fabs(angle1-angle2)<=1)
		angle = (angle1+angle2)*.5f;
	else if(fabs(linecurrenty-lineprevy) > fabs(y-linecurrenty))
		angle = angle1;
	else
		angle = angle2;
	visline[++linelength*4] = (linecurrentx+cos(angle)*linewidth)/width ;
	visline[1+linelength*4] = (linecurrenty+sin(angle)*linewidth)/height;
	visline[2+linelength*4] = 0.f;
	visline[3+linelength*4] = linedist+linechannel;
	visline[++linelength*4] = (linecurrentx-cos(angle)*linewidth)/width ;
	visline[1+linelength*4] = (linecurrenty-sin(angle)*linewidth)/height;
	visline[2+linelength*4] = 1.f;
	visline[3+linelength*4] = linedist+linechannel;
	if(linebegun == 1) {
		float tipsize = 1.8f;
		float tempdist = fmod(linedist-tipsize/177.f+1,1);
		linelength += 2;
		visline[linelength*4- 4] = visline[linelength*4-12];
		visline[linelength*4- 3] = visline[linelength*4-11];
		visline[linelength*4- 2] =  0.f;
		visline[linelength*4- 1] = linedist+linechannel;
		visline[linelength*4   ] = visline[linelength*4- 8];
		visline[linelength*4+ 1] = visline[linelength*4- 7];
		visline[linelength*4+ 2] =  1.f;
		visline[linelength*4+ 3] = linedist+linechannel;
		visline[linelength*4-12] -=sin(angle)*tipsize/width ;
		visline[linelength*4-11] +=cos(angle)*tipsize/height;
		visline[linelength*4-10] = -1.f;
		visline[linelength*4- 9] = tempdist+linechannel;
		visline[linelength*4- 8] -=sin(angle)*tipsize/width ;
		visline[linelength*4- 7] +=cos(angle)*tipsize/height;
		visline[linelength*4- 6] =  2.f;
		visline[linelength*4- 5] = tempdist+linechannel;
		++linebegun;
	}
	lineprevx = linecurrentx;
	lineprevy = linecurrenty;
	linecurrentx = x;
	linecurrenty = y;
}
void SucroseAudioProcessorEditor::paint(Graphics& g) { }

void SucroseAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < 2; ++i) {
		bool prev = websiteht[i]<.00001f;
		websiteht[i] = fmin(1,fmax(0,websiteht[i]+((-2-hover)==i?.3f:-.3f)));
		if(prev && websiteht[i] > .00001f) {
			logopos[i*3  ] = random.nextFloat()*(i==0?.2f:.28f)+(i==0?.16f:.4f);
			logopos[i*3+1] = random.nextFloat()*.09f;
			logopos[i*3+2] = (random.nextFloat()+.2f)*(i==0?.08f:.06f)*(random.nextFloat()>.5f?1:-1);
		}
	}

	for(int i = 0; i < (knobcount-1); i++) {
		knobs[i].lerpedvalue[0] = knobs[i].lerpedvalue[0]*.5f+knobs[i].value*.5f;
		knobs[i].lerpedvalue[1] = knobs[i].lerpedvalue[1]*.7f+knobs[i].value*.3f;
		knobs[i].lerpedvalue[2] = knobs[i].lerpedvalue[2]*.8f+(knobswitch[i]?1:0)*.2f;
	}
	knobs[6].lerpedvalue[0] = knobs[6].lerpedvalue[0]*.55f+.45f;
	knobs[6].lerpedvalue[1] = knobs[6].lerpedvalue[1]*.8f+.2f;
	knobs[6].lerpedvalue[2] = knobs[6].lerpedvalue[2]*.7f+.3f*(hover==6?1.f:0.f);
	for(int i = 0; i < knobcount; i++)
		knobs[i].lerpedvalue[3] = knobs[i].lerpedvalue[3]*.55f+.45f*(isdown?-.5f:1.f)*(hover==i?1.f:0.f);

	calcnext();
	calcvis();

	update();
}

void SucroseAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		if(parameterID == "algo") {
			offset[0] = .003f+random.nextFloat()*.003f;
			offset[1] = .003f+random.nextFloat()*.003f;
			knobs[6].lerpedvalue[0] = 1.f-knobs[6].lerpedvalue[0];
			knobs[6].lerpedvalue[1] = 1.f-knobs[6].lerpedvalue[1];
		}
		knobs[i].value = knobs[i].normalize(newValue);
		return;
	}
}
void SucroseAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(prevhover != hover && hover > -1) {
		knobswitch[hover] = !knobswitch[hover];
		fresh = true;
	}
}
void SucroseAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void SucroseAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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
		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20) {
				set_ui_scale(result-21);
			} else if(result == 1) { //copy preset
				SystemClipboard::copyTextToClipboard(audio_processor.get_preset(audio_processor.currentpreset));
			} else if(result == 2) { //paste preset
				audio_processor.set_preset(SystemClipboard::getTextFromClipboard(), audio_processor.currentpreset);
			} else if (result == 3) {
				audio_processor.randomize();
			}
		});
		return;
	}

	isdown = true;
	if(!fresh && hover > -1)
		knobswitch[hover] = !knobswitch[hover];
	fresh = false;
	initialdrag = hover;
	if(hover == 6) {
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.undo_manager.setCurrentTransactionName("Switched algorithm");
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(knobs[6].id)->setValueNotifyingHost(fmod(round(knobs[6].value*2)+1,3)*.5f);
	} else if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void SucroseAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(initialdrag > -1 && initialdrag != 6) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset);

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	} else if(initialdrag == -3 || initialdrag == -2) {
		hover = recalc_hover(event.x,event.y)==initialdrag?initialdrag:-1;
	}
}
void SucroseAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	isdown = false;
	if(dpi < 0) return;
	if(hover > -1)
		knobswitch[hover] = !knobswitch[hover];
	if(hover > -1 && hover != 6) {
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((knobs[hover].value-initialvalue)>=0?"Increased ":"Decreased ") += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		     if(hover == -3 && prevhover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		else if(hover == -2 && prevhover == -2) URL("https://fx.amee.ee/").launchInDefaultBrowser();
	}
}
void SucroseAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1 || hover == 6) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audio_processor.undo_manager.beginNewTransaction();
}
void SucroseAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1 || hover == 6) return;
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int SucroseAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if(x < 16 || x >= 342 || y < 90) {
		return -1;
	} else if(y < 290) {
		if(fmod(x-16,171.f) >= 155 || fmod(y-90,100.f) >= 93)
			return -1;
		return ((x>=179?1:0)+(y>=186?2:0));
	} else if(y < 350) {
		if(fmod(x-16,111.f) >= 104.f)
			return -1;
		return floor((x-16)/111.f)+4;
	} else if(y >= 358 && y < 378) {
		if(x >= 68 && x < 126) return -2;
		if(x >= 145 && x < 238) return -3;
	}
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
