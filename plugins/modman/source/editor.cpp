#include "processor.h"
#include "editor.h"

ModManAudioProcessorEditor::ModManAudioProcessorEditor(ModManAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 30+106*3, 86+100*3) {
	for(int i = 0; i < (paramcount+1); ++i) {
		knobs[i].x = (i%3)*106+68;
		knobs[i].y = (i/3)*100+68;
		if(i == 0) {
			knobs[i].id = "selection";
			knobs[i].name = "Selection";
			knobs[i].value[0] = params.selectedmodulator/(MC-1.f);
			knobs[i].defaultvalue[0] = 0;
			knobs[i].minimumvalue = 1;
			knobs[i].maximumvalue = MC;
		} else {
			knobs[i].id = params.pots[i-1].id;
			knobs[i].name = params.pots[i-1].name;
			if(i < paramcount) {
				for(int m = 0; m < MC; ++m) {
					if((i-1) == 1 && (m == 0 || m == 5)) {
						knobs[i].value[m] = 0;
						knobs[i].defaultvalue[m] = 0;
						continue;
					}
					knobs[i].value[m] = params.pots[i-1].normalize(state.values[m][i-1]);
					knobs[i].defaultvalue[m] = params.pots[i-1].normalize(params.modulators[m].defaults[i-1]);
					add_listener("m"+((String)m)+knobs[i].id);
				}
			} else {
				knobs[i].value[0] = params.pots[i-1].normalize(state.masterspeed);
				knobs[i].defaultvalue[0] = params.pots[i-1].normalize(.5f);
				add_listener(knobs[i].id);
			}
			knobs[i].minimumvalue = params.pots[i-1].minimumvalue;
			knobs[i].maximumvalue = params.pots[i-1].maximumvalue;
		}
		knobcount++;
	}
	selectedmodulator = params.selectedmodulator;

	setResizable(false,false);
	init(&look_n_feel);
}
ModManAudioProcessorEditor::~ModManAudioProcessorEditor() {
	close();
}

void ModManAudioProcessorEditor::newOpenGLContextCreated() {
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
uniform sampler2D basetex;
uniform float texscale;
uniform float offset;
out vec4 fragColor;
void main(){
	fragColor = texture(basetex,vec2(mod(uv.x*texscale+offset,1),uv.y));
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
	gl_Position = vec4(aPos.x*knobscale.x+knobpos.x,(aPos.y*knobscale.y+knobpos.y)*(1-banner)+banner,0,1);
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
	fragColor=vec4(vec2((uv.y>0&&abs(uv.x)<.02)?1:0),hoverstate,1);
})");

	creditsshader = add_shader(
//CREDITS VERT
R"(#version 150 core
in vec2 aPos;
uniform vec4 pos;
uniform float banner;
out vec2 uv;
out float xpos;
void main(){
	gl_Position = vec4(aPos.x*pos.z+pos.x,(aPos.y*pos.w+pos.y)*(1-banner)+banner,0,1);
	uv = aPos;
	xpos = aPos.x;
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 uv;
in float xpos;
uniform sampler2D creditstex;
uniform float shineprog;
out vec4 fragColor;
void main(){
	vec2 creditols = texture(creditstex,uv).rb;
	float shine = 0;
	if(xpos+shineprog < .65 && xpos+shineprog > 0)
		shine = texture(creditstex,uv+vec2(shineprog,0)).g;
	fragColor = vec4(vec2(creditols.r),shine,creditols.g);
})");

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	add_texture(&creditstex, BinaryData::credits_png, BinaryData::credits_pngSize, GL_NEAREST, GL_NEAREST);

	draw_init();
}
void ModManAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("offset",time);
	baseshader->setUniform("texscale",242.f/basetex.getWidth());
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
	for(int i = 0; i < knobcount; i++) {
		if((i-1) == 1 && (selectedmodulator == 0 || selectedmodulator == 5)) continue;
		knobshader->setUniform("knobpos",((float)knobs[i].x*2-48.f)/width-1,1-((float)knobs[i].y*2+48.f)/height);
		knobshader->setUniform("knobrot",(knobs[i].value[(i==0||i==(knobcount-1))?0:selectedmodulator]-.5f)*5.5f);
		knobshader->setUniform("hoverstate",hover==i?1:0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
	context.extensions.glDisableVertexAttribArray(coord);

	debug_font.scale = ui_scales[ui_scale_index];
	for(int i = 0; i < knobcount; i++) {
		if((i-1) == 1 && (selectedmodulator == 0 || selectedmodulator == 5)) continue;
		String name = knobs[i].name;
		if(i == 0) {
			switch(selectedmodulator) {
				case 0: name = M1; break;
				case 1: name = M2; break;
				case 2: name = M3; break;
				case 3: name = M4; break;
				case 4: name = M5; break;
				case 5: name = M6; break;
				case 6: name = M7; break;
			}
		} else if((i-1) == 2 && (selectedmodulator == 0 || selectedmodulator == 5)) {
			name = "Range";
		}
		debug_font.draw_string(1,1,hover==i?1:0,1,0,0,hover==i?1:0,1,name,0,((float)knobs[i].x)/width,((float)knobs[i].y+45)/height,.5f,1);
	}

	if(creditsalpha >= 1) {
		creditsshader->use();
		context.extensions.glActiveTexture(GL_TEXTURE0);
		creditstex.bind();
		creditsshader->setUniform("banner",banner_offset);
		creditsshader->setUniform("creditstex",0);
		creditsshader->setUniform("shineprog",websiteht);
		creditsshader->setUniform("pos",((float)-148)/width,2/(height*.5f)-1,148/(width*.5f), 46/(height*.5f));
		coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}

	else if(creditsalpha <= 0)
		debug_font.draw_string(1,1,0,1,0,0,0,1,(String)"Temporary user interface!\nThe look of this VST is \nsubject to change.",0,0,1,0,1);

	debug_font.draw_string(1,1,0,1,0,0,0,1,audio_processor.getName()+(String)" (Prototype)",0,1,0,1,0);
	debug_font.scale = 1;

	draw_end();
}
void ModManAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void ModManAudioProcessorEditor::paint(Graphics& g) { }

void ModManAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) {
		if(knobs[i].hoverstate < -1) {
			knobs[i].hoverstate++;
		}
	}
	if(held > 0) held--;

	if(creditsalpha != ((hover<=-2&&hover>=-3)?1:0)) {
		creditsalpha = fmax(fmin(creditsalpha+((hover<=-2&&hover>=-3)?.17f:-.17f),1),0);
	}
	if(creditsalpha <= 0) websiteht = -1;
	if(websiteht > -1 && creditsalpha >= 1) {
		websiteht -= .0815;
	}

	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;

	time = fmod(time+.0002f,1.f);

	update();
}

void ModManAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == knobs[knobcount-1].id)
		knobs[knobcount-1].value[0] = knobs[knobcount-1].normalize(newValue);
	for(int m = 0; m < MC; ++m)
		for(int i = 1; i < knobcount; i++)
			if(parameterID == ("m"+((String)m)+knobs[i].id))
				knobs[i].value[m] = knobs[i].normalize(newValue);
}
void ModManAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht <= -1) websiteht = .65f;
	else if(hover > -2 && prevhover <= -2) websiteht = -2;
	if(prevhover != hover && held == 0) {
		if(hover > -1) knobs[hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
	}
}
void ModManAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void ModManAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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
		initialvalue = knobs[hover].value[(hover==0||hover==(knobcount-1))?0:selectedmodulator];
		valueoffset = 0;
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
		if(hover != 0) {
			audio_processor.undo_manager.beginNewTransaction();
			if(hover == (knobcount-1))
				audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
			else
				audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->beginChangeGesture();
		}
	}
}
void ModManAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
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
		if(hover == 0) {
			selectedmodulator = (MC-1)*fmin(1,fmax(0,value-valueoffset));
			audio_processor.params.selectedmodulator = selectedmodulator;
			knobs[0].value[0] = selectedmodulator/(MC-1.f);
		} else if(hover == (knobcount-1)) {
			audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(value-valueoffset);
		} else {
			audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(value-valueoffset);
		}

		valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
	} else if(initialdrag == -3) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y)==-3?-3:-2;
		if(hover == -3 && prevhover != -3 && websiteht < -1) websiteht = .65f;
	}
}
void ModManAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(hover > -1) {
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
		if(hover != 0) {
			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((knobs[hover].value[hover==(knobcount-1)?0:selectedmodulator]-initialvalue)>=0?"Increased ":"Decreased ") += knobs[hover].name);
			if(hover == (knobcount-1))
				audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
			else
				audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->endChangeGesture();
			audio_processor.undo_manager.beginNewTransaction();
		}
	} else {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
		if(hover == -3) {
			if(prevhover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht < -1) websiteht = .65f;
		}
		else if(hover > -1) knobs[hover].hoverstate = -4;
	}
	held = 1;
}
void ModManAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	if(hover == 0) {
		selectedmodulator = 0;
		audio_processor.params.selectedmodulator = 0;
		knobs[0].value[0] = 0;
	} else if(hover == (knobcount-1)) {
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue[0]);
	} else {
		audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue[selectedmodulator]);
	}
	audio_processor.undo_manager.beginNewTransaction();
}
void ModManAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	if(hover == 0) {
		selectedmodulator = fmin(MC-1,fmax(0,selectedmodulator+(wheel.deltaY>0?1:-1)));
		audio_processor.params.selectedmodulator = selectedmodulator;
		knobs[0].value[0] = selectedmodulator/(MC-1.f);
	} else if(hover == (knobcount-1)) {
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value[0]+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	} else {
		audio_processor.apvts.getParameter("m"+((String)selectedmodulator)+knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value[selectedmodulator]+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
	}
}
int ModManAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if(y >= (height-48) && y < height) {
		if(x >= (width*.5-74) && x <= (width*.5+73) && y <= (height-4)) return -3;
		return -2;
	}
	for(int i = 0; i < knobcount; i++) {
		if(fabs(knobs[i].x-x) <= 24 && fabs(knobs[i].y-y) <= 24) return i;
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
