/*
  ==============================================================================

	Logger.h
	Created: 17 Feb 2022 3:09:02pm
	Author: unplugred

  ==============================================================================
*/

#pragma once

#define ENABLE_CONSOLE

#include <JuceHeader.h>

class CoolLogger {
public:
	void init(OpenGLContext* context, int w, int h);
	void debug(String str, bool timestamp = true);
	void debug(int str, bool timestamp = true);
	void debug(float str, bool timestamp = true);
	void debug(double str, bool timestamp = true);
	void debug(bool str, bool timestamp = true);
	void drawstring(String txty, float x = .5, float y = .5, float xa = .5f, float ya = .5f);
	void drawlog();
	void release();

private:
#ifdef ENABLE_CONSOLE
	String debuglist[16];
	int debugreadpos = 0;
	std::mutex debugmutex;
	String debugtxt = "";

	OpenGLTexture texttex;
	std::unique_ptr<OpenGLShaderProgram> textshader;
	String textvert;
	String textfrag;
	OpenGLContext* context;
	int width = 100;
	int height = 100;
#endif
};
