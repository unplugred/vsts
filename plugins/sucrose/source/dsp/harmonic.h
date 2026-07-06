#pragma once
#include "hilbert.h"
#include "util.h"

/// A phase-locked loop based subharmonic generator.
template <int H, int N>
struct PhaseLocked
{
    Hiir<H, N> hiir = {};
    f32x<N> phase = 0.0f;
    f32x<N> freq = 0.0f;

    inline f32x<N> run(const f32x<N> &z, const HiirCoeffs<H> &coeffs)
    {
        const auto freq_beta = 0.0625f / (3.14159265358979323846f); // frequency adjustment factor
        const auto phase_beta = 0.25f / (3.14159265358979323846f);  // phase adjustment factor

        // quadrature signal (Hilbert transform)
        auto [x, y] = hiir.run(z, coeffs);

        // same steps as in [`HarmonicGen`] to get the phase signal `p` and quadrature signal `q`
        auto mag = (x * x + y * y).sqrt();
        auto mag_inv = f32x<N>(1.0f) / mag.max(f32x<N>(1e-8f)); // avoid divide by zero

        auto p = x * mag_inv; // this is the phase signal as a sine wave bounded by -1..1
        auto q = y * mag_inv; // H(p), the quadrature signal

        // current oscillator output
        auto [cx, cy] = approx_sin_cos_tau(phase - 0.5f);

        // multiply phase by 2 (to lock onto the f/2 subharmonic)
        auto cx2 = f32x<N>(0.5f) - cx * cx;
        auto cy2 = cx * cy;

        // phase error between the input signal and the oscillator output
        // Im(x * conj(y)), originally this was atan2 but it was expensive & this approximates it well enough
        auto error = cx2 * p - cy2 * q;

        // adjust frequency and phase based on the error
        freq = freq + error * freq_beta;
        phase = phase + error * phase_beta + freq;
        phase = phase - phase.floor(); // wrap phase to avoid accumulating errors

        // bands have +-90 degree offset between neighboring bands (to avoid full cancellation)
        for (int i = 0; i < N; i += 2)
            std::swap(cx[i], cy[i]);

        return cy * mag; // restore magnitude
    }
};

/// @brief Hilbert-based harmonic generator
/// @tparam H Hilbert order / 2
/// @tparam N Number of parallel channels
template <int H, int N>
struct HarmonicGen
{
    Hiir<H, N> hiir = {};

    inline std::array<f32x<N>, 2> run(const f32x<N> &x, const HiirCoeffs<H> &coeffs)
    {
        // real & imag of an analytic signal (with phase shift)
        auto [r, i] = hiir.run(x, coeffs);
        auto r2 = r * r;
        auto i2 = i * i;

        auto magi2 = f32x<N>(1.0f) / (r2 + i2).max(f32x<N>(1e-14f)); // avoid divide by zero

        // let Q be the complex analytic input signal given by the hilbert transform
        // oct2 = Re(Q^2/|Q|) - twice the angle without changing the magnitude, and get the real part to get the output
        // oct3 = Re(Q^3/|Q|^2) - same thing but triple the angle and divide by the magnitude squared to keep the same magnitude
        auto oct2 = (r2 - i2) * magi2.sqrt();
        auto oct3 = (r2 - f32x<N>(3.0f) * i2) * r * magi2;

        return {oct2, oct3};
    }
};
