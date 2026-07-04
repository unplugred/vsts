#pragma once
#include "hilbert.h"
#include "util.h"

/// A phase-locked loop based subharmonic generator.
template <int N>
struct PhaseLocked
{
    f32x<N> phase = 0.0f;
    f32x<N> freq = 0.0f;
    f32x<N> corr = 0.0f;

    inline std::array<f32x<N>, 2> run(const f32x<N> &x, const f32x<N> &y)
    {
        const auto freq_beta = 0.0625f / (3.14159265358979323846f); // frequency adjustment factor
        const auto phase_beta = 0.25f / (3.14159265358979323846f);  // phase adjustment factor

        // current oscillator output
        auto [cx, cy] = approx_sin_cos_tau(phase - 0.5f);

        // multiply phase by 2 (to lock onto the f/2 subharmonic)
        auto cx2 = f32x<N>(0.5f) - cx * cx;
        auto cy2 = cx * cy;

        // phase error between the input signal and the oscillator output
        // Im(x * conj(y)), originally this was atan2 but it was expensive & this approximates it well enough
        auto error = cx2 * y - cy2 * x;

        // adjust frequency and phase based on the error
        freq = freq + error * freq_beta;
        phase = phase + error * phase_beta + freq;
        phase = phase - phase.floor(); // wrap phase to avoid accumulating errors

        // bands have +-90 degree offset between neighboring bands (to avoid full cancellation)
        for (int i = 0; i < N; i += 2)
            std::swap(cx[i], cy[i]);

        return {cx, cy};
    }
};

/// @brief Hilbert-based (sub)harmonic generator
/// @tparam H Hilbert order / 2
/// @tparam N Number of parallel channels
template <int H, int N>
struct HarmonicGen
{
    Hiir<H, N> hiir = {};
    PhaseLocked<N> pll = {};

    inline std::array<f32x<N>, 3> run(const f32x<N> &x, const HiirCoeffs<H> &coeffs, int channel)
    {
        // real & imag of an analytic signal (with phase shift)
        auto [r, i] = hiir.run(x, coeffs);

        // now we split the signal into its magnitude `mag` and phase `p`, the original signal is `r = mag * p`
        // this is the magnitude of our signal
        auto mag = (r * r + i * i).sqrt();
        auto mag_inv = f32x<N>(1.0f) / mag.max(f32x<N>(1e-8f)); // avoid divide by zero

        auto p = r * mag_inv; // this is the phase signal as a sine wave bounded by -1..1
        auto q = i * mag_inv; // H(p), the quadrature signal
        auto p2 = p * p;      // p^2

        // double the frequency, keep the magnitude
        auto oct2 = p2 * 2.0f * mag - mag;
        // triple the frequency, keep the magnitude
        auto oct3 = (p2 * 4.0f - 3.0f) * p * mag;
        // suboctave is tracked by using N parallel phase-locked loops
        auto [sub2x, sub2y] = pll.run(p, q);

        // channels have 90 degree phase offset (to avoid full cancellation)
        auto sub2 = ((channel & 1) == 0) ? sub2x * mag : sub2y * mag;

        return {sub2, oct2, oct3};
    }
};
