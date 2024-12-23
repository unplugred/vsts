#include "processor.h"
#include "editor.h"
#include "settings.h"

VUAudioProcessorEditor::VUAudioProcessorEditor(VUAudioProcessor& p, int paramcount, pluginpreset state, potentiometer pots[]) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 0, 0, 1.f, 1.f, true, false) {
	for(int i = 0; i < knobcount; i++) {
		knobs[i].id = pots[i].id;
		knobs[i].name = pots[i].name;
		knobs[i].value = state.values[i];
		knobs[i].isbool = pots[i].ttype == potentiometer::ptype::booltype;
		knobs[i].minimumvalue = pots[i].minimumvalue;
		knobs[i].maximumvalue = pots[i].maximumvalue;
		knobs[i].defaultvalue = pots[i].defaultvalue;
		add_listener(knobs[i].id);
	}
	knobs[0].svalue = String(knobs[0].value)+"dB";

	multiplier = 1.f/Decibels::decibelsToGain(knobs[0].value);
	stereodamp = knobs[2].value;

	init();

	set_size(round(audio_processor.height.get()*(32.f*(stereodamp+1)/19.f)),audio_processor.height.get());
#ifdef BANNER
	banner_offset = 21.f/getHeight();
	getConstrainer()->setFixedAspectRatio(32.f*(stereodamp+1)/19.f-21.f/width);
#else
	getConstrainer()->setFixedAspectRatio(32.f*(stereodamp+1)/19.f);
#endif
	setResizable(true,true);
	setResizeLimits(200,200,1920,1080);

	dontscale = false;
}

VUAudioProcessorEditor::~VUAudioProcessorEditor() {
	if(audio_processor.settingswindow != nullptr) {
		delete audio_processor.settingswindow;
		audio_processor.settingswindow = nullptr;
	}
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
uniform float banner;
out vec2 v_TexCoord;
out vec2 metercoords;
void main(){
	metercoords = aPos;
	if(stereo <= .001) {
		gl_Position = vec4((aPos*vec2(1,1-banner)+vec2(0,banner))*2-1,0,1);
		v_TexCoord = (aPos+vec2(2,0))*size;
	} else if(right < .5) {
		gl_Position = vec4((aPos*vec2(1,1-banner)+vec2(0,banner))*2-1,0,1);
		v_TexCoord = aPos*vec2(1+stereo,1)*size;
		metercoords.x = metercoords.x*(1+stereo)-.0387*stereo;
	} else {
		gl_Position = vec4((aPos.x+1)*(2-stereoinv)-1,(aPos.y*(1-banner)+banner)*2-1,0,1);
		v_TexCoord = (aPos+vec2(1,0))*size;
		metercoords.x+=.0465;
	}
	metercoords = (metercoords-.5)*vec2(32./19.,1)+vec2(0,.5);
	metercoords.y += stereoinv*.06+.04;
	metercoords = vec2(metercoords.x*cos(rotation)-metercoords.y*sin(rotation)+.5,metercoords.x*sin(rotation)+metercoords.y*cos(rotation));
	metercoords.y -= stereoinv*.06+.04;
})",
//FRAG
R"(#version 150 core
in vec2 v_TexCoord;
in vec2 metercoords;
uniform float peak;
uniform vec2 size;
uniform float stereo;
uniform float right;
uniform sampler2D vutex;
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
})");

	add_texture(&vutex, BinaryData::map_png, BinaryData::map_pngSize);

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

	vushader->setUniform("banner", banner_offset);
	vushader->setUniform("rotation", (leftvu-.5f)*(1.47f-stereodamp*.025f));
	vushader->setUniform("peak", leftpeaklerp);
	vushader->setUniform("right", 0.f);

	vushader->setUniform("stereo", stereodamp);
	vushader->setUniform("stereoinv", 2-(1/(stereodamp*.5f+.5f)));

	vushader->setUniform("size", 800.f/vutex.getWidth(), 475.f/vutex.getHeight());

	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	auto coord = context.extensions.glGetAttribLocation(vushader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if(stereodamp > .001) {
		vushader->setUniform("rotation", (rightvu-.5f)*1.445f);
		vushader->setUniform("peak", rightpeaklerp);
		vushader->setUniform("right", 1.f);
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

	if(positionwindow && audio_processor.settingswindow != nullptr) {
		if(audio_processor.settingsx.get() != -999 && audio_processor.settingsy.get() != -999)
			audio_processor.settingswindow->setTopLeftPosition(audio_processor.settingsx.get(),audio_processor.settingsy.get());
		else
			audio_processor.settingswindow->centreAroundComponent(this,getWidth(),getHeight());
		positionwindow = false;
	}

	if(firstframe && audio_processor.settingsopen.get()) openwindow();
	firstframe = false;
	audio_processor.settingsopen = audio_processor.settingswindow != nullptr;
}

void VUAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		if(i == 0) {
			multiplier = 1.f/Decibels::decibelsToGain((float)newValue);
			knobs[i].svalue = String(newValue)+"dB";
		}
		knobs[i].value = newValue;
		return;
	}
}
void VUAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	openwindow();
}
void VUAudioProcessorEditor::openwindow() {
	if(audio_processor.settingswindow == nullptr) {
		audio_processor.settingswindow = new VUAudioProcessorSettings(audio_processor,knobcount,knobs);
		positionwindow = true;
	} else {
		delete audio_processor.settingswindow;
		audio_processor.settingswindow = nullptr;
	}
}
