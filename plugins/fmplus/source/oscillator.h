template<typename T> int sgn(T val) {
	return (T(0)<val)-(val<T(0));
}
struct morphosc {
	float shape = .5f;
	float val = .5f;
	float gc = 1;
	void update(float s) {
		if((s*2) == shape) return;
		shape = s*2;
		if(shape <= 1) {
			val = 1-shape;
			gc = (1-powf(1-pow(val,1.74f),1.34f))*1.79f-3.04f;
			val = .5f-3.f*val*val;
		} else {
			gc = 1;
			val = .5f;
		}
	}
	float calc(float x) {
		float osc = x+x*x*x*(val*x*x-1.f-val);
		if(shape <= 1) return osc*gc;
		return (pow(1-fabs(osc*3.04f),shape)-1)*sgn(osc);
	}
};
