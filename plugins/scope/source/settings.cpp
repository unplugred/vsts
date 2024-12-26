#include "processor.h"
#include "editor.h"
#include "settings.h"

ScopeAudioProcessorSettings::ScopeAudioProcessorSettings(ScopeAudioProcessor& p, int paramcount, knob* k) : audio_processor(p), knobs(k), DocumentWindow("Scope Settings", Colours::black, DocumentWindow::closeButton), plugmachine_gui(*this, p, 200, (16+PADDING*2+MARGIN*2)*(paramcount+1)+30, 1, 1, false, false) {

	knobcount = paramcount;
	for(int i = 0; i < knobcount; i++)
		add_listener(knobs[i].id);

	font.image = debug_font.image;

	setResizable(false,false);
	init();
	setUsingNativeTitleBar(true);
	setVisible(true);
	setAlwaysOnTop(true);
}
ScopeAudioProcessorSettings::~ScopeAudioProcessorSettings() {
	audio_processor.settingswindow = nullptr;
	close();
}
void ScopeAudioProcessorSettings::closeButtonPressed() {
	delete this;
}

void ScopeAudioProcessorSettings::newOpenGLContextCreated() {
	add_font(&font);

	baseshader = add_shader(
//BASE VERT
R"(#version 150 core
in vec2 coords;
uniform vec2 scale;
uniform float time;
out vec2 uv;
void main() {
	gl_Position = vec4(coords*2-1,0,1);
	uv = coords*scale+vec2(time,0);
})",
//BASE FRAG
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
out vec4 fragColor;
void main() {
	fragColor = texture(tex,uv);
})");

	knobshader = add_shader(
//KNOB VERT
R"(#version 150 core
in vec2 coords;
uniform vec2 knobscale;
uniform vec2 knobpos;
out vec2 uv;
void main() {
	gl_Position = vec4((coords*knobscale+knobpos)*2-1,0,1);
	uv = coords;
})",
//KNOB FRAG
R"(#version 150 core
in vec2 uv;
uniform float fill;
uniform float dpi;
uniform float ratio;
uniform float border;
out vec4 fragColor;
void main() {
	vec2 circ = uv;
	if(uv.x > .5) circ.x = 1-circ.x;
	circ = circ*vec2(ratio*2,2)-1;
	circ.x = min(circ.x,0);
	float x = max(0,min(1,.5+(1-sqrt(circ.x*circ.x+circ.y*circ.y))*dpi));
	x *= uv.x<fill?1:max(0,min(1,.5-(border-sqrt(circ.x*circ.x+circ.y*circ.y))*dpi));
	fragColor = vec4(1,1,1,x);
})");

	creditsshader = add_shader(
//CREDITS VERT
R"(#version 150 core
in vec2 coords;
uniform vec4 pos;
out vec2 uv;
out float xpos;
void main() {
	gl_Position = vec4(coords*pos.zw+pos.xy,0,1);
	uv = coords;
	xpos = coords.x;
})",
//CREDITS FRAG
R"(#version 150 core
in vec2 uv;
in float xpos;
uniform sampler2D creditstex;
uniform float shineprog;
out vec4 fragColor;
void main() {
	vec2 creditols = texture(creditstex,uv).rb;
	float shine = 0;
	if(xpos+shineprog < .65 && xpos+shineprog > 0)
		shine = texture(creditstex,uv+vec2(shineprog,0)).g;
	fragColor = vec4(vec3(1-shine*.3),creditols.r);
})");

	add_texture(&tiletex, BinaryData::tile_png, BinaryData::tile_pngSize, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);
	add_texture(&creditstex, BinaryData::credits_png, BinaryData::credits_pngSize, GL_NEAREST, GL_NEAREST);

	draw_init();
}
void ScopeAudioProcessorSettings::renderOpenGL() {
	draw_begin();

	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	OpenGLHelpers::clear(Colour::fromRGB(0,0,0));
	context.extensions.glBindBuffer(GL_ARRAY_BUFFER, array_buffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, square, GL_DYNAMIC_DRAW);

	baseshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	tiletex.bind();
	baseshader->setUniform("tex",0);
	baseshader->setUniform("scale",getWidth()/128.f,getHeight()/128.f);
	baseshader->setUniform("time",time);
	auto coord = context.extensions.glGetAttribLocation(baseshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	knobshader->use();
	coord = context.extensions.glGetAttribLocation(knobshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	knobshader->setUniform("border",1-2.f/((float)font.line_height+PADDING*2.f));
	knobshader->setUniform("dpi",dpi*((float)font.line_height+PADDING*2.f)*.5f);
	knobshader->setUniform("ratio",getWidth()/((float)font.line_height+PADDING*2.f));
	knobshader->setUniform("knobscale",(getWidth()-MARGIN*2.f)/getWidth(),((float)font.line_height+PADDING*2.f)/getHeight());
	int knoby = 0;
	for(int i = 0; i < knobcount; i++) {
		if(!knobs[i].visible) continue;
		knobshader->setUniform("knobpos",((float)MARGIN)/getWidth(),1-((knoby+1.f)*(font.line_height+PADDING*2+MARGIN*2)-MARGIN)/getHeight());
		knobshader->setUniform("fill",knobs[i].value);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		++knoby;
	}
	knobshader->setUniform("knobpos",((float)MARGIN)/getWidth(),1-((knoby+1.f)*(font.line_height+PADDING*2+MARGIN*2)-MARGIN)/getHeight());
	knobshader->setUniform("fill",initialdrag==-3&&hover==-3?1.f:0.f);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	font.width = getWidth();
	font.height = getHeight();
	knoby = 0;
	font.shader->use();
	font.shader->setUniform("padding",((float)MARGIN)/getWidth());
	for(int i = 0; i < knobcount; i++) {
		if(!knobs[i].visible) continue;
		font.shader->setUniform("fill",knobs[i].value);
		font.shader->setUniform("hover",hover==i?1.f:0.f);
		String value = "";
		if(knobs[i].isbool) value = knobs[i].value>.5?"On":"Off";
		else if(knobs[i].svalue != "") value = knobs[i].svalue;
		else value = (String)(round(knobs[i].value*100)/100.f);
		font.draw_string(1,1,1,1,1,1,1,0," "+knobs[i].name+": "+value,0,((float)PADDING+MARGIN)/getWidth(),(knoby+.5f)*(font.line_height+PADDING*2+MARGIN*2)/getHeight(),0,.5f);
		++knoby;
	}
	font.shader->setUniform("fill",initialdrag==-3&&hover==-3?1.f:0.f);
	font.shader->setUniform("hover",hover==-3?1.f:0.f);
	font.draw_string(1,1,1,1,1,1,1,0,"Save as default",0,.5f,(knoby+.5f)*(font.line_height+PADDING*2+MARGIN*2)/getHeight(),.5f,.5f);

	creditsshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	creditstex.bind();
	creditsshader->setUniform("creditstex",0);
	creditsshader->setUniform("shineprog",websiteht);
	creditsshader->setUniform("pos",(-148.f+fmod(getWidth(),2))/getWidth(),2/(getHeight()*.5f)-1,148/(getWidth()*.5f),46/(getHeight()*.5f));
	coord = context.extensions.glGetAttribLocation(creditsshader->getProgramID(),"coords");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	debug_font.width = getWidth();
	debug_font.height = getHeight();
	draw_end();
}
void ScopeAudioProcessorSettings::openGLContextClosing() {
	draw_close();
}
void ScopeAudioProcessorSettings::paint(Graphics& g) { }

void ScopeAudioProcessorSettings::timerCallback() {
	if(websiteht > -1) websiteht -= .0815;
	time = fmod(time+.001f,1);
	if(time > .03f) positioned = true;
	update();
}

void ScopeAudioProcessorSettings::parameterChanged(const String& parameterID, float newValue) {
}
void ScopeAudioProcessorSettings::mouseMove(const MouseEvent& event) {
	int prevhover = hover;
	hover = recalc_hover(event.x,event.y);
	if(hover == -2 && prevhover != -2 && websiteht <= -1) websiteht = .65f;
}
void ScopeAudioProcessorSettings::mouseExit(const MouseEvent& event) {
	hover = -1;
}
void ScopeAudioProcessorSettings::mouseDown(const MouseEvent& event) {
	if(dpi < 0) return;
	initialdrag = hover;
	if(hover > -1) {
		initialvalue = knobs[initialdrag].value;
		valueoffset = 0;
		audio_processor.undo_manager.beginNewTransaction();
		audio_processor.apvts.getParameter(knobs[initialdrag].id)->beginChangeGesture();
		if(knobs[initialdrag].isbool) {
			audio_processor.apvts.getParameter(knobs[initialdrag].id)->setValueNotifyingHost(1-initialvalue);
		} else {
			dragpos = event.getScreenPosition();
			event.source.enableUnboundedMouseMovement(true);
		}
	}
}
void ScopeAudioProcessorSettings::mouseDrag(const MouseEvent& event) {
	if(initialdrag > -1) {
		if(knobs[initialdrag].isbool) {
			hover = recalc_hover(event.x,event.y);
			hover = initialdrag==hover?initialdrag:-1;
			if(knobs[initialdrag].value != (initialdrag==hover?(1-initialvalue):initialvalue))
				audio_processor.apvts.getParameter(knobs[initialdrag].id)->setValueNotifyingHost(initialdrag==hover?(1-initialvalue):initialvalue);
		} else {
			if(!finemode && (event.mods.isShiftDown() || event.mods.isAltDown())) {
				finemode = true;
				initialvalue -= (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
			} else if(finemode && !(event.mods.isShiftDown() || event.mods.isAltDown())) {
				finemode = false;
				initialvalue += (event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*.0045f;
			}

			float value = initialvalue-(event.getDistanceFromDragStartY()-event.getDistanceFromDragStartX())*(finemode?.0005f:.005f);
			audio_processor.apvts.getParameter(knobs[initialdrag].id)->setValueNotifyingHost(value-valueoffset);

			valueoffset = fmax(fmin(valueoffset,value+.1f),value-1.1f);
		}
	} else if(initialdrag == -2) {
		int prevhover = hover;
		hover = recalc_hover(event.x,event.y)==-2?-2:-1;
		if(hover == -2 && prevhover != -2 && websiteht < -1) websiteht = .65f;
	} else if(initialdrag == -3) {
		hover = recalc_hover(event.x,event.y);
		hover = initialdrag==hover?initialdrag:-1;
	}
}
void ScopeAudioProcessorSettings::mouseUp(const MouseEvent& event) {
	if(dpi < 0) return;
	if(initialdrag > -1) {
		audio_processor.apvts.getParameter(knobs[initialdrag].id)->endChangeGesture();
		if(hover > -1) {
			audio_processor.undo_manager.setCurrentTransactionName(
				(String)((knobs[initialdrag].value-initialvalue)>=0?"Increased ":"Decreased ") += knobs[initialdrag].name);
			audio_processor.undo_manager.beginNewTransaction();
		}
		if(!knobs[initialdrag].isbool) {
			event.source.enableUnboundedMouseMovement(false);
			Desktop::setMousePosition(dragpos);
		}
	} else if(initialdrag == -3 && hover == -3) {
		for(int i = 0; i < knobcount; i++) {
			knobs[i].defaultvalue = knobs[i].value;
			audio_processor.props.getUserSettings()->setValue(knobs[i].id,knobs[i].inflate(knobs[i].value));
			audio_processor.params.pots[i].defaultvalue = knobs[i].inflate(knobs[i].value);
		}
	} else {
		hover = recalc_hover(event.x,event.y);
		if(hover == -2) {
			if(initialdrag == -2) URL("https://vst.unplug.red/").launchInDefaultBrowser();
			else if(websiteht < -1) websiteht = .65f;
		}
	}
	initialdrag = -1;
}
void ScopeAudioProcessorSettings::mouseDoubleClick(const MouseEvent& event) {
	if(hover <= -1) return;
	if(knobs[hover].isbool) return;
	audio_processor.undo_manager.setCurrentTransactionName((String)"Reset " += knobs[hover].name);
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(knobs[hover].defaultvalue);
	audio_processor.undo_manager.beginNewTransaction();
}
void ScopeAudioProcessorSettings::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
	if(hover <= -1) return;
	if(knobs[hover].isbool) return;
	audio_processor.apvts.getParameter(knobs[hover].id)->setValueNotifyingHost(
		knobs[hover].value+wheel.deltaY*((event.mods.isShiftDown() || event.mods.isAltDown())?.03f:.2f));
}
int ScopeAudioProcessorSettings::recalc_hover(float x, float y) {
	int knoby = 0;
	for(int i = 0; i < knobcount; i++) {
		if(!knobs[i].visible) continue;
		if(y < ((knoby+1)*(font.line_height+PADDING*2+MARGIN*2)-MARGIN)) return i;
		++knoby;
	}
	if(y < ((knoby+1)*(font.line_height+PADDING*2+MARGIN*2)-MARGIN)) return -3;
	if(x >= (getWidth()*.5-74) && x <= (getWidth()*.5+73) && y >= (getHeight()-48) && y <= (getHeight()-4))
		return -2;
	return -1;
}
void ScopeAudioProcessorSettings::moved() {
	DocumentWindow::moved();
	if(!positioned) return;
	Point<int> p = getScreenPosition();
	audio_processor.settingsx = p.x;
	audio_processor.settingsy = p.y;
}
