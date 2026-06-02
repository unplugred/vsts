#pragma once
#include "util.h"
#include "filter.h"
#include "hilbert.h"
#include "harmonic.h"
#include <vector>

enum DspMode
{
    DIRTY,
    CLEAN8,
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
    float prefilter[4][2];
    Hiir2<4> downsample;
    Hiir2<4> upsample;

    union
    {
        struct
        {
            HarmonicGen<1> harmonic;
        } dirty;

        struct
        {
            LR4Bank8 bank;
            HarmonicGen<8> harmonic;
        } clean8;

        struct
        {
            LR4Bank16 bank;
            HarmonicGen<16> harmonic;
        } clean16;
    } data;

    DspChannel(DspMode mode) : data({})
    {
        switch (mode)
        {
        case DIRTY:
            data.dirty = {};
            break;

        case CLEAN8:
            data.clean8 = {};
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

    SVF<float> coeffs_hicut;
    SVF<float> coeffs_locut;
    SVF<float> coeffs_bank[15];

    float hicut_freq;
    float locut_freq;
    float sample_rate;

    DspEngine(int num_channels, float sample_rate) : sample_rate(sample_rate), mode(DIRTY), state(num_channels, DspChannel(DIRTY))
    {
    }

    int channels() const
    {
        return state.size();
    }

    void run(float *const *data, int offset, int samples, DspParams params)
    {
        int channels = state.size();
        float inv_channels = 1.0f / channels;

        if (params.mode != mode)
        {
            mode = params.mode;
            for (int c = 0; c < channels; ++c)
                state[c] = DspChannel(mode);

            switch (mode)
            {
            case CLEAN8:
                LR4Bank8::design(coeffs_bank, 2.0 * sample_rate);
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

                auto z = channel.upsample.run_up(wet, HIIR8_69);

                for (int n = 0; n < 2; ++n)
                {
                    switch (mode)
                    {
                    case DIRTY:
                    {
                        auto result = channel.data.dirty.harmonic.run(z[n]);
                        z[n] = result.sub2[0] * params.gain0 +
                               result.oct2[0] * params.gain2 +
                               result.oct3[0] * params.gain3;
                        break;
                    }

                    case CLEAN8:
                    {
                        auto bands = channel.data.clean8.bank.run(z[n], coeffs_bank);
                        auto result = channel.data.clean8.harmonic.run(bands);

                        result.oct2[7] = 0.0f; // reduce aliasing
                        result.oct3[6] = 0.0f;
                        result.oct3[7] = 0.0f;

                        z[n] = result.sub2.sum() * params.gain0 +
                               result.oct2.sum() * params.gain2 +
                               result.oct3.sum() * params.gain3;
                        break;
                    }

                    case CLEAN16:
                    {
                        auto bands = channel.data.clean16.bank.run(z[n], coeffs_bank);
                        auto result = channel.data.clean16.harmonic.run(bands);

                        result.oct2[15] = 0.0f; // reduce aliasing
                        result.oct3[14] = 0.0f;
                        result.oct3[15] = 0.0f;

                        z[n] = result.sub2.sum() * params.gain0 +
                               result.oct2.sum() * params.gain2 +
                               result.oct3.sum() * params.gain3;
                        break;
                    }

                    default:
                        break;
                    }
                }

                x[i] = channel.downsample.run_down(z[0], z[1], HIIR8_69);
                x[i] += dry * params.gain1;
                x[i] *= inv_channels; // cancel out the increase in gain from mid/side encoding
            }
        }

        mid_side(data, offset, samples, channels);
    }

    void reset()
    {
        for (int c = 0; c < state.size(); ++c)
            state[c] = DspChannel(mode);
    }
};
