/* font system  */

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce;
using namespace gl;

class cool_font {
public:
	cool_font();
	void draw_init(OpenGLContext* _context, float _banner_offset, int _width, int _height, float _dpi = 0);
	void draw_string(float fgr, float fgg, float fgb, float fga, float bgr, float bgg, float bgb, float bga, String _text, int channel = 0, float x = .5f, float y = .5f, float xa = .5f, float ya = .5f);
	void release();

	OpenGLContext* context;
	int width = 100;
	int height = 100;
	float banner_offset = 0;
	float dpi = 0;

	OpenGLTexture texture;
	Image image;
	bool smooth = false;
	int texture_width = 128;
	int texture_height = 256;
	std::vector<std::vector<std::vector<int>>> uv_map;
	std::vector<std::vector<std::vector<int>>> kerning;

	std::unique_ptr<OpenGLShaderProgram> shader;
	String vert =
R"(#version 150 core
in vec2 aPos;
uniform vec4 pos;
uniform vec4 texpos;
out vec2 uv;
void main(){
	gl_Position = vec4((aPos*pos.zw+pos.xy)*2-1,0,1);
	gl_Position.y *= -1;
	uv = (aPos*texpos.zw+texpos.xy);
	uv.y = 1-uv.y;
})";
	String frag =
R"(#version 150 core
in vec2 uv;
uniform sampler2D tex;
uniform int channel;
uniform vec2 res;
uniform float smoot;
uniform float dpi;
uniform vec4 bg;
uniform vec4 fg;
out vec4 fragColor;
void main(){
	vec2 nuv = uv;
	if(dpi > 1 && smoot < .5) {
		nuv *= res;
		if(mod(nuv.x,1)>.5) nuv.x = floor(nuv.x)+(1-min((1-mod(nuv.x,1))*dpi*2,.5));
		else nuv.x = floor(nuv.x)+min(mod(nuv.x,1)*dpi*2,.5);
		if(mod(nuv.y,1)>.5) nuv.y = floor(nuv.y)+(1-min((1-mod(nuv.y,1))*dpi*2,.5));
		else nuv.y = floor(nuv.y)+min(mod(nuv.y,1)*dpi*2,.5);
		nuv /= res;
	}
	float text = 0;
	if(channel == 1)
		text = texture(tex,nuv).g;
	else if(channel == 2)
		text = texture(tex,nuv).b;
	else
		text = texture(tex,nuv).r;
	if(dpi > 1 && smoot > .5)
		text = (text-.5)*dpi+.5;
	fragColor = text*fg+(1-text)*bg;
})";

	int line_height = 16;
	bool mono = false;
	int mono_width = 8;
	int mono_height = 16;
	float scale = 1;
	float slant = 0;
	bool in_frame_buffer = false;
	bool is_scaled = true;

private:
	void draw_string_mono(float fgr, float fgg, float fgb, float fga, float bgr, float bgg, float bgb, float bga, String _text, int channel = 0, float x = .5f, float y = .5f, float xa = .5f, float ya = .5f);
	int find_uv_index(int letter, int channel = 0);
};