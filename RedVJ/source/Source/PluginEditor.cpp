/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

RedVJAudioProcessorEditor* RedVJAudioProcessorEditor::singleton = nullptr;
RedVJAudioProcessorEditor::RedVJAudioProcessorEditor (RedVJAudioProcessor& p)
	: AudioProcessorEditor (&p), audioProcessor (p)
{
	singleton = this;

	statelist.push_back(state());
	statelist[0].statename = "Initial state";
	getchannelstate(0)->on = true;
	getchannel(0)->shadererror = "Welcome to RedVJ Beta.\nPress CTRL+O to load up a preset\n\nCreated by unplugred\nhttps://vst.unplug.red/";
	if(audioProcessor.presetpath != "NULL") {
		File fil = audioProcessor.presetpath;
		if(fil.existsAsFile()) {
			String ext = fil.getFileExtension();
			if(ext == ".rvjs") loadshader(fil,selectedchannel);
		}
	}

	//audioProcessor.apvts.addParameterListener("bleh",this);
	camimg = ImageCache::getFromMemory(BinaryData::txt_png,BinaryData::txt_pngSize);
	setSize (800, 600);
	setResizeLimits(4,4,10000,10000);
	context.setRenderer(this);
	context.attachTo(*this);
	cam->addListener(this);
	addKeyListener(this);
	setWantsKeyboardFocus(true);
	startTimerHz(30);
	Array<MidiDeviceInfo> b = MidiInput::getAvailableDevices();
	for(int i = 0; i < b.size(); i++) {
		devicemanager.setMidiInputDeviceEnabled(b[i].identifier,true);
		devicemanager.addMidiInputDeviceCallback(b[i].identifier,this);
	}
}
RedVJAudioProcessorEditor::~RedVJAudioProcessorEditor() {
	//audioProcessor.apvts.removeParameterListener("bleh",this);
	removeKeyListener(this);
	cam->removeListener(this);
	devicemanager.removeAllChangeListeners();
	stopTimer();
	context.detach();
	audioProcessor.dofft = false;
}
void RedVJAudioProcessorEditor::openGLContextClosing() {
	for(int i = 0; i < 9; i++) {
		getchannel(i)->shader->release();
		getchannel(i)->tex.release();
	}
	quadshader->release();
	textshader->release();

	camtex.release();
	texttex.release();

	context.extensions.glDeleteBuffers(1,&arraybuffer);
}

void RedVJAudioProcessorEditor::newOpenGLContextCreated() {
	defaultvert =
R"(#version 330 core
in vec2 aPos;
out vec2 pos;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	pos = aPos;
})";
	defaultfrag =
R"(#version 330 core
in vec2 pos;
void main(){
	float g = (1-pos.y);
	gl_FragColor = vec4(0,0,g*.4,1);
})";
	blackvert =
R"(#version 330 core
in vec2 aPos;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
})";
	blackfrag =
R"(#version 330 core
void main(){
	gl_FragColor = vec4(0,0,0,1);
})";
	getchannel(0)->shader.reset(new OpenGLShaderProgram(context));
	getchannel(0)->shader->addVertexShader(defaultvert);
	getchannel(0)->shader->addFragmentShader(defaultfrag);
	getchannel(0)->shader->link();
	for (int i = 1; i < 9; i++) {
		getchannel(i)->shader.reset(new OpenGLShaderProgram(context));
		getchannel(i)->shader->addVertexShader(blackvert);
		getchannel(i)->shader->addFragmentShader(blackfrag);
		getchannel(i)->shader->link();
	}

	quadvert =
R"(#version 330 core
in vec2 aPos;
out vec2 texcoord;
void main(){
	gl_Position = vec4(aPos*2-1,0,1);
	texcoord = aPos;
})";
	quadfrag =
R"(#version 330 core
in vec2 texcoord;
uniform sampler2D tex;
void main(){
	gl_FragColor = texture2D(tex,texcoord);
})";
	quadshader.reset(new OpenGLShaderProgram(context));
	quadshader->addVertexShader(quadvert);
	quadshader->addFragmentShader(quadfrag);
	quadshader->link();

	textvert = 
R"(#version 330 core
in vec2 aPos;
uniform vec4 size;
uniform float letter;
out vec2 texcoord;
void main(){
	gl_Position = vec4((aPos*2-1+size.xy)*size.zw,0,1);
	texcoord = (aPos+vec2(mod(letter,16),floor((letter+1)*-.0625)))*.0625;
})";
	textfrag =
R"(#version 330 core
in vec2 texcoord;
uniform sampler2D tex;
void main(){
	gl_FragColor = vec4(1)*texture2D(tex,texcoord).r;
})";
	textshader.reset(new OpenGLShaderProgram(context));
	textshader->addVertexShader(textvert);
	textshader->addFragmentShader(textfrag);
	textshader->link();

	texttex.loadImage(ImageCache::getFromMemory(BinaryData::txt_png,BinaryData::txt_pngSize));
	texttex.bind();
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

	for(int i = 0; i < 9; i++) {
		getchannel(i)->tex.initialise(context,getWidth(),getHeight());
		glBindTexture(GL_TEXTURE_2D,getchannel(i)->tex.getTextureID());
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	}

	context.extensions.glGenBuffers(1,&arraybuffer);

}
void RedVJAudioProcessorEditor::renderOpenGL() {
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	//free textures
	for (int i = 0; i < todelete.size(); i++) {
		todelete[i]->tex.release();
		todelete[i]->freed = true;
	}
	todelete.clear();

	auto coord = 0;
	context.extensions.glBindBuffer(GL_ARRAY_BUFFER,arraybuffer);
	context.extensions.glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,square,GL_DYNAMIC_DRAW);
	for(int i = 0; i < 9; i++) {
		if(getchannelstate(i)->on) {
			// reset the shader
			if(getchannel(i)->resetshader.get()) {
				getchannel(i)->shader->release();
				getchannel(i)->shader.reset(new OpenGLShaderProgram(context));
				bool success = false;
				if (!getchannel(i)->shader->addVertexShader(getchannelstate(i)->vert)) getchannel(i)->shadererror = "VERTEX ";
				else if(!getchannel(i)->shader->addFragmentShader(getchannelstate(i)->frag)) getchannel(i)->shadererror = "FRAGMENT ";
				else if(!getchannel(i)->shader->link()) getchannel(i)->shadererror = "LINKING ";
				else success = true;

				if(!success) {
					getchannel(i)->shadererror += getchannel(i)->shader->getLastError();
					getchannel(i)->shader->release();
					getchannel(i)->shader.reset(new OpenGLShaderProgram(context));
					getchannel(i)->shader->addVertexShader(defaultvert);
					getchannel(i)->shader->addFragmentShader(defaultfrag);
					getchannel(i)->shader->link();
				} else getchannel(i)->shadererror = "";

				getchannel(i)->resetshader = false;
			}

			//resize the texture
			if(getchannel(i)->texw != getWidth() || getchannel(i)->texh != getHeight()) {
				getchannel(i)->tex.release();
				getchannel(i)->tex.initialise(context,getWidth(),getHeight());
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
				getchannel(i)->texw = getWidth();
				getchannel(i)->texh = getHeight();
			}

			//????
			if(!drawncam.get()) {
				camtex.release();
				camtex.loadImage(camimg);
				camtex.bind();
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
			}

			//draw the channel
			getchannel(i)->tex.makeCurrentRenderingTarget();
			glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
			getchannel(i)->shader->use();
			int texid = 0;
			for (int p = 0; p < getchannelstate(i)->luaexports.size(); p++) {
				if(getchannelstate(i)->luaexports[p].type == "camera") {
					context.extensions.glActiveTexture(GL_TEXTURE0+texid);
					camtex.bind();
					getchannel(i)->shader->setUniform(
						getchannelstate(i)->luaexports[p].name.toRawUTF8(),
						texid);
					getchannel(i)->shader->setUniform(
						(getchannelstate(i)->luaexports[p].name+"_size").toRawUTF8(),
						((float)camw.get())/camtex.getWidth(),
						((float)camh.get())/camtex.getHeight());
					texid++;
				} else if(getchannelstate(i)->luaexports[p].type == "channel") {
					context.extensions.glActiveTexture(GL_TEXTURE0+texid);
					int tc = getchannelstate(i)->luaexports[p].channel;
					if (tc <= 0) tc += i;
					else tc--;
					while(tc < 0) tc += 9;
					while(tc >= 9) tc -= 9;
					glBindTexture(GL_TEXTURE_2D,getchannel(tc)->tex.getTextureID());
					getchannel(i)->shader->setUniform(
						getchannelstate(i)->luaexports[p].name.toRawUTF8(),
						texid);
					texid++;
				} else if(getchannelstate(i)->luaexports[p].type == "video") {
					texid++;
				} else if(getchannelstate(i)->luaexports[p].type == "image") {
					if (!images[getchannelstate(i)->luaexports[p].id]->loaded) {
						if(getchannelstate(i)->luaexports[p].path.existsAsFile()) {
							images[getchannelstate(i)->luaexports[p].id]->tex.loadImage(ImageCache::getFromFile(getchannelstate(i)->luaexports[p].path));
							images[getchannelstate(i)->luaexports[p].id]->tex.bind();
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
							images[getchannelstate(i)->luaexports[p].id]->loaded = true;
						} else images[getchannelstate(i)->luaexports[p].id]->exists = false;
					}
					if(images[getchannelstate(i)->luaexports[p].id]->exists) {
						context.extensions.glActiveTexture(GL_TEXTURE0+texid);
						images[getchannelstate(i)->luaexports[p].id]->tex.bind();
						getchannel(i)->shader->setUniform(
							getchannelstate(i)->luaexports[p].name.toRawUTF8(),
							texid);
						texid++;
					}
				} else if(getchannelstate(i)->luaexports[p].type == "float") {
					getchannel(i)->shader->setUniform(
						getchannelstate(i)->luaexports[p].name.toRawUTF8(),
						getchannelstate(i)->luaexports[p].x);
				} else if(getchannelstate(i)->luaexports[p].type == "vec2") {
					getchannel(i)->shader->setUniform(
						getchannelstate(i)->luaexports[p].name.toRawUTF8(),
						getchannelstate(i)->luaexports[p].x,
						getchannelstate(i)->luaexports[p].y);
				} else if(getchannelstate(i)->luaexports[p].type == "vec3") {
					getchannel(i)->shader->setUniform(
						getchannelstate(i)->luaexports[p].name.toRawUTF8(),
						getchannelstate(i)->luaexports[p].x,
						getchannelstate(i)->luaexports[p].y,
						getchannelstate(i)->luaexports[p].z);
				} else if(getchannelstate(i)->luaexports[p].type == "vec4") {
					getchannel(i)->shader->setUniform(
						getchannelstate(i)->luaexports[p].name.toRawUTF8(),
						getchannelstate(i)->luaexports[p].x,
						getchannelstate(i)->luaexports[p].y,
						getchannelstate(i)->luaexports[p].z,
						getchannelstate(i)->luaexports[p].w);
				}
			}
			coord = context.extensions.glGetAttribLocation(getchannel(i)->shader->getProgramID(),"aPos");
			context.extensions.glEnableVertexAttribArray(coord);
			context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			context.extensions.glDisableVertexAttribArray(coord);


			//draw error text
			if(getchannel(i)->shadererror != "") {
				textshader->use();
				context.extensions.glActiveTexture(GL_TEXTURE0);
				texttex.bind();
				textshader->setUniform("tex",0);
				coord = context.extensions.glGetAttribLocation(quadshader->getProgramID(),"aPos");
				context.extensions.glEnableVertexAttribArray(coord);
				context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);

				int i = 0; float c = 0, l = 0;
				const char* utf = getchannel(i)->shadererror.toUTF8();

				std::queue<int> linelength;
				while(utf[i] != '\0')
					if(utf[i++] == '\n' || (((++c)+2)*8 > getWidth() && utf[i] != '\n'))
						{ linelength.push(c-1); l+=.5f; c = 0; }
				linelength.push(c-1);

				i = 0; c = 0;
				int ll = linelength.front(); linelength.pop();
				float letterw = 8.f/getWidth();
				float letterh = 16.f/getHeight();
				while(utf[i] != '\0') {
					if(utf[i] == '\n') {
						l--; c = 0;
						ll = linelength.front(); linelength.pop();
					} else {
						textshader->setUniform("size",c*2-ll,l*2,letterw,letterh);
						textshader->setUniform("letter",(float)((int)utf[i]));
						glDrawArrays(GL_TRIANGLE_STRIP,0,4);
						c++;
						if((c+2)*8 > getWidth() && utf[i+1] != '\n') {
							l--; c = 0;
							ll = linelength.front(); linelength.pop();
						}
					}
					i++;
				}

				context.extensions.glDisableVertexAttribArray(coord);
			}

			getchannel(i)->tex.releaseAsRenderingTarget();
		}
	}

	quadshader->use();
	context.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,getchannel(outchannel)->tex.getTextureID());
	quadshader->setUniform("tex",0);
	coord = context.extensions.glGetAttribLocation(quadshader->getProgramID(),"aPos");
	context.extensions.glEnableVertexAttribArray(coord);
	context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	context.extensions.glDisableVertexAttribArray(coord);

	drawncam = true;
}
void RedVJAudioProcessorEditor::paint (Graphics& g) {}
void RedVJAudioProcessorEditor::resized() {}

void RedVJAudioProcessorEditor::imageReceived(const Image& image) {
	if(!drawncam.get()) return;
	camimg = image;
	camw = image.getWidth();
	camh = image.getHeight();
	drawncam = false;

}
void RedVJAudioProcessorEditor::timerCallback() {
	for(int i = 0; i < 9; i++) {
		if(getchannelstate(i)->on) {
			if (getchannelstate(i)->luac != "NULL") {
				lua_getglobal(getchannel(i)->L,"update");
				if (lua_isfunction(getchannel(i)->L, -1)) {
					lua_pushnumber(getchannel(i)->L,0.03333f);
					checkLua(i, lua_pcall(getchannel(i)->L, 1, 0, 0));
				}
				refreshexports(i);
			}
		}
	}
	for (auto &el: images) {
		if(el.second->freed)
			free(el.second);
		else {
			if(el.second->uses <= 0) el.second->cachetimer--;
			else el.second->cachetimer = 120;
			if(el.second->cachetimer <= 0)
				todelete.push_back(el.second);
		}
	}
	for (auto it = images.cbegin(); it != images.cend();) {
		if(it->second->freed)
			images.erase(it++);
		else ++it;
	}

	//desktop.setKioskModeComponent(getTopLevelComponent());
	audioProcessor.drawNextFrameOfSpectrum();
	context.triggerRepaint();
	audioProcessor.nextFFTBlockReady = false;
}
void RedVJAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue) {
	if (parameterID == "bleh") {
		//bleh = newValue;
	}
}
bool RedVJAudioProcessorEditor::keyPressed(const KeyPress& key, Component* originatingComponent) {
	if (key.getKeyCode() == 79 && key.getModifiers().isCtrlDown()) {
		flchs->launchAsync(0,[&](const FileChooser& flchs){
			audioProcessor.presetpath = flchs.getResult().getFullPathName();
			String ext = flchs.getResult().getFileExtension();
			if(ext == ".rvjs") loadshader(flchs.getResult(),selectedchannel);
		});
		return true;
	}
	return false;
}
void RedVJAudioProcessorEditor::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) {
/*
pitch: 0-127
	channel 1-16
velocity: 0-1
	note 0-127
	channel 1-16
program: 0-127
	channel 1-16
pitch wheel: 0-1
	channel 1-16
controller: 0-1
	channel 1-16
	controller 0-127
aftertouch: 0-1
	channel 1-16

*/
	String midiidentifier = source->getIdentifier();
	String midiname = source->getName();
	String miditype = "";
	int midicontrollernumber = -1;
	int midichannel = message.getChannel();
	int midinote = -1;
	float controllervalue = 0;
	if (message.isNoteOn(true)) {
		miditype = message.getVelocity() == 0 ? "Note Off" : "Note On";
		midinote = message.getNoteNumber();
		controllervalue = message.getFloatVelocity();

	} else if (message.isAllNotesOff() || message.isAllSoundOff()) {
		miditype = "Note Off";

	} else if (message.isAftertouch()) {
		miditype = "Aftertouch";
		midinote = message.getNoteNumber();
		controllervalue = message.getAfterTouchValue()/127.f;

	} else if (message.isProgramChange()) {
		miditype = "Program";
		controllervalue = message.getProgramChangeNumber();

	} else if (message.isPitchWheel()) {
		miditype = "Pitch Wheel";
		controllervalue = message.getPitchWheelValue()/16383.f;

	} else if(message.isController()) {
		midicontrollernumber = message.getControllerNumber();
		miditype = message.getControllerName(midicontrollernumber) == nullptr ? "Unknown" : message.getControllerName(midicontrollernumber);
		controllervalue = message.getControllerValue()/127.f;

	} else {
		miditype = "Other";
		return;
	}

	for(int i = 0; i < 9; i++) if(getchannelstate(i)->on)
		for (int p = 0; p < getchannelstate(i)->luaimports.size(); p++)
			if(
				midiidentifier.contains(getchannelstate(i)->luaimports[p].midiidentifier) &&

				midiname.contains(getchannelstate(i)->luaimports[p].midiname) &&

				(((getchannelstate(i)->luaimports[p].miditype == "Pitch" || getchannelstate(i)->luaimports[p].miditype == "Velocity") && (miditype == "Note On" || miditype == "Note Off")) ||
				(getchannelstate(i)->luaimports[p].miditype == miditype && miditype != "Note On" && miditype != "Note Off") ||
				(getchannelstate(i)->luaimports[p].miditype == "Controller" && (getchannelstate(i)->luaimports[p].midicontrollernumber == midicontrollernumber || getchannelstate(i)->luaimports[p].midicontrollernumber == -1) && midicontrollernumber >= 0) ||
				(getchannelstate(i)->luaimports[p].miditype == "" && (miditype == "Pitch Wheel" || midicontrollernumber >= 0) && (getchannelstate(i)->luaimports[p].midichannel > 0 || getchannelstate(i)->luaimports[p].midiidentifier != "" || getchannelstate(i)->luaimports[p].midiname != "") && getchannelstate(i)->luaimports[p].midicontrollernumber < 0) ||
				(getchannelstate(i)->luaimports[p].miditype == "" && midicontrollernumber >= 0 && getchannelstate(i)->luaimports[p].midicontrollernumber == midicontrollernumber)) &&

				(getchannelstate(i)->luaimports[p].midichannel == midichannel ||
				getchannelstate(i)->luaimports[p].midichannel <= 0 ||
				midichannel <= 0) &&

				(getchannelstate(i)->luaimports[p].midinote == midinote ||
				getchannelstate(i)->luaimports[p].midinote == -1 ||
				midinote <= -1)) {
				
				if(getchannelstate(i)->luaimports[p].miditype == "Pitch") updatevalue(i,p,midinote);
				else if(getchannelstate(i)->luaimports[p].miditype == "Program") updatevalue(i,p,(int)controllervalue);
				else updatevalue(i,p,controllervalue);
	}
}
void RedVJAudioProcessorEditor::updatevalue(int channel, int parameter, float value) {
	luaL_dostring(getchannel(channel)->L,"imports["+parameter+"].value="+value);
}
void RedVJAudioProcessorEditor::updatevalue(int channel, int parameter, int value) {

}
void RedVJAudioProcessorEditor::refreshexports(int channel) {
	for (int p = 0; p < getchannelstate(channel)->luaexports.size(); p++)
		if(getchannelstate(channel)->luaexports[p].type == "image")
			images[getchannelstate(channel)->luaexports[p].id]->uses--;

	lua_getglobal(getchannel(channel)->L,"exports");
	if(lua_istable(getchannel(channel)->L,-1)) {
		lua_pushnil(getchannel(channel)->L);
		int i = 0;
		while(lua_next(getchannel(channel)->L,-2) != 0) {
			if(getchannelstate(channel)->luaexports.size() <= i)
				getchannelstate(channel)->luaexports.push_back(exports());

			getchannelstate(channel)->luaexports[i].type = "float";
			getchannelstate(channel)->luaexports[i].name = "";
			getchannelstate(channel)->luaexports[i].path = "";
			getchannelstate(channel)->luaexports[i].x = 0.f;
			getchannelstate(channel)->luaexports[i].y = 0.f;
			getchannelstate(channel)->luaexports[i].z = 0.f;
			getchannelstate(channel)->luaexports[i].w = 0.f;
			getchannelstate(channel)->luaexports[i].device = 0;
			getchannelstate(channel)->luaexports[i].channel = 0;
			getchannelstate(channel)->luaexports[i].id = -1;

			lua_pushstring(getchannel(channel)->L,"name");
			lua_gettable(getchannel(channel)->L,-2);
			if(lua_isstring(getchannel(channel)->L,-1))
				getchannelstate(channel)->luaexports[i].name = lua_tostring(getchannel(channel)->L,-1);
			lua_pop(getchannel(channel)->L,1);

			lua_pushstring(getchannel(channel)->L,"type");
			lua_gettable(getchannel(channel)->L,-2);
			if(lua_isstring(getchannel(channel)->L,-1))
				getchannelstate(channel)->luaexports[i].type = lua_tostring(getchannel(channel)->L,-1);
			lua_pop(getchannel(channel)->L,1);

			if(getchannelstate(channel)->luaexports[i].type == "image" || getchannelstate(channel)->luaexports[i].type == "video") {
				lua_pushstring(getchannel(channel)->L,"path");
				lua_gettable(getchannel(channel)->L,-2);
				if(lua_isstring(getchannel(channel)->L,-1)) {
					String hh = lua_tostring(getchannel(channel)->L,-1);
					if(!File::isAbsolutePath(hh)) {
						if(!hh.startsWithChar('\\') && !hh.startsWithChar('/'))
							hh = "/"+hh;
						hh = getchannelstate(channel)->path.getParentDirectory().getFullPathName()+hh;
					}
					getchannelstate(channel)->luaexports[i].path = File(hh);
				}
				lua_pop(getchannel(channel)->L,1);

				if (getchannelstate(channel)->luaexports[i].type == "image"){
					for (auto &el: images)
						if (getchannelstate(channel)->luaexports[i].path == el.second->path && el.second->cachetimer > 1) {
							getchannelstate(channel)->luaexports[i].id = el.first;
							images[el.first]->uses++;
							break;
					}
					if (getchannelstate(channel)->luaexports[i].id == -1) {
						images.insert(std::make_pair(currentimgid,new imagevalue()));
						images[currentimgid]->path = getchannelstate(channel)->luaexports[i].path;
						getchannelstate(channel)->luaexports[i].id = currentimgid;
						currentimgid++;
					}
				}

			} else if(getchannelstate(channel)->luaexports[i].type == "camera") {
				lua_pushstring(getchannel(channel)->L,"device");
				lua_gettable(getchannel(channel)->L,-2);
				if(lua_isinteger(getchannel(channel)->L,-1))
					getchannelstate(channel)->luaexports[i].device = lua_tointeger(getchannel(channel)->L,-1);
				lua_pop(getchannel(channel)->L,1);

			} else if(getchannelstate(channel)->luaexports[i].type == "channel") {
				lua_pushstring(getchannel(channel)->L,"channel");
				lua_gettable(getchannel(channel)->L,-2);
				if(lua_isinteger(getchannel(channel)->L,-1))
					getchannelstate(channel)->luaexports[i].channel = lua_tointeger(getchannel(channel)->L,-1);
				lua_pop(getchannel(channel)->L,1);

			} else {
				lua_pushstring(getchannel(channel)->L,"value");
				lua_gettable(getchannel(channel)->L,-2);
				if(lua_isnumber(getchannel(channel)->L,-1))
					getchannelstate(channel)->luaexports[i].x = lua_tonumber(getchannel(channel)->L,-1);
				else if(lua_istable(getchannel(channel)->L,-1)) {
					lua_pushnil(getchannel(channel)->L);
					if(lua_next(getchannel(channel)->L,-2) != 0) {
						if(lua_isnumber(getchannel(channel)->L,-1))
							getchannelstate(channel)->luaexports[i].x = lua_tonumber(getchannel(channel)->L,-1);
						lua_pop(getchannel(channel)->L,1);
						if(lua_next(getchannel(channel)->L,-2) != 0) {
							if(lua_isnumber(getchannel(channel)->L,-1))
								getchannelstate(channel)->luaexports[i].y = lua_tonumber(getchannel(channel)->L,-1);
							lua_pop(getchannel(channel)->L,1);
							if(lua_next(getchannel(channel)->L,-2) != 0) {
								if(lua_isnumber(getchannel(channel)->L,-1))
									getchannelstate(channel)->luaexports[i].z = lua_tonumber(getchannel(channel)->L,-1);
								lua_pop(getchannel(channel)->L,1);
								if(lua_next(getchannel(channel)->L,-2) != 0) {
									if(lua_isnumber(getchannel(channel)->L,-1))
										getchannelstate(channel)->luaexports[i].w = lua_tonumber(getchannel(channel)->L,-1);
									lua_pop(getchannel(channel)->L,1);
								}
							}
						}
					}
				}
				lua_pop(getchannel(channel)->L,1);
			}

			lua_pop(getchannel(channel)->L,1);
			i++;
		}
	}
}
void RedVJAudioProcessorEditor::loadshader(File filename, int channel) {
	if(getchannelstate(channel)->fft && getchannelstate(channel)->on)
		fftusecount--;

	getchannelstate(channel)->path = filename;
	getchannelstate(channel)->fft = false;
	std::stringstream ss(filename.loadFileAsString().replace("\r\n","\n").replaceCharacter('\r','\n').toRawUTF8());
	std::string token;
	std::ostringstream luastream;
	std::ostringstream vertstream;
	std::ostringstream fragstream;
	String nowhitespace = "";
	int mode = 0;

	while(std::getline(ss,token,'\n')) {
		nowhitespace = ((String)token).removeCharacters(" \n\r\t\f\v");
		if(nowhitespace == "[comment]") mode = 0;
		else if(nowhitespace == "[lua]") mode = 1;
		else if(nowhitespace == "[vertex]") mode = 2;
		else if(nowhitespace == "[fragment]") mode = 3;
		else if(mode == 1) luastream << token << '\n';
		else if(mode == 2) vertstream << token << '\n';
		else if(mode == 3) fragstream << token << '\n';
	}

	if(vertstream.str() != "") {
		getchannelstate(channel)->vert = vertstream.str();
		getchannel(channel)->resetshader = true;
	} else getchannelstate(channel)->vert = defaultvert;

	if(fragstream.str() != "") {
		getchannelstate(channel)->frag = fragstream.str();
		getchannel(channel)->resetshader = true;
	} else getchannelstate(channel)->frag = defaultfrag;

	if(getchannelstate(channel)->luac != "NULL") lua_close(getchannel(channel)->L);
	if(luastream.str() != "") {
		getchannelstate(channel)->luac = luastream.str();
		getchannel(channel)->L = luaL_newstate();
		luaL_openlibs(getchannel(channel)->L);
		lua_register(getchannel(channel)->L,"Debug",lua_Debug);
		lua_register(getchannel(channel)->L,"GetFFT",lua_GetFFT);

		if(checkLua(channel,luaL_dostring(getchannel(channel)->L,getchannelstate(channel)->luac.toRawUTF8()))) {
			lua_getglobal(getchannel(channel)->L,"imports");
			if(lua_istable(getchannel(channel)->L,-1)) {
				lua_pushnil(getchannel(channel)->L);
				int i = 0;
				while(lua_next(getchannel(channel)->L,-2) != 0) {
					getchannelstate(channel)->luaimports.push_back(imports());

					lua_pushstring(getchannel(channel)->L,"name");
					lua_gettable(getchannel(channel)->L,-2);
					if(lua_isstring(getchannel(channel)->L,-1))
						getchannelstate(channel)->luaimports[i].name = lua_tostring(getchannel(channel)->L,-1);
					lua_pop(getchannel(channel)->L,1);

					lua_pushstring(getchannel(channel)->L,"value");
					lua_gettable(getchannel(channel)->L,-2);
					if(lua_isnumber(getchannel(channel)->L,-1))
						getchannelstate(channel)->luaimports[i].value = lua_tonumber(getchannel(channel)->L,-1);
					lua_pop(getchannel(channel)->L,1);

					lua_pushstring(getchannel(channel)->L,"miditype");
					lua_gettable(getchannel(channel)->L,-2);
					if(lua_isstring(getchannel(channel)->L,-1))
						getchannelstate(channel)->luaimports[i].miditype = lua_tostring(getchannel(channel)->L,-1);
					lua_pop(getchannel(channel)->L,1);

					lua_pushstring(getchannel(channel)->L,"midichannel");
					lua_gettable(getchannel(channel)->L,-2);
					if(lua_isinteger(getchannel(channel)->L,-1))
						getchannelstate(channel)->luaimports[i].midichannel = lua_tointeger(getchannel(channel)->L,-1);
					lua_pop(getchannel(channel)->L,1);

					lua_pop(getchannel(channel)->L,1);
					i++;
				}
			}

			refreshexports(channel);

			lua_getglobal(getchannel(channel)->L,"settings");
			if(lua_istable(getchannel(channel)->L,-1)) {
				lua_pushstring(getchannel(channel)->L,"fft");
				lua_gettable(getchannel(channel)->L,-2);
				if(lua_isboolean(getchannel(channel)->L,-1)) {
					getchannelstate(channel)->fft = lua_toboolean(getchannel(channel)->L,-1);
					if(getchannelstate(channel)->on) fftusecount++;
				}
				lua_pop(getchannel(channel)->L,1);
			}
		}
	} else getchannelstate(channel)->luac = "NULL";

	audioProcessor.dofft = fftusecount <= 0;
}
bool RedVJAudioProcessorEditor::checkLua(int channel, int results) {
	if (results != LUA_OK) {
		getchannel(channel)->shadererror = "LUA ERROR: "+(String)lua_tostring(getchannel(channel)->L, -1);
		DBG(getchannel(channel)->shadererror);
		return false;
	}
	return true;
}
int RedVJAudioProcessorEditor::lua_Debug(lua_State* lel) {
	for(int i = 0; i < 9; i++)
		if(singleton->getchannel(i)->L == lel)
			singleton->getchannel(i)->shadererror = lua_tostring(lel,1);
	return 0;
}
int RedVJAudioProcessorEditor::lua_GetFFT(lua_State* lel) {
	float input = lua_tonumber(lel,1);
	lua_pushnumber(lel,
		singleton->audioProcessor.scopeData[(int)floor(input)]*(1-fmod(input,1))+
		singleton->audioProcessor.scopeData[(int)ceil(input)]*fmod(input,1));
	return 1;
}
channel* RedVJAudioProcessorEditor::getchannel(int channel) {
	return &channellist[channel];

}
channelstate* RedVJAudioProcessorEditor::getchannelstate(int channel) {
	return &statelist[currentstate].channellist[channel];

}
channelstate* RedVJAudioProcessorEditor::getchannelstate(int channel, int statee) {
	return &statelist[statee].channellist[channel];
}
