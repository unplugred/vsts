#include "PluginProcessor.h"
#include "PluginEditor.h"

PNCHAudioProcessorEditor::PNCHAudioProcessorEditor (PNCHAudioProcessor& p, float amountt) : audio_processor(p), plugmachine_gui(p, 128, 148, 1.f, 1.f)
{
	amount = amountt;
	add_listener("amount");

	float hue = random.nextFloat();
	bool lightness = random.nextFloat() >= .5;
	c1 = Colour::fromHSV(hue,1.f,lightness?1.f:.4f,1.f);
	c2 = Colour::fromHSV(hue+.1666f*(random.nextFloat()>.5?1:-1),1.f,lightness?.4f:1.f,1.f);
	look_n_feel.c1 = c1;
	look_n_feel.c2 = c2;

	init(&look_n_feel);
}
PNCHAudioProcessorEditor::~PNCHAudioProcessorEditor() {
	close();
}

void PNCHAudioProcessorEditor::newOpenGLContextCreated() {
	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 letterscale;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1,0,1);
	uv = vec2((aPos.x-.0078125)*letterscale.x,1-(1-aPos.y)*letterscale.y);
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform float amount;
uniform vec2 texscale;
uniform vec3 colone;
uniform vec3 coltwo;
out vec4 fragColor;
void main(){
	float id = 0;
	if(uv.x < 0 || uv.x > 5) id = 6;
	else {
		if(uv.y < 0 || uv.x > 1) id = max(floor(uv.x)-floor(uv.y)*5-amount+1,1);
		if(amount >= 1 && id == 5) id = 6;
	}
	if(id >= 6) fragColor = vec4(coltwo,1);
	else {
		float o = texture(tex,vec2(mod(uv.x,1)*texscale.x,1-(1-(mod(uv.y,1)-id))*texscale.y)).r;
		fragColor = vec4(o*colone+(1-o)*coltwo,1);
	}
})");

	knobshader = add_shader(
//KNOB VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 shake;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(vec2(aPos.x,aPos.y*(1-banner)+banner)*2-1+shake,0,1);
	uv = aPos;
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform vec3 colone;
uniform vec3 coltwo;
uniform vec2 knobrot;
out vec4 fragColor;
void main(){
	vec2 c = texture(tex,uv).gb;
	vec2 ball = texture(tex,uv-knobrot).gb;
	float o = (c.r==c.g?1:0)+(ball.r>.5&&ball.g<.5?1:0);
	fragColor = vec4(o*colone+(1-o)*coltwo,((c.r+c.g)>.5)?1:0);
})");

	creditsshader = add_shader(
//CREDITS VERT
R"(#version 150 core
in vec2 aPos;
uniform float banner;
out vec2 uv;
void main(){
	gl_Position = vec4(aPos.x*1.875-0.9375,(aPos.y*(1-banner)+banner*1.03)*1.891892-0.945946,0,1);
	uv = aPos;
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D creditstex;
uniform float hover;
uniform vec3 colone;
uniform vec3 coltwo;
out vec4 fragColor;
void main(){
	vec3 c = texture(creditstex,uv).rgb;
	float o = (hover==1?c.r:0.0)+c.g+(hover==2?c.b:0.0);
	fragColor = vec4(o*colone+(1-o)*coltwo,1);
})");

	add_texture(&basetex, BinaryData::base_png, BinaryData::base_pngSize, GL_NEAREST, GL_NEAREST);
	add_texture(&creditstex, BinaryData::credits_png, BinaryData::credits_pngSize, GL_NEAREST, GL_NEAREST);

	draw_init();
}
void PNCHAudioProcessorEditor::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	baseshader->use();
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	context.extensions.glActiveTexture(GL_TEXTURE0);
	basetex.bind();
	baseshader->setUniform("tex",0);
	baseshader->setUniform("texscale",25.f/basetex.getWidth(),21.f/basetex.getHeight());
	baseshader->setUniform("banner",banner_offset);
	baseshader->setUniform("letterscale",128/25.f,148/21.f);
	baseshader->setUniform("amount",(float)floor(amount*31));
	baseshader->setUniform("colone",c1.getFloatRed(),c1.getFloatGreen(),c1.getFloatBlue());
	baseshader->setUniform("coltwo",c2.getFloatRed(),c2.getFloatGreen(),c2.getFloatBlue());
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	if(credits){
		creditsshader->use();
		coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"aPos");
		context.extensions.glActiveTexture(GL_TEXTURE0);
		creditstex.bind();
		creditsshader->setUniform("creditstex",0);
		creditsshader->setUniform("banner",banner_offset);
		creditsshader->setUniform("hover",(float)hover);
		creditsshader->setUniform("colone",c1.getFloatRed(),c1.getFloatGreen(),c1.getFloatBlue());
		creditsshader->setUniform("coltwo",c2.getFloatRed(),c2.getFloatGreen(),c2.getFloatBlue());
	} else {
		knobshader->use();
		coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"aPos");
		context.extensions.glActiveTexture(GL_TEXTURE0);
		basetex.bind();
		knobshader->setUniform("tex",0);
		knobshader->setUniform("banner",banner_offset);
		float rotato = (amount-.5f)*5.f;
		knobshader->setUniform("knobrot",round(sin(rotato)*38)/128.f,round(cos(rotato)*38)/148.f);
		knobshader->setUniform("shake",.03125f*(random.nextFloat()-.5f)*10.f*rms,.027027f*(1-banner_offset)*(random.nextFloat()-.5f)*10.f*rms);
		knobshader->setUniform("colone",c1.getFloatRed(),c1.getFloatGreen(),c1.getFloatBlue());
		knobshader->setUniform("coltwo",c2.getFloatRed(),c2.getFloatGreen(),c2.getFloatBlue());
	}
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	draw_end();
}
void PNCHAudioProcessorEditor::openGLContextClosing() {
	draw_close();
}
void PNCHAudioProcessorEditor::paint (Graphics& g) { }

void PNCHAudioProcessorEditor::timerCallback() {
	if(audio_processor.rmscount.get() > 0) {
		rms = sqrt(audio_processor.rmsadd.get()/audio_processor.rmscount.get());
		audio_processor.rmsadd = 0;
		audio_processor.rmscount = 0;
	} else rms *= .9f;

	update();
}

void PNCHAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if(parameterID == "amount") amount = newValue;
}
void PNCHAudioProcessorEditor::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
}
void PNCHAudioProcessorEditor::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void PNCHAudioProcessorEditor::mouseDown(const MouseEvent& event) {
	if(dpi < 0) return;
	if(event.mods.isRightButtonDown()) {
		hover = recalc_hover(event.x,event.y);
		std::unique_ptr<PopupMenu> rightclickmenu(new PopupMenu());
		std::unique_ptr<PopupMenu> scalemenu(new PopupMenu());
		std::unique_ptr<PopupMenu> oversamplingmenu(new PopupMenu());

		int i = 20;
		while(++i < (ui_scales.size()+21))
			scalemenu->addItem(i,(String)round(ui_scales[i-21]*100)+"%",true,(i-21)==ui_scale_index);

		bool oversampling = audio_processor.oversampling.get();
		oversamplingmenu->addItem(4,"On",true,oversampling);
		oversamplingmenu->addItem(5,"Off",true,!oversampling);

		rightclickmenu->setLookAndFeel(&look_n_feel);
		rightclickmenu->addItem(1,"'Copy preset",true);
		rightclickmenu->addItem(2,"'Paste preset",audio_processor.is_valid_preset_string(SystemClipboard::getTextFromClipboard()));
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu("'Over-Sampling",*oversamplingmenu);
		rightclickmenu->addSeparator();
		rightclickmenu->addSubMenu("'Scale",*scalemenu);
		rightclickmenu->addSeparator();
		rightclickmenu->addItem(3,"'Credits",true);
		rightclickmenu->showMenuAsync(PopupMenu::Options(),[this](int result){
			if(result <= 0) return;
			else if(result >= 20) {
				ui_scale_index = result-21;
				audio_processor.set_ui_scale(ui_scales[ui_scale_index]);
				reset_size = true;
			} else if(result == 1) { //copy preset
				SystemClipboard::copyTextToClipboard(audio_processor.get_preset(0));
			} else if(result == 2) { //paste preset
				audio_processor.set_preset(SystemClipboard::getTextFromClipboard(), 0);
			} else if(result == 3) { //credits
				credits = true;
			} else if(result == 4 || result == 5) { //oversampling on
				audio_processor.undo_manager.setCurrentTransactionName((String)"Turned "+(String)(result==4?"on":"off")+" oversampling");
				audio_processor.apvts.getParameter("oversampling")->setValueNotifyingHost(5-result);
				audio_processor.undo_manager.beginNewTransaction();
			}
		});
		return;
	}

	held = -1;
	initialdrag = hover;
	if(hover == 0) {
		initialvalue = amount;
		valueoffset = 0;
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter("amount")->beginChangeGesture();
		dragpos = event.getScreenPosition();
		event.source.enableUnboundedMouseMovement(true);
	}
}
void PNCHAudioProcessorEditor::mouseDrag(const MouseEvent& event) {
	if(event.mods.isRightButtonDown()) return;
	if(hover == -1) return;
	if(initialdrag == 0) {
		if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = true;
			initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
			finemode = false;
			initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
		}

		float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
		audio_processor.apvts.getParameter("amount")->setValueNotifyingHost(value - valueoffset);

		valueoffset = fmax(fmin(valueoffset,value+.1),value-1.1);
	}
}
void PNCHAudioProcessorEditor::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(event.mods.isRightButtonDown()) return;
	if(hover == 0) {
		audio_processor.undo_manager.setCurrentTransactionName((amount - initialvalue) >= 0 ? "Increased amount" : "Decreased amount");
		audio_processor.apvts.getParameter("amount")->endChangeGesture();
		audio_processor.undo_manager.beginNewTransaction();
		event.source.enableUnboundedMouseMovement(false);
		Desktop::setMousePosition(dragpos);
	} else {
		if(hover == 1) URL("https://vst.unplug.red/").launchInDefaultBrowser();
		else if(hover == 2) credits = false;
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y);
	}
	held = 1;
}
void PNCHAudioProcessorEditor::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover == 0)
		audio_processor.apvts.getParameter("amount")->setValueNotifyingHost(
			amount+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int PNCHAudioProcessorEditor::recalc_hover(float x, float y) {
	if(dpi < 0) return -1;
	x /= ui_scales[ui_scale_index];
	y /= ui_scales[ui_scale_index];

	if (credits) {
		if(x >= 69 && x <= 97 && y >= 93 && y <= 117) return 1;
		if(x >= 8 && x <= 59 && y >= 112 && y <= 137) return 2;
	} else {
		float xx = 60-x;
		float yy = 88-y;
		if(xx*xx+yy*yy<=2601) return 0;
	}
	return -1;
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
	g.setColour(c1);
	g.fillRect(0,0,width,height);
	g.setColour(c2);
	g.fillRect(scale,scale,width-2*scale,height-2*scale);
}
void LookNFeel::drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String &text, const String &shortcutKeyText, const Drawable *icon, const Colour *textColour) {
	if(isSeparator) {
		g.setColour(c1);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		return;
	}

	bool removeleft = text.startsWith("'");
	if(isHighlighted && isActive) {
		g.setColour(c1);
		g.fillRect(0,0,area.getWidth(),area.getHeight());
		g.setColour(c2);
	} else
		g.setColour(c1);
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
