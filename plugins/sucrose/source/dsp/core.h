#pragma once
#include "util.h"
#include "filter.h"
#include "hilbert.h"
#include "harmonic.h"
#include <vector>

// maximum block size for DspEngine processing, must be split into smaller blocks before calling `run`
const int MAX_BLOCK_SIZE = 128;

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
    float emphasis[2] = {};     // used for emphasis filtering
    float deemphasis[2] = {};   // used for deemphasis filtering
    float prefilter[4][2] = {}; // used for pre- low/high cut filtering

    Hiir<4, 1> halfband = {}; // used for bandlimiting to 1/4 when oversampling is off
    Hiir2<4> upsample = {};   // used when oversampling is on
    Hiir2<4> downsample = {}; // used when oversampling is on

    union
    {
        struct
        {
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

    float fadeout_gain = 1.0f;
    float fadeout_ramp;

    std::vector<float> sub_buffer;
    Hiir<4, 1> sub_halfband = {}; // used for bandlimiting to 1/4 when oversampling is off
    Hiir2<4> sub_upsample = {};   // used when oversampling is on
    Hiir2<4> sub_downsample = {}; // used when oversampling is on
    float sub_deemphasis[2] = {}; // used for suboctave deemphasis filtering

    union
    {
        struct
        {
            PhaseLocked<8, 1> pll = {};
        } dirty;

        struct
        {
            LR4Bank4 bank = {};
            PhaseLocked<6, 4> pll = {};
        } clean4;

        struct
        {
            LR4Bank16 bank = {};
            PhaseLocked<6, 16> pll = {};
        } clean16;
    } sub_channel;

    DspEngine(int num_channels, float sample_rate) : mode(DIRTY),
                                                     state(num_channels, DspChannel(DIRTY)),
                                                     oversample(sample_rate < 88200.0f),
                                                     sample_rate(sample_rate),
                                                     hicut_freq(-1.f),
                                                     locut_freq(-1.f),
                                                     coeffs_emphasis(440.f / sample_rate, 0.25f),
                                                     fadeout_ramp(50.0f / sample_rate),
                                                     sub_buffer(MAX_BLOCK_SIZE, 0.0f),
                                                     sub_channel({})
    {
        reset();
    }

    int channels() const
    {
        return state.size();
    }

    void run(float *const *data, int offset, int samples, DspParams params)
    {
        int channels = state.size();

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

        // gain compensation for emphasis/de-emphasis
        params.gain2 *= 0.707f;
        params.gain3 *= 0.5f;

        // clear suboctave buffer
        sub_buffer.assign(samples, 0.0f);

        // compute overtones
        for (int c = 0; c < channels; ++c)
        {
            float *x = data[c];
            DspChannel &channel = state[c];

            for (int i = 0; i < samples; ++i)
            {
                float wet = x[offset + i];

                // high/low cut prefiltering
                wet = coeffs_hicut.run(wet, channel.prefilter[0])[0];
                wet = coeffs_hicut.run(wet, channel.prefilter[1])[0];
                wet = coeffs_locut.run(wet, channel.prefilter[2])[1];
                wet = coeffs_locut.run(wet, channel.prefilter[3])[1];

                // pre-emphasis
                {
                    auto [lp, hp] = coeffs_emphasis.run(wet, channel.emphasis);
                    wet = wet + lp - 0.5f * hp; // tilt shelf
                }

                // compute mid signal and store it for later use (suboctaving)
                sub_buffer[i] += wet;

                if (oversample)
                {
                    auto z = channel.upsample.run_up(wet, HIIR8_69);

                    auto z0 = run_xsampled_overtones(z[0], channel, params);
                    auto z1 = run_xsampled_overtones(z[1], channel, params);

                    wet = channel.downsample.run_down(z0, z1, HIIR8_69);
                }
                else
                {
                    wet = channel.halfband.run_lp(wet, HIIR8_69)[0];
                    wet = run_xsampled_overtones(wet, channel, params);
                }

                // de-emphasis
                {
                    auto [lp0, hp0] = coeffs_emphasis.run(wet, channel.deemphasis);
                    wet = wet - 0.5f * lp0 + hp0; // inverse tilt shelf
                }

                x[offset + i] *= params.gain1; // apply dry gain
                x[offset + i] += wet;          // add wet signal
            }
        }

        // suboctave generation
        if (params.gain0 > 0.0f)
        {
            // suboctave generation (mid channel)
            for (int i = 0; i < samples; ++i)
            {
                float wet = sub_buffer[i];

                if (oversample)
                {
                    auto z = sub_upsample.run_up(wet, HIIR8_69);
                    auto z0 = run_xsampled_suboctave(z[0]);
                    auto z1 = run_xsampled_suboctave(z[1]);
                    wet = sub_downsample.run_down(z0, z1, HIIR8_69);
                }
                else
                {
                    wet = sub_halfband.run_lp(wet, HIIR8_69)[0];
                    wet = run_xsampled_suboctave(wet);
                }

                // tilt shift (de-emphasis) for suboctave
                {
                    auto [lp, hp] = coeffs_emphasis.run(wet, sub_deemphasis);
                    wet = wet - 0.5f * lp + hp; // inverse tilt shelf
                }

                sub_buffer[i] = wet;
            }

            // compute suboctave gain (including phase shift and channel sum compensation)
            float sub_gain = params.gain0 / (float)channels;
            switch (mode)
            {
            case DIRTY:
                sub_gain *= 1.414f; // 3db boost to compensate for 90 deg phase shift
                break;
            case CLEAN4:
                sub_gain *= 1.732f;
                break;
            case CLEAN16:
                sub_gain *= 2.0f;
                break;
            }

            // add monoed suboctave to all channels
            for (int c = 0; c < channels; ++c)
            {
                float *x = data[c];
                for (int i = 0; i < samples; ++i)
                    x[i + offset] += sub_buffer[i] * sub_gain;
            }
        }

        // click-less mode change
        if (fadeout_gain < 1.0f || mode != params.mode)
        {
            fadeout(data, offset, samples, channels,
                    mode != params.mode ? -fadeout_ramp : fadeout_ramp,
                    fadeout_gain);

            if (fadeout_gain <= 1e-5f)
            {
                mode = params.mode;
                reset();
            }
        }
    }

    void reset()
    {
        sub_upsample = Hiir2<4>();
        sub_downsample = Hiir2<4>();
        sub_halfband = Hiir<4, 1>();
        sub_deemphasis[0] = 0.0f;
        sub_deemphasis[1] = 0.0f;

        for (int c = 0; c < state.size(); ++c)
            state[c] = DspChannel(mode);

        switch (mode)
        {
        case DIRTY:
        {
            sub_channel.dirty = {};
            break;
        }

        case CLEAN4:
        {
            sub_channel.clean4 = {};
            LR4Bank4::design(coeffs_bank, 2.0 * sample_rate);
            break;
        }
        case CLEAN16:
        {
            sub_channel.clean16 = {};
            LR4Bank16::design(coeffs_bank, 2.0 * sample_rate);
            break;
        }
        default:
            break;
        }
    }

private:
    inline float run_xsampled_suboctave(float x)
    {
        switch (mode)
        {
        case DIRTY:
        {
            return sub_channel.dirty.pll.run(x, HIIR16_84)[0];
        }
        case CLEAN4:
        {
            auto bands = sub_channel.clean4.bank.run(x, coeffs_bank);
            return sub_channel.clean4.pll.run(bands, HIIR12_70).sum();
        }
        case CLEAN16:
        {
            auto bands = sub_channel.clean16.bank.run(x, coeffs_bank);
            return sub_channel.clean16.pll.run(bands, HIIR12_70).sum();
        }
        default:
            return x;
        }
    }

    inline float run_xsampled_overtones(float x, DspChannel &channel, DspParams params)
    {
        switch (mode)
        {
        case DIRTY:
        {
            // we have to do 4x oversampling here because intermodulation can create frequencies above 2x of the bandlimit
            auto z = channel.data.dirty.upsample4.run_up(x, HIIR4_120);

            for (int i = 0; i < 2; ++i)
            {
                auto [oct2, oct3] = channel.data.dirty.harmonic.run(z[i], HIIR16_84);

                z[i] = oct2[0] * params.gain2 +
                       oct3[0] * params.gain3;
            }

            return channel.data.dirty.downsample4.run_down(z[0], z[1], HIIR4_120);
        }

        case CLEAN4:
        {
            auto bands = channel.data.clean4.bank.run(x, coeffs_bank);
            auto [oct2, oct3] = channel.data.clean4.harmonic.run(bands, HIIR12_70);

            oct2[3] = 0.0f; // reduce aliasing
            oct3[3] = 0.0f;

            return oct2.sum() * params.gain2 + oct3.sum() * params.gain3;
        }

        case CLEAN16:
        {
            auto bands = channel.data.clean16.bank.run(x, coeffs_bank);
            auto [oct2, oct3] = channel.data.clean16.harmonic.run(bands, HIIR12_70);

            oct2[15] = 0.0f; // reduce aliasing
            oct3[14] = 0.0f;
            oct3[15] = 0.0f;

            return oct2.sum() * params.gain2 + oct3.sum() * params.gain3;
        }

        default:
            return x;
        }
    }
};
