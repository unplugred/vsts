/*
  ==============================================================================

	display.h
	Created: 6 Jul 2021 3:41:33pm
	Author:	 unplugred

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "functions.h"
using namespace juce;

class displayComponent : public Component, public OpenGLRenderer
{
public:
	displayComponent();
	~displayComponent() override;
	void update();

	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;

	void mouseEnter(const MouseEvent& event) override;
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	int recalchover(float x, float y);

	float leftvu = 0;
	float rightvu = 0;
	float leftrms = 0;
	float rightrms = 0;
	float leftvelocity = 0;
	float rightvelocity = 0;
	float multiplier = 1;

	bool leftpeak = false;
	bool rightpeak = false;
	float leftpeaklerp = 0;
	float rightpeaklerp = 0;
	float leftpeakvelocity = 0;
	float rightpeakvelocity = 0;
	int lefthold = 0;
	int righthold = 0;

	bool stereo = false;
	int damping = 5;
	int nominal = -18;

	float stereodamp = 0;
	int settingstimer = 0;
	float settingsfade = 0;
	float settingsvelocity = 0;
	int hover = 0;
	int initialdrag = 0;
	bool held = false;
	int initialvalue = 0;
	float stereovelocity = 0;
	float websiteht = 0;

private:
	OpenGLContext openGLContext;
	unsigned int arraybuffer;
	OpenGLTexture vutex;
	OpenGLTexture mptex;
	OpenGLTexture lgtex;
	float square[8]{
		0.f,0.f,
		1.f,0.f,
		0.f,1.f,
		1.f,1.f};

	std::unique_ptr<OpenGLShaderProgram> vushader;
	static char* vertshader() {
		return
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;"
"uniform float rotation;"
"uniform float right;"
"uniform float stereo;"
"uniform float stereoinv;"
"uniform vec2 size;"
"uniform vec4 lgsize;"
"out vec2 v_TexCoord;"
"out vec2 metercoords;"
"out vec2 txtcoords;"
"out vec2 lgcoords;"
"void main(){"
"	metercoords = aPos;"
"	if(stereo <= .001) {"
"		gl_Position = vec4(aPos*2-1,0,1);"
"		v_TexCoord = (aPos+vec2(2,0))*size;"
"		txtcoords = aPos;"
"	} else if(right < .5) {"
"		gl_Position = vec4(aPos*2-1,0,1);"
"		v_TexCoord = aPos*vec2(1+stereo,1)*size;"
"		txtcoords = vec2(aPos.x*(1+stereo)-.5*stereo,aPos.y);"
"		metercoords.x = metercoords.x*(1+stereo)-.0387*stereo;"
"	} else {"
"		gl_Position = vec4((aPos.x+1)*(2-stereoinv)-1,aPos.y*2-1,0,1);"
"		v_TexCoord = (aPos+vec2(1,0))*size;"
"		txtcoords = aPos+vec2(1-stereo*.5,0);"
"		metercoords.x+=.0465;"
"	}"
"	metercoords = (metercoords-.5)*vec2(32./19.,1)+vec2(0,.5);"
"	metercoords.y += stereoinv*.06+.04;"
"	metercoords = vec2(metercoords.x*cos(rotation)-metercoords.y*sin(rotation)+.5,metercoords.x*sin(rotation)+metercoords.y*cos(rotation));"
"	metercoords.y -= stereoinv*.06+.04;"
"	if(right < .5) lgcoords = aPos*lgsize.xy+vec2(lgsize.z-lgsize.x,lgsize.w);"
"	else lgcoords = aPos*lgsize.xy+vec2(lgsize.z,lgsize.w);"
"}";
	}
	static char* fragshader() {
		return
"#version 330 core\n"
"in vec2 v_TexCoord;"
"in vec2 metercoords;"
"in vec2 txtcoords;"
"in vec2 lgcoords;"
"uniform float peak;"
"uniform vec2 size;"
"uniform vec2 txtsize;"
"uniform vec3 lines;"
"uniform float stereo;"
"uniform vec4 lineht;"
"uniform float right;"
"uniform float pause;"
"uniform sampler2D vutex;"
"uniform sampler2D mptex;"
"uniform sampler2D lgtex;"
"void main(){"
""//vu
"	if(peak >= .999) gl_FragColor = texture2D(vutex,v_TexCoord+vec2(0,1-size.y*2));"
"	else {"
"		gl_FragColor = texture2D(vutex,v_TexCoord+vec2(0,1-size.y));"
"		if(peak > .001) gl_FragColor = gl_FragColor*(1-peak)+texture2D(vutex,v_TexCoord+vec2(0,1-size.y*2))*peak;"
"	}"
"	vec4 mask = texture2D(vutex,v_TexCoord+vec2(0,1-size.y*3));"
"	vec4 ids = texture2D(vutex,v_TexCoord+vec2(0,1-size.y*4));"
"	if(right < .5 && stereo > .001 && stereo < .999) {"
"		vec3 single = texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y)).rgb;"
"		if(peak >= .999) single = texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y*2)).rgb;"
"		else single = single*(1-peak)+texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y*2)).rgb*peak;"
"		gl_FragColor = v_TexCoord.x>size.x?gl_FragColor:(gl_FragColor*stereo+vec4(single,1.)*(1-stereo));"
"		mask = mask*stereo+texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y*3))*(1-stereo);"
"		ids = ids*stereo+texture2D(vutex,v_TexCoord+vec2(size.x*2,1-size.y*4))*(1-stereo);"
"	}"
""//meter
"	vec3 meter = vec3(0.);"
"	if(metercoords.x > .001 && metercoords.y > .001 && metercoords.x < .999 && metercoords.y < .999)"
"		meter = texture2D(vutex,min(max(metercoords,0),1)*vec2(size.x*.59375,size.y)+vec2(size.x*3,1-size.y)).rgb*ids.b;"
"	vec3 shadow = vec3(1.);"
"	if(metercoords.x > .016 && metercoords.y > -.014 && metercoords.x < 1.14 && metercoords.y < .984)"
"		shadow = 1-texture2D(vutex,min(max(metercoords+vec2(-.015,.015),0),1)*vec2(size.x*.59375,size.y)+vec2(size.x*3,1-size.y*2)).rgb*ids.r*.15;"
"	gl_FragColor = vec4(gl_FragColor.rgb*(1-meter)*shadow+mask.rgb*meter,1.);"
""//pause text
"	if(pause > .001) {"
"		vec2 txdiv = vec2(3.8,7.4);"
"		float bg = ids.g*pause;"
"		if(stereo > .001) bg = texture2D(vutex,vec2(min(max(v_TexCoord.x+size.x*(2-stereo*.5),size.x*2),size.x*3),v_TexCoord.y+(1-size.y*4))).g*pause;"
"		if(abs(txtcoords.x-.5) <= 1/txdiv.x && abs(txtcoords.y-.5) <= 1.5/txdiv.y) {"
"			bool highlight = false;"
"			float line = 0;"
"			if(abs(txtcoords.y-.5) > .5/txdiv.y) {"
"				if(txtcoords.y>.5) {"
"					line = 1.5+lines.x;"
"					highlight = lineht.x>=.5;"
"				} else {"
"					line = -.5+lines.z;"
"					highlight = lineht.z>=.5;"
"				}"
"			} else {"
"				line = .5+lines.y;"
"				highlight = lineht.y>=.5;"
"			}"
"			vec3 txcoordss = texture2D(mptex,((txtcoords-.5)*txdiv-vec2(1,line))*vec2(.5,.03125)).rgb;"
"			float tx = texture2D(vutex,(txcoordss.rg+mod(((txtcoords-.5)*txdiv*vec2(8,1)-vec2(0,.5)),1.)*vec2(.125,.25))*txtsize+vec2(size.x*3,1-size.y*3)).g*(1-txcoordss.b*(highlight?.4:0));"
""//pause bg
"			gl_FragColor = abs(gl_FragColor-bg)*(1-tx)+max(gl_FragColor-bg,0)*tx;"
"		} else gl_FragColor = abs(gl_FragColor-bg);"
""//pause logo
"		if(lgcoords.x > 0 && lgcoords.y > 0 && lgcoords.y < 1) {"
"			vec2 lg = texture2D(lgtex,lgcoords).rb;"
"			if(lineht.w > -1) gl_FragColor = gl_FragColor*(1-lg.g)+(1-(1-vec4(1.,.4549,.2588,1.))*(1-texture2D(lgtex,lgcoords+vec2(lineht.w,0)).g))*lg.r;"
"			else gl_FragColor = gl_FragColor*(1-lg.g)+vec4(1.,.4549,.2588,1.)*lg.r;"
"		}"
"	}"
"}";
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(displayComponent)
};
