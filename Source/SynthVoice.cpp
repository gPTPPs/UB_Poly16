#include "SynthVoice.h"

namespace
{
    inline float polyBlep (double t, double dt)
    {
        if (t < dt)           { t /= dt;          return float (t + t - t * t - 1.0); }
        if (t > 1.0 - dt)     { t = (t - 1.0) / dt; return float (t * t + t + t + 1.0); }
        return 0.0f;
    }

    inline float sawSample (double phase, double dt)
    {
        return float (2.0 * phase - 1.0) - polyBlep (phase, dt);
    }

    inline float pulseSample (double phase, double dt, float pw)
    {
        float v = phase < (double) pw ? 1.0f : -1.0f;
        v += polyBlep (phase, dt);
        double t2 = phase + (1.0 - (double) pw);
        if (t2 >= 1.0) t2 -= 1.0;
        v -= polyBlep (t2, dt);
        return v;
    }

    // Naive triangle: its harmonics fall off at 12 dB/oct so aliasing stays
    // inaudible at musical pitches — no BLEP/BLAMP needed.
    inline float triSample (double phase)
    {
        return float (4.0 * std::abs (phase - 0.5) - 1.0);
    }

    inline double advance (double& phase, double inc)
    {
        phase += inc;
        while (phase >= 1.0) phase -= 1.0;
        return phase;
    }
}

UBVoice::UBVoice (juce::AudioProcessorValueTreeState& s) : state (s)
{
    p.glide    = state.getRawParameterValue (ID::glide);
    p.pbRange  = state.getRawParameterValue (ID::pbRange);
    p.o1Octave = state.getRawParameterValue (ID::o1Octave);
    p.o1Saw    = state.getRawParameterValue (ID::o1Saw);
    p.o1Tri    = state.getRawParameterValue (ID::o1Tri);
    p.o1Pulse  = state.getRawParameterValue (ID::o1Pulse);
    p.o1Sub    = state.getRawParameterValue (ID::o1Sub);
    p.o1Pw     = state.getRawParameterValue (ID::o1Pw);
    p.o1Pwm    = state.getRawParameterValue (ID::o1Pwm);
    p.o2On     = state.getRawParameterValue (ID::o2On);
    p.o2Octave = state.getRawParameterValue (ID::o2Octave);
    p.o2Detune = state.getRawParameterValue (ID::o2Detune);
    p.o2Saw    = state.getRawParameterValue (ID::o2Saw);
    p.o2Tri    = state.getRawParameterValue (ID::o2Tri);
    p.o2Pulse  = state.getRawParameterValue (ID::o2Pulse);
    p.o2Sub    = state.getRawParameterValue (ID::o2Sub);
    p.o2Pw     = state.getRawParameterValue (ID::o2Pw);
    p.o2Pwm    = state.getRawParameterValue (ID::o2Pwm);
    p.o2Sync   = state.getRawParameterValue (ID::o2Sync);
    p.noise    = state.getRawParameterValue (ID::noise);
    p.hpf      = state.getRawParameterValue (ID::hpf);
    p.uniVoices = state.getRawParameterValue (ID::uniVoices);
    p.uniDetune = state.getRawParameterValue (ID::uniDetune);
    p.uniSpread = state.getRawParameterValue (ID::uniSpread);
    p.fType    = state.getRawParameterValue (ID::fType);
    p.fCutoff  = state.getRawParameterValue (ID::fCutoff);
    p.fReso    = state.getRawParameterValue (ID::fReso);
    p.fDrive   = state.getRawParameterValue (ID::fDrive);
    p.fEnvAmt  = state.getRawParameterValue (ID::fEnvAmt);
    p.fKeyTrack= state.getRawParameterValue (ID::fKeyTrack);
    p.fLfo     = state.getRawParameterValue (ID::fLfo);
    p.fVel     = state.getRawParameterValue (ID::fVelAmt);
    p.aA = state.getRawParameterValue (ID::aA);
    p.aD = state.getRawParameterValue (ID::aD);
    p.aS = state.getRawParameterValue (ID::aS);
    p.aR = state.getRawParameterValue (ID::aR);
    p.feA = state.getRawParameterValue (ID::fA);
    p.feD = state.getRawParameterValue (ID::fD);
    p.feS = state.getRawParameterValue (ID::fS);
    p.feR = state.getRawParameterValue (ID::fR);
    p.lfoRate  = state.getRawParameterValue (ID::lfoRate);
    p.lfoShape = state.getRawParameterValue (ID::lfoShape);
    p.lfoDelay = state.getRawParameterValue (ID::lfoDelay);
    p.lfoPitch = state.getRawParameterValue (ID::lfoPitch);
    p.atCutoff    = state.getRawParameterValue (ID::atCutoff);
    p.atVib       = state.getRawParameterValue (ID::atVib);
    p.attackClick = state.getRawParameterValue (ID::attackClick);
}

void UBVoice::prepare (double sampleRate, int blockSize)
{
    setCurrentPlaybackSampleRate (sampleRate);
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) blockSize, 2 };
    svfA.prepare (spec);
    svfB.prepare (spec);
    svfA.reset();
    svfB.reset();
    ampEnv.setSampleRate (sampleRate);
    filtEnv.setSampleRate (sampleRate);
}

bool UBVoice::canPlaySound (juce::SynthesiserSound* s)
{
    return dynamic_cast<UBSound*> (s) != nullptr;
}

void UBVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int currentPitchWheel)
{
    noteNumber = midiNote;
    velLevel   = juce::jmax (0.05f, velocity);
    targetFreq = juce::MidiMessage::getMidiNoteInHertz (midiNote);

    if (p.glide->load() <= 0.0001f)
        currentFreq = targetFreq;
    else if (! ampEnv.isActive())
    {
        // fresh voice: glide from the last note played on ANY voice, so
        // portamento carries across retriggered mono notes and poly playing
        const float last = glideSrc != nullptr ? glideSrc->load() : 0.0f;
        currentFreq = last > 0.0f ? (double) last : targetFreq;
    }
    if (glideSrc != nullptr)
        glideSrc->store ((float) targetFreq);

    pitchWheelNorm = (float) (currentPitchWheel - 8192) / 8192.0f;

    ampEnv.setParameters  ({ p.aA->load(),  p.aD->load(),  p.aS->load(),  p.aR->load() });
    filtEnv.setParameters ({ p.feA->load(), p.feD->load(), p.feS->load(), p.feR->load() });
    ampEnv.noteOn();
    filtEnv.noteOn();

    lfoDelaySamplesDone = 0;
    hpfPrevIn[0] = hpfPrevIn[1] = hpfPrevOut[0] = hpfPrevOut[1] = 0.0f;
    clickAmp = 1.0f;   // arm the attack-transient burst
}

void UBVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnv.noteOff();
        filtEnv.noteOff();
    }
    else
    {
        ampEnv.reset();
        filtEnv.reset();
        clearCurrentNote();
    }
}

void UBVoice::legatoChange (int newNote)
{
    noteNumber = newNote;   // keytrack follows too
    targetFreq = juce::MidiMessage::getMidiNoteInHertz (newNote);
    if (glideSrc != nullptr)
        glideSrc->store ((float) targetFreq);
}

void UBVoice::pitchWheelMoved (int newValue)
{
    pitchWheelNorm = (float) (newValue - 8192) / 8192.0f;
}

void UBVoice::controllerMoved (int controller, int value)
{
    if (controller == 1) // mod wheel -> extra vibrato
        modWheel = (float) value / 127.0f;
}

void UBVoice::channelPressureChanged (int newValue)
{
    aftertouch = (float) newValue / 127.0f;
}

void UBVoice::aftertouchChanged (int newValue)
{
    aftertouch = (float) newValue / 127.0f;
}

void UBVoice::renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    if (! ampEnv.isActive())
        return;

    const double sr = getSampleRate();

    // ---- per-block parameter snapshot ----
    ampEnv.setParameters  ({ p.aA->load(),  p.aD->load(),  p.aS->load(),  p.aR->load() });
    filtEnv.setParameters ({ p.feA->load(), p.feD->load(), p.feS->load(), p.feR->load() });

    const int   o1Oct = (int) p.o1Octave->load();
    const int   o2Oct = (int) p.o2Octave->load();
    const float o1SawL = p.o1Saw->load(),  o1TriL = p.o1Tri->load(), o1PulL = p.o1Pulse->load(), o1SubL = p.o1Sub->load();
    const float o1PwBase = p.o1Pw->load(), o1PwmD = p.o1Pwm->load();
    const bool  o2On = p.o2On->load() > 0.5f;
    const bool  syncOn = p.o2Sync->load() > 0.5f;
    const float o2SawL = p.o2Saw->load(),  o2TriL = p.o2Tri->load(), o2PulL = p.o2Pulse->load(), o2SubL = p.o2Sub->load();
    const float o2PwBase = p.o2Pw->load(), o2PwmD = p.o2Pwm->load();
    const float o2DetuneRatio = std::pow (2.0f, p.o2Detune->load() / 1200.0f);
    const float noiseL = p.noise->load();

    // ---- unison setup: detune ratio + constant-power pan per sub-voice ----
    const int   uniN   = juce::jlimit (1, maxUnison, 1 + (int) p.uniVoices->load());
    const float uniDet = p.uniDetune->load();
    const float uniSpr = p.uniSpread->load();
    // Non-linear spacing (voices clustered towards the centre) + a full centre
    // voice on odd counts — the JP-8000 recipe; 7-voice row follows Szabo's
    // measured supersaw detune curve, normalised.
    static constexpr float uniOffs[maxUnison][maxUnison] = {
        {  0.0f,   0.0f,   0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
        { -1.0f,   1.0f,   0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
        { -1.0f,   0.0f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f },
        { -1.0f, -0.33f,  0.33f,  1.0f,  0.0f,  0.0f,  0.0f },
        { -1.0f, -0.40f,   0.0f,  0.40f, 1.0f,  0.0f,  0.0f },
        { -1.0f, -0.55f, -0.18f,  0.18f, 0.55f, 1.0f,  0.0f },
        { -1.0f, -0.57f, -0.18f,  0.0f,  0.18f, 0.57f, 1.0f },
    };
    float detRatio[maxUnison], gainL[maxUnison], gainR[maxUnison];
    const float ampComp = 1.0f / std::sqrt ((float) uniN);
    for (int k = 0; k < uniN; ++k)
    {
        const float o = uniOffs[uniN - 1][k];
        detRatio[k] = std::pow (2.0f, o * uniDet / 1200.0f);
        if (uniN == 1)
            gainL[k] = gainR[k] = 1.0f;   // exactly the pre-unison mono path
        else
        {
            const float pan = o * uniSpr;
            gainL[k] = ampComp * std::sqrt (0.5f * (1.0f - pan));
            gainR[k] = ampComp * std::sqrt (0.5f * (1.0f + pan));
        }
    }

    const int   hpfPos = (int) p.hpf->load();
    const int   fType  = (int) p.fType->load();
    const float baseCutoff = p.fCutoff->load();
    const float reso   = p.fReso->load();
    const float drive  = p.fDrive->load();
    const float envAmt = p.fEnvAmt->load();
    const float keyTrk = p.fKeyTrack->load();
    const float lfoToF = p.fLfo->load();
    const float velAmt = p.fVel->load();

    const float lfoHz   = p.lfoRate->load();
    const int   lfoShp  = (int) p.lfoShape->load();
    const float lfoDel  = p.lfoDelay->load();
    const float lfoPit  = p.lfoPitch->load();
    const float pbRange = p.pbRange->load();

    // aftertouch destinations + attack transient (aftertouch is constant over
    // the block; the click burst decays per-sample from its armed level)
    const float atCutAmt = p.atCutoff->load();
    const float atVibAmt = p.atVib->load();
    const float clickAmt = p.attackClick->load();
    const float atCutOct = atCutAmt * aftertouch * 4.0f;                    // up to +4 octaves
    const float clickDecay = std::exp (-1.0f / (0.008f * (float) sr));      // ~8 ms burst

    // fType: 0 LP24, 1 LP12, 2 HP24, 3 HP12, 4 BP24, 5 BP12.
    using ST = juce::dsp::StateVariableTPTFilterType;
    const ST svfType = fType < 2 ? ST::lowpass : (fType < 4 ? ST::highpass : ST::bandpass);
    const bool cascade = (fType % 2) == 0;   // 24 dB modes are the even indices
    const float svfReso = juce::jmap (juce::jlimit (0.0f, 1.0f, reso), 0.5f, 10.0f);
    svfA.setType (svfType);
    svfB.setType (svfType);
    svfA.setResonance (svfReso);
    svfB.setResonance (svfReso);

    const double lfoInc = lfoHz / sr;
    const int    lfoDelaySamples = (int) (lfoDel * sr);

    // glide coefficient
    const float glideSec = p.glide->load();
    const double glideCoef = glideSec > 0.0001f ? (1.0 - std::exp (-1.0 / (glideSec * sr))) : 1.0;

    // HPF coefficient (one-pole, non-resonant); 0 = off
    const float hpfFreqs[4] = { 0.0f, 120.0f, 240.0f, 480.0f };
    float hpfA = 0.0f;
    const bool hpfOn = hpfPos > 0;
    if (hpfOn)
    {
        const double rc = 1.0 / (2.0 * juce::MathConstants<double>::pi * hpfFreqs[hpfPos]);
        const double dt = 1.0 / sr;
        hpfA = (float) (rc / (rc + dt));
    }

    const float pitchBendFactor = std::pow (2.0f, pitchWheelNorm * pbRange / 12.0f);
    const float vibDepth = juce::jlimit (0.0f, 1.0f, lfoPit + modWheel * 0.5f + aftertouch * atVibAmt * 0.5f);

    const int numCh = output.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // glide
        currentFreq += (targetFreq - currentFreq) * glideCoef;

        // ---- LFO ----
        prevLfoPhase = lfoPhase;
        advance (lfoPhase, lfoInc);
        const bool wrapped = lfoPhase < prevLfoPhase;
        if (wrapped)
            shValue = rng.nextFloat() * 2.0f - 1.0f;

        float lfo = 0.0f;
        switch (lfoShp)
        {
            case 0: lfo = 1.0f - 4.0f * std::abs ((float) lfoPhase - 0.5f); break;      // triangle
            case 1: lfo = std::sin ((float) lfoPhase * juce::MathConstants<float>::twoPi); break; // sine
            case 2: lfo = 2.0f * (float) lfoPhase - 1.0f; break;                         // saw
            case 3: lfo = lfoPhase < 0.5 ? 1.0f : -1.0f; break;                          // square
            case 4: lfo = shValue; break;                                                // S&H
            default: break;
        }

        if (lfoDelaySamples > 0 && lfoDelaySamplesDone < lfoDelaySamples)
        {
            lfo *= (float) lfoDelaySamplesDone / (float) lfoDelaySamples;
            ++lfoDelaySamplesDone;
        }

        // ---- pitch ----
        const double vib = std::pow (2.0, (double) (vibDepth * lfo) * (2.0 / 12.0)); // up to +/-2 semis
        const double f1 = currentFreq * std::pow (2.0, o1Oct) * pitchBendFactor * vib;
        const double f2 = currentFreq * std::pow (2.0, o2Oct) * o2DetuneRatio * pitchBendFactor * vib;

        // ---- DCOs, one pass per unison sub-voice, panned into L/R ----
        const float pw1 = juce::jlimit (0.05f, 0.95f, o1PwBase + o1PwmD * lfo);
        const float pw2 = juce::jlimit (0.05f, 0.95f, o2PwBase + o2PwmD * lfo);
        float mixL = 0.0f, mixR = 0.0f;

        for (int k = 0; k < uniN; ++k)
        {
            const double f1k = f1 * detRatio[k];
            const double inc1 = juce::jlimit (0.0, 0.49, f1k / sr);
            const double subInc1 = juce::jlimit (0.0, 0.49, f1k * 0.5 / sr);
            advance (phase1[k], inc1);
            advance (subPhase1[k], subInc1);
            const bool wrapped1 = phase1[k] < inc1;   // master cycle restarted this sample
            float s = o1SawL * sawSample (phase1[k], inc1)
                    + o1TriL * triSample (phase1[k])
                    + o1PulL * pulseSample (phase1[k], inc1, pw1)
                    + o1SubL * pulseSample (subPhase1[k], subInc1, 0.5f);

            if (o2On)
            {
                const double f2k = f2 * detRatio[k];
                const double inc2 = juce::jlimit (0.0, 0.49, f2k / sr);
                const double subInc2 = juce::jlimit (0.0, 0.49, f2k * 0.5 / sr);
                advance (phase2[k], inc2);
                advance (subPhase2[k], subInc2);
                // hard sync: DCO1 wrap resets DCO2, offset by the sub-sample
                // wrap position (sub-osc 2 stays free-running)
                if (syncOn && wrapped1 && inc1 > 0.0)
                    phase2[k] = (phase1[k] / inc1) * inc2;
                s += o2SawL * sawSample (phase2[k], inc2)
                   + o2TriL * triSample (phase2[k])
                   + o2PulL * pulseSample (phase2[k], inc2, pw2)
                   + o2SubL * pulseSample (subPhase2[k], subInc2, 0.5f);
            }

            mixL += s * gainL[k];
            mixR += s * gainR[k];
        }

        const float ns = noiseL * (rng.nextFloat() * 2.0f - 1.0f);  // noise stays centred
        const float clk = clickAmt * clickAmp * (rng.nextFloat() * 2.0f - 1.0f); // attack burst
        clickAmp *= clickDecay;
        mixL = mixL * 0.3f + (ns + clk) * 0.3f;
        mixR = mixR * 0.3f + (ns + clk) * 0.3f;

        // ---- HPF (per channel) ----
        if (hpfOn)
        {
            float outL = hpfA * (hpfPrevOut[0] + mixL - hpfPrevIn[0]);
            hpfPrevIn[0] = mixL;  hpfPrevOut[0] = outL;  mixL = outL;
            float outR = hpfA * (hpfPrevOut[1] + mixR - hpfPrevIn[1]);
            hpfPrevIn[1] = mixR;  hpfPrevOut[1] = outR;  mixR = outR;
        }

        // ---- envelopes ----
        const float fe = filtEnv.getNextSample();
        const float ae = ampEnv.getNextSample();

        // ---- cutoff modulation (exponential, in octaves) ----
        // velOct: full velocity keeps the preset brightness, soft notes darken
        const float keyOct = keyTrk * (float) (noteNumber - 60) / 12.0f;
        const float velOct = velAmt * 3.0f * (velLevel - 1.0f);
        float cutoff = baseCutoff * std::pow (2.0f, envAmt * 5.0f * fe + keyOct + lfoToF * lfo * 3.0f + velOct + atCutOct);
        cutoff = juce::jlimit (20.0f, 20000.0f, cutoff);

        // drive: soft saturation for the "more aggressive" filter character
        float drivenL = drive > 1.0f ? std::tanh (mixL * drive) : mixL;
        float drivenR = drive > 1.0f ? std::tanh (mixR * drive) : mixR;
        svfA.setCutoffFrequency (cutoff);
        float filtL = svfA.processSample (0, drivenL);
        float filtR = svfA.processSample (1, drivenR);
        if (cascade)
        {
            svfB.setCutoffFrequency (cutoff);
            filtL = svfB.processSample (0, filtL);
            filtR = svfB.processSample (1, filtR);
        }
        const float outL = filtL * ae * velLevel;
        const float outR = filtR * ae * velLevel;

        if (numCh >= 2)
        {
            output.addSample (0, startSample + i, outL);
            output.addSample (1, startSample + i, outR);
        }
        else
            output.addSample (0, startSample + i, 0.5f * (outL + outR));

        if (! ampEnv.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}
