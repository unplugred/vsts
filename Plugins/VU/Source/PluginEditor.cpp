#include "PluginProcessor.h"
#include "PluginEditor.h"

VuAudioProcessorEditor::VuAudioProcessorEditor(VuAudioProcessor& p, int paramcount, pluginpreset state, potentiometer pots[])
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	for(int i = 0; i < knobcount; i++) {
		knobs[i].id = pots[i].id;
		knobs[i].name = pots[i].name;
		knobs[i].value = state.values[i];
		knobs[i].minimumvalue = pots[i].minimumvalue;
		knobs[i].maximumvalue = pots[i].maximumvalue;
		knobs[i].defaultvalue = pots[i].defaultvalue;
		audioProcessor.apvts.addParameterListener(knobs[i].id,this);
	}

	multiplier = 1.f/Decibels::decibelsToGain(knobs[0].value);
	stereodamp = knobs[2].value;

	setOpaque(true);
	if((SystemStats::getOperatingSystemType() & SystemStats::OperatingSystemType::MacOSX != 0)
		context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
	context.setRenderer(this);
	context.attachTo(*this);

	setResizable(true,true);
	int h = audioProcessor.height.get();
	int w = round(h*((stereodamp>.5f?64.f:32.f)/19.f));
	audioProcessor.width = w;
#ifdef BANNER
	setSize(w,h+21);
	banneroffset = 21.f/(h+21);
#else
	setSize(w,h);
#endif
	setResizeLimits(3,2,3200,950);
	//getConstrainer()->setFixedAspectRatio((audioProcessor.stereo?64.f:32.f)/19.f);
	dpi = Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;

	startTimerHz(30);
}

VuAudioProcessorEditor::~VuAudioProcessorEditor() {
	for(int i = 0; i < knobcount; i++) audioProcessor.apvts.removeParameterListener(knobs[i].id,this);
	stopTimer();
	context.detach();
}

void VuAudioProcessorEditor::resized() {
	int w = getWidth();
#ifdef BANNER
	int h = getHeight()-21;
#else
	int h = getHeight();
#endif
	if(h != audioProcessor.height.get() && h != prevh) {
		prevw = w;
		w = round(h*((32.f+stereodamp*32.f)/19.f));
	} else if(w != audioProcessor.width.get() && w != prevw) {
		prevh = h;
		h = round(w*((19.f-stereodamp*9.5f)/32.f));
	} else return;

	audioProcessor.width = w;
	audioProcessor.height = h;

	/*
	audioProcessor.width = getWidth();
	audioProcessor.height = getHeight();
	displaycomp.setBounds(0,0,getWidth(),getHeight());
	*/
}

void VuAudioProcessorEditor::newOpenGLContextCreated() {
	audioProcessor.logger.init(&context,getWidth(),getHeight());

	vushader.reset(new OpenGLShaderProgram(context));
	if(!vushader->addVertexShader(
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
})"))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",vushader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	if(!vushader->addFragmentShader(
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
})"))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",vushader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	vushader->link();

	vutex.loadImage(ImageCache::getFromMemory(BinaryData::map_png, BinaryData::map_pngSize));
	vutex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	mptex.loadImage(ImageCache::getFromMemory(BinaryData::txtmap_png, BinaryData::txtmap_pngSize));
	mptex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	lgtex.loadImage(ImageCache::getFromMemory(BinaryData::genuine_soundware_png, BinaryData::genuine_soundware_pngSize));
	lgtex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

#ifdef BANNER
	bannershader.reset(new OpenGLShaderProgram(context));
	if(!bannershader->addVertexShader(
//BANNER VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 size;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*vec2(1,size.y))*2-1,0,1);
	uv = vec2(aPos.x*size.x,1-(1-aPos.y)*texscale.y);
})"))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",bannershader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	if(!bannershader->addFragmentShader(
//BANNER FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform vec2 texscale;
uniform float pos;
uniform float free;
uniform float dpi;
out vec4 fragColor;
void main(){
	vec2 col = max(min((texture(tex,vec2(mod(uv.x+pos,1)*texscale.x,uv.y)).rg-.5)*dpi+.5,1),0);
	fragColor = vec4(vec3(col.r*free+col.g*(1-free)),1);
})"))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",bannershader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at arihanan@proton.me. THANKS!","OK!");
	bannershader->link();

	bannertex.loadImage(ImageCache::getFromMemory(BinaryData::banner_png, BinaryData::banner_pngSize));
	bannertex.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#endif

	context.extensions.glGenBuffers(1, &arraybuffer);
}
void VuAudioProcessorEditor::renderOpenGL() {
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, arraybuffer);
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

	vushader->setUniform("banner", banneroffset);
	vushader->setUniform("rotation", (leftvu-.5f)*(1.47f-stereodamp*.025f));
	vushader->setUniform("peak", leftpeaklerp);
	vushader->setUniform("right", 0.f);
	vushader->setUniform("lgsize",
		((float)getWidth())/lgtex.getWidth(),
		((1-banneroffset)*getHeight())/lgtex.getHeight(),
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

	if (stereodamp > .001) {
		vushader->setUniform("rotation", (rightvu-.5f)*1.445f);
		vushader->setUniform("peak", rightpeaklerp);
		vushader->setUniform("right", 1.f);
		vushader->setUniform("lgsize",
			((float)getWidth())/lgtex.getWidth()*(1/(stereodamp+1)),
			((1-banneroffset)*getHeight())/lgtex.getHeight(),
			164.f/lgtex.getWidth()-(stereodamp/152)*(1-banneroffset)*getHeight()*1.56f,
			(62.f/lgtex.getHeight())*(1-settingsfade));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

#ifdef BANNER
	bannershader->use();
	coord = context.extensions.glGetAttribLocation(bannershader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	bannertex.bind();
	bannershader->setUniform("tex",0);
	bannershader->setUniform("dpi",dpi);
#ifdef BETA
	bannershader->setUniform("texscale",494.f/bannertex.getWidth(),21.f/bannertex.getHeight());
	bannershader->setUniform("size",getWidth()/494.f,21.f/getHeight());
	bannershader->setUniform("free",0.f);
#else
	bannershader->setUniform("texscale",426.f/bannertex.getWidth(),21.f/bannertex.getHeight());
	bannershader->setUniform("size",getWidth()/426.f,21.f/getHeight());
	bannershader->setUniform("free",1.f);
#endif
	bannershader->setUniform("pos",bannerx);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);
#endif

	audioProcessor.logger.drawlog();
}
void VuAudioProcessorEditor::openGLContextClosing() {
	vushader->release();

	vutex.release();
	mptex.release();
	lgtex.release();

#ifdef BANNER
	bannershader->release();
	bannertex.release();
#endif

	audioProcessor.logger.release();

	context.extensions.glDeleteBuffers(1, &arraybuffer);
}
void VuAudioProcessorEditor::paint(Graphics& g) {}

void VuAudioProcessorEditor::timerCallback() {
	int buffercount = audioProcessor.buffercount.get();
	if(buffercount > 0) {
		leftrms = sqrt(audioProcessor.leftvu.get()/buffercount);
		rightrms = sqrt(audioProcessor.rightvu.get()/buffercount);
		audioProcessor.leftvu = 0;
		audioProcessor.rightvu = 0;
		audioProcessor.buffercount = 0;

		if(audioProcessor.leftpeak.get()) {
			leftpeak = true;
			audioProcessor.leftpeak = false;
		}
		if(audioProcessor.rightpeak.get()) {
			rightpeak = true;
			audioProcessor.rightpeak = false;
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
	stereodamp = functions::smoothdamp(stereodamp,knobs[2].value,&stereovelocity,0.3,-1,.03333f);
	websiteht -= .05f;

	int h = audioProcessor.height.get();
	int w = 0;
	if (stereodamp > .001 && stereodamp < .999) {
		w = round(h*((32.f+stereodamp*32.f)/19.f));
		audioProcessor.width = w;
	} else w = audioProcessor.width.get();
#ifdef BANNER
	setSize(w,h+21);
	banneroffset = 21.f/(h+21);
	bannerx = fmod(bannerx+.0005f,1.f);
#else
	setSize(w,h);
#endif
	audioProcessor.logger.width = w;
	audioProcessor.logger.height = h;
	/*
	if (displaycomp.stereodamp > .001 && displaycomp.stereodamp < .999) {
		if(getConstrainer()->getFixedAspectRatio() != 0) getConstrainer()->setFixedAspectRatio(0);
		setSize(round(getHeight()*((32.f+displaycomp.stereodamp*32.f)/19.f)),getHeight());
		audioProcessor.width = getWidth();
		audioProcessor.height = getHeight();
	} else {
		double ratio = (knobs[2].value*32.f+32.f)/19.f;
		if(getConstrainer()->getFixedAspectRatio() != ratio)
			getConstrainer()->setFixedAspectRatio(ratio);
	}
	*/

	context.triggerRepaint();
}

void VuAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		if(i == 0) multiplier = 1.f/Decibels::decibelsToGain((float)newValue);
		knobs[i].value = newValue;
		return;
	}
}
void VuAudioProcessorEditor::mouseEnter(const MouseEvent& event) {
	settingstimer = 120;
}
void VuAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	settingstimer = fmax(settingstimer,60);
	int prevhover = hover;
	hover = recalchover(event.x,event.y);
	if(hover == 3 && prevhover != 3 && websiteht < -.6) websiteht = 1.0f;
}
void VuAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	if(!held) settingstimer = 0;
}
void VuAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	held = true;
	initialdrag = hover;
	if(hover == 0 || hover == 1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audioProcessor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		audioProcessor.undoManager.beginNewTransaction();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void VuAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(initialdrag == 2) hover = recalchover(event.x,event.y)==2?2:-1;
	else if(initialdrag == 3) hover = recalchover(event.x,event.y)==3?3:-1;
	else if(hover == 0 || hover == 1) {
		float dist = (event.getDistanceFromDragStartY()+event.getDistanceFromDragStartX())*-.04f;
		float val = dist+valueoffset;
		int clampval = initialvalue-(val>0?floor(val):ceil(val));
		audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].normalize(clampval));
		valueoffset = fmax(fmin(valueoffset,initialvalue-dist-knobs[hover].minimumvalue+1),initialvalue-dist-knobs[hover].maximumvalue-1);
	}
}
void VuAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(hover == 0 || hover == 1) {
		audioProcessor.undoManager.setCurrentTransactionName(
			(String)((knobs[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
		audioProcessor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audioProcessor.undoManager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else if(hover == 2) {
		bool stereo = knobs[2].value<.5;
		audioProcessor.apvts.getParameter("stereo")->setValueNotifyingHost(stereo?1:0);
		audioProcessor.undoManager.setCurrentTransactionName(stereo?"Set to stereo":"Set to mono");
		audioProcessor.undoManager.beginNewTransaction();
	} else if(hover == 3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
	held = false;
	hover = recalchover(event.x,event.y);
}
void VuAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover != 0 && hover != 1) return;
	audioProcessor.undoManager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].normalize(knobs[hover].defaultvalue));
	audioProcessor.undoManager.beginNewTransaction();
}
void VuAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if((hover != 0 && hover != 1) || wheel.isSmooth) return;
	audioProcessor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].normalize(knobs[hover].value+(wheel.deltaY>0?1:-1)));
}
int VuAudioProcessorEditor::recalchover(float x, float y) {
	float xx = (x/getWidth()-.5f)*8*3.8*(knobs[2].value+1);
	float yy = (y/((1-banneroffset)*getHeight())-.5f)*(7.4f);
	if(xx >= 1 && ((xx <= 8 && knobs[0].value <= -10) || xx <= 7) && yy > -1.5 && yy <= -.5)
		return 0;
	if(xx >= 1 && xx <= 4 && yy > -.5 && yy <= .5)
		return 1;
	if((yy > .5 && yy <= 1.5) && ((knobs[2].value<.5 && xx >= -8 && xx <= -2) || (knobs[2].value>.5 && xx >= -1 && xx <= 3)))
		return 2;
	if(x >= (getWidth()-151) && y >= ((1-banneroffset)*getHeight()-49) && x < (getWidth()-1) && y < ((1-banneroffset)*getHeight()-1))
		return 3;
	return -1;
}
