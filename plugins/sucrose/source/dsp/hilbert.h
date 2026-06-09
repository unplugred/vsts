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

/// @brief 8th order transformer coeffs, used for oversampling (x2)
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

/// @brief 4th order transformer coeffs, used for oversampling (x4)
/// Transition band: 0.261666
/// Stopband attenuation: -120db
const HiirCoeffs<2> HIIR4_120 = {
    {0.1665604775598164f, 0.74155297339931314f},
    {0.041180778598107023f, 0.38702422374344198f}};

/// @brief 12th order transformer coeffs, used for harmonic generation (clean)
/// Transition band: 0.00115
/// Stopband attenuation: -70db
const HiirCoeffs<6> HIIR12_70 = {
    {
        0.258620263567817754f,
        0.653045995906771037f,
        0.870867102105314927f,
        0.956217919035841302f,
        0.986304981768198696f,
        0.997949470566919294f,
    },
    {
        0.0746709346961870191f,
        0.471175379274842487f,
        0.78462034608944331f,
        0.924237993384099288f,
        0.975104877836096229f,
        0.993202838043383607f,
    }};

/// @brief 14th order transformer coeffs, used for harmonic generation (dirty)
/// Transition band: 0.0005
/// Stopband attenuation: -70db
const HiirCoeffs<7> HIIR14_70 = {
    {
        0.243678614141544375f,
        0.630177510841846589f,
        0.855633041082200174f,
        0.948577557225476875f,
        0.982382795199339021f,
        0.994349983611077515f,
        0.999140192497308233f,
    },
    {
        0.0697089125778496826f,
        0.449303733605533995f,
        0.764860516499448861f,
        0.913262353580889807f,
        0.969782280828782040f,
        0.989859175494667509f,
        0.997166242289061033f,
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

    inline std::array<f32x<M>, 2> run(const f32x<M> &x, const HiirCoeffs<N> &coeffs)
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

    inline f32x<M> run_lp(const f32x<M> &x, const HiirCoeffs<N> &coeffs)
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

    inline std::array<float, 2> run_up(float x, const HiirCoeffs<N> &coeffs)
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

    inline float run_down(float s0, float s1, const HiirCoeffs<N> &coeffs)
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
