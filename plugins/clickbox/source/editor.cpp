#include "processor.h"
#include "editor.h"

ClickBoxAudioProcessorEditor::ClickBoxAudioProcessorEditor(ClickBoxAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), plugmachine_gui(p, 256, 256, 1.f, .5f, false) {
	sliders[0].hy = 116;
	sliders[0].hh = 146;
	sliders[0].y = .4453125f;

	sliders[1].hy = 151;
	sliders[1].hw = 126;
	sliders[1].hh = 181;
	sliders[1].y = .58203125f;
	sliders[1].w = .5;

	sliders[2].hx = 130;
	sliders[2].hy = 151;
	sliders[2].hh = 181;
	sliders[2].x = .5;
	sliders[2].y = .58203125f;
	sliders[2].w = .5;

	sliders[3].isslider = false;
	sliders[3].hy = 186;
	sliders[3].hh = 216;
	sliders[3].y = .71875f;

	sliders[4].isslider = false;
	sliders[4].hy = 221;
	sliders[4].hw = 126;
	sliders[4].hh = 251;
	sliders[4].y = .85546875f;
	sliders[4].h = .14453125f;
	sliders[4].w = .5;

	sliders[5].hx = 130;
	sliders[5].hy = 221;
	sliders[5].hh = 251;
	sliders[5].x = .5;
	sliders[5].y = .85546875f;
	sliders[5].w = .5;
	sliders[5].h = .14453125f;

	for(int i = 0; i < 6; i++) {
		sliders[i].id = params.pots[i].id;
		sliders[i].name = params.pots[i].name;
		sliders[i].value = params.pots[i].normalize(state.values[i]);
		sliders[i].minimumvalue = params.pots[i].minimumvalue;
		sliders[i].maximumvalue = params.pots[i].maximumvalue;
		sliders[i].defaultvalue = params.pots[i].normalize(params.pots[i].defaultvalue);
		slidercount++;
		add_listener(sliders[i].id);

		if(sliders[i].isslider) sliders[i].coloffset = random.nextFloat();
		float r = sliders[i].isslider?(sliders[i].value*3*sliders[i].w+sliders[i].coloffset):random.nextFloat();
		sliders[i].r = getr(r);
		sliders[i].g = getg(r);
		sliders[i].b = getb(r);
	}

	mousecolor = (int)floor(random.nextFloat()*6);
	prevpos[0].col = (int)fmod(mousecolor+1+floor(random.nextFloat()*5),6);

	for(int i = 0; i < 8; i++) randoms[i] = random.nextFloat();

	init(&look_n_feel);
}
ClickBoxAudioProcessorEditor::~ClickBoxAudioProcessorEditor() {
	close();
}

void ClickBoxAudioProcessorEditor::newOpenGLContextCreated() {
	clearshader = add_shader(
//CLEAR VERT
R"(#version 150 core
in vec2 aPos;
void main() {
	gl_Position = vec4(aPos*2-1,0,1);
})",
//CLEAR FRAG
R"(#version 150 core
out vec4 fragColor;
void main() {
	fragColor = vec4(.10546875,.10546875,.10546875,.15);
})");

	slidershader = add_shader(
//SLIDER VERT
R"(#version 150 core
in vec2 aPos;
uniform vec4 texscale;
uniform vec2 margin;
uniform float scale;
out vec2 texcoord;
out float sliderpos;
void main() {
	texcoord = vec2(aPos.x*texscale.z+texscale.x,1-((1-aPos.y)*texscale.w+texscale.y));
	gl_Position = vec4(texcoord/scale*2-1,0,1);
	sliderpos = aPos.x*margin.x-margin.y;
})",
//SLIDER FRAG
R"(#version 150 core
in vec2 texcoord;
in float sliderpos;
uniform sampler2D tex;
uniform vec3 col;
uniform float value;
uniform float hover;
out vec4 fragColor;
void main() {
	vec2 t = texture(tex,texcoord).rb;
	vec3 color = col;
	float alpha = 0;

	if(sliderpos <= value) alpha = t.g;
	else alpha = t.r;
	if(hover > .5 && t.r > .5 && t.g < .5) {
		alpha = 1;
		color += .8;
	}

	fragColor = vec4(color,alpha);
})");

	creditsshader = add_shader(
//CREDITS VERT
R"(#version 150 core
in vec2 aPos;
uniform float texscale;
uniform vec3 rot;
uniform float scale;
out vec2 texcoord;
out vec2 shadercoord;
void main() {
	gl_Position = vec4(aPos/scale*2*vec2(1,texscale)-1,0,1);
	shadercoord = (aPos*2*vec2(1,texscale)-vec2(1,.5))*1.2;
	shadercoord = vec2(
		shadercoord.x*cos(rot.z)-shadercoord.y*sin(rot.z),
		shadercoord.x*sin(rot.z)+shadercoord.y*cos(rot.z));
	texcoord = vec2(aPos.x,aPos.y*texscale);
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 texcoord;
in vec2 shadercoord;
uniform sampler2D tex;
uniform float htpos;
uniform vec3 rot;
uniform float lineoffset;
uniform vec3 color;
uniform float htback;
out vec4 fragColor;
void main() {
	vec4 map = texture(tex,texcoord);

	if(map.a < .5) {
		fragColor = vec4(color+htback,1);
	} else if(map.r > .5) {
		float ht = 0;
		if(map.b > .5 && (texcoord.x - htpos) < 1 && (texcoord.x - htpos) > 0)
			ht = texture(tex,texcoord-vec2(htpos,0)).g;

		if(ht > .5) {
			fragColor = vec4(color+.8,1);
		} else {
			vec3 ro = vec3(rot.x,0,rot.y);
			vec3 cu = normalize(cross(-ro,vec3(sin(.35),cos(.35),0)));
			vec3 rd = normalize(mat3(cu,normalize(cross(cu,-ro)),ro)*vec3(shadercoord,1));
			float col = 0;
			float b = dot(rd,ro);
			float d = b*b-(dot(ro,ro)-1);
			if(d > 0) {
				d = sqrt(abs(d));
				col = mod((.5-asin(ro.x+rd.x*(d-b))/3.14159265359)*6+lineoffset,1)>.9?1:0;
				col = max(mod((.5-asin(ro.x+rd.x*(-d-b))/3.14159265359)*6+lineoffset,1)>.9?1:0,col);
			}

			if(map.b < .5) col = 1-col;
			fragColor = vec4(col>.5?color:vec3(.10546875),1);
		}
	} else fragColor = vec4(0);
})");

	ppshader = add_shader(
//PP VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
out vec2 texcoord;
void main() {
	gl_Position = vec4(aPos*2-1,0,1);
	texcoord = aPos;
})",
//PP FRAG
R"(#version 150 core
in vec2 texcoord;
uniform sampler2D tex;
uniform sampler2D noisetex;
uniform float intensity;
uniform vec4 randomsone;
uniform vec4 randomstwo;
uniform float randomsblend;
out vec4 fragColor;
void main() {
	vec4 col = texture(tex,texcoord);

	float noise = 0;
	if(col.r > .2 || col.g > .2 || col.b > .2) {
		noise += texture(noisetex,texcoord+randomsone.xy).g;
		noise += texture(noisetex,texcoord+randomsone.zw).g;
		noise += texture(noisetex,texcoord+randomstwo.xy).g*(1-randomsblend);
		noise += texture(noisetex,texcoord+randomstwo.zw).g*randomsblend;
		noise /= 3;

		vec2 coord = floor(texcoord*256)/256;
		if(mod(coord.x*3482+coord.y*43928,1)>.5) {
			if(noise < intensity) coord.x -= .00390625;
			else if(noise > (1-intensity)) coord.x += .00390625;
		} else {
			if(noise < intensity) coord.y -= .00390625;
			else if(noise > (1-intensity)) coord.y += .00390625;
		}

		col = texture(tex,coord);
	}

	fragColor = vec4(col.rgb*col.a+.10546875*(1-col.a),1);
})");

	cursorshader = add_shader(
//CURSOR VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 pos;
uniform float automated;
out vec2 texcoord;
out vec2 basecoord;
void main() {
	gl_Position = vec4(aPos.x*.125-1.03125+pos.x,aPos.y*.125+.90625-pos.y,0,1);
	texcoord = aPos*vec2(.5,1)+vec2(automated*.5,0);
	basecoord = gl_Position.xy*.5+.5;
})",
//CURSOR FRAG
R"(#version 150 core
in vec2 texcoord;
in vec2 basecoord;
uniform sampler2D tex;
uniform sampler2D base;
uniform vec3 col;
uniform float clamp;
out vec4 fragColor;
void main() {
	if(clamp > .5 && basecoord.y < .5546875) fragColor = vec4(0);
	else {
		vec2 texx = texture(tex,texcoord).rg;
		if(clamp > .5) texx.g *= texture(base,basecoord).b;
		fragColor = vec4(texx.g>.5?col:(clamp<.5?(col+.8):vec3(.10546875)),texx.r>.5?1:0);
	}
})");

	add_texture(&slidertex, BinaryData::tex_png,BinaryData::tex_pngSize, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	add_texture(&creditstex, BinaryData::credits_png,BinaryData::credits_pngSize, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	add_texture(&cursortex, BinaryData::cursor_png,BinaryData::cursor_pngSize, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	add_frame_buffer(&framebuffer, width, height, false, false, GL_NEAREST, GL_NEAREST);

	draw_init();
}
void ClickBoxAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER,array_buffer);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	framebuffer.makeCurrentRenderingTarget();
	auto coord = context.extensions.glGetAttribLocation(clearshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,square,GL_DYNAMIC_DRAW);
	clearshader->use();
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	slidershader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	slidertex.bind();
	slidershader->setUniform("tex",0);
	slidershader->setUniform("value",0.f);
	slidershader->setUniform("texscale",0.f,0.f,1.f,.4453125f);
	slidershader->setUniform("col",.23828125f,.23828125f,.23828125f);
	slidershader->setUniform("margin",0.f);
	slidershader->setUniform("hover",0.f);
	slidershader->setUniform("scale",ui_scales[ui_scale_index]);
	coord = context.extensions.glGetAttribLocation(slidershader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	if(!credits) {
		for(int i = 0; i < slidercount; i++) {
			slidershader->setUniform("texscale",sliders[i].x,sliders[i].y,sliders[i].w,sliders[i].h);
			slidershader->setUniform("col",sliders[i].r,sliders[i].g,sliders[i].b);
			slidershader->setUniform("value",sliders[i].value);
			slidershader->setUniform("hover",hover==i?1.f:0.f);
			if(sliders[i].w < .75f) {
				if(sliders[i].x > .25f)
					slidershader->setUniform("margin",1.0859375f,.03125f);
				else
					slidershader->setUniform("margin",1.0859375f,.0546875f);
			} else
				slidershader->setUniform("margin",1.0546875f,.02734375f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	} else {
		context.extensions.glDisableVertexAttribArray(coord);

		creditsshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		creditstex.bind();
		creditsshader->setUniform("tex",0);
		creditsshader->setUniform("texscale",.5546875f);
		creditsshader->setUniform("htpos",websiteht);
		creditsshader->setUniform("lineoffset",shadertime*.5f);
		creditsshader->setUniform("scale",ui_scales[ui_scale_index]);
		creditsshader->setUniform("rot",
			sin(shadertime*.37f)*3,
			cos(shadertime*.37f)*3,
			shadertime*.13f);
		creditsshader->setUniform("color",getr(shadertime*.02f),getg(shadertime*.02f),getb(shadertime*.02f));
		creditsshader->setUniform("htback",hover==-4?.8f:0.f);

		coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	context.extensions.glBindFramebuffer(GL_FRAMEBUFFER, context.getFrameBufferID());

	ppshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,framebuffer.getTextureID());
	ppshader->setUniform("tex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	slidertex.bind();
	ppshader->setUniform("noisetex",1);
	ppshader->setUniform("intensity",ppamount);
	ppshader->setUniform("randomsone",
			((float)floor(randoms[0]*width ))/width ,
			((float)floor(randoms[1]*height))/height,
			((float)floor(randoms[2]*width ))/width ,
			((float)floor(randoms[3]*height))/height);
	ppshader->setUniform("randomstwo",
			((float)floor(randoms[4]*width ))/width ,
			((float)floor(randoms[5]*height))/height,
			((float)floor(randoms[6]*width ))/width ,
			((float)floor(randoms[7]*height))/height);
	ppshader->setUniform("randomsblend",randomsblend);
	coord = context.extensions.glGetAttribLocation(ppshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	cursorshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	cursortex.bind();
	cursorshader->setUniform("tex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	slidertex.bind();
	cursorshader->setUniform("base",1);
	cursorshader->setUniform("clamp",1.f);
	coord = context.extensions.glGetAttribLocation(cursorshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 6; i >= 0; i--) if(prevpos[i].x > -100 && (i != 6 || prevpos[i].automated)) {
		cursorshader->setUniform("col",colors[prevpos[i].col*3],colors[prevpos[i].col*3+1],colors[prevpos[i].col*3+2]);
		cursorshader->setUniform("pos",prevpos[i].x*2,prevpos[i].y*2);
		cursorshader->setUniform("automated",prevpos[i].automated?1.f:0.f);
		if(i == 0) cursorshader->setUniform("clamp",0.f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void ClickBoxAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void ClickBoxAudioProcessorEditor::paint(Graphics& g) {}

void ClickBoxAudioProcessorEditor::timerCallback() {
	shadertime += .02f;
	websiteht += .05f;

	randomsblend += .03f;
	if(randomsblend >= 1) {
		randomsblend = fmod(randomsblend,1.f);
		int rnd = floor(random.nextFloat()*3)*2;
		randoms[4] = randoms[rnd];
		randoms[5] = randoms[rnd+1];
		randoms[rnd] = randoms[6];
		randoms[rnd+1] = randoms[7];
		randoms[6] = random.nextFloat();
		randoms[7] = random.nextFloat();
	}
	ppamount = ppamount*.7f + .3f*fmin(sqrt(sqrt(audio_processor.i.get()*20)),.4f);

	if(!overridee) {
		prevpos[1].x = floor(audio_processor.x.get()*246+5)/((float)width);
		prevpos[1].y = floor(audio_processor.y.get()*106+5)/((float)height);
		prevpos[1].automated = true;
		if(prevpos[1].x == prevpos[2].x && prevpos[1].y == prevpos[2].y && prevpos[2].automated == true)
			prevpos[2].x = -1000;
		else
			mousecolor = (mousecolor+1)%6;
		prevpos[1].col = mousecolor;
	} else if(prevpos[0].x == prevpos[1].x && prevpos[0].y == prevpos[1].y && prevpos[1].automated == false) {
		prevpos[1].x = -1000;
	} else {
		mousecolor = (mousecolor+1)%6;
		prevpos[0].col = mousecolor;
	}

	for(int i = 6; i > 0; i--) {
		prevpos[i].x = prevpos[i-1].x;
		prevpos[i].y = prevpos[i-1].y;
		prevpos[i].automated = prevpos[i-1].automated;
		prevpos[i].col = prevpos[i-1].col;
	}

	update();
}

void ClickBoxAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < slidercount; i++) if(sliders[i].id == parameterID) {
		if(sliders[i].value == sliders[i].normalize(newValue)) return;
		sliders[i].value = sliders[i].normalize(newValue);
		float r = sliders[i].isslider?(sliders[i].value*3*sliders[i].w+sliders[i].coloffset):random.nextFloat();
		sliders[i].r = getr(r);
		sliders[i].g = getg(r);
		sliders[i].b = getb(r);
		return;
	}
}
void ClickBoxAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	prevpos[0].x = event.x/ui_scales[ui_scale_index]/width;
	prevpos[0].y = event.y/ui_scales[ui_scale_index]/height;
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht > .16015625f) websiteht = -.78515625;
}
void ClickBoxAudioProcessorEditor::mouseEnter(const MouseEvent& event) {
	setMouseCursor(MouseCursor::NoCursor);
}
void ClickBoxAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	setMouseCursor(MouseCursor::NormalCursor);
	prevpos[0].x = -1000;
	hover = -1;
}
void ClickBoxAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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
		rightclickmenu->addSeparator();
		rightclickmenu->addItem(3,"'Credits",true);
		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20) {
				set_ui_scale(result-21);
			} else if(result == 1) { //copy preset
				SystemClipboard::copyTextToClipboard(audio_processor.get_preset(audio_processor.currentpreset));
			} else if(result == 2) { //paste preset
				audio_processor.set_preset(SystemClipboard::getTextFromClipboard(), audio_processor.currentpreset);
			} else if(result == 3) { //credits
				credits = true;
			}
		});
		return;
	}

	initialdrag = hover;
	if(hover > -1) {
		initialvalue = sliders[hover].value;
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(sliders[hover].id)->beginChangeGesture();
		audio_processor.lerpchanged[hover] = true;
		if(sliders[hover].isslider)
			audio_processor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(((float)event.x/ui_scales[ui_scale_index]-sliders[hover].hx-2)/(sliders[hover].hw-sliders[hover].hx-4));
		else
			audio_processor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(1-initialvalue);
	} else if(hover == -2) {
		audio_processor.undo_manager.beginNewTransaction();
		overridee = true;
		audio_processor.apvts.getParameter("x")->setValueNotifyingHost((event.x/ui_scales[ui_scale_index]-5)*.0040650407f);
		audio_processor.apvts.getParameter("y")->setValueNotifyingHost((event.y/ui_scales[ui_scale_index]-5)*.0094339623f);
		audio_processor.apvts.getParameter("override")->setValueNotifyingHost(1.f);
		mousecolor = prevpos[0].col;
	}
}
void ClickBoxAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(event.mods.isRightButtonDown()) return;
	prevpos[0].x = event.x/ui_scales[ui_scale_index]/width;
	prevpos[0].y = event.y/ui_scales[ui_scale_index]/height;
	if(initialdrag > -1) {
		if(sliders[initialdrag].isslider)
			audio_processor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(((float)event.x/ui_scales[ui_scale_index]-sliders[hover].hx-2)/(sliders[hover].hw-sliders[hover].hx-4));
		else {
			hover = recalc_hover(event.x,event.y)==initialdrag?initialdrag:-1;
			audio_processor.apvts.getParameter(sliders[initialdrag].id)->setValueNotifyingHost(hover==-1?initialvalue:(1-initialvalue));
		}
	} else if(hover == -2) {
		audio_processor.apvts.getParameter("x")->setValueNotifyingHost((event.x/ui_scales[ui_scale_index]-5)*.0040650407f);
		audio_processor.apvts.getParameter("y")->setValueNotifyingHost((event.y/ui_scales[ui_scale_index]-5)*.0094339623f);
	} else if(initialdrag == -3) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y)==-3?-3:-1;
		if(initialdrag == -3 && hover == -3 && prevhover != -3 && websiteht > .16015625) websiteht = -.78515625;
	} else if(initialdrag == -4) {
		hover = recalc_hover(event.x,event.y)==-4?-4:-1;
	}
}
void ClickBoxAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(event.mods.isRightButtonDown()) return;
	if(hover > -1) {
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((sliders[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += sliders[hover].name);
		audio_processor.apvts.getParameter(sliders[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
	} else if(hover == -2) {
		overridee = false;
		audio_processor.apvts.getParameter("override")->setValueNotifyingHost(0.f);
		audio_processor.undo_manager.setCurrentTransactionName("Altered XY");
		audio_processor.undo_manager.beginNewTransaction();
	} else if(hover == -3) {
		URL("https://vst.unplug.red/").launchInDefaultBrowser();
	} else if(hover == -4) {
		credits = false;
	}
}
void ClickBoxAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	if(!sliders[hover].isslider) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += sliders[hover].name);
	audio_processor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(sliders[hover].defaultvalue);
	audio_processor.undo_manager.beginNewTransaction();
}
void ClickBoxAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	if(!sliders[hover].isslider) return;
	audio_processor.apvts.getParameter(sliders[hover].id)->setValueNotifyingHost(
		sliders[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int ClickBoxAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if(x>=5 && x<=251 && y>=5 && y<=111) return -2;
	if(credits) {
		if(x>=55 && x<=201 && y>=161 && y<=205) return -3;
		if(x>=189 && x<=253 && y>=240 && y<=253) return -4;
	} else for(int i = 0; i < slidercount; i++) {
		if( x >= sliders[i].hx &&
			x <= sliders[i].hw &&
			y >= sliders[i].hy &&
			y <= sliders[i].hh)
			return hover = i;
	}
	return -1;
}
float ClickBoxAudioProcessorEditor::getr(float hue) {
	float h = hue*6;
	float m = fmod(h,1.f);
	return colors[(int)fmod(floor(h),6)*3]*(1-m)+colors[(int)fmod(ceil(h),6)*3]*m;
}
float ClickBoxAudioProcessorEditor::getg(float hue) {
	float h = hue*6;
	float m = fmod(h,1.f);
	return colors[(int)fmod(floor(h),6)*3+1]*(1-m)+colors[(int)fmod(ceil(h),6)*3+1]*m;
}
float ClickBoxAudioProcessorEditor::getb(float hue) {
	float h = hue*6;
	float m = fmod(h,1.f);
	return colors[(int)fmod(floor(h),6)*3+2]*(1-m)+colors[(int)fmod(ceil(h),6)*3+2]*m;
}

LookNFeel::LookNFeel() {
	setColour(PopupMenu::backgroundColourId,Colour::fromFloatRGBA(0.f,0.f,0.f,0.f));
	font = find_font("Arial|Helvetica Neue|Helvetica|Roboto");
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	Font fontt = Font(font,"Regular",18.f*scale);
	fontt.setBold(true);
	fontt.setExtraKerningFactor(-.05f);
	return fontt;
}
int LookNFeel::getMenuWindowFlags() {
	//return ComponentPeer::windowHasDropShadow;
	return 0;
}
void LookNFeel::drawPopupMenuBackground(Graphics &g, int width, int height) {
	g.setColour(bg);
	g.fillRoundedRectangle(0,0,width,height,2*scale);
}
void LookNFeel::drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) {
	if(isSeparator) {
		g.setColour(fg);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		return;
	}

	float h = fabs((text.hashCode()%12288)/2048.f);
	float m = fmod(h,1.f);
	Colour ht = Colour::fromFloatRGBA(
		colors[(int)fmod(floor(h),6)*3  ]*(1-m)+colors[(int)fmod(ceil(h),6)*3  ]*m,
		colors[(int)fmod(floor(h),6)*3+1]*(1-m)+colors[(int)fmod(ceil(h),6)*3+1]*m,
		colors[(int)fmod(floor(h),6)*3+2]*(1-m)+colors[(int)fmod(ceil(h),6)*3+2]*m,1.f);

	bool removeleft = text.startsWith("'");
	if(isHighlighted && isActive) {
		g.setColour(ht);
		g.fillRoundedRectangle(0,0,area.getWidth(),area.getHeight(),2*scale);
		g.setColour(bg);
	} else g.setColour(ht);
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
	return 0;
}
void LookNFeel::getIdealPopupMenuItemSize(const String& text, const bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight)
{
	if(isSeparator) {
		idealWidth = 50*scale;
		idealHeight = (int)round(2*scale);
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
