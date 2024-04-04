#include "cool_font.h"

cool_font::cool_font() {
}
void cool_font::draw_init(OpenGLContext* _context, float _banner_offset, int _width, int _height, float _dpi) {
	context = _context;
	width = _width;
	height = _height;
	banner_offset = _banner_offset;
	dpi = _dpi;

	shader.reset(new OpenGLShaderProgram(*context));
	if(!shader->addVertexShader(vert))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Vertex shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at melody@unplug.red. THANKS!","OK!");
	if(!shader->addFragmentShader(frag))
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Fragment shader error",shader->getLastError()+"\n\nPlease mail me this info along with your graphics card and os details at melody@unplug.red. THANKS!","OK!");
	shader->link();

	texture.loadImage(image);
	texture.bind();
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
}

void cool_font::draw_string_mono(float fgr, float fgg, float fgb, float fga, float bgr, float bgg, float bgb, float bga, String _text, int channel, float x, float y, float xa, float ya) {
	shader->use();
	context->extensions.glActiveTexture(GL_TEXTURE0);
	texture.bind();
	if((fmod(dpi,1) > .00001f && fmod(dpi,1) < .99999f) || smooth) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		shader->setUniform("dpi",dpi*scale);
	} else {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		shader->setUniform("dpi",0.f);
	}
	shader->setUniform("tex",0);
	shader->setUniform("channel",channel);
	shader->setUniform("fg",fgr,fgg,fgb,fga);
	shader->setUniform("bg",bgr,bgg,bgb,bga);
	shader->setUniform("res",texture_width,texture_height);
	shader->setUniform("smoot",smooth?1.f:0.f);
	float coord = context->extensions.glGetAttribLocation(shader->getProgramID(),"aPos");
	context->extensions.glEnableVertexAttribArray(coord);
	context->extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);

	int letter = 0;
	int letter_x = 0;
	float line = -ya;
	std::queue<float> line_length;
	const char* text = _text.toUTF8();
	float letter_w = ((float)mono_width)*scale/width;
	float letterh = ((float)mono_height)*scale/height;
	while(text[letter] != '\0') {
		if(text[letter] == '\n' || (((++letter_x)+1)*letter_w > 1 && text[letter] != '\n')) {
			line_length.push(letter_x);
			line -= ya;
			letter_x = 0;
		}
		++letter;
	}
	line_length.push(letter_x);

	letter = 0;
	letter_x = 0;
	int current_line_length = line_length.front();
	line_length.pop();
	while(text[letter] != '\0') {
		if(text[letter] != '\n') {
			shader->setUniform("pos",
				x+(letter_x-current_line_length*xa)*letter_w,
				y*(1-banner_offset)+(in_frame_buffer?banner_offset:0)+line*line_height*scale/height,letter_w,letterh);
			shader->setUniform("texpos",
				.00001f+fmod(text[letter],16)*.0625f,
				.00001f+floor(text[letter]*.0625f)*.0625f,
				.0625f,.0625f);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			++letter_x;
		}
		if(text[letter] == '\n' || (letter_x+1)*letter_w > 1) {
			++line;
			letter_x = 0;
			current_line_length = line_length.front();
			line_length.pop();
		}
		++letter;
	}

	context->extensions.glDisableVertexAttribArray(coord);
}
void cool_font::draw_string(float fgr, float fgg, float fgb, float fga, float bgr, float bgg, float bgb, float bga, String _text, int channel, float x, float y, float xa, float ya) {
	if(mono) {
		draw_string_mono(fgr,fgg,fgb,fga,bgr,bgg,bgb,bga,_text,channel,x,y,xa,ya);
		return;
	}

	shader->use();
	context->extensions.glActiveTexture(GL_TEXTURE0);
	texture.bind();
	if((fmod(dpi,1) > .00001f && fmod(dpi,1) < .99999f) || smooth) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		shader->setUniform("dpi",dpi*scale);
	} else {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		shader->setUniform("dpi",0.f);
	}
	shader->setUniform("tex",0);
	shader->setUniform("channel",channel);
	shader->setUniform("fg",fgr,fgg,fgb,fga);
	shader->setUniform("bg",bgr,bgg,bgb,bga);
	shader->setUniform("res",texture_width,texture_height);
	shader->setUniform("smoot",smooth?1.f:0.f);
	float coord = context->extensions.glGetAttribLocation(shader->getProgramID(),"aPos");
	context->extensions.glEnableVertexAttribArray(coord);
	context->extensions.glVertexAttribPointer(coord,2,GL_FLOAT,GL_FALSE,0,0);

	int letter = 0;
	float letter_x = 0;
	float line = -ya;
	std::queue<float> line_length;
	const char* text = _text.toUTF8();
	while(text[letter] != '\0') {
		float letter_w = 0;
		if(text[letter] != '\n') {
			int uv_index = find_uv_index(text[letter],channel);
			if(uv_index >= 0) {
				letter_w = ((float)uv_map[channel][uv_index][7])*scale/width;
				letter_x += letter_w;
			}
		}
		if(text[letter] == '\n' || letter_x > 1) {
			if(letter_x > 1)
				letter_x -= letter_w;
			line_length.push(letter_x);
			line -= ya;
			letter_x = letter_w;
		}
		++letter;
	}
	line_length.push(letter_x);

	letter = 0;
	letter_x = 0;
	float current_line_length = line_length.front();
	line_length.pop();
	while(text[letter] != '\0') {
		int uv_index = find_uv_index(text[letter],channel);
		if(text[letter] == '\n' || (uv_index != -1 && (letter_x+((float)uv_map[channel][uv_index][7])*scale/width) > 1)) {
			++line;
			letter_x = 0;
			current_line_length = line_length.front();
			line_length.pop();
		}
		if(text[letter] != '\n' && uv_index != -1) {
			float current_x = letter_x+uv_map[channel][uv_index][5]*scale/width-current_line_length*xa;
			shader->setUniform("pos",
				x+current_x,
				y*(1-banner_offset)+(in_frame_buffer?banner_offset:0)+(line*line_height+uv_map[channel][uv_index][6])*scale/height+(current_x+((float)uv_map[channel][uv_index][7])*.5f*scale/width)*slant,
				((float)uv_map[channel][uv_index][3])*scale/width,
				((float)uv_map[channel][uv_index][4])*scale/height);
			shader->setUniform("texpos",
				.00001f+((float)uv_map[channel][uv_index][1])/texture_width,
				.00001f+((float)uv_map[channel][uv_index][2])/texture_height,
				((float)uv_map[channel][uv_index][3])/texture_width,
				((float)uv_map[channel][uv_index][4])/texture_height);
			shader->setUniform("letter",(float)letter);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			letter_x += ((float)uv_map[channel][uv_index][7])*scale/width;
		}
		++letter;
	}

	context->extensions.glDisableVertexAttribArray(coord);
}
int cool_font::find_uv_index(int letter, int channel) {
	int iterator = -1;
	while(true) {
		++iterator;
		if(uv_map[channel][iterator][0] == letter)
			return iterator;
		if(uv_map[channel][iterator][0] == -1)
			return -1;
	}
}
void cool_font::release() {
	shader->release();
	texture.release();
}
