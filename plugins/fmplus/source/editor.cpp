#include "processor.h"
#include "editor.h"

FMPlusAudioProcessorEditor::FMPlusAudioProcessorEditor(FMPlusAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 300, 300, 2.f, .5f) {
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 3; y++) {
			int i = x+y*2;
			if(i >= paramcount) break;
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

	calcvis();

	setResizable(false,false);
	init(&look_n_feel);
}
FMPlusAudioProcessorEditor::~FMPlusAudioProcessorEditor() {
	close();
}

void FMPlusAudioProcessorEditor::newOpenGLContextCreated() {
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 coords;
uniform float banner;
uniform float tabid;
out vec2 uv;
out vec2 tabuv;
out vec2 headeruv;
void main(){
	gl_Position = vec4(vec2(coords.x,coords.y*(1-banner)+banner)*2-1,0,1);
	uv = coords;
	tabuv = (coords-vec2(0,.96-.036667*tabid))*vec2(10,21.42857);
	headeruv = (coords-vec2(.1,.043333))      *vec2(2.83019,10 );
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
in vec2 tabuv;
in vec2 headeruv;
uniform sampler2D basetex;
uniform sampler2D tabselecttex;
uniform sampler2D headertex;
uniform float dpi;
uniform float tabid;
uniform vec3 col_bg;
uniform vec3 col_bg_mod;
uniform vec3 col_conf;
uniform vec3 col_conf_mod;
uniform vec3 col_outline;
uniform vec3 col_vis;
uniform vec3 col_vis_mod;
uniform vec3 col_highlight;
out vec4 fragColor;
void main() {
	if(tabuv.x < 1 && tabuv.y > 0 && tabuv.y < 1) {
		vec3 col = max(min((texture(tabselecttex,tabuv).rgb-.5)*dpi+.5,1),0);
		fragColor = vec4(col,1);
		if(col.g > 0) {
			if(tabid < 3) col.b = 0;
			col = col_conf*(1-col.r-col.g)+col_outline*(col.r+col.b)+col_highlight*(col.g-col.b);
		} else if(tabid == 10 && tabuv.x < .95 && tabuv.y < .107143) {
			col = col_bg  *(1-col.r      )+col_outline* col.r                                   ;
		} else {
			col = col_conf*(1-col.r-col.b)+col_outline* col.r       +col_conf_mod *       col.b ;
		}
		fragColor = vec4(col,1);
	} else if(headeruv.x > 0 && headeruv.x < 1 && headeruv.y > 0 && headeruv.y < 1) {
		vec3 col = max(min((texture(headertex,vec2(headeruv.x,(headeruv.y+(10-tabid))/11)).rgb-.5)*dpi+.5,1),0);
		fragColor = vec4(col_conf*(1-col)+col_conf_mod*col,1);
	} else {
		vec3 col = max(min((texture(basetex,uv).rgb-.5)*dpi+.5,1),0);
		if(col.g > 0) {
		if(col.r > 0) {
			col = col_outline*(1-col.g)+col_vis *(col.g-col.b)+col_vis_mod *col.b;
		} else {
			col = col_outline*(1-col.g)+col_conf*(col.g-col.b)+col_conf_mod*col.b;
		}
		} else {
			col = col_outline*(1-col.r)+col_bg  *(col.r-col.b)+col_bg_mod  *col.b;
		}
		fragColor = vec4(col,1);
	}
})");

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize);
	add_texture(&tabselecttex, BinaryData::tabselect_png, BinaryData::tabselect_pngSize);
	add_texture(&headertex, BinaryData::header_png, BinaryData::header_pngSize);

	draw_init();
}
void FMPlusAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"coords");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	tabselecttex.bind();
	baseshader->setUniform("tabselecttex",1);
	context.extensions.glActiveTexture(GL_TEXTURE2);
	headertex.bind();
	baseshader->setUniform("headertex",2);
	baseshader->setUniform("tabid",(float)audio_processor.selectedtab.get());
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("dpi",(float)fmax(scaled_dpi*.5f,1));
	baseshader->setUniform("col_bg"				,col_bg[0]			,col_bg[1]			,col_bg[2]			);
	baseshader->setUniform("col_bg_mod"			,col_bg_mod[0]		,col_bg_mod[1]		,col_bg_mod[2]		);
	baseshader->setUniform("col_conf"			,col_conf[0]		,col_conf[1]		,col_conf[2]		);
	baseshader->setUniform("col_conf_mod"		,col_conf_mod[0]	,col_conf_mod[1]	,col_conf_mod[2]	);
	baseshader->setUniform("col_outline"		,col_outline[0]		,col_outline[1]		,col_outline[2]		);
	baseshader->setUniform("col_vis"			,col_vis[0]			,col_vis[1]			,col_vis[2]			);
	baseshader->setUniform("col_vis_mod"		,col_vis_mod[0]		,col_vis_mod[1]		,col_vis_mod[2]		);
	baseshader->setUniform("col_highlight"		,col_highlight[0]	,col_highlight[1]	,col_highlight[2]	);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void FMPlusAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void FMPlusAudioProcessorEditor::calcvis() {
	is_stereo = knobs[4].value > 0 && knobs[3].value < 1 && knobs[5].value > 0;
	pluginpreset pp;
	for(int i = 0; i < knobcount; i++)
		pp.values[i] = knobs[i].inflate(knobs[i].value);
	for(int c = 0; c < (is_stereo ? 2 : 1); c++) {
		for(int i = 0; i < 226; i++) {
			visline[c][i*2] = (i/112.5f)*(1-(16.f/width))+(16.f/width)-1;
			visline[c][i*2+1] = 1-(62+audio_processor.plasticfuneral(sin(i/35.8098621957f)*.8f,c,2,pp,audio_processor.normalizegain(pp.values[1],pp.values[3]))*38)/(height*.5f);
		}
	}
}
void FMPlusAudioProcessorEditor::paint(Graphics& g) { }

void FMPlusAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < knobcount; i++) if(knobs[i].hoverstate < -1) knobs[i].hoverstate++;
	if(held > 0) held--;

	if(websiteht > -1) websiteht -= .0815;

	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		if(knobs[5].value > .4f) rms = rms/knobs[5].value;
		else rms *= 2.5f;
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;

	time = fmod(time+.0002f,1.f);

	update();
}

void FMPlusAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = knobs[i].normalize(newValue);
		calcvis();
		return;
	}
}
void FMPlusAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == -3 && prevhover != -3 && websiteht <= -1) websiteht = .65f;
	else if(hover > -2 && prevhover <= -2) websiteht = -2;
	if(prevhover != hover && held == 0) {
		if(hover > -1) knobs[hover].hoverstate = -4;
		if(prevhover > -1) knobs[prevhover].hoverstate = -3;
	}
}
void FMPlusAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void FMPlusAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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
	if(hover == -1) return;
	if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		audio_processor.lerpchanged[hover] = true;
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	} else if(hover >= -12) {
		audio_processor.selectedtab = hover+12;
	}
}
void FMPlusAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
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
	} else if(initialdrag == -3) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y)==-3?-3:-2;
		if(hover == -3 && prevhover != -3 && websiteht < -1) websiteht = .65f;
	}
}
void FMPlusAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(hover > -1) {
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((knobs[hover].value-initialvalue)>=0?"Increased ":"Decreased ") += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
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
void FMPlusAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audio_processor.undo_manager.beginNewTransaction();
}
void FMPlusAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int FMPlusAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if(x <= 29) {
		int i = floor(y/11.f);
		if(i >= 11) return -1;
		return i-12;
	}

	for(int i = 0; i < knobcount; i++)
		if(fabs(knobs[i].x-x) <= 24 && fabs(knobs[i].y-y) <= 24) return i;

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
