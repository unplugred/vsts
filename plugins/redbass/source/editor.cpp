#include "processor.h"
#include "editor.h"

RedBassAudioProcessorEditor::RedBassAudioProcessorEditor(RedBassAudioProcessor& p, int paramcount, pluginpreset state, pluginparams params) : audio_processor(p), AudioProcessorEditor(&p), plugmachine_gui(*this, p, 322, 408, 1.f, .5f) {
	for(int x = 0; x < 2; x++) {
		for(int y = 0; y < 4; y++) {
			int i = x+y*2;
			int ii = i<5?i:(i-1);
			knobs[x*4+y].x = x*248+37;
			knobs[x*4+y].y = y*100+46;
			if(i != 5) {
				knobs[i].id = params.pots[ii].id;
				knobs[i].name = params.pots[ii].name;
				knobs[i].value = params.pots[ii].normalize(state.values[ii]);
				knobs[i].minimumvalue = params.pots[ii].minimumvalue;
				knobs[i].maximumvalue = params.pots[ii].maximumvalue;
				knobs[i].defaultvalue = params.pots[ii].normalize(params.pots[ii].defaultvalue);
				add_listener(knobs[i].id);
			}
			knobcount++;
		}
	}
	knobs[5].id = "monitor";
	knobs[5].name = "Monitor Sidechain";
	knobs[5].value = params.monitorsmooth.getTargetValue() >.5;
	add_listener("monitor");

	knobs[0].description = "Frequency of the sub-oscillator";
	knobs[1].description = "Threshold for side-chain signal detection";
	knobs[2].description = "Attack time of envelope follower controlling the sub-oscillator";
	knobs[3].description = "Release time of envelope follower controlling the sub-oscillator";
	knobs[4].description = "Low-pass filter cutoff removing higher frequencies from the side-chain signal controlling the envelope follower";
	knobs[5].description = "Listen to side-chain signal controlling the envelope follower";
	knobs[6].description = "Gain of the original signal";
	knobs[7].description = "Gain of the sub-oscillator";

	for(int i = 0; i < knobcount; i++)
		knobs[i].description = look_n_feel.add_line_breaks(knobs[i].description);

	recalclabels();

	init(&look_n_feel);
}
RedBassAudioProcessorEditor::~RedBassAudioProcessorEditor() {
	close();
}

void RedBassAudioProcessorEditor::newOpenGLContextCreated() {
	feedbackshader = add_shader(
//FEEDBACK VERT
R"(#version 150 core
in vec2 aPos;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	uv = aPos*2.1+.003;
})",
//FEEDBACK FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D buffertex;
uniform float vis;
out vec4 fragColor;
void main(){
	if(uv.x > 1) fragColor = vec4(vis,vis,vis,1);
	else fragColor = texture(buffertex,uv);
})");

	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 texuv;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	texuv = aPos;
	uv = vec2(aPos.x,(aPos.y-0.0318627)*1.0652742);
})",
//BASE FRAG
R"(#version 150 core
in vec2 texuv;
in vec2 uv;
uniform sampler2D basetex;
uniform sampler2D buffertex;
uniform float hover;
uniform float vis;
out vec4 fragColor;
void main(){
	vec3 c = texture(basetex,texuv).rgb;
	if(c.r > .5) {
		if(uv.x > .2 && uv.x < .8) {
			if((hover > ((uv.x-.42236024844720496894409937888199)*4.5352112676056338028169014084507) && c.b > .5) || vis > uv.y) {
				fragColor = vec4(1,1,1,1);
			} else {
				fragColor = vec4(vec3((mod(uv.x*322,2)+mod(uv.y*383,2))>2.9?0.2:0.0),1);
			}
		} else {
			fragColor = vec4(vec3(min(.75,vis)),1);
		}
	} else if(c.g > .5) {
		fragColor = vec4(1,1,1,1);
	} else if(c.b > .5) {
		fragColor = vec4(0,0,0,1);
	} else {
		fragColor = texture(buffertex,vec2((uv.x>.5?(1-uv.x):(uv.x))*2,uv.y))*vec4(.5,.5,.5,1);
	}
})");

	knobshader = add_shader(
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform float ratio;
uniform float knobrot;
uniform vec2 knobpos;
uniform vec2 knobscale;
uniform float banner;
uniform float ext;
void main(){
	vec2 pos = (vec2(aPos.x,1-(1-aPos.y)*ext)*2*knobscale-vec2(knobscale.x,0))/vec2(ratio,1);
	gl_Position = vec4(
		(pos.x*cos(knobrot)-pos.y*sin(knobrot))*ratio-1+knobpos.x,
		(pos.x*sin(knobrot)+pos.y*cos(knobrot)-1+knobpos.y)*(1-banner)+banner,0,1);
})",
//KNOB FRAG
R"(#version 150 core
out vec4 fragColor;
void main(){
	fragColor = vec4(1);
})");

	toggleshader = add_shader(
//TOGGLE VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
uniform vec2 knobpos;
uniform vec2 knobscale;
out vec2 basecoord;
void main(){
	gl_Position = vec4(aPos*2*knobscale-knobscale-1+knobpos,0,1);
	basecoord = gl_Position.xy*.5+.5;
	gl_Position.y = gl_Position.y*(1-banner)+banner;
})",
//TOGGLE FRAG
R"(#version 150 core
in vec2 basecoord;
uniform sampler2D basetex;
uniform float toggle;
out vec4 fragColor;
void main(){
	vec2 c = texture(basetex,basecoord).rb;
	if(toggle > .5)
		c.r *= (1-c.g);
	else
		c.r *= c.g;
	fragColor = vec4(1,1,1,c.r);
})");

	textshader = add_shader(
//TEXT VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 size;
uniform vec2 pos;
uniform int letter;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x*size.x+pos.x,(aPos.y*size.y+pos.y)*(1-banner)+banner)*2-1,0,1);
	uv = vec2((aPos.x+letter)/21,aPos.y);
})",
//TEXT FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
out vec4 fragColor;
void main(){
	fragColor = vec4(1,1,1,texture(tex,uv).r);
})");

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize, GL_NEAREST, GL_NEAREST);
	add_texture(&texttex, BinaryData::txt_png, BinaryData::txt_pngSize, GL_NEAREST, GL_NEAREST);

	add_frame_buffer(&framebuffer, (int)(width*.5), 1, true, false, GL_NEAREST, GL_NEAREST);

	draw_init();
}
void RedBassAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	framebuffer.makeCurrentRenderingTarget();

	feedbackshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	feedbackshader->setUniform("buffertex",0);
	feedbackshader->setUniform("vis",vis);
	coord = context.extensions.glGetAttribLocation(feedbackshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);
	framebuffer.releaseAsRenderingTarget();

	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("basetex",0);
	context.extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
	baseshader->setUniform("buffertex",1);
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("vis",vis);
	baseshader->setUniform("hover",credits<0.5?4*credits*credits*credits:1-(float)pow(-2*credits+2,3)/2);
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	knobshader->use();
	knobshader->setUniform("banner",banner_offset);
	knobshader->setUniform("knobscale",2.f/width,27.f/height);
	knobshader->setUniform("ratio",((float)height)/width);
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < knobcount; i++) {
		if(i != 5) {
			knobshader->setUniform("ext",(i==6||i==7)?1.f:.5f);
			knobshader->setUniform("knobpos",((float)knobs[i].x*2)/width,2-((float)knobs[i].y*2)/height);
			knobshader->setUniform("knobrot",(float)fmod((knobs[i].value-.5f)*.748f,1)*-6.2831853072f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);

	toggleshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	toggleshader->setUniform("basetex",0);
	toggleshader->setUniform("banner",banner_offset);
	toggleshader->setUniform("knobscale",54.f/width,54.f/height);
	toggleshader->setUniform("knobpos",((float)knobs[5].x*2)/width,2-((float)knobs[5].y*2)/height);
	toggleshader->setUniform("toggle",knobs[5].value>.5?1.f:0.f);
	coord = context.extensions.glGetAttribLocation(toggleshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	textshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	texttex.bind();
	textshader->setUniform("tex",0);
	textshader->setUniform("banner",banner_offset);
	textshader->setUniform("size",8.f/width,16.f/height);
	coord = context.extensions.glGetAttribLocation(textshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	for(int i = 0; i < 5; ++i) {
		int len = knobs[i].interpolatedvalue.length();
		for(int t = 0; t < len; ++t) {
			int n = 0;
			for(n = 0; n < 21; ++n)
				if(textindex[n] == knobs[i].interpolatedvalue[t])
					break;
			textshader->setUniform("pos",((float)knobs[i].x+8*t-4*len)/width,1-((float)knobs[i].y+8)/height);
			textshader->setUniform("letter",n);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
	}
	context.extensions.glDisableVertexAttribArray(coord);


	draw_end();
}
void RedBassAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void RedBassAudioProcessorEditor::paint(Graphics& g) { }

void RedBassAudioProcessorEditor::timerCallback() {
	if(audio_processor.rmscount.get() > 0) {
		vis = audio_processor.rmsadd.get()/audio_processor.rmscount.get();
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else vis *= .9f;

	if(credits!=(hover<-1?1:0))
		credits = fmax(fmin(credits+(hover<-1?.1f:-.1f),1),0);

	update();
}

void RedBassAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	for(int i = 0; i < knobcount; i++) if(knobs[i].id == parameterID) {
		knobs[i].value = newValue;
		if(i <= 4) recalclabels();
		return;
	}
}
void RedBassAudioProcessorEditor::recalclabels() {
	knobs[0].interpolatedvalue = (String)round(audio_processor.calculatefrequency(knobs[0].value))+"Hz";

	int val = round(Decibels::gainToDecibels(fmax(.000001f,audio_processor.calculatethreshold(knobs[1].value))));
	if(val <= -96)
		knobs[1].interpolatedvalue = "Off";
	else
		knobs[1].interpolatedvalue = (String)val+"dB";

	knobs[2].interpolatedvalue = (String)round(audio_processor.calculateattack(knobs[2].value))+"ms";

	knobs[3].interpolatedvalue = (String)round(audio_processor.calculaterelease(knobs[3].value))+"ms";

	if(knobs[4].value >= 1) {
		knobs[4].interpolatedvalue = "Off";
	} else {
		val = audio_processor.calculatelowpass(knobs[4].value);
		if(val >= 10000)
			knobs[4].interpolatedvalue = (String)round(val*.001f)+"kHz";
		else
			knobs[4].interpolatedvalue = (String)round(val)+"Hz";
	}
}
void RedBassAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
}
void RedBassAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void RedBassAudioProcessorEditor::mouseDown(const MouseEvent& event) {
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

		String description = "";
		if(hover > -1)
			description = knobs[hover].description;
		else if(hover == -3)
			description = look_n_feel.add_line_breaks("Trust the process!");
		if(description != "") {
			rightclickmenu->addSeparator();
			rightclickmenu->addItem(-1,"'"+description,false);
		}

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

	initialdrag = hover;
	if(hover == 5) {
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.undo_manager.setCurrentTransactionName(
			((knobs[5].value > .5) ? "Turned Sidechain Monitoring off" : "Turned Sidechain Monitoring on"));
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(knobs[5].id)->setValueNotifyingHost(knobs[hover].value>.5?0:1);

	} else if(hover > -1) {
		initialvalue = knobs[hover].value;
		valueoffset = 0;
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(knobs[hover].id)->beginChangeGesture();
		audio_processor.lerpchanged[hover<5?hover:(hover-1)] = true;
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void RedBassAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(hover == -1) return;
	if(initialdrag > -1 && initialdrag != 5) {
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
	}
}
void RedBassAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(hover > -1 && hover != 5) {
		audio_processor.undo_manager.setCurrentTransactionName(
			(String)((knobs[hover].value - initialvalue) >= 0 ? "Increased " : "Decreased ") += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		//if(hover == -3) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
	}
}
void RedBassAudioProcessorEditor::mouseDoubleClick(const MouseEvent& event) {
	if(hover > -1 && hover != 5) {
		audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
		audio_processor.undo_manager.beginNewTransaction();
	}
}
void RedBassAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover > -1 && hover != 5)
		audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
			knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int RedBassAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if(x >= 134 && x <= 209 && y >= 144 && y <= 242) {
		if(x >= 135 && x <= 181 && y >= 195 && y <= 241)
			return -3;
		return -2;
	}
	float xx = 0, yy = 0;
	for(int i = 0; i < knobcount; i++) {
		xx = knobs[i].x-x;
		yy = knobs[i].y-y;
		if((xx*xx+yy*yy)<=841) return i;
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
