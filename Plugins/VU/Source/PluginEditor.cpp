#include "PluginProcessor.h"
#include "PluginEditor.h"

VUAudioProcessorEditor::VUAudioProcessorEditor(VUAudioProcessor& p, int paramcount, pluginpreset state, potentiometer pots[]) : audio_processor(p), plugmachine_gui(p, 0, 0, 1.f, 1.f, true, false) {
	for(int i = 0; i < knobcount; i++) {
		knobs[i].id = pots[i].id;
		knobs[i].name = pots[i].name;
		knobs[i].value = state.values[i];
		knobs[i].minimumvalue = pots[i].minimumvalue;
		knobs[i].maximumvalue = pots[i].maximumvalue;
		knobs[i].defaultvalue = pots[i].defaultvalue;
		add_listener(knobs[i].id);
	}

	multiplier = 1.f/Decibels::decibelsToGain(knobs[0].value);
	stereodamp = knobs[2].value;

	init(&look_n_feel);

	int h = audio_processor.height.get();
	float w = h*(32.f*(stereodamp+1)/19.f);
#ifdef BANNER
	getConstrainer()->setFixedAspectRatio(32.f*(stereodamp+1)/19.f-21.f/w);
#else
	getConstrainer()->setFixedAspectRatio(32.f*(stereodamp+1)/19.f);
#endif
	set_size(round(w),h);
	setResizable(true,true);
	setResizeLimits(200,200,1920,1080);

	dontscale = false;
}

VUAudioProcessorEditor::~VUAudioProcessorEditor() {
	close();
}

void VUAudioProcessorEditor::resized() {
	if(dontscale) return;
	width = getWidth();
#ifdef BANNER
	height = getHeight()-21;
	banner_offset = 21.f/getHeight();
	float w = height*(32.f*(stereodamp+1)/19.f);
	getConstrainer()->setFixedAspectRatio(32.f*(stereodamp+1)/19.f-21.f/w);
#else
	height = getHeight();
#endif
	audio_processor.height = height;
	debug_font.width = getWidth();
	debug_font.height = getHeight();
	debug_font.banner_offset = banner_offset;
}

void VUAudioProcessorEditor::newOpenGLContextCreated() {
	vushader = add_shader(
//VERT
R"(#version 150 core
in vec2 aPos;
uniform float rotation;
uniform float right;
uniform float stereo;
uniform float stereoinv;
uniform vec2 size;
uniform vec4 lgsize;
uniform float banner;
out vec2 v_TexCoord;
out vec2 metercoords;
out vec2 txtcoords;
out vec2 lgcoords;
void main(){
	metercoords = aPos;
	if(stereo <= .001) {
		gl_Position = vec4((aPos*vec2(1,1-banner)+vec2(0,banner))*2-1,0,1);
		v_TexCoord = (aPos+vec2(2,0))*size;
		txtcoords = aPos;
	} else if(right < .5) {
		gl_Position = vec4((aPos*vec2(1,1-banner)+vec2(0,banner))*2-1,0,1);
		v_TexCoord = aPos*vec2(1+stereo,1)*size;
		txtcoords = vec2(aPos.x*(1+stereo)-.5*stereo,aPos.y);
		metercoords.x = metercoords.x*(1+stereo)-.0387*stereo;
	} else {
		gl_Position = vec4((aPos.x+1)*(2-stereoinv)-1,(aPos.y*(1-banner)+banner)*2-1,0,1);
		v_TexCoord = (aPos+vec2(1,0))*size;
		txtcoords = aPos+vec2(1-stereo*.5,0);
		metercoords.x+=.0465;
	}
	metercoords = (metercoords-.5)*vec2(32./19.,1)+vec2(0,.5);
	metercoords.y += stereoinv*.06+.04;
	metercoords = vec2(metercoords.x*cos(rotation)-metercoords.y*sin(rotation)+.5,metercoords.x*sin(rotation)+metercoords.y*cos(rotation));
	metercoords.y -= stereoinv*.06+.04;
	if(right < .5) lgcoords = aPos*lgsize.xy+vec2(lgsize.z-lgsize.x,lgsize.w);
	else lgcoords = aPos*lgsize.xy+vec2(lgsize.z,lgsize.w);
})",
//FRAG
R"(#version 150 core
in vec2 v_TexCoord;
in vec2 metercoords;
in vec2 txtcoords;
in vec2 lgcoords;
uniform float peak;
uniform vec2 size;
uniform vec2 txtsize;
uniform vec3 lines;
uniform float stereo;
uniform vec4 lineht;
uniform float right;
uniform float pause;
uniform sampler2D vutex;
uniform sampler2D mptex;
uniform sampler2D lgtex;
out vec4 fragColor;
void main(){

	if(peak >= .999) fragColor = texture(vutex,v_TexCoord+vec2(0,1-size.y*2));
	else {
		fragColor = texture(vutex,v_TexCoord+vec2(0,1-size.y));
		if(peak > .001) fragColor = fragColor*(1-peak)+texture(vutex,v_TexCoord+vec2(0,1-size.y*2))*peak;
	}
	vec4 mask = texture(vutex,v_TexCoord+vec2(0,1-size.y*3));
	vec4 ids = texture(vutex,v_TexCoord+vec2(0,1-size.y*4));
	if(right < .5 && stereo > .001 && stereo < .999) {
		vec3 single = texture(vutex,v_TexCoord+vec2(size.x*2,1-size.y)).rgb;
		if(peak >= .999) single = texture(vutex,v_TexCoord+vec2(size.x*2,1-size.y*2)).rgb;
		else single = single*(1-peak)+texture(vutex,v_TexCoord+vec2(size.x*2,1-size.y*2)).rgb*peak;
		fragColor = v_TexCoord.x>size.x?fragColor:(fragColor*stereo+vec4(single,1.)*(1-stereo));
		mask = mask*stereo+texture(vutex,v_TexCoord+vec2(size.x*2,1-size.y*3))*(1-stereo);
		ids = ids*stereo+texture(vutex,v_TexCoord+vec2(size.x*2,1-size.y*4))*(1-stereo);
	}

	vec3 meter = vec3(0.);
	if(metercoords.x > .001 && metercoords.y > .001 && metercoords.x < .999 && metercoords.y < .999)
		meter = texture(vutex,min(max(metercoords,0),1)*vec2(size.x*.59375,size.y)+vec2(size.x*3,1-size.y)).rgb*ids.b;
	vec3 shadow = vec3(1.);
	if(metercoords.x > .016 && metercoords.y > -.014 && metercoords.x < 1.14 && metercoords.y < .984)
		shadow = 1-texture(vutex,min(max(metercoords+vec2(-.015,.015),0),1)*vec2(size.x*.59375,size.y)+vec2(size.x*3,1-size.y*2)).rgb*ids.r*.15;
	fragColor = vec4(fragColor.rgb*(1-meter)*shadow+mask.rgb*meter,1.);

	if(pause > .001) {
		vec2 txdiv = vec2(3.8,7.4);
		float bg = ids.g*pause;
		if(stereo > .001) bg = texture(vutex,vec2(min(max(v_TexCoord.x+size.x*(2-stereo*.5),size.x*2),size.x*3),v_TexCoord.y+(1-size.y*4))).g*pause;
		if(abs(txtcoords.x-.5) <= 1/txdiv.x && abs(txtcoords.y-.5) <= 1.5/txdiv.y) {
			bool highlight = false;
			float line = 0;
			if(abs(txtcoords.y-.5) > .5/txdiv.y) {
				if(txtcoords.y>.5) {
					line = 1.5+lines.x;
					highlight = lineht.x>=.5;
				} else {
					line = -.5+lines.z;
					highlight = lineht.z>=.5;
				}
			} else {
				line = .5+lines.y;
				highlight = lineht.y>=.5;
			}
			vec3 txcoordss = texture(mptex,((txtcoords-.5)*txdiv-vec2(1,line))*vec2(.5,.03125)).rgb;
			float tx = texture(vutex,(txcoordss.rg+mod(((txtcoords-.5)*txdiv*vec2(8,1)-vec2(0,.5)),1.)*vec2(.125,.25))*txtsize+vec2(size.x*3,1-size.y*3)).g*(1-txcoordss.b*(highlight?0.4:0.0));

			fragColor = abs(fragColor-bg)*(1-tx)+max(fragColor-bg,0)*tx;
		} else fragColor = abs(fragColor-bg);

		if(lgcoords.x > 0 && lgcoords.y > 0 && lgcoords.y < 1) {
			vec2 lg = texture(lgtex,lgcoords).rb;
			if(lineht.w > -1) fragColor = fragColor*(1-lg.g)+(1-(1-vec4(1.,.4549,.2588,1.))*(1-texture(lgtex,lgcoords+vec2(lineht.w,0)).g))*lg.r;
			else fragColor = fragColor*(1-lg.g)+vec4(1.,.4549,.2588,1.)*lg.r;
		}
	}

	if(right > .5) fragColor.rgb *= .5;
})");

	add_texture(&vutex, BinaryData::map_png, BinaryData::map_pngSize);
	add_texture(&mptex, BinaryData::txtmap_png, BinaryData::txtmap_pngSize, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	add_texture(&lgtex, BinaryData::genuine_soundware_png, BinaryData::genuine_soundware_pngSize, GL_NEAREST, GL_NEAREST);

	draw_init();
}
void VUAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	vushader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	vutex.bind();
	vushader->setUniform("vutex", 0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	mptex.bind();
	vushader->setUniform("mptex", 1);
	context.extensions.glActiveTexture(GL_TEXTURE2);
	lgtex.bind();
	vushader->setUniform("lgtex", 2);

	vushader->setUniform("banner", banner_offset);
	vushader->setUniform("rotation", (leftvu-.5f)*(1.47f-stereodamp*.025f));
	vushader->setUniform("peak", leftpeaklerp);
	vushader->setUniform("right", 0.f);
	vushader->setUniform("lgsize",
		((float)getWidth())/lgtex.getWidth(),
		((1-banner_offset)*getHeight())/lgtex.getHeight(),
		164.f/lgtex.getWidth(),
		(62.f/lgtex.getHeight())*(1-settingsfade));

	vushader->setUniform("stereo", stereodamp);
	vushader->setUniform("stereoinv", 2-(1/(stereodamp*.5f+.5f)));
	vushader->setUniform("pause", settingsfade);
	vushader->setUniform("lines", 24.f+knobs[0].value, -4.f-knobs[1].value, hover==2?31.f:(30.f-knobs[2].value));
	vushader->setUniform("lineht", hover==0?1.f:0.f,hover==1?1.f:0.f,1.f,websiteht);

	vushader->setUniform("size", 800.f/vutex.getWidth(), 475.f/vutex.getHeight());
	vushader->setUniform("txtsize", 384.f/vutex.getWidth(), 354.f/vutex.getHeight());

	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	auto coord = context.extensions.glGetAttribLocation(vushader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if(stereodamp > .001) {
		vushader->setUniform("rotation", (rightvu-.5f)*1.445f);
		vushader->setUniform("peak", rightpeaklerp);
		vushader->setUniform("right", 1.f);
		vushader->setUniform("lgsize",
			((float)getWidth())/lgtex.getWidth()*(1/(stereodamp+1)),
			((1-banner_offset)*getHeight())/lgtex.getHeight(),
			164.f/lgtex.getWidth()-(stereodamp/152)*(1-banner_offset)*getHeight()*1.56f,
			(62.f/lgtex.getHeight())*(1-settingsfade));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void VUAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void VUAudioProcessorEditor::paint(Graphics& g) {}

void VUAudioProcessorEditor::timerCallback() {
	int buffercount = audio_processor.buffercount.get();
	if(buffercount > 0) {
		leftrms = sqrt(audio_processor.leftvu.get()/buffercount);
		rightrms = sqrt(audio_processor.rightvu.get()/buffercount);
		audio_processor.leftvu = 0;
		audio_processor.rightvu = 0;
		audio_processor.buffercount = 0;

		if(audio_processor.leftpeak.get()) {
			leftpeak = true;
			audio_processor.leftpeak = false;
		}
		if(audio_processor.rightpeak.get()) {
			rightpeak = true;
			audio_processor.rightpeak = false;
		}

		bypassdetection = 0;
	} else if(bypassdetection > 5) {
		leftrms = 0;
		rightrms = 0;
		leftpeak = false;
		rightpeak = false;
	} else bypassdetection++;

	leftvu = functions::smoothdamp(leftvu,fmin(leftrms*multiplier,1),&leftvelocity,knobs[1].value*.05f,-1,.03333f);
	if(knobs[2].value>.5) rightvu = functions::smoothdamp(rightvu,fmin(rightrms*multiplier,1),&rightvelocity,knobs[1].value*.05f,-1,.03333f);
	leftpeaklerp = functions::smoothdamp(leftpeaklerp,leftpeak?1:0,&leftpeakvelocity,0.027f,-1,.03333f);
	rightpeaklerp = functions::smoothdamp(rightpeaklerp,rightpeak?1:0,&leftpeakvelocity,0.027f,-1,.03333f);

	if(leftpeaklerp >= .999 && ++lefthold >= 2) {
		leftpeak = false;
		lefthold = 0;
	}
	if(rightpeaklerp >= .999 && ++righthold >= 2) {
		rightpeak = false;
		righthold = 0;
	}

	settingsfade = functions::smoothdamp(settingsfade,fmin(settingstimer,1),&settingsvelocity,0.3,-1,.03333f);
	settingstimer = held?60:fmax(settingstimer-1,0);
	websiteht -= .05f;

	float prevstereodamp = stereodamp;
	stereodamp = functions::smoothdamp(stereodamp,knobs[2].value,&stereovelocity,0.3,-1,.03333f);
	if(prevstereodamp > .0001 && prevstereodamp < .9999) {
		int h = audio_processor.height.get();
		float w = h*32.f*(stereodamp+1)/19.f;
#ifdef BANNER
		getConstrainer()->setFixedAspectRatio(32.f*(stereodamp+1)/19.f-21.f/w);
#else
		getConstrainer()->setFixedAspectRatio(32.f*(stereodamp+1)/19.f);
#endif
		set_size(round(w),h);
	}

	update();
}

void VUAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		if(i == 0) multiplier = 1.f/Decibels::decibelsToGain((float)newValue);
		knobs[i].value = newValue;
		return;
	}
}
void VUAudioProcessorEditor::mouseEnter(const MouseEvent& event) {
	settingstimer = 120;
}
void VUAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	settingstimer = fmax(settingstimer,60);
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == 3 && prevhover != 3 && websiteht < -.6) websiteht = 1.0f;
}
void VUAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	if(!held) settingstimer = 0;
}
void VUAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	held = true;
	initialdrag = hover;
	if(hover == 0 || hover == 1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void VUAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(initialdrag == 2) hover = recalc_hover(event.x,event.y)==2?2:-1;
	else if(initialdrag == 3) hover = recalc_hover(event.x,event.y)==3?3:-1;
	else if(hover == 0 || hover == 1) {
		float dist = (event.getDistanceFromDragStartY()+event.getDistanceFromDragStartX())*-.04f;
		float val = dist+valueoffset;
		int clampval = initialvalue-(val>0?floor(val):ceil(val));
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].normalize(clampval));
		valueoffset = fmax(fmin(valueoffset,initialvalue-dist-knobs[hover].minimumvalue+1),initialvalue-dist-knobs[hover].maximumvalue-1);
	}
}
void VUAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(hover == 0 || hover == 1) {
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((knobs[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else if(hover == 2) {
		bool stereo = knobs[2].value<.5;
		audio_processor.apvts.getParameter("stereo")->setValueNotifyingHost(stereo?1:0);
		audio_processor.undo_manager.setCurrentTransactionName(stereo?"Set to stereo":"Set to mono");
		audio_processor.undo_manager.beginNewTransaction();
	} else if(hover == 3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
	held = false;
	hover = recalc_hover(event.x,event.y);
}
void VUAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover != 0 && hover != 1) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].normalize(knobs[hover].defaultvalue));
	audio_processor.undo_manager.beginNewTransaction();
}
void VUAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if((hover != 0 && hover != 1) || wheel.isSmooth) return;
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].normalize(knobs[hover].value+(wheel.deltaY>0?1:-1)));
}
int VUAudioProcessorEditor::recalc_hover(float x, float y) {
	float xx = (x/getWidth()-.5f)*8*3.8*(knobs[2].value+1);
	float yy = (y/((1-banner_offset)*getHeight())-.5f)*(7.4f);
	if(xx >= 1 && ((xx <= 8 && knobs[0].value <= -10) || xx <= 7) && yy > -1.5 && yy <= -.5)
		return 0;
	if(xx >= 1 && xx <= 4 && yy > -.5 && yy <= .5)
		return 1;
	if((yy > .5 && yy <= 1.5) && ((knobs[2].value<.5 && xx >= -8 && xx <= -2) || (knobs[2].value>.5 && xx >= -1 && xx <= 3)))
		return 2;
	if(x >= (getWidth()-151) && y >= ((1-banner_offset)*getHeight()-49) && x < (getWidth()-1) && y < ((1-banner_offset)*getHeight()-1))
		return 3;
	return -1;
}
