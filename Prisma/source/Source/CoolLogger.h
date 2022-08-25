/* cool logger created by me, unplugred.  */

#pragma once
#include "includes.h"
using namespace gl;

#define ENABLE_CONSOLE
#define ENABLE_TEXT
#define CONSOLE_LENGTH 20

class CoolLogger {
public:
	void init(OpenGLContext* context, int w, int h);
	void debug(String str, bool timestamp = true);
	void debug(int str, bool timestamp = true);
	void debug(float str, bool timestamp = true);
	void debug(double str, bool timestamp = true);
	void debug(bool str, bool timestamp = true);
	void drawstring(String txty, float x = .5, float y = .5, float xa = .5f, float ya = .5f, std::unique_ptr<OpenGLShaderProgram>* shader = nullptr);
	void drawlog();
	void release();

#ifdef ENABLE_TEXT
	String textvert;
	String textfrag;
#endif
private:
#ifdef ENABLE_TEXT
	OpenGLTexture texttex;
	std::unique_ptr<OpenGLShaderProgram> textshader;
	OpenGLContext* context;
	int width = 100;
	int height = 100;
#endif
#ifdef ENABLE_CONSOLE
	String debuglist[CONSOLE_LENGTH];
	int debugreadpos = 0;
	std::mutex debugmutex;
	String debugtxt = "";
#endif
};
