// based on https://github.com/SRombauts/SimplexNoise/blob/master/src/SimplexNoise.cpp

#include "perlin.h"

void perlin::init() {
	for(int i = 0; i < 256; ++i) perm[i] = i;
	Random random;
	int n = 256;
	while(n > 1) {
		int k = random.nextInt(n--);
		uint8_t temp = perm[n];
		perm[n] = perm[k];
		perm[k] = temp;
	}
}
float perlin::grad(int32_t hash, float xin) {
	const int32_t h = hash&0x0F;
	float grad = 1+(h&7);
	if((h&8) != 0) grad = -grad;
	return (grad*xin);
}
float perlin::grad(int32_t hash, float xin, float yin) {
	const int32_t h = hash&0x3F;
	const float u = h<4?xin:yin;
	const float v = h<4?yin:xin;
	return ((h&1)?-u:u)+((h&2)?-2*v:2*v);
}
float perlin::grad(int32_t hash, float xin, float yin, float zin) {
	int h = hash&15;
	float u = h<8?xin:yin;
	float v = h<4?yin:h==12||h==14?xin:zin;
	return ((h&1)?-u:u)+((h&2)?-v:v);
}
uint8_t perlin::hash(int32_t i) {
	return perm[static_cast<uint8_t>(i)];
}
float perlin::noise(float xin) {
	float n0, n1;

	int32_t i0 = floor(xin);
	int32_t i1 = i0+1;
	float x0 = xin-i0;
	float x1 = x0-1;

	float t0 = 1-x0*x0;
	t0 *= t0;
	n0 = t0*t0*grad(hash(i0),x0);

	float t1 = 1-x1*x1;
	t1 *= t1;
	n1 = t1*t1*grad(hash(i1),x1);

	return .395f*(n0+n1);
}
float perlin::noise(float xin, float yin) {
	float n0, n1, n2;

	static const float F2 = .366025403f;
	static const float G2 = .211324865f;

	const float s = (xin+yin)*F2;
	const float xs = xin+s;
	const float ys = yin+s;
	const int32_t i = floor(xs);
	const int32_t j = floor(ys);

	const float t = static_cast<float>(i+j)*G2;
	const float X0 = i-t;
	const float Y0 = j-t;
	const float x0 = xin-X0;
	const float y0 = yin-Y0;

	int32_t i1, j1;
	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	} else {
		i1 = 0;
		j1 = 1;
	}

	const float x1 = x0-i1+G2;
	const float y1 = y0-j1+G2;
	const float x2 = x0-1+2*G2;
	const float y2 = y0-1+2*G2;

	const int gi0 = hash(i+hash(j));
	const int gi1 = hash(i+i1+hash(j+j1));
	const int gi2 = hash(i+1+hash(j+1));

	float t0 = .5f-x0*x0-y0*y0;
	if (t0 < 0) {
		n0 = 0;
	} else {
		t0 *= t0;
		n0 = t0*t0*grad(gi0,x0,y0);
	}

	float t1 = .5f-x1*x1-y1*y1;
	if (t1 < 0) {
		n1 = 0;
	} else {
		t1 *= t1;
		n1 = t1*t1*grad(gi1,x1,y1);
	}

	float t2 = .5f-x2*x2-y2*y2;
	if (t2 < 0) {
		n2 = 0;
	} else {
		t2 *= t2;
		n2 = t2*t2*grad(gi2,x2,y2);
	}

	return 45.23065f*(n0+n1+n2);
}
float perlin::noise(float xin, float yin, float zin) {
	float n0, n1, n2, n3;

	static const float F3 = 1/3.f;
	static const float G3 = 1/6.f;

	float s = (xin+yin+zin)*F3;
	int i = floor(xin+s);
	int j = floor(yin+s);
	int k = floor(zin+s);
	float t = (i+j+k)*G3;
	float X0 = i-t;
	float Y0 = j-t;
	float Z0 = k-t;
	float x0 = xin-X0;
	float y0 = yin-Y0;
	float z0 = zin-Z0;

	int i1, j1, k1;
	int i2, j2, k2;
	if(x0 >= y0) {
		if(y0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} else if(x0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} else {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		}
	} else {
		if(y0 < z0) {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} else if(x0 < z0) {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} else {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		}
	}

	float x1 = x0-i1+G3;
	float y1 = y0-j1+G3;
	float z1 = z0-k1+G3;
	float x2 = x0-i2+2*G3;
	float y2 = y0-j2+2*G3;
	float z2 = z0-k2+2*G3;
	float x3 = x0-1+3*G3;
	float y3 = y0-1+3*G3;
	float z3 = z0-1+3*G3;

	int gi0 = hash(i+hash(j+hash(k)));
	int gi1 = hash(i+i1+hash(j+j1+hash(k+k1)));
	int gi2 = hash(i+i2+hash(j+j2+hash(k+k2)));
	int gi3 = hash(i+1+hash(j+1+hash(k+1)));

	float t0 = .6f-x0*x0-y0*y0-z0*z0;
	if (t0 < 0) {
		n0 = 0;
	} else {
		t0 *= t0;
		n0 = t0*t0*grad(gi0,x0,y0,z0);
	}
	float t1 = .6f-x1*x1-y1*y1-z1*z1;
	if (t1 < 0) {
		n1 = 0;
	} else {
		t1 *= t1;
		n1 = t1*t1*grad(gi1,x1,y1,z1);
	}
	float t2 = .6f-x2*x2-y2*y2-z2*z2;
	if (t2 < 0) {
		n2 = 0;
	} else {
		t2 *= t2;
		n2 = t2*t2*grad(gi2,x2,y2,z2);
	}
	float t3 = .6f-x3*x3-y3*y3-z3*z3;
	if (t3 < 0) {
		n3 = 0;
	} else {
		t3 *= t3;
		n3 = t3*t3*grad(gi3,x3,y3,z3);
	}
	return 32*(n0+n1+n2+n3);
}
