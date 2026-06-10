#pragma once
#include "util.h"
#include "filter.h"
#include "hilbert.h"
#include "harmonic.h"
#include <vector>

enum DspMode
{
    DIRTY,
    CLEAN4,
    CLEAN16,
};

struct DspParams
{
    float gain0;
    float gain1;
    float gain2;
    float gain3;

    float locut;
    float hicut;

    DspMode mode;
};

struct DspChannel
{
    float prefilter[4][2] = {}; // used for pre- low/high cut filtering
    Hiir<4, 1> halfband = {};   // used for bandlimiting to 1/4 when oversampling is off
    Hiir2<4> downsample = {};   // used when oversampling is on
    Hiir2<4> upsample = {};     // used when oversampling is on

    union
    {
        struct
        {
            float emphasis[2] = {};    // used for emphasis filtering
            float deemphasis[2] = {};  // used for deemphasis filtering
            Hiir2<2> downsample4 = {}; // used for x4 downsampling
            Hiir2<2> upsample4 = {};   // used for x4 upsampling
            HarmonicGen<8, 1> harmonic = {};
        } dirty;

        struct
        {
            LR4Bank4 bank = {};
            HarmonicGen<6, 4> harmonic = {};
        } clean4;

        struct
        {
            LR4Bank16 bank = {};
            HarmonicGen<6, 16> harmonic = {};
        } clean16;
    } data;

    DspChannel(DspMode mode) : data({})
    {
        switch (mode)
        {
        case DIRTY:
            data.dirty = {};
            break;

        case CLEAN4:
            data.clean4 = {};
            break;

        case CLEAN16:
            data.clean16 = {};
            break;

        default:
            break;
        }
    }
};

struct DspEngine
{
    std::vector<DspChannel> state;

    DspMode mode;
    bool oversample;

    SVF<float> coeffs_emphasis = {};
    SVF<float> coeffs_hicut = {};
    SVF<float> coeffs_locut = {};
    SVF<float> coeffs_bank[15] = {};

    float hicut_freq;
    float locut_freq;
    float sample_rate;

    DspEngine(int num_channels, float sample_rate) : mode(DIRTY),
                                                     state(num_channels, DspChannel(DIRTY)),
                                                     oversample(sample_rate < 88200.0f),
                                                     sample_rate(sample_rate),
                                                     hicut_freq(-1.f),
                                                     locut_freq(-1.f),
                                                     coeffs_emphasis(440.f / sample_rate, 0.25f)
    {
    }

    int channels() const
    {
        return state.size();
    }

    void run(float *const *data, int offset, int samples, DspParams params)
    {
        int channels = state.size();

        if (params.mode != mode)
        {
            mode = params.mode;
            for (int c = 0; c < channels; ++c)
                state[c] = DspChannel(mode);

            switch (mode)
            {
            case CLEAN4:
                LR4Bank4::design(coeffs_bank, 2.0 * sample_rate);
                break;
            case CLEAN16:
                LR4Bank16::design(coeffs_bank, 2.0 * sample_rate);
                break;
            default:
                break;
            }
        }

        if (params.hicut != hicut_freq)
        {
            hicut_freq = params.hicut;
            coeffs_hicut = SVF<float>(hicut_freq / sample_rate, 0.7071f);
        }

        if (params.locut != locut_freq)
        {
            locut_freq = params.locut;
            coeffs_locut = SVF<float>(locut_freq / sample_rate, 0.7071f);
        }

        mid_side(data, offset, samples, channels);

        float wet_gain = 1.0f / channels;
        float dry_gain = params.gain1 * wet_gain;

        for (int c = 0; c < channels; ++c)
        {
            float *x = data[c];
            DspChannel &channel = state[c];

            for (int i = offset; i < offset + samples; ++i)
            {
                float dry = x[i];
                float wet = x[i];

                wet = coeffs_hicut.run(wet, channel.prefilter[0])[0];
                wet = coeffs_hicut.run(wet, channel.prefilter[1])[0];
                wet = coeffs_locut.run(wet, channel.prefilter[2])[1];
                wet = coeffs_locut.run(wet, channel.prefilter[3])[1];

                if (mode == DIRTY)
                {
                    auto [lp, hp] = coeffs_emphasis.run(wet, channel.data.dirty.emphasis);
                    wet = wet + lp - 0.5f * hp; // tilt shelf
                }

                if (oversample)
                {
                    auto z = channel.upsample.run_up(wet, HIIR8_69);
                    z[0] = run_xsampled_path(z[0], channel, params);
                    z[1] = run_xsampled_path(z[1], channel, params);
                    wet = channel.downsample.run_down(z[0], z[1], HIIR8_69);
                }
                else
                {
                    wet = channel.halfband.run_lp(wet, HIIR8_69)[0];
                    wet = run_xsampled_path(wet, channel, params);
                }

                if (mode == DIRTY)
                {
                    auto [lp, hp] = coeffs_emphasis.run(wet, channel.data.dirty.deemphasis);
                    wet = wet - 0.5f * lp + hp; // inverse tilt shelf
                }

                x[i] = wet * wet_gain + dry * dry_gain;
            }
        }

        mid_side(data, offset, samples, channels);
    }

    void reset()
    {
        for (int c = 0; c < state.size(); ++c)
            state[c] = DspChannel(mode);
    }

private:
    inline float run_xsampled_path(float x, DspChannel &channel, DspParams params)
    {
        switch (mode)
        {
        case DIRTY:
        {
            // we have to do 4x oversampling here because intermodulation can create frequencies above 2x of the bandlimit
            auto z = channel.data.dirty.upsample4.run_up(x, HIIR4_120);

            for (int i = 0; i < 2; ++i)
            {
                auto result = channel.data.dirty.harmonic.run(z[i], HIIR16_84);
                z[i] = result.sub2[0] * params.gain0 +
                       result.oct2[0] * (params.gain2 * 0.707f) + // emphasis-deemphasis correction gain
                       result.oct3[0] * (params.gain3 * 0.5f);
            }

            return channel.data.dirty.downsample4.run_down(z[0], z[1], HIIR4_120);
        }

        case CLEAN4:
        {
            auto bands = channel.data.clean4.bank.run(x, coeffs_bank);
            auto result = channel.data.clean4.harmonic.run(bands, HIIR12_70);

            result.oct2[3] = 0.0f; // reduce aliasing
            result.oct3[3] = 0.0f;

            return result.sub2.sum() * params.gain0 +
                   result.oct2.sum() * params.gain2 +
                   result.oct3.sum() * params.gain3;
        }

        case CLEAN16:
        {
            auto bands = channel.data.clean16.bank.run(x, coeffs_bank);
            auto result = channel.data.clean16.harmonic.run(bands, HIIR12_70);

            result.oct2[15] = 0.0f; // reduce aliasing
            result.oct3[14] = 0.0f;
            result.oct3[15] = 0.0f;

            return result.sub2.sum() * params.gain0 +
                   result.oct2.sum() * params.gain2 +
                   result.oct3.sum() * params.gain3;
        }

        default:
            return x;
        }
    }
};
