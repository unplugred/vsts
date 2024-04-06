#include "PluginProcessor.h"
#include "PluginEditor.h"

MPaintAudioProcessorEditor::MPaintAudioProcessorEditor(MPaintAudioProcessor& p, unsigned char soundd) : audio_processor(p), plugmachine_gui(p, 234, 20, 2.f, 1.f, false)
{
	add_listener("sound");
	sound = soundd;
	error = audio_processor.error.get();
	loaded = error ? true : audio_processor.loaded.get();
	if(error) set_size(234,90);

	init(&look_n_feel);
}
MPaintAudioProcessorEditor::~MPaintAudioProcessorEditor() {
	close();
}

void MPaintAudioProcessorEditor::newOpenGLContextCreated() {
	shader = add_shader(
//VERT
R"(#version 150 core
in vec2 aPos;
uniform float texscale;
uniform vec4 pos;
uniform int sound;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos*2*pos.xy-1+pos.zw,0,1);
	uv = vec2((aPos.x+sound)*texscale,aPos.y);
})",
//FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform int errorhover;
uniform float dpi;
uniform vec2 res;
out vec4 fragColor;
void main(){
	vec2 nuv = uv*res;
	if(mod(nuv.x,1)>.5) nuv.x = floor(nuv.x)+(1-min((1-mod(nuv.x,1))*dpi,.5));
	else nuv.x = floor(nuv.x)+min(mod(nuv.x,1)*dpi,.5);
	if(mod(nuv.y,1)>.5) nuv.y = floor(nuv.y)+(1-min((1-mod(nuv.y,1))*dpi,.5));
	else nuv.y = floor(nuv.y)+min(mod(nuv.y,1)*dpi,.5);
	nuv /= res;
	fragColor = texture(tex,nuv);
	if((errorhover < .5 || uv.x < .8 || uv.y > .15) && res.y > 80)
		fragColor.r = fragColor.g;
})");

	add_texture(&errortex, BinaryData::error_png, BinaryData::error_pngSize);
	add_texture(&rulertex, BinaryData::ruler_png, BinaryData::ruler_pngSize);
	add_texture(&icontex, BinaryData::largeicon_png, BinaryData::largeicon_pngSize, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);

	draw_init();
}
void MPaintAudioProcessorEditor::renderOpenGL() {
	if(needtoupdate < 0) return;

	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	auto coord = context.extensions.glGetAttribLocation(shader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	shader->use();
	shader->setUniform("sound",0);
	shader->setUniform("dpi",(float)fmax(scaled_dpi,1));
	shader->setUniform("res",(float)width,(float)height);
	shader->setUniform("pos",1.f,1.f,0.f,0.f);
	shader->setUniform("texscale",1.f);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	if(error) {
		errortex.bind();
		shader->setUniform("tex",0);
		shader->setUniform("errorhover",errorhover?1:0);

	} else {
		rulertex.bind();
		shader->setUniform("tex",0);
		shader->setUniform("errorhover",0);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);

		context.extensions.glActiveTexture(GL_TEXTURE0);
		icontex.bind();
		shader->setUniform("tex",0);
		shader->setUniform("texscale",1.f/15.f);
		shader->setUniform("sound",sound);
		shader->setUniform("pos",16.f/width,14.f/height,8.f/width,4.f/height);
		shader->setUniform("res",240,14);
	}
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	context.extensions.glDisableVertexAttribArray(coord);
	needtoupdate--;

	draw_end();
}
void MPaintAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void MPaintAudioProcessorEditor::paint (Graphics& g) { }

void MPaintAudioProcessorEditor::timerCallback() {
	if(!loaded) {
		error = audio_processor.error.get();
		if(error) {
			loaded = true;
			set_size(234, 90);
			needtoupdate = 2;
		} else {
			loaded = audio_processor.loaded.get();
		}
	}

	update();
}

void MPaintAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID != "sound") return;
	sound = newValue;
	needtoupdate = 2;
	audio_processor.undo_manager.setCurrentTransactionName((String)("Changed sound to sound") += (String)newValue);
	audio_processor.undo_manager.beginNewTransaction();
}
void MPaintAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	if(!error) return;

	bool prevhover = errorhover;
	errorhover = ((event.x/ui_scales[ui_scale_index]) >= 192 && (event.y/ui_scales[ui_scale_index]) >= 81);
	if(prevhover != errorhover) needtoupdate = 2;
}
void MPaintAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	errorhover = false;
}
void MPaintAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(dpi < 0) return;
	if(event.mods.isRightButtonDown()) {
		std::unique_ptr<PopupMenu> rightclickmenu(new PopupMenu());
		std::unique_ptr<PopupMenu> scalemenu(new PopupMenu());
		std::unique_ptr<PopupMenu> limitmenu(new PopupMenu());

		int i = 20;
		while(++i < (ui_scales.size()+21))
			scalemenu->addItem(i,(String)round(ui_scales[i-21]*100)+"%",true,(i-21)==ui_scale_index);

		bool limit = audio_processor.limit.get();
		limitmenu->addItem(1,"On",true,limit);
		limitmenu->addItem(2,"Off",true,!limit);

		rightclickmenu->setLookAndFeel(&look_n_feel);
		rightclickmenu->addSubMenu("'Limit voices",*limitmenu);
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu("'Scale",*scalemenu);
		rightclickmenu->addSeparator();
		rightclickmenu->addItem(3,"'Website",true);
		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20) {
				set_ui_scale(result-21);
				needtoupdate = 2;
			} else if(result == 3) { //credits
				URL("https://vst.unplug.red/").launchInDefaultBrowser();
			} else if(result == 1 || result == 2) { //limit on
				audio_processor.undo_manager.setCurrentTransactionName((String)"Turned "+(String)(result==4?"on":"off")+" voice limiting");
				audio_processor.apvts.getParameter("limit")->setValueNotifyingHost(2-result);
				audio_processor.undo_manager.beginNewTransaction();
			}
		});
		return;
	}

	if(error) {
		if(errorhover) {
			error = false;
			audio_processor.error = false;
			set_size(234, 20);
			needtoupdate = 2;
		}
		return;
	}

	float x = event.x/ui_scales[ui_scale_index];
	float y = event.y/ui_scales[ui_scale_index];
	if(x < 24 || x > 233 || y < 5 || y > 19) return;
	if(fmod(x-24,14) == 13) return;

	audio_processor.apvts.getParameter("sound")->setValueNotifyingHost(floorf((x-24)/14)/14.f);
}

LookNFeel::LookNFeel() {
	setColour(PopupMenu::backgroundColourId,Colour::fromFloatRGBA(0.f,0.f,0.f,0.f));
	font = find_font("Tahoma|Helvetica Neue|Helvetica|Roboto");
}
LookNFeel::~LookNFeel() {
}
Font LookNFeel::getPopupMenuFont() {
	return Font(font,"Regular",14.f*scale);
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
		g.setColour(ht);
		g.fillRect(scale,0.f,area.getWidth()-2*scale,(float)area.getHeight());
		g.setColour(fg);
	} else
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
