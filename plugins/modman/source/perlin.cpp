#include "perlin.h"
#define F2 .366025403
#define G2 .211324865

double perlin::dot(double g[], double x, double y) { return g[0]*x+g[1]*y; }
void perlin::init() {
	for (int i = 0; i < 512; i++)
		preperm[i] = floor(random.nextFloat()*256);
	for (int i = 0; i < 512; i++)
		perm[i] = preperm[i&255];
}
double perlin::noise(double xin, double yin) {
	double n0, n1, n2;
	double s = (xin+yin)*F2;
	int i = floor(xin+s);
	int j = floor(yin+s);
	double t = (i+j)*G2;
	double x0 = xin-(i-t);
	double y0 = yin-(j-t);
	int i1, j1;
	if(x0>y0) { i1=1; j1=0; }
	else { i1=0; j1=1; }
	double x1 = x0-i1+G2;
	double y1 = y0-j1+G2;
	double x2 = x0-1+2*G2;
	double y2 = y0-1+2*G2;
	int ii = i&0xff;
	int jj = j&0xff;
	double t0 = .5f-x0*x0-y0*y0;
	if(t0 < 0.f) n0 = 0.f;
	else {
		t0 *= t0;
		n0 = t0*t0*dot(grad3[perm[ii+perm[jj]]%12],x0,y0);
	}
	double t1 = .5f-x1*x1-y1*y1;
	if(t1 < 0) n1 = 0;
	else {
		t1 *= t1;
		n1 = t1*t1*dot(grad3[perm[ii+i1+perm[jj+j1]]%12],x1,y1);
	}
	double t2 = .5f-x2*x2-y2*y2;
	if(t2 < 0) n2 = 0;
	else {
		t2 *= t2;
		n2 = t2*t2*dot(grad3[perm[ii+1+perm[jj+1]]%12],x2,y2);
	}
	return 70*(n0+n1+n2);
}
