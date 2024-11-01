#include "processor.h"
#include "editor.h"

DietAudioAudioProcessorEditor::DietAudioAudioProcessorEditor(DietAudioAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 24*LETTER_W, 24*LETTER_H, 2.f, 1.f) {
	for(int i = 0; i < paramcount; ++i) {
		knobs[i].id = params.pots[i].id;
		knobs[i].name = params.pots[i].name;
		knobs[i].value = params.pots[i].normalize(state.values[i]);
		knobs[i].minimumvalue = params.pots[i].minimumvalue;
		knobs[i].maximumvalue = params.pots[i].maximumvalue;
		knobs[i].defaultvalue = params.pots[i].normalize(params.pots[i].defaultvalue);
		++knobcount;
		add_listener(knobs[i].id);
		recalcknob(i);
	}

	String prebaked = (String)
"  DIET AUDIO -----------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"--Threshold---Release---"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"------------------------"+
"--Transients--The rest--"+
"------------------------"+
"------------------------"+
"- https://vst.unplug.red";
	for(int i = 0; i < 576; ++i) {
		if(prebaked[i] == '-') {
			bodypixel[i][0] = 0;
			bodypixel[i][1] = 0;
		} else {
			bodypixel[i][0] = prebaked[i];
			bodypixel[i][1] = (i < 12 || i >= 554) ? 1 : 2;
		}
	}
	bodypixel[0][0] = 14;
	bodypixel[12][0] = 16;
	bodypixel[553][0] = 17;

	clouds.init();
	calcvis();

	for(int i = 0; i < 576; ++i) {
		bodybuffer[i*24   ] =        (i%24  )/24.f;
		bodybuffer[i*24+ 1] = 1-floor(i/24  )/24.f;

		bodybuffer[i*24+ 4] =        (i%24+1)/24.f;
		bodybuffer[i*24+ 5] = 1-floor(i/24  )/24.f;

		bodybuffer[i*24+ 8] =        (i%24  )/24.f;
		bodybuffer[i*24+ 9] = 1-floor(i/24+1)/24.f;

		bodybuffer[i*24+12] =        (i%24+1)/24.f;
		bodybuffer[i*24+13] = 1-floor(i/24  )/24.f;

		bodybuffer[i*24+16] =        (i%24  )/24.f;
		bodybuffer[i*24+17] = 1-floor(i/24+1)/24.f;

		bodybuffer[i*24+20] =        (i%24+1)/24.f;
		bodybuffer[i*24+21] = 1-floor(i/24+1)/24.f;
	}

	init(&look_n_feel);
}
DietAudioAudioProcessorEditor::~DietAudioAudioProcessorEditor() {
	close();
}

void DietAudioAudioProcessorEditor::newOpenGLContextCreated() {
	textshader = add_shader(
//TEXT VERT
R"(#version 150 core
in vec4 pos;
uniform float banner;
out vec2 uv;
out float color;
void main(){
	gl_Position = vec4(vec2(pos.x,pos.y*(1-banner))*2-1,0,1);
	uv = vec2(mod(pos.z,2),pos.w);
	color = floor(pos.z/2);
})",
//TEXT FRAG
R"(#version 150 core
in vec2 uv;
in float color;
uniform sampler2D texttex;
out vec4 fragColor;
void main(){
	if(color == 3) {
		fragColor = vec4(0);
	} else {
		fragColor = texture(texttex,uv);
		if(color == 1)
			fragColor.rgb = 1-fragColor.rgb;
		fragColor.rgb *= color == 0 ? vec3(0,.15,0) : vec3(1,.3,0);
	}
})");

	feedbackshader = add_shader(
//FEEDBACK VERT
R"(#version 150 core
in vec2 pos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(pos.x,pos.y*(1-banner))*2-1,0,1);
	uv = pos;
})",
//FEEDBACK FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D feedbacktex;
uniform sampler2D maintex;
out vec4 fragColor;
vec3 hueshift(vec3 color, float shift) {
	vec3 P = vec3(.55735)*dot(vec3(.55735),color);
	vec3 U = color-P;
	vec3 V = cross(vec3(.55735),U);
	color = U*cos(shift*6.2832)+V*sin(shift*6.2832)+P;
	return color;
}
void main(){
	vec3 main = texture(maintex,uv).rgb;
	vec3 feedback = texture(feedbacktex,uv).rgb;
	if((main.r+main.g+main.b) >= (feedback.r+feedback.g+feedback.b)) {
		fragColor = vec4(main,1);
	} else {
		float speed = feedback.r>feedback.g?.16:.03;
		fragColor = vec4(hueshift(feedback,speed*-.5)-speed,1);
	}
})");

	buffershader = add_shader(
//BUFFER VERT
R"(#version 150 core
in vec2 pos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(pos.x,pos.y*(1-banner)+banner)*2-1,0,1);
	uv = pos;
})",
//BUFFER FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D feedbacktex;
out vec4 fragColor;
void main(){
	fragColor = texture(feedbacktex,uv);
})");


	add_texture(&texttex, BinaryData::text_png, BinaryData::text_pngSize, GL_NEAREST, GL_NEAREST);

	add_frame_buffer(&feedbackbuffer, width, height);
	add_frame_buffer(&mainbuffer, width, height);

	draw_init();
}
void DietAudioAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mainbuffer.makeCurrentRenderingTarget();

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(textshader->getProgramID(),"pos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(576+20)*24, bodybuffer, GL_DYNAMIC_DRAW);
	textshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	texttex.bind();
	textshader->setUniform("texttex",0);
	textshader->setUniform("banner",banner_offset);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,4,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLES,0,(576+20)*6);
	context.extensions.glDisableVertexAttribArray(coord);

	mainbuffer.releaseAsRenderingTarget();
	feedbackbuffer.makeCurrentRenderingTarget();

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	feedbackshader->use();
	coord = context.extensions.glGetAttribLocation(feedbackshader->getProgramID(),"pos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, feedbackbuffer.getTextureID());
	feedbackshader->setUniform("feedbacktex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mainbuffer.getTextureID());
	feedbackshader->setUniform("maintex",1);
	feedbackshader->setUniform("banner",banner_offset);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	feedbackbuffer.releaseAsRenderingTarget();

	buffershader->use();
	coord = context.extensions.glGetAttribLocation(buffershader->getProgramID(),"pos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, feedbackbuffer.getTextureID());
	buffershader->setUniform("feedbacktex",0);
	buffershader->setUniform("banner",banner_offset);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void DietAudioAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void DietAudioAudioProcessorEditor::calcvis() {
	if(bypass) {
		int randindex = floor(random.nextFloat()*576);
		if(bodypixel[randindex][1] == 0)
			bodypixel[randindex][0] = random.nextFloat()>.75f?' ':(floor(random.nextFloat()*95)+32);
	} else {
		for(int y = 0; y < 24; ++y) {
			for(int x = 0; x < 24; ++x) {
				int i = y*24+x;
				if(bodypixel[i][1] != 0) continue;
				/*/
				bodypixel[i][0] = bright_map[(int)round(fmin(255,fmax(0,getvis(x/24.f,y/24.f)*255)))];
				/*/
				unsigned char currentbit = 1;
				unsigned char outbyte = 0;
				for(int b = 0; b < 8; ++b) {
					if((getvis((x+(b%4)*.25f)/24.f,(y+(b%2)*.25f+floor(b*.25f)*.5f)/24.f)+blue_noise[i*8+b]/256.f) > 1)
						outbyte += currentbit;
					currentbit *= 2;
				}
				bodypixel[i][0] = subpixel_map[outbyte];
				//*/
			}
		}
	}

	for(int i = 0; i < 576; ++i) {
		bodybuffer[i*24+ 2] =        (bodypixel[i][0]%16  )/16.f+bodypixel[i][1]*2;
		bodybuffer[i*24+ 3] = 1-floor(bodypixel[i][0]/16  )/16.f;
		bodybuffer[i*24+ 6] =        (bodypixel[i][0]%16+1)/16.f+bodypixel[i][1]*2;
		bodybuffer[i*24+ 7] = 1-floor(bodypixel[i][0]/16  )/16.f;
		bodybuffer[i*24+10] =        (bodypixel[i][0]%16  )/16.f+bodypixel[i][1]*2;
		bodybuffer[i*24+11] = 1-floor(bodypixel[i][0]/16+1)/16.f;
		bodybuffer[i*24+14] =        (bodypixel[i][0]%16+1)/16.f+bodypixel[i][1]*2;
		bodybuffer[i*24+15] = 1-floor(bodypixel[i][0]/16  )/16.f;
		bodybuffer[i*24+18] =        (bodypixel[i][0]%16  )/16.f+bodypixel[i][1]*2;
		bodybuffer[i*24+19] = 1-floor(bodypixel[i][0]/16+1)/16.f;
		bodybuffer[i*24+22] =        (bodypixel[i][0]%16+1)/16.f+bodypixel[i][1]*2;
		bodybuffer[i*24+23] = 1-floor(bodypixel[i][0]/16+1)/16.f;
	}
}
float DietAudioAudioProcessorEditor::getvis(float x, float y) {
	float scale1 = .15f;
	float scale2 = .15f;
	float screwamount = 5.f;
	return (clouds.noise(x*.2f+clouds.noise(-x*scale2,-y*scale2,time2)*screwamount,y*.2f+clouds.noise(-y*scale2,-x*scale2,time2)*screwamount,time1)+(knobs[0].value-.5f)*2.5f)*1+.5;
}
void DietAudioAudioProcessorEditor::paint(Graphics& g) { }

void DietAudioAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; ++i)
		if(knobs[i].hoverstate < -1)
			++knobs[i].hoverstate;
	if(held > 0) held--;

	if(audio_processor.rmscount.get() > 0) {
		float rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
		bypass = false;
		time1 += .0008f;
		time2 += .00008f+rms*.05f;
	} else bypass = true;

	if(++subframe > 1) {
		subframe = 0;
		calcvis();
	}

	update();
}

void DietAudioAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; ++i) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		recalcknob(i);
	}
}
void DietAudioAudioProcessorEditor::recalcknob(int index) {
	String label = "";
	if(index == 0) {
		float val = fmax(-96,Decibels::gainToDecibels(fmax(.000001f,pow(knobs[index].value,2)*10000))-58.4);
		if(val >= 10)
			label = String(floor(val))+"dB";
		else if(val > 0)
			label = String(val,1)+"dB";
		else if(val > -.1)
			label = "0dB";
		else if(val > -1)
			label = "-."+String(floor(fabs(val*10)))+"dB";
		else
			label = String(ceil(val))+"dB";
	} else if(index == 1) {
		float val = pow(knobs[index].value*38.7298334621,2);
		if(val >= 1000)
			label = String(val*.001f,1)+"s";
		else
			label = String(round(val))+"ms";
	} else {
		float val = Decibels::gainToDecibels(fmax(.000001f,knobs[index].value));
		if(val <= -96)
			label = "Off";
		else if(val > -.1)
			label = "0dB";
		else if(val > -1)
			label = "-."+String(floor(fabs(val*10)))+"dB";
		else
			label = String(ceil(val))+"dB";
	}
	if(knobs[index].value > .5) {
		String reversed = "";
		for(int i = label.length(); i > 0; --i)
			reversed += label[i-1];
		label = reversed;
	}

	float x = 6+11*     (index%2);
	float y = 6+10*floor(index/2);
	float xd = sin((.5f-knobs[index].value)*5.8f);
	float yd = cos((.5f-knobs[index].value)*5.8f);
	float normalize = 1.f/fmax(fabs(xd),fabs(yd));
	xd *= normalize;
	yd *= normalize;

	for(int i = 0; i < 5; ++i) {
		float xl = x+xd*(i-2);
		float yl = y+yd*(i-2);
		int cl = (i==0?1:3)*2;
		char cc = ' ';
		if(i < label.length()) {
			cl = fmin(cl,4);
			cc = label[i];
		}
		bodybuffer[(576+index*5+i)*24   ] =   (xl  )/24.f;
		bodybuffer[(576+index*5+i)*24+ 1] = 1-(yl  )/24.f;
		bodybuffer[(576+index*5+i)*24+ 2] =        (cc%16  )/16.f+cl;
		bodybuffer[(576+index*5+i)*24+ 3] = 1-floor(cc/16  )/16.f;
		bodybuffer[(576+index*5+i)*24+ 4] =   (xl+1)/24.f;
		bodybuffer[(576+index*5+i)*24+ 5] = 1-(yl  )/24.f;
		bodybuffer[(576+index*5+i)*24+ 6] =        (cc%16+1)/16.f+cl;
		bodybuffer[(576+index*5+i)*24+ 7] = 1-floor(cc/16  )/16.f;
		bodybuffer[(576+index*5+i)*24+ 8] =   (xl  )/24.f;
		bodybuffer[(576+index*5+i)*24+ 9] = 1-(yl+1)/24.f;
		bodybuffer[(576+index*5+i)*24+10] =        (cc%16  )/16.f+cl;
		bodybuffer[(576+index*5+i)*24+11] = 1-floor(cc/16+1)/16.f;
		bodybuffer[(576+index*5+i)*24+12] =   (xl+1)/24.f;
		bodybuffer[(576+index*5+i)*24+13] = 1-(yl  )/24.f;
		bodybuffer[(576+index*5+i)*24+14] =        (cc%16+1)/16.f+cl;
		bodybuffer[(576+index*5+i)*24+15] = 1-floor(cc/16  )/16.f;
		bodybuffer[(576+index*5+i)*24+16] =   (xl  )/24.f;
		bodybuffer[(576+index*5+i)*24+17] = 1-(yl+1)/24.f;
		bodybuffer[(576+index*5+i)*24+18] =        (cc%16  )/16.f+cl;
		bodybuffer[(576+index*5+i)*24+19] = 1-floor(cc/16+1)/16.f;
		bodybuffer[(576+index*5+i)*24+20] =   (xl+1)/24.f;
		bodybuffer[(576+index*5+i)*24+21] = 1-(yl+1)/24.f;
		bodybuffer[(576+index*5+i)*24+22] =        (cc%16+1)/16.f+cl;
		bodybuffer[(576+index*5+i)*24+23] = 1-floor(cc/16+1)/16.f;
	}
}
void DietAudioAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(prevhover != hover && held == 0) {
		if(hover > -1) knobs[hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
	}
}
void DietAudioAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void DietAudioAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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
	}
}
void DietAudioAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
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

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	}
}
void DietAudioAudioProcessorEditor::mouseUp(const MouseEvent& event) {
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
		if(hover > -1) knobs[hover].hoverstate = -4;
	}
	held = 1;
}
void DietAudioAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audio_processor.undo_manager.beginNewTransaction();
}
void DietAudioAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int DietAudioAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= LETTER_W*ui_scales[ui_scale_index];
	y /= LETTER_H*ui_scales[ui_scale_index];

	for(int i = 0; i < knobcount; ++i)
		if(fabs((i%2)*11+6.5f-x) <= 2.5f && fabs(floor(i/2)*10+6.5f-y) <= 2.5f)
			return i;
	return -1;
}

LookNFeel::LookNFeel() {
	setColour(PopupMenu::backgroundColourId,Colour::fromFloatRGBA(0.f,0.f,0.f,0.f));
	font = find_font("Consolas|Noto Mono|DejaVu Sans Mono|Menlo|Andale Mono|SF Mono|Lucida Console|Liberation Mono");
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	return Font(FontOptions(font,"Bold",12.f*scale));
}
void LookNFeel::drawPopupMenuBackground(Graphics &g, int width, int height) {
	g.setColour(fg);
	g.fillRect(0,0,width,height);
	g.setColour(bg);
	g.fillRect(scale,scale,width-2*scale,height-2*scale);
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
