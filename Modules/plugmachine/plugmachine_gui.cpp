#include "plugmachine_gui.h"

plugmachine_look_n_feel::plugmachine_look_n_feel() {
}
plugmachine_look_n_feel::~plugmachine_look_n_feel() {
}
String plugmachine_look_n_feel::find_font(String query) {
	std::stringstream ss(query.toRawUTF8());
	std::string token;
	std::vector<String> words;
	while(std::getline(ss, token, '|'))
		words.push_back(token);

	int index = -1;
	auto ft = Font::findAllTypefaceNames();
	for(const auto &q : ft)
		for(int i = 0; i < words.size(); ++i)
			if(q == words[i] && index < i)
				index = i;

	if(index == -1)
		return words[words.size()-1];
	return words[index];
}
String plugmachine_look_n_feel::add_line_breaks(String input, int width) {
	Font font(getPopupMenuFont());
	std::stringstream ss(input.toRawUTF8());
	std::string token;
	String out = "";
	float line = 0;
	while(std::getline(ss, token, ' ')) {
		float word = 0;
		if(((String)token).containsChar('\n'))
			word = font.getStringWidthFloat((line==0?"":" ")+((String)token).upToFirstOccurrenceOf("\n",false,false));
		else
			word = font.getStringWidthFloat((line==0?"":" ")+token);
		if(line == 0 || (line+word) < (width*scale)) {
			if(line != 0) out += (String)" ";
			out += (String)token;
		} else {
			line = 0;
			out += (String)"\n"+token;
		}
		if(((String)token).containsChar('\n'))
			line = font.getStringWidthFloat(((String)token).fromLastOccurrenceOf("\n",false,false));
		else
			line += word;
	}
	return out;
}

plugmachine_gui::plugmachine_gui(plugmachine_dsp& p, int _width, int _height, float _target_dpi, float _scale_step, bool _do_banner, bool _do_scale) : AudioProcessorEditor(&p), audio_processor(p) {
	width = _width;
	height = _height;
	target_dpi = _target_dpi;
	scale_step = _scale_step;
	do_banner = _do_banner;
	do_scale = _do_scale;

	listeners.clear();
}
plugmachine_gui::~plugmachine_gui() {
}
void plugmachine_gui::init(plugmachine_look_n_feel* _look_n_feel) {
	if(_look_n_feel != nullptr)
		look_n_feel = _look_n_feel;

	scaled_dpi = audio_processor.ui_scale;
	ui_scales.push_back(scaled_dpi);
#ifdef BANNER
	setSize(width*scaled_dpi,(height+(do_banner?21:0)/target_dpi)*scaled_dpi);
	banner_offset = do_banner?(21.f*scaled_dpi/getHeight()):0;
#else
	setSize(width*scaled_dpi,height*scaled_dpi);
	banner_offset = 0;
#endif
	setResizable(false, false);
	setOpaque(true);

	context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
	context.setRenderer(this);
	context.attachTo(*this);
	startTimerHz(30);
}
void plugmachine_gui::close() {
	for(int i = 0; i < listeners.size(); ++i)
		audio_processor.apvts_ref->removeParameterListener(listeners[i], this);

	stopTimer();
	context.detach();
}

void plugmachine_gui::draw_init() {
	audio_processor.logger.init(&context,banner_offset,getWidth(),getHeight());

#ifdef BANNER
	if(do_banner) {
		banner_shader = add_shader(
//BANNER VERT
R"(#version 150 core
in vec2 aPos;
uniform vec2 texscale;
uniform vec2 size;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*vec2(1,size.y))*2-1,0,1);
	uv = vec2(aPos.x*size.x,1-(1-aPos.y)*texscale.y);
})",
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
	vec2 col = max(min((texture(tex,vec2(mod(uv.x+pos,1.)*texscale.x,uv.y)).rg-.5)*dpi+.5,1),0);
	fragColor = vec4(vec3(col.r*free+col.g*(1-free)),1);
})");

		add_texture(&banner_tex, (const char*)banner_png, banner_pngSize, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);
	}
#endif

	context.extensions.glGenBuffers(1, &array_buffer);
}
void plugmachine_gui::draw_begin() {
	if(dpi != context.getRenderingScale()) {
		dpi = context.getRenderingScale();
		if(do_scale) {
			int i = 0;
			double s = 1.f;
			double dif = 100;
			Rectangle<int> r = Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
			int w = r.getWidth();
			int h = r.getHeight();
			double pw = width;
			double ph = height;
#ifdef BANNER
			if(do_banner) ph += 21/target_dpi;
#endif
			ui_scales.clear();
			while((s/dpi)*pw < w && (s/dpi)*ph < h) {
				ui_scales.push_back(s/dpi);
				if(abs(audio_processor.ui_scale-s/dpi) <= dif) {
					dif = abs(audio_processor.ui_scale-s/dpi);
					ui_scale_index = i;
				}
				if(i < 4) s += scale_step;
				else if(i < 8) s += scale_step*2;
				else if(i < 16) s += scale_step*3;
				i++;
			}
			reset_size = true;
		}
	}

	if(do_scale)
		scaled_dpi = ui_scales[ui_scale_index]*dpi;
	else
		scaled_dpi = dpi;
}
void plugmachine_gui::draw_end() {

#ifdef BANNER
	if(do_banner) {
		banner_shader->use();
		auto coord = context.extensions.glGetAttribLocation(banner_shader->getProgramID(),"aPos");
		context.extensions.glEnableVertexAttribArray(coord);
		context.extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);
		context.extensions.glActiveTexture(GL_TEXTURE0);
		banner_tex.bind();
		banner_shader->setUniform("tex",0);
		banner_shader->setUniform("dpi",(float)fmax(scaled_dpi,1.f));
#ifdef BETA
		banner_shader->setUniform("texscale",494.f/banner_tex.getWidth(),21.f/banner_tex.getHeight());
		banner_shader->setUniform("size",getWidth()/(494.f/target_dpi*ui_scales[ui_scale_index]),21.f/target_dpi*ui_scales[ui_scale_index]/getHeight());
		banner_shader->setUniform("free",0.f);
#else
		banner_shader->setUniform("texscale",426.f/banner_tex.getWidth(),21.f/banner_tex.getHeight());
		banner_shader->setUniform("size",getWidth()/(426.f/target_dpi*ui_scales[ui_scale_index]),21.f/target_dpi*ui_scales[ui_scale_index]/getHeight());
		banner_shader->setUniform("free",1.f);
#endif
		banner_shader->setUniform("pos",banner_x);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}
#endif

	audio_processor.logger.drawlog();
}
void plugmachine_gui::draw_close() {
	for(int i = 0; i < textures.size(); ++i)
		textures[i]->release();
	for(int i = 0; i < shaders.size(); ++i)
		shaders[i]->release();

	audio_processor.logger.font.release();

	context.extensions.glDeleteBuffers(1,&array_buffer);
}

void plugmachine_gui::update() {
	if(reset_size) {
		reset_size = false;
		if(do_scale) {
#ifdef BANNER
			setSize(width*ui_scales[ui_scale_index],(height+(do_banner?21:0)/target_dpi)*ui_scales[ui_scale_index]);
#else
			setSize(width*ui_scales[ui_scale_index],height*ui_scales[ui_scale_index]);
#endif
		}
#ifdef BANNER
		if(do_banner)
			banner_offset = 21.f*ui_scales[ui_scale_index]/getHeight();
#endif
		audio_processor.logger.font.width = getWidth();
		audio_processor.logger.font.height = getHeight();
		audio_processor.logger.font.banneroffset = banner_offset;
		look_n_feel->scale = ui_scales[ui_scale_index];
	}

#ifdef BANNER
	if(do_banner) banner_x = fmod(banner_x+.0005f,1.f);
#endif

	context.triggerRepaint();
}

void plugmachine_gui::add_listener(String name) {
	audio_processor.apvts_ref->addParameterListener(name, this);
	listeners.push_back(name);
}
std::shared_ptr<OpenGLShaderProgram> plugmachine_gui::add_shader(String vertexshader, String fragmentshader) {
	std::shared_ptr<OpenGLShaderProgram> shader = std::make_shared<OpenGLShaderProgram>(context);
	if(!shader->addVertexShader(vertexshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at melody@unplug.red. THANKS!","OK!");
	if(!shader->addFragmentShader(fragmentshader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at melody@unplug.red. THANKS!","OK!");
	shader->link();
	shaders.push_back(shader);
	return shader;
}
void plugmachine_gui::add_texture(OpenGLTexture* texture, const char* binary, const int binary_size, int min_filter, int mag_filter, int wrap_s, int wrap_t) {
	texture->loadImage(ImageCache::getFromMemory(binary, binary_size));
	texture->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
	textures.push_back(texture);
}
