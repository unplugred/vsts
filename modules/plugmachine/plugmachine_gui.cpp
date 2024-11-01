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

	int index = 999;
	auto ft = Font::findAllTypefaceNames();
	for(const auto &q : ft)
		for(int i = 0; i < words.size(); ++i)
			if(q == words[i] && index > i)
				index = i;

	if(index == 999)
		return words[0];
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

plugmachine_gui::plugmachine_gui(Component& c, plugmachine_dsp& p, int _width, int _height, float _target_dpi, float _scale_step, bool _do_banner, bool _do_scale) : audio_processor(p), component(c) {
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
	look_n_feel = _look_n_feel;

	if(audio_processor.ui_scale == -1)
		audio_processor.set_ui_scale(target_dpi);
	scaled_dpi = audio_processor.ui_scale;
	ui_scales.push_back(scaled_dpi);
	if(width != 0 && height != 0) {
#ifdef BANNER
		component.setSize(width*scaled_dpi,floor((height+(do_banner?(21.f/target_dpi):0))*scaled_dpi));
		banner_offset = do_banner?floor(21.f/target_dpi*scaled_dpi)/component.getHeight():0;
	}
#else
		component.setSize(width*scaled_dpi,height*scaled_dpi);
	}
	banner_offset = 0;
#endif
	component.setOpaque(true);

	context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
	context.setRenderer(this);
	context.attachTo(component);
	startTimerHz(30);
}
void plugmachine_gui::close() {
	for(int i = 0; i < listeners.size(); ++i)
		audio_processor.apvts_ref->removeParameterListener(listeners[i], this);

	stopTimer();
	context.detach();
}

void plugmachine_gui::draw_init() {
	add_font(&debug_font);

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
			if(do_banner) ph += floor(21.f/target_dpi);
#endif
			ui_scales.clear();
			while((s/dpi)*pw < w && (s/dpi)*ph < h) {
				ui_scales.push_back(s/dpi);
				if(fabs(audio_processor.ui_scale-s/dpi) <= dif) {
					dif = fabs(audio_processor.ui_scale-s/dpi);
					ui_scale_index = i;
				}
				if(i < 4) s += scale_step;
				else if(i < 8) s += scale_step*2;
				else if(i < 16) s += scale_step*3;
				++i;
			}
		}
		reset_size = true;
	}

	if(do_scale)
		scaled_dpi = ui_scales[ui_scale_index]*dpi;
	else
		scaled_dpi = dpi;

	if(scaled_dpi != prev_scaled_dpi || !frame_buffers_initiated || frame_buffers_resize) {
		frame_buffers_resize = false;
		for(int i = 0; i < frame_buffers.size(); ++i) {
			if(frame_buffers_initiated) {
				if(!frame_buffers[i].scaled_x && !frame_buffers[i].scaled_y && !frame_buffers[i].to_resize) continue;
				frame_buffers[i].to_resize = false;
				frame_buffers[i].buffer->release();
			}

			frame_buffers[i].buffer->initialise(context, frame_buffers[i].width*(frame_buffers[i].scaled_x?scaled_dpi:1), frame_buffers[i].height*(frame_buffers[i].scaled_y?scaled_dpi:1));

			glBindTexture(GL_TEXTURE_2D, frame_buffers[i].buffer->getTextureID());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, frame_buffers[i].min_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, frame_buffers[i].mag_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, frame_buffers[i].wrap_s);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, frame_buffers[i].wrap_t);

			frame_buffers[i].buffer->clear(Colours::black);
		}
		frame_buffers_initiated = true;
	}

	prev_scaled_dpi = scaled_dpi;
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
		banner_shader->setUniform("size",component.getWidth()/(494.f/target_dpi*ui_scales[ui_scale_index]),floor(21.f/target_dpi*ui_scales[ui_scale_index])/component.getHeight());
		banner_shader->setUniform("free",0.f);
#else
		banner_shader->setUniform("texscale",426.f/banner_tex.getWidth(),21.f/banner_tex.getHeight());
		banner_shader->setUniform("size",component.getWidth()/(426.f/target_dpi*ui_scales[ui_scale_index]),floor(21.f/target_dpi*ui_scales[ui_scale_index])/component.getHeight());
		banner_shader->setUniform("free",1.f);
#endif
		banner_shader->setUniform("pos",banner_x);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		context.extensions.glDisableVertexAttribArray(coord);
	}
#endif

	std::lock_guard<std::mutex> guard(audio_processor.debug_mutex);
	debug_font.draw_string(1,1,1,1,0,0,0,.5,audio_processor.debug_text,0,.01,.99,0,1);
}
void plugmachine_gui::draw_close() {
	for(int i = 0; i < textures.size(); ++i)
		textures[i]->release();
	for(int i = 0; i < shaders.size(); ++i)
		shaders[i]->release();
	for(int i = 0; i < fonts.size(); ++i)
		fonts[i]->release();
	for(int i = 0; i < frame_buffers.size(); ++i)
		frame_buffers[i].buffer->release();

	context.extensions.glDeleteBuffers(1,&array_buffer);
}

void plugmachine_gui::update() {
	if(reset_size) {
		reset_size = false;
		if(width != 0 && height != 0) {
#ifdef BANNER
			component.setSize(width*ui_scales[ui_scale_index],floor((height+(do_banner?(21.f/target_dpi):0))*ui_scales[ui_scale_index]));
			if(do_banner)
				banner_offset = floor(21.f/target_dpi*ui_scales[ui_scale_index])/component.getHeight();
#else
			component.setSize(width*ui_scales[ui_scale_index],height*ui_scales[ui_scale_index]);
#endif
		}

		for(int i = 0; i < fonts.size(); ++i) {
			if(fonts[i]->is_scaled) {
				fonts[i]->width = ((float)component.getWidth())/ui_scales[ui_scale_index];
				fonts[i]->height = ((float)component.getHeight())/ui_scales[ui_scale_index];
				fonts[i]->dpi = dpi*ui_scales[ui_scale_index];
			} else {
				fonts[i]->width = component.getWidth();
				fonts[i]->height = component.getHeight();
			}
			fonts[i]->banner_offset = banner_offset;
		}
		if(look_n_feel != nullptr) look_n_feel->scale = ui_scales[ui_scale_index];
	}

#ifdef BANNER
	if(do_banner) banner_x = fmod(banner_x+.0005f,1.f);
#endif

	context.triggerRepaint();
}

void plugmachine_gui::set_size(int _width, int _height) {
	if(width == _width && height == _height) {
		int w = width*ui_scales[ui_scale_index];
#ifdef BANNER
		int h = floor((height+(do_banner?(21.f/target_dpi):0))*ui_scales[ui_scale_index]);
#else
		int h = height*ui_scales[ui_scale_index];
#endif
		if(w != component.getWidth() || h != component.getHeight())
			component.setSize(w,h);
		return;
	}
	width = _width;
	height = _height;
	reset_size = true;
}
void plugmachine_gui::set_ui_scale(int index) {
	ui_scale_index = index;
	audio_processor.set_ui_scale(ui_scales[ui_scale_index]);
	reset_size = true;
}

void plugmachine_gui::add_listener(String name) {
	audio_processor.apvts_ref->addParameterListener(name, this);
	listeners.push_back(name);
}
std::shared_ptr<OpenGLShaderProgram> plugmachine_gui::add_shader(String vertex_shader, String fragment_shader) {
	std::shared_ptr<OpenGLShaderProgram> shader = std::make_shared<OpenGLShaderProgram>(context);
	if(!shader->addVertexShader(vertex_shader))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at melody@unplug.red. THANKS!","OK!");
	if(!shader->addFragmentShader(fragment_shader))
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
void plugmachine_gui::add_frame_buffer(OpenGLFrameBuffer* frame_buffer, int width, int height, bool scaled_x, bool scaled_y, int min_filter, int mag_filter, int wrap_s, int wrap_t) {
	frame_buffer_plus buffer;
	buffer.buffer = frame_buffer;
	buffer.width = width;
	buffer.height = height;
	buffer.scaled_x = scaled_x;
	buffer.scaled_y = scaled_y;
	buffer.min_filter = min_filter;
	buffer.mag_filter = mag_filter;
	buffer.wrap_s = wrap_s;
	buffer.wrap_t = wrap_t;
	frame_buffers.push_back(buffer);
}
void plugmachine_gui::set_frame_buffer_size(OpenGLFrameBuffer* frame_buffer, int w, int h) {
	for(int i = 0; i < frame_buffers.size(); ++i) {
		if(frame_buffers[i].buffer == frame_buffer) {
			frame_buffers[i].width = w;
			frame_buffers[i].height = h;
			frame_buffers[i].to_resize = true;
			frame_buffers_resize = true;
		}
	}
}
void plugmachine_gui::add_font(cool_font* font) {
	if(font->is_scaled)
		font->draw_init(&context,banner_offset,368,334,dpi);
	else
		font->draw_init(&context,banner_offset,component.getWidth(),component.getHeight());
	fonts.push_back(font);
}

void plugmachine_gui::remove_listener(String name) {
	audio_processor.apvts_ref->removeParameterListener(name, this);
	for(int i = 0; i < listeners.size(); ++i)
		if(listeners[i] == name)
			listeners.erase(listeners.begin()+i);
}
void plugmachine_gui::remove_texture(OpenGLTexture* texture) {
	for(int i = 0; i < textures.size(); ++i)
		if(textures[i] == texture)
			textures.erase(textures.begin()+i);
	texture->release();
}
void plugmachine_gui::remove_frame_buffer(OpenGLFrameBuffer* frame_buffer) {
	for(int i = 0; i < frame_buffers.size(); ++i)
		if(frame_buffers[i].buffer == frame_buffer)
			frame_buffers.erase(frame_buffers.begin()+i);
	frame_buffer->release();
}
void plugmachine_gui::remove_shader(std::shared_ptr<OpenGLShaderProgram> shader) {
	for(int i = 0; i < shaders.size(); ++i)
		if(shaders[i] == shader)
			shaders.erase(shaders.begin()+i);
	shader->release();
}
void plugmachine_gui::remove_font(cool_font* font) {
	for(int i = 0; i < fonts.size(); ++i)
		if(fonts[i] == font)
			fonts.erase(fonts.begin()+i);
	font->release();
}

void plugmachine_gui::debug(String	str, bool timestamp) { audio_processor.debug(str, timestamp); }
void plugmachine_gui::debug(int		str, bool timestamp) { audio_processor.debug(str, timestamp); }
void plugmachine_gui::debug(float	str, bool timestamp) { audio_processor.debug(str, timestamp); }
void plugmachine_gui::debug(double	str, bool timestamp) { audio_processor.debug(str, timestamp); }
void plugmachine_gui::debug(bool	str, bool timestamp) { audio_processor.debug(str, timestamp); }
