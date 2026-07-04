#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"

// The single voice sound (UB_Poly16 plays everything on every channel).
struct UBSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override     { return true; }
    bool appliesToChannel (int) override  { return true; }
};

// One polyphonic voice: 2 DCOs (saw/pulse/sub) + noise -> HPF -> Ladder VCF -> VCA,
// with amp & filter ADSR envelopes and a per-voice LFO.
class UBVoice : public juce::SynthesiserVoice
{
public:
    explicit UBVoice (juce::AudioProcessorValueTreeState& s);

    void prepare (double sampleRate, int blockSize);

    bool canPlaySound (juce::SynthesiserSound* s) override;
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int currentPitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newValue) override;
    void controllerMoved (int controller, int value) override;
    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override;

    // Legato (single-trigger) pitch change: retune without restarting the envelopes.
    void legatoChange (int newNote);

    // Shared "last note played" frequency, owned by the processor, so glide can
    // carry over from one voice to the next (mono-style portamento).
    void setGlideSource (std::atomic<float>* s) { glideSrc = s; }

private:
    // cached atomic parameter pointers
    struct P
    {
        std::atomic<float>* glide; std::atomic<float>* pbRange;
        std::atomic<float>* o1Octave; std::atomic<float>* o1Saw; std::atomic<float>* o1Tri; std::atomic<float>* o1Pulse;
        std::atomic<float>* o1Sub; std::atomic<float>* o1Pw; std::atomic<float>* o1Pwm;
        std::atomic<float>* o2On; std::atomic<float>* o2Octave; std::atomic<float>* o2Detune;
        std::atomic<float>* o2Saw; std::atomic<float>* o2Tri; std::atomic<float>* o2Pulse; std::atomic<float>* o2Sub;
        std::atomic<float>* o2Pw; std::atomic<float>* o2Pwm; std::atomic<float>* o2Sync;
        std::atomic<float>* noise; std::atomic<float>* hpf;
        std::atomic<float>* uniVoices; std::atomic<float>* uniDetune; std::atomic<float>* uniSpread;
        std::atomic<float>* fType; std::atomic<float>* fCutoff; std::atomic<float>* fReso;
        std::atomic<float>* fDrive; std::atomic<float>* fEnvAmt; std::atomic<float>* fKeyTrack; std::atomic<float>* fLfo;
        std::atomic<float>* fVel;
        std::atomic<float>* aA; std::atomic<float>* aD; std::atomic<float>* aS; std::atomic<float>* aR;
        std::atomic<float>* feA; std::atomic<float>* feD; std::atomic<float>* feS; std::atomic<float>* feR;
        std::atomic<float>* lfoRate; std::atomic<float>* lfoShape; std::atomic<float>* lfoDelay; std::atomic<float>* lfoPitch;
    } p;

    juce::AudioProcessorValueTreeState& state;

    juce::ADSR ampEnv, filtEnv;
    // VCF: two TPT state-variable stages. One stage = 12 dB/oct, both cascaded = 24 dB/oct.
    juce::dsp::StateVariableTPTFilter<float> svfA, svfB;

    // oscillator phases — one set per unison sub-voice (max 7), staggered
    // start phases so stacked voices don't begin perfectly aligned
    static constexpr int maxUnison = 7;
    double phase1[maxUnison]    { 0.00, 0.31, 0.57, 0.83, 0.17, 0.45, 0.71 };
    double phase2[maxUnison]    { 0.13, 0.42, 0.68, 0.94, 0.26, 0.53, 0.79 };
    double subPhase1[maxUnison] { 0.07, 0.36, 0.61, 0.88, 0.11, 0.63, 0.91 };
    double subPhase2[maxUnison] { 0.21, 0.49, 0.74, 0.99, 0.33, 0.58, 0.86 };
    double currentFreq = 440.0, targetFreq = 440.0;

    // LFO
    double lfoPhase = 0.0;
    int    lfoDelaySamplesDone = 0;
    float  shValue = 0.0f;
    double prevLfoPhase = 0.0;

    // HPF one-pole state (per stereo channel)
    float hpfPrevIn[2] { 0.0f, 0.0f }, hpfPrevOut[2] { 0.0f, 0.0f };

    // note state
    int    noteNumber = 60;
    float  velLevel   = 1.0f;
    float  pitchWheelNorm = 0.0f;
    float  modWheel = 0.0f;

    std::atomic<float>* glideSrc = nullptr;

    juce::Random rng;
};
