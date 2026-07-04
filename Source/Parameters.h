#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Centralised parameter IDs + layout for UB_Poly16 (Juno-106-style poly synth).
namespace ID
{
    // Master / global
    inline constexpr auto masterGain   = "masterGain";
    inline constexpr auto pbRange      = "pbRange";
    inline constexpr auto glide        = "glide";
    inline constexpr auto voiceMode    = "voiceMode";   // Poly / Mono / Legato
    inline constexpr auto notePrio     = "notePrio";    // Last / Low / High

    // DCO 1
    inline constexpr auto o1Octave = "o1Octave";
    inline constexpr auto o1Saw    = "o1Saw";
    inline constexpr auto o1Tri    = "o1Tri";
    inline constexpr auto o1Pulse  = "o1Pulse";
    inline constexpr auto o1Sub    = "o1Sub";
    inline constexpr auto o1Pw     = "o1Pw";
    inline constexpr auto o1Pwm    = "o1Pwm";

    // DCO 2
    inline constexpr auto o2On     = "o2On";
    inline constexpr auto o2Octave = "o2Octave";
    inline constexpr auto o2Detune = "o2Detune";
    inline constexpr auto o2Saw    = "o2Saw";
    inline constexpr auto o2Tri    = "o2Tri";
    inline constexpr auto o2Pulse  = "o2Pulse";
    inline constexpr auto o2Sub    = "o2Sub";
    inline constexpr auto o2Pw     = "o2Pw";
    inline constexpr auto o2Pwm    = "o2Pwm";
    inline constexpr auto o2Sync   = "o2Sync";   // hard sync: DCO1 master resets DCO2

    inline constexpr auto noise    = "noise";

    // Unison
    inline constexpr auto uniVoices = "uniVoices";
    inline constexpr auto uniDetune = "uniDetune";
    inline constexpr auto uniSpread = "uniSpread";

    // HPF (Juno non-resonant high-pass, 4 positions)
    inline constexpr auto hpf      = "hpf";

    // VCF
    inline constexpr auto fType     = "fType";
    inline constexpr auto fCutoff   = "fCutoff";
    inline constexpr auto fReso     = "fReso";
    inline constexpr auto fDrive    = "fDrive";
    inline constexpr auto fEnvAmt   = "fEnvAmt";
    inline constexpr auto fKeyTrack = "fKeyTrack";
    inline constexpr auto fLfo      = "fLfo";
    inline constexpr auto fVelAmt   = "fVelAmt";

    // Amp envelope
    inline constexpr auto aA = "aA";
    inline constexpr auto aD = "aD";
    inline constexpr auto aS = "aS";
    inline constexpr auto aR = "aR";

    // Filter envelope
    inline constexpr auto fA = "fA";
    inline constexpr auto fD = "fD";
    inline constexpr auto fS = "fS";
    inline constexpr auto fR = "fR";

    // LFO
    inline constexpr auto lfoRate  = "lfoRate";
    inline constexpr auto lfoShape = "lfoShape";
    inline constexpr auto lfoDelay = "lfoDelay";
    inline constexpr auto lfoPitch = "lfoPitch";

    // Chorus
    inline constexpr auto chorus = "chorus";

    // Delay
    inline constexpr auto dlyOn   = "dlyOn";
    inline constexpr auto dlySync = "dlySync";
    inline constexpr auto dlyNote = "dlyNote";
    inline constexpr auto dlyTime = "dlyTime";
    inline constexpr auto dlyFb   = "dlyFb";
    inline constexpr auto dlyTone = "dlyTone";
    inline constexpr auto dlyPing = "dlyPing";
    inline constexpr auto dlyMix  = "dlyMix";

    // Reverb
    inline constexpr auto rvbOn    = "rvbOn";
    inline constexpr auto rvbSize  = "rvbSize";
    inline constexpr auto rvbDamp  = "rvbDamp";
    inline constexpr auto rvbWidth = "rvbWidth";
    inline constexpr auto rvbPre   = "rvbPre";
    inline constexpr auto rvbMix   = "rvbMix";

    // Arpeggiator
    inline constexpr auto arpOn   = "arpOn";
    inline constexpr auto arpMode = "arpMode";
    inline constexpr auto arpRate = "arpRate";
    inline constexpr auto arpOct  = "arpOct";
    inline constexpr auto arpGate = "arpGate";
    inline constexpr auto arpHold = "arpHold";
    inline constexpr auto arpSync = "arpSync";
    inline constexpr auto bpm     = "bpm";

    // Chord memory
    inline constexpr auto chordOn = "chordOn";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pid = [] (const char* s) { return ParameterID { s, 1 }; };
    auto lin = [] (float a, float b) { return NormalisableRange<float> (a, b); };
    auto logR = [] (float a, float b)
    {
        NormalisableRange<float> r (a, b);
        r.setSkewForCentre (std::sqrt (a * b));
        return r;
    };
    auto timeR = [] ()
    {
        NormalisableRange<float> r (0.001f, 8.0f);
        r.setSkewForCentre (0.4f);
        return r;
    };

    // ---- Master ----
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::masterGain), "Master", lin (0.0f, 1.0f), 0.6f));
    p.push_back (std::make_unique<AudioParameterInt>   (pid (ID::pbRange), "Pitch Bend Range", 0, 24, 2));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::glide), "Glide", timeR(), 0.0f));
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::voiceMode), "Voice Mode",
        StringArray { "Poly", "Mono", "Legato" }, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::notePrio), "Note Priority",
        StringArray { "Last", "Low", "High" }, 0));

    // ---- DCO 1 ----
    p.push_back (std::make_unique<AudioParameterInt>   (pid (ID::o1Octave), "DCO1 Octave", -2, 2, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o1Saw),   "DCO1 Saw",   lin (0.0f, 1.0f), 0.8f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o1Tri),   "DCO1 Tri",   lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o1Pulse), "DCO1 Pulse", lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o1Sub),   "DCO1 Sub",   lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o1Pw),    "DCO1 PW",    lin (0.05f, 0.95f), 0.5f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o1Pwm),   "DCO1 PWM",   lin (0.0f, 0.45f), 0.0f));

    // ---- DCO 2 ----
    p.push_back (std::make_unique<AudioParameterBool>  (pid (ID::o2On), "DCO2 On", false));
    p.push_back (std::make_unique<AudioParameterInt>   (pid (ID::o2Octave), "DCO2 Octave", -2, 2, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o2Detune), "DCO2 Detune", lin (-50.0f, 50.0f), 7.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o2Saw),   "DCO2 Saw",   lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o2Tri),   "DCO2 Tri",   lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o2Pulse), "DCO2 Pulse", lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o2Sub),   "DCO2 Sub",   lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o2Pw),    "DCO2 PW",    lin (0.05f, 0.95f), 0.5f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::o2Pwm),   "DCO2 PWM",   lin (0.0f, 0.45f), 0.0f));
    p.push_back (std::make_unique<AudioParameterBool>  (pid (ID::o2Sync),  "DCO2 Hard Sync", false));

    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::noise), "Noise", lin (0.0f, 1.0f), 0.0f));

    // ---- Unison ----
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::uniVoices), "Unison Voices",
        StringArray { "1", "2", "3", "4", "5", "6", "7" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::uniDetune), "Unison Detune", lin (0.0f, 50.0f), 12.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::uniSpread), "Unison Spread", lin (0.0f, 1.0f), 0.7f));

    // ---- HPF ----
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::hpf), "HPF",
        StringArray { "Off", "1", "2", "3" }, 0));

    // ---- VCF ----
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::fType), "Filter Type",
        StringArray { "LP24 (Juno)", "LP12", "HP24", "HP12", "BP24", "BP12" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fCutoff), "Cutoff", logR (20.0f, 20000.0f), 8000.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fReso),   "Resonance", lin (0.0f, 1.0f), 0.1f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fDrive),  "Drive", lin (1.0f, 10.0f), 1.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fEnvAmt), "Env -> Cutoff", lin (-1.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fKeyTrack), "Key Track", lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fLfo), "LFO -> Cutoff", lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fVelAmt), "Vel -> Cutoff", lin (0.0f, 1.0f), 0.0f));

    // ---- Amp env ----
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::aA), "Amp Attack",  timeR(), 0.01f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::aD), "Amp Decay",   timeR(), 0.30f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::aS), "Amp Sustain", lin (0.0f, 1.0f), 0.8f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::aR), "Amp Release", timeR(), 0.20f));

    // ---- Filter env ----
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fA), "Filt Attack",  timeR(), 0.01f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fD), "Filt Decay",   timeR(), 0.40f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fS), "Filt Sustain", lin (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::fR), "Filt Release", timeR(), 0.30f));

    // ---- LFO ----
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::lfoRate), "LFO Rate", logR (0.05f, 20.0f), 5.0f));
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::lfoShape), "LFO Shape",
        StringArray { "Triangle", "Sine", "Saw", "Square", "S&H" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::lfoDelay), "LFO Delay", lin (0.0f, 5.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::lfoPitch), "LFO -> Pitch", lin (0.0f, 1.0f), 0.0f));

    // ---- Chorus ----
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::chorus), "Chorus",
        StringArray { "Off", "I", "II", "I+II" }, 1));

    // ---- Delay ----
    p.push_back (std::make_unique<AudioParameterBool>  (pid (ID::dlyOn), "Delay On", false));
    p.push_back (std::make_unique<AudioParameterBool>  (pid (ID::dlySync), "Delay Sync", true));
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::dlyNote), "Delay Note",
        StringArray { "1/4", "1/4.", "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/16T" }, 3));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::dlyTime), "Delay Time", logR (10.0f, 2000.0f), 350.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::dlyFb),   "Delay Feedback", lin (0.0f, 0.95f), 0.45f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::dlyTone), "Delay Tone", logR (500.0f, 16000.0f), 6000.0f));
    p.push_back (std::make_unique<AudioParameterBool>  (pid (ID::dlyPing), "Delay Ping-Pong", true));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::dlyMix),  "Delay Mix", lin (0.0f, 1.0f), 0.3f));

    // ---- Reverb ----
    p.push_back (std::make_unique<AudioParameterBool>  (pid (ID::rvbOn), "Reverb On", false));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::rvbSize),  "Reverb Size",  lin (0.0f, 1.0f), 0.55f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::rvbDamp),  "Reverb Damp",  lin (0.0f, 1.0f), 0.45f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::rvbWidth), "Reverb Width", lin (0.0f, 1.0f), 1.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::rvbPre),   "Reverb PreDelay", lin (0.0f, 200.0f), 20.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::rvbMix),   "Reverb Mix",   lin (0.0f, 1.0f), 0.3f));

    // ---- Arp ----
    p.push_back (std::make_unique<AudioParameterBool>  (pid (ID::arpOn), "Arp On", false));
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::arpMode), "Arp Mode",
        StringArray { "Up", "Down", "Up/Down", "Random", "As Played" }, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (pid (ID::arpRate), "Arp Rate",
        StringArray { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3));
    p.push_back (std::make_unique<AudioParameterInt>   (pid (ID::arpOct), "Arp Octaves", 1, 4, 1));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::arpGate), "Arp Gate", lin (0.05f, 1.0f), 0.5f));
    p.push_back (std::make_unique<AudioParameterBool>  (pid (ID::arpHold), "Arp Hold", false));
    p.push_back (std::make_unique<AudioParameterBool>  (pid (ID::arpSync), "Arp Sync DAW", true));
    p.push_back (std::make_unique<AudioParameterFloat> (pid (ID::bpm), "Tempo", lin (40.0f, 300.0f), 120.0f));

    // ---- Chord memory ----
    p.push_back (std::make_unique<AudioParameterBool> (pid (ID::chordOn), "Chord On", false));

    return { p.begin(), p.end() };
}
