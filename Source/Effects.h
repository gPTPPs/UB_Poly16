#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

// Master-bus FX for UB_Poly16: stereo/ping-pong delay + reverb with pre-delay.
// Both are bypassed (and their buffers cleared) when their On switch is off,
// so patches without FX are bit-identical to v0.1.

// Tempo-syncable stereo delay with darkening repeats (one-pole LP in the
// feedback path) and optional ping-pong routing. Delay time is smoothed, so
// moving the Time knob tape-bends instead of clicking.
class UBDelay
{
public:
    struct Params
    {
        bool  on = false, pingPong = true;
        float timeSec = 0.35f, feedback = 0.45f, toneHz = 6000.0f, mix = 0.3f;
    };

    static constexpr double maxDelaySeconds = 3.0;

    void prepare (double sr, int maxBlock)
    {
        sampleRate = sr;
        const juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 1 };
        const int maxSamps = (int) std::ceil (maxDelaySeconds * sr) + 4;
        for (auto* d : { &dlL, &dlR })
        {
            d->prepare (spec);
            d->setMaximumDelayInSamples (maxSamps);
        }
        timeSm.reset (sr, 0.12);
        reset();
    }

    void reset()
    {
        dlL.reset(); dlR.reset();
        lpL = lpR = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buf, const Params& p)
    {
        if (buf.getNumChannels() < 2)
            return;

        if (! p.on)
        {
            if (wasOn) reset();   // drop the tail so re-enabling starts clean
            wasOn = false;
            return;
        }

        const float targetSamps = (float) juce::jlimit (1.0, maxDelaySeconds * sampleRate - 8.0,
                                                        p.timeSec * sampleRate);
        if (! wasOn)              // avoid a tape-slew sweep on first enable
            timeSm.setCurrentAndTargetValue (targetSamps);
        else
            timeSm.setTargetValue (targetSamps);
        wasOn = true;

        const float fb     = juce::jlimit (0.0f, 0.95f, p.feedback);
        const float lpCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                                              * juce::jlimit (100.0f, 18000.0f, p.toneHz) / (float) sampleRate);
        auto* L = buf.getWritePointer (0);
        auto* R = buf.getWritePointer (1);

        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            const float t = timeSm.getNextValue();
            dlL.setDelay (t);
            dlR.setDelay (t);
            const float outL = dlL.popSample (0);
            const float outR = dlR.popSample (0);

            // darken the repeats
            lpL += lpCoef * (outL - lpL);
            lpR += lpCoef * (outR - lpR);

            const float inL = L[i], inR = R[i];
            if (p.pingPong)
            {
                const float m = 0.5f * (inL + inR);
                dlL.pushSample (0, m + fb * lpR);   // cross-feedback = L/R alternation
                dlR.pushSample (0,     fb * lpL);
            }
            else
            {
                dlL.pushSample (0, inL + fb * lpL);
                dlR.pushSample (0, inR + fb * lpR);
            }

            L[i] = inL + p.mix * lpL;
            R[i] = inR + p.mix * lpR;
        }
    }

private:
    using DL = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;
    DL dlL { 48000 * 4 }, dlR { 48000 * 4 };
    juce::SmoothedValue<float> timeSm;
    double sampleRate = 44100.0;
    float  lpL = 0.0f, lpR = 0.0f;
    bool   wasOn = false;
};

// juce::Reverb on a separate wet bus, preceded by a pre-delay (keeps the dry
// attack readable on pads), added back under the Mix knob.
class UBReverbFx
{
public:
    struct Params
    {
        bool  on = false;
        float size = 0.55f, damp = 0.45f, width = 1.0f, preMs = 20.0f, mix = 0.3f;
    };

    void prepare (double sr, int maxBlock)
    {
        sampleRate = sr;
        reverb.setSampleRate (sr);
        const juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 1 };
        const int maxSamps = (int) std::ceil (0.25 * sr) + 4;
        for (auto* d : { &preL, &preR })
        {
            d->prepare (spec);
            d->setMaximumDelayInSamples (maxSamps);
        }
        wet.setSize (2, maxBlock);
        reset();
    }

    void reset()
    {
        reverb.reset();
        preL.reset(); preR.reset();
    }

    void process (juce::AudioBuffer<float>& buf, const Params& p)
    {
        if (buf.getNumChannels() < 2)
            return;

        if (! p.on)
        {
            if (wasOn) reset();
            wasOn = false;
            return;
        }
        wasOn = true;

        juce::Reverb::Parameters rp;
        rp.roomSize = p.size;
        rp.damping  = p.damp;
        rp.width    = p.width;
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        reverb.setParameters (rp);

        const int n = buf.getNumSamples();
        wet.copyFrom (0, 0, buf, 0, 0, n);
        wet.copyFrom (1, 0, buf, 1, 0, n);

        const float preSamps = juce::jlimit (0.0f, 0.24f * (float) sampleRate,
                                             p.preMs * 0.001f * (float) sampleRate);
        preL.setDelay (preSamps);
        preR.setDelay (preSamps);
        auto* wL = wet.getWritePointer (0);
        auto* wR = wet.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            preL.pushSample (0, wL[i]);
            preR.pushSample (0, wR[i]);
            wL[i] = preL.popSample (0);
            wR[i] = preR.popSample (0);
        }

        reverb.processStereo (wL, wR, n);
        buf.addFrom (0, 0, wet, 0, 0, n, p.mix);
        buf.addFrom (1, 0, wet, 1, 0, n, p.mix);
    }

private:
    using DL = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>;
    juce::Reverb reverb;
    DL preL { 12000 }, preR { 12000 };
    juce::AudioBuffer<float> wet;
    double sampleRate = 44100.0;
    bool   wasOn = false;
};
