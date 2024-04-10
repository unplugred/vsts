#include "PluginProcessor.h"
#include "PluginEditor.h"

PFAudioProcessorEditor::PFAudioProcessorEditor(PFAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), plugmachine_gui(p, 242, 462) {
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 3; y++) {
			int i = x+y*2;
			knobs[i].x = x*106+68;
			knobs[i].y = y*100+144;
			knobs[i].id = params.pots[i].id;
			knobs[i].name = params.pots[i].name;
			knobs[i].value = params.pots[i].normalize(state.values[i]);
			knobs[i].minimumvalue = params.pots[i].minimumvalue;
			knobs[i].maximumvalue = params.pots[i].maximumvalue;
			knobs[i].defaultvalue = params.pots[i].normalize(params.pots[i].defaultvalue);
			knobcount++;
			add_listener(knobs[i].id);
		}
	}
	oversampling = params.oversampling;
	oversamplinglerped = oversampling;
	add_listener("oversampling");

	calcvis();

	init(&look_n_feel);
}
PFAudioProcessorEditor::~PFAudioProcessorEditor() {
	close();
}

void PFAudioProcessorEditor::newOpenGLContextCreated() {
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*vec2(2,(1-banner)*2)-1,0,1);
	uv = aPos;
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D basetex;
out vec4 fragColor;
void main(){
	fragColor = vec4(texture(basetex,uv).r,0,0,1);
})");

	knobshader = add_shader(
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform float knobrot;
uniform vec2 knobscale;
uniform vec2 knobpos;
uniform float ratio;
uniform float banner;
out vec2 uv;
out vec2 hovercoord;
out vec2 basecoord;
void main(){
	vec2 pos = (aPos*2*knobscale-vec2(knobscale.x,knobscale.y*.727272727))/vec2(ratio,1);
	float rot = mod(knobrot,1)*-6.2831853072;
	gl_Position = vec4(
		(pos.x*cos(rot)-pos.y*sin(rot))*ratio+knobpos.x*2-1,
		pos.x*sin(rot)+pos.y*cos(rot)-knobpos.y*2+1,0,1);
	uv = vec2(aPos.x,1-(1-aPos.y)/6);
	hovercoord = (aPos-vec2(.5,.3636363636))/vec2(1.1379310345,1);
	hovercoord = vec2(
		(hovercoord.x*cos(rot*2)-hovercoord.y*sin(rot*2))*1.1379310345+.5,
		hovercoord.x*sin(rot*2)+hovercoord.y*cos(rot*2)+.36363636);
	basecoord = gl_Position.xy*.5+.5;
	gl_Position.y = gl_Position.y*(1-banner)-banner;
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
in vec2 hovercoord;
in vec2 basecoord;
uniform float knobcolor;
uniform float knobrot;
uniform sampler2D knobtex;
uniform sampler2D basetex;
uniform float id;
uniform float hoverstate;
out vec4 fragColor;
void main(){
	float index = floor(knobrot*6);
	fragColor = texture(knobtex,uv-vec2(0,mod(index,6)/6));
	if(fragColor.r > 0) {
		float lerp = mod(knobrot*6,1);
		float col = fragColor.b*(1-lerp)+texture(knobtex,uv-vec2(0,mod(index+1,6)/6)).b*lerp;
		if(col<.5) col = knobcolor*col*2;
		else col = knobcolor+(col-.5)*.8;
		if(hoverstate != 0) {
			float hover = 0;
			if(hovercoord.x < .95 && hovercoord.x > .05 && hovercoord.y > .005 && hovercoord.y < .78)
				hover = texture(knobtex,vec2(hovercoord.x,1-(1-hovercoord.y+id)/6)).g;
			if(hoverstate < -1) col += (hoverstate==-3?(1-hover):hover)*.3;
			else col = col*(1-hover)+.01171875*hover;
		}
		fragColor = vec4(col+texture(basetex,basecoord).g-.5,0,0,fragColor.r);
	} else fragColor = vec4(0);
})");

	visshader = add_shader(
//VIS VERT
R"(#version 150 core
in vec3 aPos;
uniform float banner;
out vec2 basecoord;
out float linepos;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)-banner),0,1);
	basecoord = vec2((aPos.x+1)*.5,1-(1-aPos.y)*.5);
	linepos = aPos.z;
})",
//VIS FRAG
R"(#version 150 core
in vec2 basecoord;
in float linepos;
uniform sampler2D basetex;
uniform float alpha;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec2 base = texture(basetex,basecoord).rb*vec2(1,alpha);
	fragColor = vec4(base.r*(1-base.g)+base.g,0,0,min((1-abs(linepos*2-1))*dpi*1.3,1));
})");

	oversamplingshader = add_shader(
//OVERSAMPLING VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform float selection;
out vec2 basecoord;
out vec2 highlightcoord;
void main(){
	gl_Position = vec4(aPos.x-.5,aPos.y*(1-banner)*.2+.7-banner*1.7,0,1);
	basecoord = vec2((aPos.x+.5)*.5,1-(1.5-aPos.y)*.1);
	highlightcoord = vec2((aPos.x-selection)*4.3214285714,aPos.y*3.3-1.1642857143);
})",
//OVERSAMPLING FRAG
R"(#version 150 core
in vec2 basecoord;
in vec2 highlightcoord;
uniform sampler2D basetex;
uniform float alpha;
uniform float dpi;
out vec4 fragColor;
void main(){
	float tex = min(max((texture(basetex,basecoord-vec2(0,.5)).b-.5)*dpi+.5,0),1);
	if(highlightcoord.x>0&&highlightcoord.x<1&&highlightcoord.y>0&&highlightcoord.y<1)
		tex = 1-tex;
	fragColor = vec4(1,0,0,tex*texture(basetex,basecoord).b*alpha);
})");

	creditsshader = add_shader(
//CREDITS VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos.x*2-1,1-(1-aPos.y*(1-banner)*(59./462.))*2,0,1);
	uv = aPos;
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D creditstex;
uniform float alpha;
uniform float shineprog;
out vec4 fragColor;
void main(){
	vec2 creditols = texture(creditstex,uv).rb;
	float shine = 0;
	if(uv.x+shineprog < 1 && uv.x+shineprog > .582644628099)
		shine = texture(creditstex,uv+vec2(shineprog,0)).g*creditols.r*.8;
	fragColor = vec4(creditols.g+shine,0,0,alpha);
})");

	ppshader = add_shader(
//PP VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform float shake;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = aPos+vec2(shake,0);
})",
//PP FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D buffertex;
uniform float chroma;
out vec4 fragColor;
void main(){
	fragColor = vec4(vec3((
		texture(buffertex,uv+vec2(chroma,0)).r+
		texture(buffertex,uv-vec2(chroma,0)).r+
		texture(buffertex,uv).r)*.333333333),1.);
})");

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize);
	add_texture(&knobtex, BinaryData::knob_png, BinaryData::knob_pngSize, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);
	add_texture(&creditstex, BinaryData::credits_png, BinaryData::credits_pngSize);

	add_frame_buffer(&framebuffer, width, height);

	draw_init();
}
void PFAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	if(needtoupdate > 0) {
		framebuffer.makeCurrentRenderingTarget();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		baseshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		basetex.bind();
		baseshader->setUniform("basetex",0);
		baseshader->setUniform("banner",banner_offset);
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);

		knobshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		knobtex.bind();
		knobshader->setUniform("knobtex",0);
		context.extensions.glActiveTexture(GL_TEXTURE1);
		basetex.bind();
		knobshader->setUniform("basetex",1);
		knobshader->setUniform("banner",banner_offset);
		knobshader->setUniform("knobscale",58.f/width,66.f/height);
		knobshader->setUniform("ratio",((float)height)/width);
		coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		knobshader->setUniform("knobcolor",.2265625f);
		for(int i = 0; i < knobcount; i++) {
			if(i == 2) knobshader->setUniform("knobcolor",.61328125f);
			knobshader->setUniform("knobpos",((float)knobs[i].x)/width,((float)knobs[i].y)/height);
			knobshader->setUniform("knobrot",(knobs[i].value-.5f)*.748f);
			knobshader->setUniform("id",(float)i);
			knobshader->setUniform("hoverstate",(float)(knobs[i].hoverstate==-1?(hover==i?-1:0):knobs[i].hoverstate));
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
		context.extensions.glDisableVertexAttribArray(coord);

		float osalpha = oversamplingalpha;
		if(oversamplingalpha < 1) {
			if(oversamplingalpha > 0)
				osalpha = oversamplingalpha<0.5?4*oversamplingalpha*oversamplingalpha*oversamplingalpha:1-(float)pow(-2*oversamplingalpha+2,3)/2;
			visshader->use();
			coord = context.extensions.glGetAttribLocation(visshader->getProgramID(),"aPos");
			context.extensions.glActiveTexture(GL_TEXTURE0);
			basetex.bind();
			visshader->setUniform("basetex",0);
			visshader->setUniform("banner",banner_offset);
			visshader->setUniform("alpha",1-osalpha);
			visshader->setUniform("dpi",(float)fmax(scaled_dpi,1));
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,3,GL_FLOAT,GL_FALSE,0,0);
			context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2712, visline[0], GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLE_STRIP,0,904);
			if(is_stereo) {
				context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2712, visline[1], GL_DYNAMIC_DRAW);
				glDrawArrays(GL_TRIANGLE_STRIP,0,904);
			}
			context.extensions.glDisableVertexAttribArray(coord);
			context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
		}

		if(oversamplingalpha > 0) {
			oversamplingshader->use();
			coord = context.extensions.glGetAttribLocation(oversamplingshader->getProgramID(),"aPos");
			context.extensions.glActiveTexture(GL_TEXTURE0);
			basetex.bind();
			oversamplingshader->setUniform("basetex",0);
			oversamplingshader->setUniform("banner",banner_offset);
			oversamplingshader->setUniform("alpha",osalpha);
			oversamplingshader->setUniform("selection",.458677686f+oversamplinglerped*.2314049587f);
			oversamplingshader->setUniform("dpi",(float)fmax(scaled_dpi,1));
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			context.extensions.glDisableVertexAttribArray(coord);
		}

		if(creditsalpha > 0) {
			creditsshader->use();
			context.extensions.glActiveTexture(GL_TEXTURE0);
			creditstex.bind();
			creditsshader->setUniform("creditstex",0);
			creditsshader->setUniform("banner",banner_offset);
			creditsshader->setUniform("alpha",creditsalpha<0.5?4*creditsalpha*creditsalpha*creditsalpha:1-(float)pow(-2*creditsalpha+2,3)/2);
			creditsshader->setUniform("shineprog",websiteht);
			coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			context.extensions.glDisableVertexAttribArray(coord);
		}

		needtoupdate--;
		framebuffer.releaseAsRenderingTarget();
	}

	ppshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	ppshader->setUniform("pptex",0);
	ppshader->setUniform("banner",banner_offset);
	ppshader->setUniform("chroma",rms*.006f);
	ppshader->setUniform("shake",rms*.004f*(random.nextFloat()-.5f));
	coord = context.extensions.glGetAttribLocation(ppshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void PFAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void PFAudioProcessorEditor::calcvis() {
	is_stereo = knobs[4].value > 0 && knobs[3].value < 1 && knobs[5].value > 0;
	pluginpreset pp;
	for(int i = 0; i < knobcount; i++)
		pp.values[i] = knobs[i].inflate(knobs[i].value);
	double prevy = 48;
	double currenty = 48;
	double nexty = 48;
	double nextnexty = 48;
	for(int c = 0; c < (is_stereo ? 2 : 1); c++) {
		for(int i = 0; i < 452; i++) {
			if((i%2)==0) {
				nexty = nextnexty;
			} else {
				nextnexty = 48+audio_processor.plasticfuneral(sin((i*.5+.5)/35.8098621957f)*.8f,c,2,pp,audio_processor.normalizegain(pp.values[1],pp.values[3]))*38;
				nexty = (currenty+nextnexty)*.5;
			}
			double angle1 = std::atan2(currenty-prevy, .5);
			double angle2 = std::atan2(currenty-nexty,-.5);
			while((angle1-angle2)<(-1.5707963268))angle1+=3.1415926535*2;
			while((angle1-angle2)>( 1.5707963268))angle1-=3.1415926535*2;
			double angle = (angle1+angle2)*.5;
			visline[c][i*6  ] = (i*.5+8+cos(angle)*1.3f)/121.f-1;
			visline[c][i*6+3] = (i*.5+8-cos(angle)*1.3f)/121.f-1;
			visline[c][i*6+1] = 1-(currenty+sin(angle)*1.3f)/231.f;
			visline[c][i*6+4] = 1-(currenty-sin(angle)*1.3f)/231.f;
			visline[c][i*6+2] = 0.f;
			visline[c][i*6+5] = 1.f;
			prevy = currenty;
			currenty = nexty;
		}
	}
}
void PFAudioProcessorEditor::paint(Graphics& g) { }

void PFAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) {
		if(knobs[i].hoverstate < -1) {
			needtoupdate = 2;
			knobs[i].hoverstate++;
		}
	}
	if(held > 0) held--;

	if(oversamplingalpha != (hover<=-4?1:0)) {
		oversamplingalpha = fmax(fmin(oversamplingalpha+(hover<=-4?.07f:-.07f),1),0);
		needtoupdate = 2;
	}

	float os = oversampling?1:0;
	if(oversamplinglerped != os?1:0) {
		needtoupdate = 2;
		if(fabs(oversamplinglerped-os) <= .001f) oversamplinglerped = os;
		oversamplinglerped = oversamplinglerped*.75f+os*.25f;
	}

	if(creditsalpha != ((hover<=-2&&hover>=-3)?1:0)) {
		creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.07f:-.07f),1),0);
		needtoupdate = 2;
	}
	if(creditsalpha <= 0) websiteht = -1;
	if(websiteht >= -.227273) {
		websiteht -= .05;
		needtoupdate = 2;
	}

	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;

	if(reset_size)
		needtoupdate = 2;

	update();
}

void PFAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		oversampling = newValue>.5f;
		needtoupdate = 2;
		return;
	}
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		calcvis();
		needtoupdate = 2;
		return;
	}
}
void PFAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
	if(prevhover != hover && held == 0) {
		if(hover > -1) knobs[hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
	}
}
void PFAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void PFAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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

	held = -1;
	initialdrag = hover;
	if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		audio_processor.lerpchanged[hover] = true;
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	} else if(hover < -4) {
		oversampling = hover == -6;
		audio_processor.apvts.getParameter("oversampling")->setValueNotifyingHost(oversampling?1.f:0.f);
		audio_processor.undo_manager.setCurrentTransactionName(oversampling?"Turned oversampling on":"Turned oversampling off");
		audio_processor.undo_manager.beginNewTransaction();
	}
}
void PFAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(hover == -1) return;
	if(initialdrag > -1) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset);

		valueoffset = fmax(fmin(valueoffset,value+.1),value-1.1);
	} else if(initialdrag == -3) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y)==-3?-3:-2;
		if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
	}
}
void PFAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(hover > -1) {
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((knobs[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		if(hover == -3) {
			if(prevhover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht < -.227273) websiteht = 0.7933884298f;
			if(hover > -1) knobs[hover].hoverstate = -4;
		}
	}
	held = 1;
}
void PFAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audio_processor.undo_manager.beginNewTransaction();
}
void PFAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int PFAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if(x >= 8 && x <= 234 && y >= 8 && y <= 87) {
		if(y < 39 || y > 53) return -4;
		if(x >= 115 && x <= 143) return -5;
		if(x >= 144 && x <= 172) return -6;
		return -4;
	} else if(y >= 403 && y < 462) {
		if(x >= 50 && x <= 196 && y >= 412 && y <= 456) return -3;
		return -2;
	}
	float xx = 0, yy = 0;
	for(int i = 0; i < knobcount; i++) {
		xx = knobs[i].x-x;
		yy = knobs[i].y-y;
		if((xx*xx+yy*yy)<=576) return i;
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
	return Font(font,"Regular",18.f*scale);
}
void LookNFeel::drawPopupMenuBackground(Graphics &g, int width, int height) {
	g.setColour(bg);
	g.fillRect(0,0,width,height);
}
void LookNFeel::drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) {
	if(isSeparator) {
		g.setColour(fg);
		g.fillRect(2*scale,0.f,area.getWidth()-4*scale,(float)area.getHeight());
		return;
	}

	bool removeleft = text.startsWith("'");
	if(isHighlighted && isActive) {
		g.setColour(fg);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		g.setColour(bg);
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
		idealHeight = (int)round(scale);
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
