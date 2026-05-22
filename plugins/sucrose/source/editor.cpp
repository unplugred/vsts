#include "processor.h"
#include "editor.h"

SucroseAudioProcessorEditor::SucroseAudioProcessorEditor(SucroseAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 356, 382) {
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
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 2; y++) {
			int i = x+y*2;
			knobs[i].x = x*170+95;
			knobs[i].y = y*100+120;
		}
	}
	for(int i = 0; i < 3; i++) {
		knobs[i+4].x = i*110+70;
		knobs[i+4].y = 2*100+120;
	}

	for(int i = 0; i < 2; ++i) {
		logopos[i*3  ] = random.nextFloat()*(i==0?.2f:.28f)+(i==0?.16f:.4f);
		logopos[i*3+1] = random.nextFloat()*.09f;
		logopos[i*3+2] = (random.nextFloat()+.2f)*(i==0?.08f:.06f)*(random.nextFloat()>.5f?1:-1);
		offset[i] = .003f+random.nextFloat()*.003f;
	}

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
out vec2 uv;
out vec4 webuv;
void main() {
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner))*2-1,0,1);
	uv = aPos;
	webuv = vec4(
		 (uv.x-rota.x)*cos(rota.z)-(uv.y-rota.y)*.9319371728*sin(rota.z)             +rota.x,
		((uv.x-rota.x)*sin(rota.z)+(uv.y-rota.y)*.9319371728*cos(rota.z))/.9319371728+rota.y,
		 (uv.x-rotb.x)*cos(rotb.z)-(uv.y-rotb.y)*.9319371728*sin(rotb.z)             +rotb.x,
		((uv.x-rotb.x)*sin(rotb.z)+(uv.y-rotb.y)*.9319371728*cos(rotb.z))/.9319371728+rotb.y);
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
in vec4 webuv;
uniform sampler2D fgtex;
uniform float algo;
out vec4 fragColor;
void main() {
	fragColor = texture(fgtex,uv);
	if(uv.y > .09 && uv.y < .24) {
		if(uv.x > .7) {
			if(algo > 1.5) {
				fragColor.r += fragColor.g;
			} else if(algo > 0.5) {
				fragColor.r += texture(fgtex,uv-vec2(.5,0)).g;
			} else {
				fragColor.r += fragColor.b;
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

	knobshader = add_shader(
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform float knobrot;
uniform vec2 knobscale;
uniform vec2 knobpos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos.x*knobscale.x+knobpos.x,(aPos.y*knobscale.y+knobpos.y)*(1-banner)-banner,0,1);
	uv = vec2(
		(aPos.x-.5)*cos(knobrot)-(aPos.y-.5)*sin(knobrot),
		(aPos.x-.5)*sin(knobrot)+(aPos.y-.5)*cos(knobrot));
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
uniform int hoverstate;
out vec4 fragColor;
void main(){
	fragColor = vec4(1,0,hoverstate,(uv.y>0&&abs(uv.x)<.02)?1:0);
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
uniform float algo;
out vec4 fragColor;
void main(){
	vec4 col = vec4(0);

	vec3 fgcol = texture(buffertex,uv).rgb;
	if(uv.y > .73) {
		if(fgcol.g > .00001 && fgcol.b > .00001) {
			col += vec4(.5,.7890625,.5078125,1)*fgcol.g;
		} else if(uv.x > .4) {
			if(uv.x > .6) {
				col += vec4(.97265625,.48828125,.5078125,1)*fgcol.g;
			} else {
				col += vec4(.984375,.52734375,.7578125,1)*fgcol.g;
			}
			col += vec4(.47265625,.57421875,.84375,1)*fgcol.b;
		} else {
			col += vec4(.97265625,.48828125,.5078125,1)*fgcol.g;
			col += vec4(.9921875,.8046875,.28515625,1)*fgcol.b;
		}
	} else if(uv.y < .09) {
		if(fgcol.g > .00001 && fgcol.b > .00001) {
			col += vec4(.2421875,.375,.7734375,1)*fgcol.g;
		} else {
			col += vec4(.95703125,.34765625,.53515625,1)*fgcol.g;
			col += vec4(.22265625,.62109375,.52734375,1)*fgcol.b;
		}
	}
	col += vec4(.23828125,.1953125,.22265625,1)*fgcol.r;

	vec3 bgcol = texture(bgtex,uv).rgb;
	if(algo > 1.5) bgcol.r = bgcol.b; else
	if(algo < 0.5) bgcol.r = bgcol.g;
	vec3 fgoffset = texture(buffertex,uv+misalignment).rgb;
	bgcol.r *= 1-min(1,fgcol   .r+fgcol   .g+fgcol   .b);
	bgcol.r *= 1-min(1,fgoffset.r+fgoffset.g+fgoffset.b);
	col += vec4(.8671875,.8984375,.99609375,1)*bgcol.r*(1-col.a);
	// TODO bg offset

	// TODO abberation;

	fragColor = vec4(col.rgb+vec3(.984375)*(1-col.a),1);
	//fragColor = texture(buffertex,uv);
})");

	add_texture(&bgtex,BinaryData::bg_png,BinaryData::bg_pngSize);
	add_texture(&fgtex,BinaryData::fg_png,BinaryData::fg_pngSize);

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
	baseshader->setUniform("algo",knobs[6].value*3.f);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("rota",logopos[0],logopos[1],logopos[2]*(1-powf(1-websiteht[0],2)));
	baseshader->setUniform("rotb",logopos[3],logopos[4],logopos[5]*(1-powf(1-websiteht[1],2)));
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	knobshader->use();
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	knobshader->setUniform("knobscale",48.f*2.f/width,48.f*2.f/height);
	knobshader->setUniform("banner",banner_offset);
	for(int i = 0; i < 6; i++) {
		knobshader->setUniform("knobpos",((float)knobs[i].x*2-48.f)/width-1,1-((float)knobs[i].y*2+48.f)/height);
		knobshader->setUniform("knobrot",(knobs[i].value-.5f)*5.5f);
		knobshader->setUniform("hoverstate",hover==i?1:0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	frame_buffer.releaseAsRenderingTarget();

	ppshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_buffer.getTextureID());
	ppshader->setUniform("buffertex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	bgtex.bind();
	ppshader->setUniform("bgtex",1);
	ppshader->setUniform("misalignment",offset[0],offset[1]);
	ppshader->setUniform("chromatic",.3f/(getWidth()*dpi));
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
void SucroseAudioProcessorEditor::calcvis() {
}
void SucroseAudioProcessorEditor::paint(Graphics& g) { }

void SucroseAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++)
		if(knobs[i].hoverstate < -1)
			knobs[i].hoverstate++;
	if(held > 0) held--;

	for(int i = 0; i < 2; ++i) {
		bool prev = websiteht[i]<.00001f;
		websiteht[i] = fmin(1,fmax(0,websiteht[i]+((-2-hover)==i?.3f:-.3f)));
		if(prev && websiteht[i] > .00001f) {
			logopos[i*3  ] = random.nextFloat()*(i==0?.2f:.28f)+(i==0?.16f:.4f);
			logopos[i*3+1] = random.nextFloat()*.09f;
			logopos[i*3+2] = (random.nextFloat()+.2f)*(i==0?.08f:.06f)*(random.nextFloat()>.5f?1:-1);
		}
	}

	time = fmod(time+.0002f,1.f);

	update();
}

void SucroseAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		if(parameterID == "algo") {
			offset[0] = .003f+random.nextFloat()*.003f;
			offset[1] = .003f+random.nextFloat()*.003f;
		}
		knobs[i].value = knobs[i].normalize(newValue);
		calcvis();
		return;
	}
}
void SucroseAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(prevhover != hover && held == 0) {
		if(    hover > -1) knobs[    hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
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
		audio_processor.lerpchanged[hover] = true;
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
	if(dpi < 0) return;
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
		if(hover > -1) knobs[hover].hoverstate = -4;
		else if(hover == -3 && prevhover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		else if(hover == -2 && prevhover == -2) URL("https://fx.amee.ee/").launchInDefaultBrowser();
	}
	held = 1;
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
