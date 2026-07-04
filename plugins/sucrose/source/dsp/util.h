#pragma once
#include <cmath>
#include <array>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

/// @brief In-place fade-in of a buffer, with a given ramp and gain state.
/// Gain is clamped to 0..1 and updated in-place.
inline void fadeout(float *const *x, int offset, int samples, int channels, float ramp, float &gain)
{
    float current = gain;
    for (int c = 0; c < channels; ++c)
    {
        current = gain;
        for (int i = offset; i < offset + samples; ++i)
        {
            current = std::clamp(current + ramp, 0.0f, 1.0f);
            x[c][i] *= current;
        }
    }

    gain = current;
}

/// @brief A simple N-wide SIMD-like type for float, aligned to 4N bytes.
/// We just hope that the compiler will optimize this to actual SSE/SSE2 (available on -march=x86-64)
/// instructions with a little bit of help from us.
/// Makes our life a little easier without having to deal w/ intrinsics
template <int N>
struct alignas(N * sizeof(float)) f32x
{
    float data[N];

    f32x(float x = 0.0f)
    {
        for (int i = 0; i < N; ++i)
            data[i] = x;
    }

    f32x(const std::array<float, N> &x)
    {
        for (int i = 0; i < N; ++i)
            data[i] = x[i];
    }

    inline f32x<2 * N> interleave(const f32x<N> &other) const
    {
        f32x<2 * N> result;
        for (int i = 0; i < N; ++i)
        {
            result.data[2 * i] = data[i];
            result.data[2 * i + 1] = other.data[i];
        }
        return result;
    }

    /// Turns 0 1 2 3 ... into 1 0 3 2
    inline f32x<N> rotate2() const
    {
        f32x<N> result;
        for (int i = 0; i < N; i += 2)
        {
            result.data[i] = data[i + 1];
            result.data[i + 1] = data[i];
        }
        return result;
    }

    inline float sum() const
    {
        float result = 0.0f;
        for (int i = 0; i < N; ++i)
            result += data[i];
        return result;
    }

    inline f32x max(const f32x &other) const
    {
        f32x result;
        for (int i = 0; i < N; ++i)
            result.data[i] = std::max(data[i], other.data[i]);
        return result;
    }

    // there is also rsqrt but we dont use it because it is _noticeably_ less accurate
    inline f32x sqrt() const
    {
// sqrt doesnt seem to be auto-vectorized that well, checked in godbolt, so we do it manually
//
// _mm_sqrt_ps is available on baseline x86-64 so we should be ok
#if defined(__SSE__)
        if constexpr (N % 4 == 0)
        {
            f32x result;
            for (int i = 0; i < N; i += 4)
            {
                __m128 x = _mm_load_ps(&data[i]);
                __m128 y = _mm_sqrt_ps(x);
                _mm_store_ps(&result.data[i], y);
            }
            return result;
        }
#endif

        f32x result;
        for (int i = 0; i < N; ++i)
            result.data[i] = std::sqrt(data[i]);
        return result;
    }

    inline f32x abs() const
    {
        f32x result;
        for (int i = 0; i < N; ++i)
            result.data[i] = std::abs(data[i]);
        return result;
    }

    inline f32x floor() const
    {
        f32x result;
        for (int i = 0; i < N; ++i)
            result.data[i] = std::floor(data[i]);
        return result;
    }

    inline f32x copysign(const f32x &other) const
    {
        f32x result;
        for (int i = 0; i < N; ++i)
            result.data[i] = std::copysign(data[i], other.data[i]);
        return result;
    }

    inline f32x operator+(const f32x &other) const
    {
        f32x result;
        for (int i = 0; i < N; ++i)
            result.data[i] = data[i] + other.data[i];
        return result;
    }

    inline f32x operator-(const f32x &other) const
    {
        f32x result;
        for (int i = 0; i < N; ++i)
            result.data[i] = data[i] - other.data[i];
        return result;
    }

    inline f32x operator*(const f32x &other) const
    {
        f32x result;
        for (int i = 0; i < N; ++i)
            result.data[i] = data[i] * other.data[i];
        return result;
    }

    inline f32x operator/(const f32x &other) const
    {
        f32x result;
        for (int i = 0; i < N; ++i)
            result.data[i] = data[i] / other.data[i];
        return result;
    }

    inline float &operator[](int index)
    {
        return data[index];
    }

    inline float operator[](int index) const
    {
        return data[index];
    }
};

/// @brief Fast `sincos(2 * pi * x)` approximation.
/// * Chebyshev polynomial based.
/// * Maximum absolute error for `cos` is 6e-7
/// * Maximum absolute error for `sin` is 4e-6
/// * Range is [-0.5, 0.5].
template <int N>
std::array<f32x<N>, 2> approx_sin_cos_tau(const f32x<N> &z)
{
    const float s0 = 1.000013316162347f;    // 1
    const float s1 = -1.2336047188237933f;  // x^2
    const float s2 = 0.2529001895868434f;   // x^4
    const float s3 = -0.01930878692539701f; // x^6

    const float c0 = 1.5707909643039777f;    // x
    const float c1 = -0.6458924755296254f;   // x^3
    const float c2 = 0.07943359676384154f;   // x^5
    const float c3 = -0.004332668018125086f; // x^7

    // mirroring to exploit symmetry, saves some multiplies/improves accuracy
    auto x = f32x<N>(1.0f) - z.abs() * 4.0f;

    // estrin scheme
    auto x2 = x * x;
    auto x4 = x2 * x2;

    auto poly1 = x4 * (x2 * c3 + c2) + (x2 * c1 + c0);
    auto poly2 = x4 * (x2 * s3 + s2) + (x2 * s1 + s0);

    auto sin = poly2.copysign(z);
    auto cos = poly1 * x;

    return {sin, cos};
}