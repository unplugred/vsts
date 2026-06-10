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

/// @brief Hilbert-based (sub)harmonic generator
/// @tparam H Hilbert order / 2
/// @tparam N Number of parallel channels
template <int H, int N>
struct HarmonicGen
{
    Hiir<H, N> hiir = {};
    f32x<N> prev_i = {};
    f32x<N> prev_r = {};
    bool sign[N] = {};

    HarmonicGen() {}

    inline HarmonicResult<N> run(const f32x<N> &x, const HiirCoeffs<H> &coeffs)
    {
        // real & imag of an analytic signal (with phase shift)
        auto [r, i] = hiir.run(x, coeffs);

        // now we split the signal into its magnitude `mag` and phase `p`, the original signal is `r = mag * p`
        // this is the magnitude of our signal
        auto mag = (r * r + i * i).sqrt();

        // this is the phase signal as a sine wave bounded by -1..1
        auto p = r / mag.max(f32x<N>(1e-12f)); // avoid divide by zero
        auto p2 = p * p;                       // p^2
        auto p2_2 = p2 + p2;                   // 2p^2

        // double the frequency, keep the magnitude
        auto oct2 = p2_2 * mag - mag;
        // triple the frequency, keep the magnitude
        auto oct3 = (p2_2 + p2_2 - 3.0f) * p * mag;
        // half the frequency (modulo sign), keep the magnitude, max(0.0) to prevent nans due to precision issues
        auto sub2 = (p + 1.0f).max(f32x<N>(0.0f)).sqrt() * mag * 0.70710678f;

        // a flip-flop used to keep track of the sign for suboctaving, otherwise it is phase-aligned to our analytic signal
        // this is in fact the main source of glitchiness
        auto delta = prev_i * r - prev_r * i;
        for (int n = 0; n < N; ++n)
        {
            bool cross_dn = prev_i[n] >= 0.0f && i[n] < 0.0f;
            bool cross_up = prev_i[n] < 0.0f && i[n] >= 0.0f;
            bool positive = delta[n] > 0.0f;
            bool cross = positive ? cross_up : cross_dn;

            sign[n] = sign[n] ^ cross;
            sub2[n] = sign[n] ? -sub2[n] : sub2[n];
        }

        prev_i = i;
        prev_r = r;

        int consensus = 0;
        for (int n = 0; n < N; ++n)
            if (mag[n] >= 1e-5f)
                consensus += sign[n] ? 1 : -1;
        for (int n = 0; n < N; ++n)
            if (mag[n] < 1e-5f)
                sign[n] = consensus > 0;

        return HarmonicResult<N>{oct2, oct3, sub2};
    }
};