from PIL import Image
from math import *
im = Image.open('../resources/text.png')
pix = im.load()
lw = int(im.size[0]/16)
lh = int(im.size[1]/16)

values = []
for y in range(16):
	for x in range(16):
		id = x+y*16
		values.append([0,0,0,0,0,0,0,0])
		for ly in range(lh):
			for lx in range(lw):
				rlx = lx/lw
				rly = ly/lh
				sub = floor(rlx*2)*2+floor(rly*2)*4+(1 if fmod(rlx,.5) >= (.125 if fmod(rly,.5) >= .25 else .375) else 0)
				#sub += fmod(rlx,.5)>=(fmod(rly,.5)>=.25?.125:.375)?1:0
				values[id][sub] += pix[x*lw+lx,y*lh+ly]/255

characters = [
	' ','☺','☻','♥','♦','♣','♠','•','◘','○','◙','♂','♀','♪','♫','☼',
	'►','◄','↕','‼','¶','§','▬','↨','↑','↓','→','←','∟','↔','▲','▼',
	' ','!','"','#','$','%','&','\'','(',')','*','+',',','-','.','/',
	'0','1','2','3','4','5','6','7','8','9',':',';','<','=','>','?',
	'@','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W','X','Y','Z','[','\\',']','^','_',
	'`','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
	'p','q','r','s','t','u','v','w','x','y','z','{','|','}','~','⌂',
	'Ç','ü','é','â','ä','à','å','ç','ê','ë','è','ï','î','ì','Ä','Å',
	'É','æ','Æ','ô','ö','ò','û','ù','ÿ','Ö','Ü','¢','£','¥','₧','ƒ',
	'á','í','ó','ú','ñ','Ñ','ª','º','¿','⌐','¬','½','¼','¡','«','»',
	'░','▒','▓','│','┤','╡','╢','╖','╕','╣','║','╗','╝','╜','╛','┐',
	'└','┴','┬','├','─','┼','╞','╟','╚','╔','╩','╦','╠','═','╬','╧',
	'╨','╤','╥','╙','╘','╒','╓','╫','╪','┘','┌','█','▄','▌','▐','▀',
	'ɑ','ϐ','ᴦ','ᴨ','∑','ơ','µ','ᴛ','ɸ','ϴ','Ω','ẟ','∞','∅','∈','∩',
	'≡','±','≥','≤','⌠','⌡','÷','≈','°','∙','·','√','ⁿ','²','■',' ']

subpixel = []
bright = []
for i in range(256):
	byte = i.to_bytes(1, byteorder='big')[0]
	bits = []
	for b in range(8):
		bits.append(8 if (byte >> b) & 1 else 0)

	sl = -1
	sv = 1000
	bl = -1
	bv = 1000
	for l in range (32,126):
		diff = 0
		for b in range(8):
			diff += abs(values[l][b]-bits[b])
		if diff < sv or (diff <= sv and (l not in subpixel)):
			sl = l
			sv = diff

		diff = abs((i*27/256)-sum(values[l]))
		if diff < bv or (diff <= bv and (l not in bright)):
			bl = l
			bv = diff

	subpixel.append(sl)
	bright.append(bl)

for i in range(256):
	subpixel[i] = characters[subpixel[i]]
	bright[i] = characters[bright[i]]

im = Image.open('../resources/noise.png')
pix = im.load()
bluenoise = []
for y in range(24):
	for x in range(24):
		for b in range(8):
			bluenoise.append(pix[x*4+b%4,y*2+floor(b/4)][0])

with open("out.txt","w") as f:
	f.write("	char subpixel_map[256] = {"+str(subpixel)[1:-1].replace(', ',',').replace('"\'"',"\'\\\'\'")+"};\n	char bright_map[256] = {"+str(bright)[1:-1].replace(', ',',').replace('"\'"',"\'\\\'\'")+"};\n	unsigned char blue_noise[576*8] = {"+str(bluenoise)[1:-1].replace(', ',',')+"};")
