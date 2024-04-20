#include "includes.h"

class tahoma8 : public cool_font {
public:
	tahoma8() {
		image = ImageCache::getFromMemory(BinaryData::tahoma8_png, BinaryData::tahoma8_pngSize);
		texture_width = 128;
		texture_height = 64;
		line_height = 13;
		in_frame_buffer = true;
		uv_map = {{{41,1,1,3,11,0,2,4},{40,6,1,3,11,0,2,4},{47,11,1,3,11,0,2,4},{91,16,1,3,11,0,2,4},{92,21,1,3,11,0,2,4},{93,26,1,3,11,0,2,4},{123,31,1,4,11,0,2,5},{36,37,1,5,11,0,2,6},{125,44,1,4,11,0,2,5},{124,50,1,1,11,0,2,2},{81,53,1,7,10,0,3,8},{106,62,1,2,10,0,3,3},{100,66,1,5,9,0,2,6},{98,73,1,5,9,0,2,6},{64,80,1,9,9,0,3,10},{108,91,1,1,9,0,2,2},{104,94,1,5,9,0,2,6},{107,101,1,5,9,0,2,6},{102,108,1,3,9,0,2,4},{83,113,1,5,8,0,3,6},{82,120,1,6,8,0,3,7},{74,1,14,4,8,0,3,5},{75,7,14,5,8,0,3,6},{80,14,14,5,8,0,3,6},{76,21,14,4,8,0,3,5},{78,27,14,6,8,0,3,7},{84,35,14,5,8,0,3,6},{77,42,14,7,8,0,3,8},{90,51,14,5,8,0,3,6},{105,58,14,1,8,0,3,2},{85,61,14,6,8,0,3,7},{121,69,14,5,8,0,5,6},{113,76,14,5,8,0,5,6},{112,83,14,5,8,0,5,6},{103,90,14,5,8,0,5,6},{86,97,14,5,8,0,3,6},{73,104,14,3,8,0,3,4},{87,109,14,9,8,0,3,10},{89,120,14,5,8,0,3,6},{116,1,24,3,8,0,3,4},{88,6,24,5,8,0,3,6},{79,13,24,7,8,0,3,8},{52,22,24,5,8,0,3,6},{57,29,24,5,8,0,3,6},{53,36,24,5,8,0,3,6},{55,43,24,5,8,0,3,6},{56,50,24,5,8,0,3,6},{54,57,24,5,8,0,3,6},{51,64,24,5,8,0,3,6},{33,71,24,1,8,0,3,2},{35,74,24,7,8,0,3,8},{72,83,24,6,8,0,3,7},{37,91,24,10,8,0,3,11},{48,103,24,5,8,0,3,6},{49,110,24,3,8,0,3,4},{38,115,24,7,8,0,3,8},{50,1,34,5,8,0,3,6},{65,8,34,6,8,0,3,7},{68,16,34,6,8,0,3,7},{66,24,34,5,8,0,3,6},{67,31,34,6,8,0,3,7},{63,39,34,4,8,0,3,5},{69,45,34,5,8,0,3,6},{59,52,34,2,8,0,5,3},{70,56,34,5,8,0,3,6},{71,63,34,6,8,0,3,7},{43,71,34,7,7,0,4,8},{62,80,34,6,7,0,4,7},{60,88,34,6,7,0,4,7},{111,96,34,5,6,0,5,6},{115,103,34,4,6,0,5,5},{114,109,34,3,6,0,5,4},{117,114,34,5,6,0,5,6},{122,121,34,4,6,0,5,5},{110,1,44,5,6,0,5,6},{120,8,44,5,6,0,5,6},{118,15,44,5,6,0,5,6},{119,22,44,7,6,0,5,8},{97,31,44,5,6,0,5,6},{58,38,44,1,6,0,5,2},{101,41,44,5,6,0,5,6},{109,48,44,7,6,0,5,8},{99,57,44,4,6,0,5,5},{42,63,44,5,5,0,2,6},{94,70,44,7,4,0,3,8},{44,79,44,2,4,0,9,3},{39,83,44,1,3,0,2,2},{126,86,44,7,3,0,6,8},{34,95,44,3,3,0,2,4},{61,100,44,7,3,0,6,8},{46,109,44,1,2,0,9,2},{96,112,44,2,2,0,2,3},{45,116,44,3,1,0,7,4},{95,1,52,6,1,0,12,7},{32,9,52,0,0,0,11,5},{127,11,52,0,0,0,11,8},{-1,-1,-1,-1,-1,-1,-1,-1}},{{92,1,1,5,11,0,2,6},{91,8,1,4,11,0,2,5},{47,14,1,5,11,0,2,6},{93,21,1,4,11,0,2,5},{41,27,1,4,11,0,2,5},{40,33,1,4,11,0,2,5},{125,39,1,6,11,0,2,7},{123,47,1,6,11,0,2,7},{124,55,1,2,11,2,2,5},{36,59,1,6,11,0,2,7},{81,67,1,7,10,0,3,8},{106,76,1,3,10,0,3,4},{104,81,1,6,9,0,2,7},{100,89,1,6,9,0,2,7},{64,97,1,9,9,0,3,10},{102,108,1,4,9,0,2,5},{98,114,1,6,9,0,2,7},{107,1,14,6,9,0,2,7},{108,9,14,2,9,0,2,3},{35,13,14,8,9,0,3,9},{82,23,14,7,8,0,3,8},{121,32,14,6,8,0,5,7},{83,40,14,6,8,0,3,7},{80,48,14,6,8,0,3,7},{75,56,14,6,8,0,3,7},{84,64,14,6,8,0,3,7},{76,72,14,5,8,0,3,6},{78,79,14,6,8,0,3,7},{77,87,14,9,8,0,3,10},{85,98,14,7,8,0,3,8},{113,107,14,6,8,0,5,7},{105,115,14,2,8,0,3,3},{112,119,14,6,8,0,5,7},{103,1,25,6,8,0,5,7},{74,9,25,5,8,0,3,6},{116,16,25,4,8,0,3,5},{86,22,25,6,8,0,3,7},{90,30,25,6,8,0,3,7},{87,38,25,10,8,0,3,11},{89,50,25,6,8,0,3,7},{88,58,25,6,8,0,3,7},{79,66,25,7,8,0,3,8},{52,75,25,6,8,0,3,7},{57,83,25,6,8,0,3,7},{53,91,25,6,8,0,3,7},{55,99,25,6,8,0,3,7},{56,107,25,6,8,0,3,7},{54,115,25,6,8,0,3,7},{73,1,35,4,8,0,3,5},{33,7,35,2,8,2,3,5},{50,11,35,6,8,0,3,7},{37,19,35,12,8,0,3,13},{48,33,35,6,8,0,3,7},{49,41,35,4,8,0,3,5},{38,47,35,8,8,0,3,9},{51,57,35,6,8,0,3,7},{66,65,35,6,8,0,3,7},{71,73,35,7,8,0,3,8},{69,82,35,5,8,0,3,6},{67,89,35,6,8,0,3,7},{70,97,35,5,8,0,3,6},{68,104,35,7,8,0,3,8},{72,113,35,7,8,0,3,8},{59,122,35,2,8,0,5,3},{65,1,45,7,8,0,3,8},{63,10,45,5,8,0,3,6},{60,17,45,7,7,0,4,8},{43,26,45,7,7,0,4,8},{62,35,45,7,7,0,4,8},{115,44,45,5,6,0,5,6},{111,51,45,6,6,0,5,7},{114,59,45,4,6,0,5,5},{118,65,45,6,6,0,5,7},{122,73,45,5,6,0,5,6},{117,80,45,6,6,0,5,7},{120,88,45,6,6,0,5,7},{110,96,45,6,6,0,5,7},{119,104,45,8,6,0,5,9},{58,114,45,2,6,0,5,3},{97,118,45,6,6,0,5,7},{101,1,55,6,6,0,5,7},{99,9,55,5,6,0,5,6},{109,16,55,10,6,0,5,11},{42,28,55,6,5,0,2,7},{44,36,55,2,4,0,9,3},{94,40,55,7,4,0,3,8},{61,49,55,7,4,0,5,8},{39,58,55,2,3,0,2,3},{34,62,55,5,3,0,2,6},{126,69,55,8,3,0,6,9},{96,79,55,3,2,0,2,4},{46,84,55,2,2,0,9,3},{45,88,55,4,1,0,7,5},{95,94,55,7,1,0,12,8},{32,103,55,0,0,0,11,5},{127,105,55,0,0,0,11,8},{-1,-1,-1,-1,-1,-1,-1,-1}}};
		kerning = {{},{}};
	}
	//[0]=id
	//[1]=x
	//[2]=y
	//[3]=width
	//[4]=height
	//[5]=xoffset
	//[6]=yoffset
	//[7]=xadvance
};
