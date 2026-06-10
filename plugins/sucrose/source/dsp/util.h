#pragma once
#include <cmath>
#include <array>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

/// @brief Multi-channel mid-side encoder/decoder, in-place.
/// Turns N channels into 1 mid and N-1 side channels, each side channel is `mid - 2 * channel`.
/// Inverse of itself (up to gain of N).
inline void mid_side(float *const *x, int offset, int samples, int channels)
{
    for (int c = 1; c < channels; ++c)
        for (int i = offset; i < offset + samples; ++i)
            x[0][i] += x[c][i];

    for (int c = 1; c < channels; ++c)
        for (int i = offset; i < offset + samples; ++i)
            x[c][i] = x[0][i] - x[c][i] - x[c][i];
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
