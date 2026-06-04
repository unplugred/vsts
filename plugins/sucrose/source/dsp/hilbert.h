#pragma once
#include <array>
#include "util.h"

/// @file hilbert.h
///
/// @brief An approximation of a Hilbert transformer implemented as a cascade of 2nd-order allpass
/// based on https://dsp.stackexchange.com/a/37748

/// @brief Coefficients for a Hilbert transformer
/// @tparam N Order of the filter / 2
template <int N>
struct HiirCoeffs
{
    float i[N];
    float q[N];
};

/// @brief 8th order transformer coeffs, used for oversampling
/// Transition band: 0.01
/// Stopband attenuation: -69db
const HiirCoeffs<4> HIIR8_69 = {
    {
        0.2659685265210946f,
        0.6651041532634957f,
        0.8841015085506159f,
        0.9820054141886075f,
    },
    {
        0.07711507983241622f,
        0.4820706250610472f,
        0.7968204713315797f,
        0.9412514277740471f,
    }};

/// @brief 14th order transformer coeffs, used for harmonic generation
/// Transition band: 0.001
/// Stopband attenuation: -75db
const HiirCoeffs<7> HIIR14_75 = {
    {
        0.208520598737909096f,
        0.571092619019306746f,
        0.812402367443785245f,
        0.925584736606092973f,
        0.971855071619908695f,
        0.99018771226054636f,
        0.998436446653414578f,
    },
    {
        0.0583961696557416948f,
        0.395512247218759827f,
        0.711249268432477932f,
        0.880936083750608279f,
        0.953996713311390687f,
        0.983059774008774645f,
        0.994930539951151438f,
    }};

/// @brief State of a Hilbert transformer
/// @tparam N Order of the filter / 2
/// @tparam M Number of parallel channels
template <int N, int M>
struct Hiir
{
    f32x<M> s_i[N][4] = {};
    f32x<M> s_q[N][4] = {};
    f32x<M> d_i = {};

    inline std::array<f32x<M>, 2> run(const f32x<M> &x, HiirCoeffs<N> coeffs)
    {
        auto i = x;
        auto q = x;

        // allpass cascade
        for (int n = 0; n < N; ++n)
        {
            allpass2(i, coeffs.i[n], s_i[n]);
            allpass2(q, coeffs.q[n], s_q[n]);
        }

        // z^-1
        auto y_i = d_i;
        d_i = i;
        i = y_i;

        return {i, q};
    }

    inline f32x<M> run_lp(const f32x<M> &x, HiirCoeffs<N> coeffs)
    {
        auto i = x;
        auto q = x;

        // allpass cascade
        for (int n = 0; n < N; ++n)
        {
            allpass2_lp(i, coeffs.i[n], s_i[n]);
            allpass2_lp(q, coeffs.q[n], s_q[n]);
        }

        // z^-1
        auto y_i = d_i;
        d_i = i;
        i = y_i;

        return (i + q) * f32x<M>(0.5f);
    }

private:
    static inline void allpass2(f32x<M> &x, float c, f32x<M> s[4])
    {
        auto y = f32x<M>(c) * (x + s[1]) - s[3];
        s[1] = s[0];
        s[0] = y;
        s[3] = s[2];
        s[2] = x;
        x = y;
    }

    static inline void allpass2_lp(f32x<M> &x, float c, f32x<M> s[4])
    {
        auto y = f32x<M>(c) * (x - s[1]) + s[3];
        s[1] = s[0];
        s[0] = y;
        s[3] = s[2];
        s[2] = x;
        x = y;
    }
};

/// @brief State of a Hilbert-based oversampling filter (halfband polyphase IIR)
/// @tparam N Order of the filter / 2
template <int N>
struct Hiir2
{
    float s_i[N][2] = {};
    float s_q[N][2] = {};
    float d_i = {};

    inline std::array<float, 2> run_up(float x, HiirCoeffs<N> coeffs)
    {
        float s0 = x;
        float s1 = x;

        // allpass cascade
        for (int n = 0; n < N; ++n)
        {
            s1 = allpass1(s1, coeffs.i[n], s_i[n]);
            s0 = allpass1(s0, coeffs.q[n], s_q[n]);
        }

        return {s0, s1};
    }

    inline float run_down(float s0, float s1, HiirCoeffs<N> coeffs)
    {
        // allpass cascade
        for (int n = 0; n < N; ++n)
        {
            s1 = allpass1(s1, coeffs.i[n], s_i[n]);
            s0 = allpass1(s0, coeffs.q[n], s_q[n]);
        }

        // z^-1
        float y_i = d_i;
        d_i = s1;
        s1 = y_i;

        return (s0 + s1) * 0.5f;
    }

private:
    static inline float allpass1(float x, float c, float s[2])
    {
        float y = c * (x - s[1]) + s[0];
        s[0] = x;
        s[1] = y;
        return y;
    }
};
