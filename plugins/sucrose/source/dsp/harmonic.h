#pragma once
#include "hilbert.h"
#include <cmath>

template <int N>
struct HarmonicResult
{
    f32x<N> oct2;
    f32x<N> oct3;
    f32x<N> sub2;
};

template <int N>
struct HarmonicGen
{
    Hiir<7, N> hiir;
    bool side[N] = {};
    bool sign[N] = {};

    HarmonicGen() : hiir() {}

    inline HarmonicResult<N> run(const f32x<N> &x)
    {
        // real & imag of an analytic signal (with phase shift)
        auto Q = hiir.run(x, HIIR14_75);
        auto r = Q[0];
        auto i = Q[1];

        auto r2 = r * r;
        auto i2 = i * i;
        auto mag = (r2 + i2).sqrt();
        auto imag = f32x<N>(1.0f) / mag.max(f32x<N>(1e-5f)); // avoid divide by zero

        // let Q be the complex analytic input signal given by the hilbert transform
        // oct2 = Re(Q^2/|Q|) - twice the angle without changing the magnitude, and get the real part to get the output
        // oct3 = Re(Q^3/|Q|^2) - same thing but triple the angle and divide by the magnitude squared to keep the same magnitude
        auto oct2 = (r2 - i2) * imag;
        auto oct3 = r * (r2 - f32x<N>(3.0f) * i2) * imag * imag;
        auto sub2 = ((i2 + r2 + mag * r) * 0.5f).max(f32x<N>(0.0f)).sqrt(); // prevent nans due to precision issues

        // a flip-flop used to keep track of the sign for suboctaving, otherwise it is phase-aligned to our analytic signal
        // this is in fact the main source of glitchiness
        for (int n = 0; n < N; ++n)
        {
            bool reset = mag[n] < 1e-5f;
            bool cross = (side[n] ^ (i[n] > 0.0f)) & r[n] < 0.0f;
            sign[n] = (sign[n] ^ cross) & !reset;
            side[n] = i[n] > 0.0f;
            sub2[n] = sign[n] ? -sub2[n] : sub2[n];
        }

        return HarmonicResult<N>{oct2, oct3, sub2};
    }
};