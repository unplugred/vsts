#include "processor.h"
#include "editor.h"

PisstortionAudioProcessorEditor::PisstortionAudioProcessorEditor(PisstortionAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), plugmachine_gui(p, 242, 462) {
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 3; y++) {
			int i = x+y*2;
			knobs[i].x = x*106+68;
			knobs[i].y = y*100+144;
			knobs[i].id = params.pots[i].id;
			knobs[i].name = params.pots[i].name;
			knobs[i].value = params.pots[i].normalize(state.values[i]);
			knobs[i].lerpedvalue = knobs[i].value;
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

	for(int i = 0; i < 20; i++) {
		bubbleregen(i);
		bubbles[i].wiggleage = random.nextFloat();
		bubbles[i].moveage = random.nextFloat();
	}

	calcvis();

	init(&look_n_feel);
}
PisstortionAudioProcessorEditor::~PisstortionAudioProcessorEditor() {
	close();
}

void PisstortionAudioProcessorEditor::newOpenGLContextCreated() {
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = aPos;
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D circletex;
uniform sampler2D basetex;
uniform float dpi;
out vec4 fragColor;
void main(){
	float bubbles = texture(circletex,uv).r;
	vec3 c = max(min((texture(basetex,uv).rgb-.5)*dpi+.5,1),0);
	if(c.g >= 1) c.b = 0;
	if(bubbles > 0 && uv.y > .7)
		c = vec3(c.r,c.g*(1-bubbles),c.b+c.g*bubbles);
	float gradient = (1-uv.y)*.5+bubbles*.3;
	float grayscale = c.g*.95+c.b*.85+(1-c.r-c.g-c.b)*.05;
	fragColor = vec4(vec3(grayscale)+c.r*gradient+c.r*(1-gradient)*vec3(0.984,0.879,0.426),1);
})");

	knobshader = add_shader(
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 knobscale;
uniform float ratio;
uniform vec2 knobpos;
uniform float knobrot;
out vec2 uv;
out vec2 circlecoord;
void main(){
	vec2 pos = ((aPos*2-1)*knobscale)/vec2(ratio,1);
	gl_Position = vec4(
		(pos.x*cos(knobrot)-pos.y*sin(knobrot))*ratio-1+knobpos.x,
		pos.x*sin(knobrot)+pos.y*cos(knobrot)-1+knobpos.y,0,1);
	uv = aPos;
	circlecoord = gl_Position.xy*.5+.5;
	gl_Position.y = gl_Position.y*(1-banner)+banner;
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
in vec2 circlecoord;
uniform sampler2D knobtex;
uniform sampler2D circletex;
uniform float hover;
out vec4 fragColor;
void main(){
	vec3 c = texture(knobtex,uv).rgb;
	if(c.r > 0) {
		float bubbles = texture(circletex,circlecoord).r;
		float col = max(min(c.g*4-(1-hover)*3,1),0);
		col = (1-col)*bubbles+col;
		col = .05 + c.b*.8 + col*.1;
		fragColor = vec4(vec3(col),c.r);
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
	gl_Position = vec4(aPos.x,aPos.y*(1-banner)+banner,0,1);
	basecoord = aPos.xy*.5+.5;
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
	fragColor = vec4(.05,.05,.05,texture(basetex,basecoord).r<=0?alpha*min((1-abs(linepos*2-1))*dpi*1.3,1):0.0);
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
	gl_Position = vec4(aPos.x-.5,aPos.y*(1-banner)*.2+.7+banner*.3,0,1);
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
	vec3 tex = texture(basetex,basecoord).rgb;
	if(tex.r <= 0) {
		float bleh = 0;
		if(tex.g >= .99 && tex.b > .02) bleh = min(max((tex.b-.5)*dpi+.5,0),1);
		if(highlightcoord.x>0&&highlightcoord.x<1&&highlightcoord.y>0&&highlightcoord.y<1)
			bleh = 1-bleh;
		fragColor = vec4(.05,.05,.05,bleh*alpha);
	} else {
		fragColor = vec4(0);
	}
})");

	creditsshader = add_shader(
//CREDITS VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 uv;
out vec2 basecoord;
void main(){
	gl_Position = vec4(aPos.x*2-1,1-(1-aPos.y*(57./462.)*(1-banner)-banner)*2,0,1);
	uv = aPos;
	basecoord = aPos*vec2(1,57./462.);
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 uv;
in vec2 basecoord;
uniform sampler2D basetex;
uniform sampler2D creditstex;
uniform float alpha;
uniform float shineprog;
uniform float dpi;
out vec4 fragColor;
void main(){
	float y = (uv.y+alpha)*1.1875;
	float creditols = 0;
	float shine = 0;
	vec3 base = texture(basetex,basecoord).rgb;
	if(base.r <= 0) {
		if(y < 1)
			creditols = texture(creditstex,vec2(uv.x,y)).b;
		else if(y > 1.1875 && y < 2.1875) {
			y -= 1.1875;
			creditols = texture(creditstex,vec2(uv.x,y)).r;
			if(uv.x+shineprog < 1 && uv.x+shineprog > .582644628099)
				shine = max(min((texture(creditstex,vec2(uv.x+shineprog,y)).g*min(base.g+base.b,1)-.5)*dpi+.5,1),0);
		}
	}
	fragColor = vec4(vec3(.05+shine*.8),max(min((creditols-.5)*dpi+.5,1),0));
})");

	circleshader = add_shader(
//CIRCLE VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 pos;
uniform float ratio;
uniform float banner;
out vec2 uv;
void main(){
	float f = .1;
	gl_Position = vec4((aPos*2-1)*vec2(1,ratio)*f+pos*2-1,0,1);
	gl_Position.y = gl_Position.y*(1-banner)-banner;
	uv = aPos*2-1;
})",
//CIRCLE FRAG
R"(#version 150 core
in vec2 uv;
uniform float dpi;
out vec4 fragColor;
void main(){
	float x = sqrt(uv.x*uv.x+uv.y*uv.y);
	float f = .7071;
	fragColor = vec4(1,1,1,(x>(1-(1-f)*.5)?(1-x):(x-f))*dpi*12);
})");

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize);
	add_texture(&knobtex, BinaryData::knob_png, BinaryData::knob_pngSize, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);
	add_texture(&creditstex, BinaryData::credits_png, BinaryData::credits_pngSize);

	add_frame_buffer(&framebuffer, width, height);

	draw_init();
}
void PisstortionAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	framebuffer.makeCurrentRenderingTarget();
	OpenGLHelpers::clear(Colour::fromRGB(0,0,0));
	circleshader->use();
	circleshader->setUniform("banner",banner_offset);
	circleshader->setUniform("ratio",((float)width)/height);
	circleshader->setUniform("dpi",(float)fmax(scaled_dpi,1));
	coord = context.extensions.glGetAttribLocation(circleshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < 20; i++) {
		circleshader->setUniform("pos",bubbles[i].xoffset+
			sin(bubbles[i].moveage*bubbles[i].movespeed+bubbles[i].moveoffset)*bubbles[i].moveamount+
			sin(bubbles[i].wiggleage)*bubbles[i].wiggleamount,
			bubbles[i].moveage+.05f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);
	framebuffer.releaseAsRenderingTarget();

	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	baseshader->setUniform("circletex",1);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("dpi",(float)fmax(scaled_dpi,1));
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	knobshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	knobtex.bind();
	knobshader->setUniform("knobtex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	knobshader->setUniform("circletex",1);
	knobshader->setUniform("banner",banner_offset);
	knobshader->setUniform("knobscale",54.f/width,54.f/height);
	knobshader->setUniform("ratio",((float)height)/width);
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < knobcount; i++) {
		knobshader->setUniform("knobpos",((float)knobs[i].x*2)/width,2-((float)knobs[i].y*2)/height);
		knobshader->setUniform("knobrot",(float)fmod((knobs[i].lerpedvalue-.5f)*.748f-.125f,1)*-6.2831853072f);
		knobshader->setUniform("hover",knobs[i].hover<0.5?4*knobs[i].hover*knobs[i].hover*knobs[i].hover:1-(float)pow(2-2*knobs[i].hover,3)/2);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	float osalpha = oversamplingalpha;
	if(oversamplingalpha < 1) {
		if(oversamplingalpha > 0)
			osalpha = oversamplingalpha<0.5?4*oversamplingalpha*oversamplingalpha*oversamplingalpha:1-(float)pow(2-2*oversamplingalpha,3)/2;
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
		oversamplingshader->setUniform("dpi",(float)fmax(scaled_dpi,1));
		oversamplingshader->setUniform("banner",banner_offset);
		oversamplingshader->setUniform("alpha",osalpha);
		oversamplingshader->setUniform("selection",.458677686f+oversamplinglerped*.2314049587f);
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	creditsshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	context.extensions.glActiveTexture(GL_TEXTURE1);
	creditstex.bind();
	creditsshader->setUniform("basetex",0);
	creditsshader->setUniform("dpi",(float)fmax(scaled_dpi,1));
	creditsshader->setUniform("banner",banner_offset);
	creditsshader->setUniform("creditstex",1);
	creditsshader->setUniform("alpha",creditsalpha<0.5?4*creditsalpha*creditsalpha*creditsalpha:1-(float)pow(2-2*creditsalpha,3)/2);
	creditsshader->setUniform("shineprog",websiteht);
	coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void PisstortionAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void PisstortionAudioProcessorEditor::calcvis() {
	is_stereo = knobs[4].value > 0 && knobs[1].value > 0 && knobs[5].value > 0;
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
				nextnexty = 48+audio_processor.pisstortion(sin((i*.5+.5)/35.8098621957f)*.8f,c,2,pp,false)*38;
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
void PisstortionAudioProcessorEditor::paint(Graphics& g) { }

void PisstortionAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) {
		knobs[i].lerpedvalue = knobs[i].lerpedvalue*.6f+knobs[i].value*.4;
		if(knobs[i].hover != (hover==i?1:0))
			knobs[i].hover = fmax(fmin(knobs[i].hover+(hover==i?.07f:-.07f), 1), 0);
	}
	if(held > 0) held--;

	if(oversamplingalpha != (hover<=-4?1:0))
		oversamplingalpha = fmax(fmin(oversamplingalpha+(hover<=-4?.07f:-.07f),1),0);

	float os = oversampling?1:0;
	if(oversamplinglerped != os?1:0) {
		if(fabs(oversamplinglerped-os) <= .001f) oversamplinglerped = os;
		oversamplinglerped = oversamplinglerped*.75f+os*.25f;
	}

	if(creditsalpha != ((hover<=-2&&hover>=-3)?1:0))
		creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.05f:-.05f),1),0);
	if(creditsalpha <= 0) websiteht = -1;
	if(websiteht >= -.227273) websiteht -= .05;

	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;
	rmslerped = rmslerped*.6f+rms*.4f;

	for(int i = 0; i < 20; i++) {
		bubbles[i].moveage += bubbles[i].yspeed*(1+rmslerped*30);
		if(bubbles[i].moveage >= 1)
			bubbleregen(i);
		else
			bubbles[i].wiggleage = fmod(bubbles[i].wiggleage+bubbles[i].wigglespeed,6.2831853072f);
	}

	if(audio_processor.updatevis.get()) {
		calcvis();
		audio_processor.updatevis = false;
	}

	update();
}
void PisstortionAudioProcessorEditor::bubbleregen(int i) {
	float tradeoff = random.nextFloat();
	bubbles[i].wiggleamount = tradeoff*.05f+random.nextFloat()*.005f;
	bubbles[i].wigglespeed = (1-tradeoff)*.15f+random.nextFloat()*.015f;
	bubbles[i].wiggleage = random.nextFloat()*6.2831853072f;
	bubbles[i].moveamount = random.nextFloat()*.5f;
	bubbles[i].movespeed = random.nextFloat()*5.f;
	bubbles[i].moveoffset = random.nextFloat()*6.2831853072f;
	bubbles[i].moveage = 0;
	bubbles[i].yspeed = random.nextFloat()*.0015f+.00075f;
	bubbles[i].xoffset = random.nextFloat();
}

void PisstortionAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "oversampling") {
		oversampling = newValue>.5f;
		return;
	}
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		calcvis();
		return;
	}
}
void PisstortionAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht < -.227273) websiteht = 0.7933884298f;
}
void PisstortionAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void PisstortionAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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
void PisstortionAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
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
void PisstortionAudioProcessorEditor::mouseUp(const MouseEvent& event) {
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
		}
	}
	held = 1;
}
void PisstortionAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audio_processor.undo_manager.beginNewTransaction();
}
void PisstortionAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int PisstortionAudioProcessorEditor::recalc_hover(float x, float y) {
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
	font = find_font("Arial|Helvetica Neue|Helvetica|Roboto");
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	Font fontt = Font(font,"Regular",18.f*scale);
	fontt.setHorizontalScale(.9f);
	fontt.setExtraKerningFactor(-.05f);
	return fontt;
}
int LookNFeel::getMenuWindowFlags() {
	//return ComponentPeer::windowHasDropShadow;
	return 0;
}
void LookNFeel::drawPopupMenuBackground(Graphics &g, int width, int height) {
	g.setColour(fg);
	g.fillRoundedRectangle(0,0,width,height,4*scale);
	g.setColour(bg);
	g.fillRoundedRectangle(2*scale,2*scale,width-4*scale,height-4*scale,2*scale);
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
		g.setColour(ht);
		g.fillRect(2*scale,0.f,area.getWidth()-4*scale,(float)area.getHeight());
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
	return (int)floor(2*scale);
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
