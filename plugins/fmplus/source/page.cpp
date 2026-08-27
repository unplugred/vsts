#include "page.h"

FMPlusPage::FMPlusPage() {}
FMPlusPage::~FMPlusPage() {}

void FMPlusPage::calcvis(int lineupdated) {}
void FMPlusPage::rebuildtab() {}
void FMPlusPage::addtext(int i) {}
bool FMPlusPage::updatevalue(int param) { return false; }
void FMPlusPage::getframe() {}

void meshdata::addtext(float x, float y, String txt, float color, bool rotated) {
	int initpos = textlength;
	float s = x;
	for(int n = 0; n < txt.length(); ++n) {

	text  [++textlength  *4] =     s    /width  ; // top    left
	text  [1+textlength  *4] = 1-( y    /height);
	text  [2+textlength  *4] = color;

	for(int i = 0; i < 2; ++i) {
	text  [++textlength  *4] =     s    /width  ; // bottom left
	text  [1+textlength  *4] = 1-((y+10)/height);
	text  [2+textlength  *4] = color;

	text  [++textlength  *4] =    (s+ 8)/width  ; // top    right
	text  [1+textlength  *4] = 1-( y    /height);
	text  [2+textlength  *4] = color;
	}

	text  [++textlength  *4] =    (s+ 8)/width  ; // bottom right
	text  [1+textlength  *4] = 1-((y+10)/height);
	text  [2+textlength  *4] = color;

	s += 7;
	}

	if(rotated) {
	textlength = initpos;

	for(int n = 0; n < txt.length(); ++n) {
	text  [++textlength  *4] += 9.f/width ; // top    left
	text  [1+textlength  *4] -= 1.f/height;

	for(int i = 0; i < 2; ++i) {
	text  [++textlength  *4] -= 1.f/width ; // bottom left
	text  [1+textlength  *4] += 9.f/height;

	text  [++textlength  *4] += 1.f/width ; // top    right
	text  [1+textlength  *4] -= 9.f/height;
	}

	text  [++textlength  *4] -= 9.f/width ; // bottom right
	text  [1+textlength  *4] += 1.f/height;
	}

	}

	replacetext(initpos,txt);
}
void meshdata::replacetext(int id, String txtt, int length) {
	const char* txt = txtt.toUTF8();
	int n = -1;
	bool ended = false;
	while(++n < length || (!ended && txt[n] != '\0')) {
	if(txt[n] == '\0') ended = true;
	int c = ' ';
	if(!ended) c = txt[n]+floor(txt[n]/16.f);
	text  [3+(1+n*6+id)  *4] = c   ;
	text  [3+(2+n*6+id)  *4] = c+17;
	text  [3+(3+n*6+id)  *4] = c+ 1;
	text  [3+(4+n*6+id)  *4] = c+17;
	text  [3+(5+n*6+id)  *4] = c+ 1;
	text  [3+(6+n*6+id)  *4] = c+18;
	}
}
void meshdata::addsquare(float x, float y, float w, float h, float color, bool corner) {
	square[++squarelength*4] =     x    /width  ; // top    left
	square[1+squarelength*4] = 1-( y    /height);
	square[2+squarelength*4] = color;
	square[3+squarelength*4] =          100;

	for(int i = 0; i < 2; ++i) {
	square[++squarelength*4] =     x    /width  ; // bottom left
	square[1+squarelength*4] = 1-((y+ h)/height);
	square[2+squarelength*4] = color;
	square[3+squarelength*4] = corner?w:100;

	square[++squarelength*4] =    (x+ w)/width  ; // top    right
	square[1+squarelength*4] = 1-( y    /height);
	square[2+squarelength*4] = color;
	square[3+squarelength*4] = corner?h:100;
	}

	square[++squarelength*4] =    (x+ w)/width  ; // bottom right
	square[1+squarelength*4] = 1-((y+ h)/height);
	square[2+squarelength*4] = color;
	square[3+squarelength*4] = corner?0:100;
}
void meshdata::beginline(float x, float y) {
	lineprevx = x;
	lineprevy = y;
	linecurrentx = x;
	linecurrenty = y;
	linebegun = true;
}
void meshdata::endline() {
	float inversedpi = .5f/scaled_dpi;
	linewritepos += 2;
	float angle = std::atan2(
		(line[linewritepos*3-5]-line[linewritepos*3-11])*height,
		(line[linewritepos*3-6]-line[linewritepos*3-12])*width )+1.5707963268f;
	line[linewritepos*3-3] = line[linewritepos*3-9]+ sin(angle)*inversedpi/width ;
	line[linewritepos*3-2] = line[linewritepos*3-8]- cos(angle)*inversedpi/height;
	line[linewritepos*3-1] = -1.f;
	line[linewritepos*3  ] = line[linewritepos*3-6]+ sin(angle)*inversedpi/width ;
	line[linewritepos*3+1] = line[linewritepos*3-5]- cos(angle)*inversedpi/height;
	line[linewritepos*3+2] =  2.f;
	line[linewritepos*3-9]                         -=sin(angle)*inversedpi/width ;
	line[linewritepos*3-8]                         +=cos(angle)*inversedpi/height;
	line[linewritepos*3-7] =  0.f;
	line[linewritepos*3-6]                         -=sin(angle)*inversedpi/width ;
	line[linewritepos*3-5]                         +=cos(angle)*inversedpi/height;
	line[linewritepos*3-4] =  1.f;
}
void meshdata::nextpoint(float x, float y, bool knee) {
	float linewidth = .5f+(.5f/scaled_dpi);
	float angle1 = std::atan2(linecurrenty-   lineprevy,linecurrentx-   lineprevx)+1.5707963268f;
	float angle2 = std::atan2(           y-linecurrenty,           x-linecurrentx)+1.5707963268f;
	float angle = 0;
	for(int i = ((linebegun||!knee)?1:0); i < 2; ++i) {
		       if(knee) {
			angle = i==0?angle1:angle2;
		} else if(linebegun) {
			angle = angle2;
		} else if(fabs(angle1-angle2)<=1) {
			angle = (angle1+angle2)*.5f;
		} else if(fabs(linecurrenty-lineprevy) > fabs(y-linecurrenty)) {
			angle = angle1;
		} else {
			angle = angle2;
		}
		line[++linewritepos*3] = (linecurrentx+cos(angle)*linewidth)/width ;
		line[1+linewritepos*3] = (linecurrenty+sin(angle)*linewidth)/height;
		line[2+linewritepos*3] = 0.f;
		line[++linewritepos*3] = (linecurrentx-cos(angle)*linewidth)/width ;
		line[1+linewritepos*3] = (linecurrenty-sin(angle)*linewidth)/height;
		line[2+linewritepos*3] = 1.f;
	}
	if(linebegun) {
		float inversedpi = .5f/scaled_dpi;
		linewritepos += 2;
		line[linewritepos*3-3] = line[linewritepos*3-9]+ sin(angle)*inversedpi/width ;
		line[linewritepos*3-2] = line[linewritepos*3-8]- cos(angle)*inversedpi/height;
		line[linewritepos*3-1] =  0.f;
		line[linewritepos*3  ] = line[linewritepos*3-6]+ sin(angle)*inversedpi/width ;
		line[linewritepos*3+1] = line[linewritepos*3-5]- cos(angle)*inversedpi/height;
		line[linewritepos*3+2] =  1.f;
		line[linewritepos*3-9]                         -=sin(angle)*inversedpi/width ;
		line[linewritepos*3-8]                         +=cos(angle)*inversedpi/height;
		line[linewritepos*3-7] = -1.f;
		line[linewritepos*3-6]                         -=sin(angle)*inversedpi/width ;
		line[linewritepos*3-5]                         +=cos(angle)*inversedpi/height;
		line[linewritepos*3-4] =  2.f;
		linebegun = false;
	}
	lineprevx = linecurrentx;
	lineprevy = linecurrenty;
	linecurrentx = x;
	linecurrenty = y;
}
