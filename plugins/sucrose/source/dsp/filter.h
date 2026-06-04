#pragma once
#include "util.h"
#include <cmath>
#include <array>

/// @file filter.h
///
/// @brief An IIR bandpass filterbank implementation based on this paper:
/// https://iwaenc.org/proceedings/2010/HTML/Uploads/975.pdf
///
/// State is separate from the coefficients so we can share those between multiple channels.

/// @brief Coefficients for a single 2nd order state variable filter
/// @tparam T Sample type
template <typename T>
struct SVF
{
    T g, a3;

    SVF() = default;

    /// @param freq Normalized frequency (0 to 0.5)
    /// @param q Q factor (critical @ 0.707)
    SVF(float freq, float q)
    {
        freq = std::clamp(freq, 1e-5f, 0.5f - 1e-4f); // guard against instabilities if sample rate is too low
        g = std::tan(3.14159265359f * (0.5f - freq));
        a3 = q / (q + q * g * g + g);
    }

    inline std::array<T, 2> run(const T &x, T s[2]) const
    {
        auto h2 = g * s[0] + x - s[1];
        auto h0 = a3 * h2;
        auto h1 = g * h0;

        auto lp = s[1] + h0;
        auto hp = g * (h1 - s[0]);

        s[0] = h1 + h1 - s[0];
        s[1] = h0 + h0 + s[1];

        return {lp, hp};
    }
};

template <typename T>
inline std::array<T, 2> lr4_split(const T &x, const SVF<T> &svf, T state[4])
{
    auto c0 = svf.run(x, state);
    auto ap = c0[0] + c0[0] + c0[1] + c0[1] - x;
    auto c1 = svf.run(c0[0], state + 2);
    auto lp2 = c1[0];

    return {lp2, ap - lp2};
}

template <typename T>
inline T lr4_comp(const T &x, const SVF<T> &svf, T state[2])
{
    auto c0 = svf.run(x, state);
    return c0[0] + c0[0] + c0[1] + c0[1] - x;
}

/// @brief State of an 8-band Linkwitz-Riley (4th order) filterbank
struct LR4Bank8
{
    float comp1[6][2];
    float split1[3][4];

    f32x<4> comp2[2];
    f32x<4> split2[4];

    inline f32x<8> run(float x, SVF<float> coeffs[7])
    {
        SVF<f32x<4>> c2;
        c2.a3 = {{coeffs[0].a3, coeffs[2].a3, coeffs[4].a3, coeffs[6].a3}};
        c2.g = {{coeffs[0].g, coeffs[2].g, coeffs[4].g, coeffs[6].g}};

        auto [u1_l, u1_h] = lr4_split(x, coeffs[3], split1[0]);

        u1_h = lr4_comp(u1_h, coeffs[0], comp1[0]);
        u1_h = lr4_comp(u1_h, coeffs[1], comp1[1]);
        u1_h = lr4_comp(u1_h, coeffs[2], comp1[2]);
        u1_l = lr4_comp(u1_l, coeffs[4], comp1[3]);
        u1_l = lr4_comp(u1_l, coeffs[5], comp1[4]);
        u1_l = lr4_comp(u1_l, coeffs[6], comp1[5]);

        auto [u2_1, u2_0] = lr4_split(u1_l, coeffs[1], split1[1]);
        auto [u2_3, u2_2] = lr4_split(u1_h, coeffs[5], split1[2]);

        f32x<4> u2 = {{u2_0, u2_1, u2_2, u2_3}};
        u2 = lr4_comp(u2, c2, comp2);

        auto [u3_l, u3_h] = lr4_split(u2.rotate2(), c2, split2);
        return u3_l.interleave(u3_h);
    }

    static void design(SVF<float> coeffs[7], float sample_rate)
    {
        const float freqs[7] = {
            229.6875, 459.375, 918.75, 1837.5, 3675.0, 7350.0, 11025.0};

        for (int i = 0; i < 7; ++i)
        {
            coeffs[i] = SVF<float>(freqs[i] / sample_rate, 0.7071f);
        }
    }
};

/// @brief State of a 16-band Linkwitz-Riley (4th order) filterbank
struct LR4Bank16
{
    float comp1[14][2];
    float split1[3][4];

    f32x<4> comp2[3][2];
    f32x<4> split2[4];

    f32x<8> comp3[2];
    f32x<8> split3[4];

    inline f32x<16> run(float x, SVF<float> coeffs[15])
    {
        SVF<f32x<4>> c2_0, c2_1, c2_2;
        SVF<f32x<8>> c3;

        c2_0.a3 = {{coeffs[0].a3, coeffs[4].a3, coeffs[8].a3, coeffs[12].a3}};
        c2_0.g = {{coeffs[0].g, coeffs[4].g, coeffs[8].g, coeffs[12].g}};
        c2_1.a3 = {{coeffs[1].a3, coeffs[5].a3, coeffs[9].a3, coeffs[13].a3}};
        c2_1.g = {{coeffs[1].g, coeffs[5].g, coeffs[9].g, coeffs[13].g}};
        c2_2.a3 = {{coeffs[2].a3, coeffs[6].a3, coeffs[10].a3, coeffs[14].a3}};
        c2_2.g = {{coeffs[2].g, coeffs[6].g, coeffs[10].g, coeffs[14].g}};
        c3.a3 = {{coeffs[0].a3, coeffs[2].a3, coeffs[4].a3, coeffs[6].a3,
                  coeffs[8].a3, coeffs[10].a3, coeffs[12].a3, coeffs[14].a3}};
        c3.g = {{coeffs[0].g, coeffs[2].g, coeffs[4].g, coeffs[6].g,
                 coeffs[8].g, coeffs[10].g, coeffs[12].g, coeffs[14].g}};

        auto [u0, u1] = lr4_split(x, coeffs[7], split1[0]);

        lr4_comp(u1, coeffs[0], comp1[0]);
        lr4_comp(u1, coeffs[1], comp1[1]);
        lr4_comp(u1, coeffs[2], comp1[2]);
        lr4_comp(u1, coeffs[3], comp1[3]);
        lr4_comp(u1, coeffs[4], comp1[4]);
        lr4_comp(u1, coeffs[5], comp1[5]);
        lr4_comp(u1, coeffs[6], comp1[6]);
        lr4_comp(u0, coeffs[8], comp1[7]);
        lr4_comp(u0, coeffs[9], comp1[8]);
        lr4_comp(u0, coeffs[10], comp1[9]);
        lr4_comp(u0, coeffs[11], comp1[10]);
        lr4_comp(u0, coeffs[12], comp1[11]);
        lr4_comp(u0, coeffs[13], comp1[12]);
        lr4_comp(u0, coeffs[14], comp1[13]);

        auto [u2_1, u2_0] = lr4_split(u0, coeffs[3], split1[1]);
        auto [u2_3, u2_2] = lr4_split(u1, coeffs[11], split1[2]);

        f32x<4> u2 = {{u2_0, u2_1, u2_2, u2_3}};
        u2 = lr4_comp(u2, c2_0, comp2[0]);
        u2 = lr4_comp(u2, c2_1, comp2[1]);
        u2 = lr4_comp(u2, c2_2, comp2[2]);

        auto [u3_l, u3_h] = lr4_split(u2.rotate2(), c2_1, split2);
        f32x<8> u3 = u3_h.interleave(u3_l);

        u3 = lr4_comp(u3, c3, comp3);
        auto [u4_l, u4_h] = lr4_split(u3.rotate2(), c3, split3);
        return u4_l.interleave(u4_h);
    }

    static void design(SVF<float> coeffs[15], float sample_rate)
    {
        const float freqs[15] = {
            174.614,
            232.818,
            310.424,
            413.899,
            551.865,
            735.823,
            981.098,
            1308.13,
            1744.19,
            2325.58,
            3100.78,
            4134.36,
            5512.50,
            7350.00,
            11025.0,
        };

        for (int i = 0; i < 15; ++i)
        {
            coeffs[i] = SVF<float>(freqs[i] / sample_rate, 0.7071f);
        }
    }
};
