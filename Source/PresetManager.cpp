#include "PresetManager.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s)
{
    userDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                  .getChildFile ("UB_Poly16").getChildFile ("Presets");
    if (! userDir.exists())
        userDir.createDirectory();

    buildFactory();
    currentName = factory.empty() ? "Init" : factory.front().name;
}

void PresetManager::buildFactory()
{
    using V = std::vector<std::pair<juce::String, float>>;
    auto add = [this] (const char* n, V v) { factory.push_back ({ n, std::move (v) }); };

    add ("Init", {});

    // ---------------- BASS ----------------
    add ("BS Deep Sub", { { ID::o1Saw, 0.0f }, { ID::o1Sub, 1.0f }, { ID::o1Octave, -1.0f },
        { ID::fCutoff, 500.0f }, { ID::fReso, 0.2f }, { ID::fEnvAmt, 0.4f }, { ID::fD, 0.3f },
        { ID::aD, 0.3f }, { ID::aS, 0.5f }, { ID::aR, 0.1f }, { ID::chorus, 0.0f } });
    add ("BS Round Saw", { { ID::o1Saw, 0.9f }, { ID::o1Sub, 0.5f }, { ID::o1Octave, -1.0f },
        { ID::fCutoff, 700.0f }, { ID::fReso, 0.25f }, { ID::fEnvAmt, 0.5f }, { ID::fD, 0.35f }, { ID::fS, 0.1f },
        { ID::aS, 0.6f }, { ID::aR, 0.12f }, { ID::chorus, 0.0f } });
    add ("BS Reese", { { ID::o1Saw, 1.0f }, { ID::o1Octave, -1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 1.0f },
        { ID::o2Octave, -1.0f }, { ID::o2Detune, 25.0f }, { ID::fCutoff, 600.0f }, { ID::fReso, 0.3f },
        { ID::fEnvAmt, 0.3f }, { ID::aS, 0.7f }, { ID::aR, 0.2f }, { ID::chorus, 2.0f } });
    add ("BS Acid", { { ID::o1Saw, 1.0f }, { ID::o1Octave, -1.0f }, { ID::fType, 0.0f }, { ID::fCutoff, 400.0f },
        { ID::fReso, 0.8f }, { ID::fEnvAmt, 0.7f }, { ID::fD, 0.25f }, { ID::fS, 0.1f }, { ID::fDrive, 3.0f },
        { ID::aS, 0.4f }, { ID::aR, 0.08f }, { ID::glide, 0.04f } });
    add ("BS Square", { { ID::o1Pulse, 1.0f }, { ID::o1Pw, 0.5f }, { ID::o1Octave, -1.0f },
        { ID::fCutoff, 800.0f }, { ID::fReso, 0.2f }, { ID::fEnvAmt, 0.4f }, { ID::fD, 0.3f },
        { ID::aS, 0.6f }, { ID::aR, 0.1f } });
    add ("BS Pluck", { { ID::o1Saw, 0.8f }, { ID::o1Sub, 0.6f }, { ID::o1Octave, -1.0f },
        { ID::fCutoff, 1200.0f }, { ID::fReso, 0.4f }, { ID::fEnvAmt, 0.7f }, { ID::fD, 0.18f }, { ID::fS, 0.0f },
        { ID::aA, 0.005f }, { ID::aD, 0.2f }, { ID::aS, 0.0f }, { ID::aR, 0.1f } });
    add ("BS Growl", { { ID::o1Saw, 1.0f }, { ID::o1Pulse, 0.5f }, { ID::o1Octave, -1.0f },
        { ID::fCutoff, 500.0f }, { ID::fReso, 0.5f }, { ID::fEnvAmt, 0.5f }, { ID::fLfo, 0.3f }, { ID::lfoRate, 6.0f },
        { ID::fDrive, 4.0f }, { ID::aS, 0.6f } });
    add ("BS Wide Sub", { { ID::o1Sub, 1.0f }, { ID::o1Saw, 0.4f }, { ID::o1Octave, -1.0f }, { ID::o2On, 1.0f },
        { ID::o2Sub, 1.0f }, { ID::o2Octave, -1.0f }, { ID::o2Detune, 12.0f }, { ID::fCutoff, 600.0f },
        { ID::chorus, 1.0f } });
    add ("BS Mono Glide", { { ID::o1Saw, 1.0f }, { ID::o1Octave, -1.0f }, { ID::glide, 0.08f },
        { ID::fCutoff, 700.0f }, { ID::fReso, 0.3f }, { ID::fEnvAmt, 0.4f }, { ID::aS, 0.7f } });
    add ("BS Hard Saw", { { ID::o1Saw, 1.0f }, { ID::o1Octave, -1.0f }, { ID::fCutoff, 900.0f }, { ID::fReso, 0.35f },
        { ID::fEnvAmt, 0.6f }, { ID::fDrive, 2.0f }, { ID::fD, 0.25f }, { ID::fS, 0.2f }, { ID::aS, 0.5f }, { ID::aR, 0.1f } });

    // ---------------- LEAD ----------------
    add ("LD Saw Lead", { { ID::o1Saw, 1.0f }, { ID::fCutoff, 5000.0f }, { ID::fReso, 0.2f }, { ID::fEnvAmt, 0.2f },
        { ID::aS, 0.8f }, { ID::aR, 0.2f }, { ID::lfoPitch, 0.08f }, { ID::lfoDelay, 0.5f }, { ID::lfoRate, 5.5f },
        { ID::chorus, 1.0f } });
    add ("LD Square Lead", { { ID::o1Pulse, 1.0f }, { ID::o1Pw, 0.5f }, { ID::fCutoff, 4000.0f }, { ID::fReso, 0.25f },
        { ID::aS, 0.8f }, { ID::lfoPitch, 0.1f }, { ID::lfoDelay, 0.6f }, { ID::chorus, 0.0f } });
    add ("LD PWM Lead", { { ID::o1Pulse, 1.0f }, { ID::o1Pw, 0.5f }, { ID::o1Pwm, 0.3f }, { ID::lfoRate, 1.0f },
        { ID::fCutoff, 4500.0f }, { ID::aS, 0.8f }, { ID::chorus, 1.0f } });
    add ("LD Octave", { { ID::o1Saw, 0.8f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.8f }, { ID::o2Octave, 1.0f },
        { ID::fCutoff, 5000.0f }, { ID::fReso, 0.2f }, { ID::aS, 0.8f }, { ID::chorus, 1.0f } });
    add ("LD Soft", { { ID::o1Saw, 0.6f }, { ID::o1Sub, 0.3f }, { ID::fCutoff, 3000.0f }, { ID::fReso, 0.1f },
        { ID::aA, 0.05f }, { ID::aS, 0.8f }, { ID::aR, 0.3f }, { ID::chorus, 3.0f } });
    add ("LD Hard", { { ID::o1Saw, 1.0f }, { ID::o1Pulse, 0.5f }, { ID::fCutoff, 7000.0f }, { ID::fReso, 0.4f },
        { ID::fEnvAmt, 0.3f }, { ID::fDrive, 3.0f }, { ID::aS, 0.7f }, { ID::glide, 0.03f } });
    add ("LD Porta", { { ID::o1Saw, 1.0f }, { ID::glide, 0.12f }, { ID::fCutoff, 5000.0f }, { ID::fReso, 0.25f },
        { ID::aS, 0.8f }, { ID::lfoPitch, 0.1f } });
    add ("LD Bright", { { ID::o1Saw, 1.0f }, { ID::o1Pulse, 0.5f }, { ID::fCutoff, 9000.0f }, { ID::fReso, 0.15f },
        { ID::aS, 0.85f }, { ID::chorus, 1.0f } });
    add ("LD Hollow", { { ID::o1Pulse, 1.0f }, { ID::o1Pw, 0.85f }, { ID::fCutoff, 3500.0f }, { ID::fReso, 0.3f },
        { ID::aS, 0.8f }, { ID::lfoPitch, 0.07f }, { ID::lfoDelay, 0.4f }, { ID::chorus, 1.0f } });
    add ("LD Vibrato", { { ID::o1Saw, 1.0f }, { ID::fCutoff, 5000.0f }, { ID::lfoPitch, 0.25f }, { ID::lfoRate, 6.0f },
        { ID::lfoDelay, 0.3f }, { ID::aS, 0.8f }, { ID::chorus, 1.0f } });
    add ("LD Echo", { { ID::o1Saw, 1.0f }, { ID::fCutoff, 5500.0f }, { ID::fReso, 0.2f },
        { ID::aS, 0.8f }, { ID::aR, 0.25f }, { ID::lfoPitch, 0.08f }, { ID::lfoDelay, 0.5f }, { ID::chorus, 1.0f },
        { ID::dlyOn, 1.0f }, { ID::dlyFb, 0.45f }, { ID::dlyMix, 0.35f } });
    add ("LD Supersaw", { { ID::o1Saw, 1.0f }, { ID::uniVoices, 6.0f }, { ID::uniDetune, 22.0f }, { ID::uniSpread, 1.0f },
        { ID::hpf, 1.0f }, { ID::fCutoff, 8000.0f }, { ID::fReso, 0.1f },
        { ID::aA, 0.005f }, { ID::aS, 0.8f }, { ID::aR, 0.35f }, { ID::chorus, 0.0f },
        { ID::dlyOn, 1.0f }, { ID::dlyNote, 2.0f }, { ID::dlyFb, 0.35f }, { ID::dlyMix, 0.18f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.6f }, { ID::rvbMix, 0.25f } });

    // ---------------- PAD ----------------
    add ("PD Warm", { { ID::o1Saw, 0.7f }, { ID::o1Sub, 0.4f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.6f }, { ID::o2Detune, 10.0f },
        { ID::fCutoff, 1800.0f }, { ID::fReso, 0.15f }, { ID::fEnvAmt, 0.3f }, { ID::fA, 0.8f },
        { ID::aA, 0.8f }, { ID::aD, 1.0f }, { ID::aS, 0.8f }, { ID::aR, 1.2f }, { ID::lfoPitch, 0.05f }, { ID::chorus, 3.0f } });
    add ("PD String", { { ID::o1Saw, 0.8f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.8f }, { ID::o2Detune, 8.0f },
        { ID::fCutoff, 2500.0f }, { ID::fEnvAmt, 0.2f }, { ID::fA, 1.0f }, { ID::aA, 0.6f }, { ID::aR, 1.0f }, { ID::chorus, 3.0f } });
    add ("PD Glass", { { ID::o1Pulse, 0.7f }, { ID::o1Pwm, 0.2f }, { ID::lfoRate, 0.5f }, { ID::o2On, 1.0f },
        { ID::o2Pulse, 0.7f }, { ID::o2Detune, 6.0f }, { ID::fType, 1.0f }, { ID::fCutoff, 3000.0f },
        { ID::aA, 1.0f }, { ID::aR, 1.5f }, { ID::chorus, 3.0f } });
    add ("PD Sweep", { { ID::o1Saw, 0.9f }, { ID::fCutoff, 800.0f }, { ID::fReso, 0.4f }, { ID::fEnvAmt, 0.8f },
        { ID::fA, 2.0f }, { ID::fD, 2.0f }, { ID::fS, 0.5f }, { ID::aA, 1.0f }, { ID::aR, 1.5f }, { ID::chorus, 2.0f } });
    add ("PD Dark", { { ID::o1Saw, 0.6f }, { ID::o1Sub, 0.5f }, { ID::fCutoff, 900.0f }, { ID::fReso, 0.2f },
        { ID::aA, 1.0f }, { ID::aR, 1.8f }, { ID::chorus, 3.0f } });
    add ("PD Choir", { { ID::o1Pulse, 0.6f }, { ID::o1Pw, 0.5f }, { ID::o2On, 1.0f }, { ID::o2Pulse, 0.6f },
        { ID::o2Detune, 7.0f }, { ID::hpf, 1.0f }, { ID::fCutoff, 2200.0f }, { ID::aA, 0.9f }, { ID::aR, 1.4f }, { ID::chorus, 3.0f } });
    add ("PD Wide Detune", { { ID::o1Saw, 0.8f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.8f }, { ID::o2Detune, 20.0f },
        { ID::fCutoff, 2000.0f }, { ID::aA, 0.7f }, { ID::aR, 1.2f }, { ID::chorus, 2.0f } });
    add ("PD Slow Swell", { { ID::o1Saw, 0.7f }, { ID::o1Sub, 0.3f }, { ID::fCutoff, 1500.0f }, { ID::fEnvAmt, 0.5f },
        { ID::fA, 3.0f }, { ID::aA, 3.0f }, { ID::aD, 2.0f }, { ID::aS, 0.8f }, { ID::aR, 2.5f }, { ID::chorus, 3.0f } });
    add ("PD Air", { { ID::o1Saw, 0.5f }, { ID::noise, 0.15f }, { ID::hpf, 2.0f }, { ID::fCutoff, 4000.0f },
        { ID::aA, 1.2f }, { ID::aR, 1.6f }, { ID::chorus, 3.0f } });
    add ("PD Mello", { { ID::o1Saw, 0.6f }, { ID::o1Sub, 0.4f }, { ID::fCutoff, 1300.0f }, { ID::fReso, 0.1f },
        { ID::aA, 0.7f }, { ID::aD, 1.0f }, { ID::aS, 0.7f }, { ID::aR, 1.2f }, { ID::chorus, 1.0f } });
    add ("PD Cathedral", { { ID::o1Saw, 0.6f }, { ID::o1Sub, 0.3f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.5f },
        { ID::o2Detune, 9.0f }, { ID::fCutoff, 2000.0f }, { ID::aA, 1.2f }, { ID::aS, 0.8f }, { ID::aR, 2.5f },
        { ID::chorus, 3.0f }, { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.92f }, { ID::rvbDamp, 0.35f },
        { ID::rvbPre, 40.0f }, { ID::rvbMix, 0.45f } });

    // ---- JP-8000 style supersaw pads ----
    add ("PD JP Super", { { ID::o1Saw, 1.0f }, { ID::uniVoices, 6.0f }, { ID::uniDetune, 15.0f }, { ID::uniSpread, 1.0f },
        { ID::hpf, 1.0f }, { ID::fCutoff, 3200.0f }, { ID::fReso, 0.1f },
        { ID::aA, 0.9f }, { ID::aD, 1.0f }, { ID::aS, 0.85f }, { ID::aR, 1.8f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.7f }, { ID::rvbPre, 30.0f }, { ID::rvbMix, 0.3f } });
    add ("PD JP Strings", { { ID::o1Saw, 0.9f }, { ID::uniVoices, 4.0f }, { ID::uniDetune, 10.0f }, { ID::uniSpread, 0.8f },
        { ID::hpf, 1.0f }, { ID::fCutoff, 2600.0f },
        { ID::aA, 0.4f }, { ID::aD, 1.0f }, { ID::aS, 0.8f }, { ID::aR, 1.2f }, { ID::chorus, 2.0f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.55f }, { ID::rvbMix, 0.22f } });
    add ("PD JP Ocean", { { ID::o1Saw, 0.9f }, { ID::o1Sub, 0.25f }, { ID::uniVoices, 6.0f }, { ID::uniDetune, 18.0f },
        { ID::uniSpread, 1.0f }, { ID::fCutoff, 700.0f }, { ID::fReso, 0.25f }, { ID::fEnvAmt, 0.55f },
        { ID::fA, 3.0f }, { ID::fD, 3.0f }, { ID::fS, 0.6f }, { ID::fR, 2.0f },
        { ID::aA, 1.5f }, { ID::aS, 0.85f }, { ID::aR, 2.5f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.85f }, { ID::rvbPre, 50.0f }, { ID::rvbMix, 0.38f } });
    add ("PD JP Heaven", { { ID::o1Saw, 0.8f }, { ID::uniVoices, 6.0f }, { ID::uniDetune, 12.0f }, { ID::uniSpread, 1.0f },
        { ID::hpf, 2.0f }, { ID::fCutoff, 6500.0f }, { ID::lfoPitch, 0.04f }, { ID::lfoRate, 0.8f },
        { ID::aA, 1.8f }, { ID::aS, 0.8f }, { ID::aR, 3.0f },
        { ID::dlyOn, 1.0f }, { ID::dlyNote, 1.0f }, { ID::dlyFb, 0.4f }, { ID::dlyMix, 0.2f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.8f }, { ID::rvbDamp, 0.3f }, { ID::rvbMix, 0.4f } });
    add ("PD JP Deep Wide", { { ID::o1Saw, 0.9f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.7f }, { ID::o2Octave, -1.0f },
        { ID::o2Detune, 5.0f }, { ID::uniVoices, 4.0f }, { ID::uniDetune, 12.0f }, { ID::uniSpread, 0.9f },
        { ID::fCutoff, 1800.0f }, { ID::fReso, 0.15f },
        { ID::aA, 0.7f }, { ID::aD, 1.2f }, { ID::aS, 0.8f }, { ID::aR, 2.0f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.65f }, { ID::rvbMix, 0.28f } });

    // ---------------- KEYS ----------------
    add ("KY E-Piano", { { ID::o1Pulse, 0.7f }, { ID::o1Pw, 0.4f }, { ID::o1Sub, 0.2f }, { ID::fCutoff, 3500.0f },
        { ID::fReso, 0.2f }, { ID::fEnvAmt, 0.4f }, { ID::fD, 0.6f }, { ID::fS, 0.1f },
        { ID::aA, 0.005f }, { ID::aD, 0.8f }, { ID::aS, 0.3f }, { ID::aR, 0.4f }, { ID::chorus, 2.0f } });
    add ("KY Clav", { { ID::o1Pulse, 1.0f }, { ID::o1Pw, 0.3f }, { ID::fCutoff, 4000.0f }, { ID::fReso, 0.4f },
        { ID::fEnvAmt, 0.5f }, { ID::fD, 0.3f }, { ID::fS, 0.0f }, { ID::aA, 0.003f }, { ID::aD, 0.25f },
        { ID::aS, 0.2f }, { ID::aR, 0.15f }, { ID::chorus, 1.0f } });
    add ("KY Organ", { { ID::o1Saw, 0.4f }, { ID::o1Sub, 0.6f }, { ID::o2On, 1.0f }, { ID::o2Sub, 0.5f }, { ID::o2Octave, 1.0f },
        { ID::fCutoff, 5000.0f }, { ID::aA, 0.01f }, { ID::aS, 1.0f }, { ID::aR, 0.1f }, { ID::chorus, 2.0f } });
    add ("KY Bell", { { ID::o1Pulse, 0.6f }, { ID::o1Octave, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Pulse, 0.5f }, { ID::o2Detune, 15.0f },
        { ID::fType, 1.0f }, { ID::fCutoff, 6000.0f }, { ID::fReso, 0.3f }, { ID::fEnvAmt, 0.3f }, { ID::fD, 1.0f },
        { ID::aA, 0.005f }, { ID::aD, 1.5f }, { ID::aS, 0.0f }, { ID::aR, 1.0f }, { ID::chorus, 2.0f } });
    add ("KY Harpsi", { { ID::o1Saw, 0.8f }, { ID::o1Pulse, 0.4f }, { ID::fCutoff, 5000.0f }, { ID::fReso, 0.3f },
        { ID::fEnvAmt, 0.5f }, { ID::fD, 0.4f }, { ID::fS, 0.0f }, { ID::aA, 0.003f }, { ID::aD, 0.5f },
        { ID::aS, 0.0f }, { ID::aR, 0.3f }, { ID::chorus, 1.0f } });
    add ("KY Synth Piano", { { ID::o1Saw, 0.7f }, { ID::o1Sub, 0.3f }, { ID::fCutoff, 4000.0f }, { ID::fEnvAmt, 0.4f },
        { ID::fD, 0.7f }, { ID::fS, 0.2f }, { ID::aA, 0.005f }, { ID::aD, 1.0f }, { ID::aS, 0.2f }, { ID::aR, 0.4f }, { ID::chorus, 2.0f } });
    add ("KY Soft Keys", { { ID::o1Saw, 0.5f }, { ID::o1Sub, 0.4f }, { ID::fCutoff, 2500.0f },
        { ID::aA, 0.02f }, { ID::aD, 0.8f }, { ID::aS, 0.4f }, { ID::aR, 0.5f }, { ID::chorus, 3.0f } });
    add ("KY Pluck Keys", { { ID::o1Saw, 0.8f }, { ID::o1Pulse, 0.3f }, { ID::fCutoff, 5000.0f }, { ID::fReso, 0.3f },
        { ID::fEnvAmt, 0.6f }, { ID::fD, 0.3f }, { ID::fS, 0.0f }, { ID::aA, 0.004f }, { ID::aD, 0.35f },
        { ID::aS, 0.0f }, { ID::aR, 0.25f }, { ID::chorus, 1.0f } });
    add ("KY Velo Touch", { { ID::o1Saw, 0.7f }, { ID::o1Sub, 0.3f }, { ID::fCutoff, 5000.0f }, { ID::fReso, 0.2f },
        { ID::fEnvAmt, 0.3f }, { ID::fD, 0.6f }, { ID::fS, 0.1f }, { ID::fVelAmt, 0.7f },
        { ID::aA, 0.004f }, { ID::aD, 0.9f }, { ID::aS, 0.3f }, { ID::aR, 0.4f }, { ID::chorus, 2.0f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.4f }, { ID::rvbMix, 0.18f } });

    // ---------------- PLUCK ----------------
    add ("PL Synth", { { ID::o1Saw, 0.8f }, { ID::o1Pulse, 0.4f }, { ID::fCutoff, 6000.0f }, { ID::fReso, 0.4f },
        { ID::fEnvAmt, 0.7f }, { ID::fD, 0.25f }, { ID::fS, 0.0f }, { ID::aA, 0.005f }, { ID::aD, 0.25f },
        { ID::aS, 0.0f }, { ID::aR, 0.15f }, { ID::chorus, 1.0f } });
    add ("PL Short", { { ID::o1Saw, 1.0f }, { ID::fCutoff, 5000.0f }, { ID::fReso, 0.5f }, { ID::fEnvAmt, 0.8f },
        { ID::fD, 0.15f }, { ID::fS, 0.0f }, { ID::aA, 0.002f }, { ID::aD, 0.15f }, { ID::aS, 0.0f }, { ID::aR, 0.1f } });
    add ("PL Bright", { { ID::o1Saw, 0.7f }, { ID::o1Pulse, 0.5f }, { ID::fCutoff, 8000.0f }, { ID::fReso, 0.3f },
        { ID::fEnvAmt, 0.5f }, { ID::fD, 0.2f }, { ID::fS, 0.0f }, { ID::aA, 0.002f }, { ID::aD, 0.2f },
        { ID::aS, 0.0f }, { ID::aR, 0.12f }, { ID::chorus, 1.0f } });
    add ("PL Mallet", { { ID::o1Pulse, 0.6f }, { ID::o1Sub, 0.4f }, { ID::o1Octave, 1.0f }, { ID::fType, 1.0f },
        { ID::fCutoff, 4000.0f }, { ID::fEnvAmt, 0.6f }, { ID::fD, 0.25f }, { ID::aA, 0.002f }, { ID::aD, 0.3f },
        { ID::aS, 0.0f }, { ID::aR, 0.2f } });
    add ("PL Stab", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 1.0f }, { ID::o2Detune, 10.0f },
        { ID::fCutoff, 4000.0f }, { ID::fReso, 0.4f }, { ID::fEnvAmt, 0.6f }, { ID::fD, 0.2f }, { ID::fS, 0.0f },
        { ID::aA, 0.003f }, { ID::aD, 0.22f }, { ID::aS, 0.0f }, { ID::aR, 0.15f }, { ID::chorus, 1.0f } });
    add ("PL Click", { { ID::o1Pulse, 1.0f }, { ID::o1Pw, 0.2f }, { ID::noise, 0.1f }, { ID::fCutoff, 6000.0f },
        { ID::fReso, 0.4f }, { ID::fEnvAmt, 0.7f }, { ID::fD, 0.12f }, { ID::fS, 0.0f }, { ID::aA, 0.001f },
        { ID::aD, 0.12f }, { ID::aS, 0.0f }, { ID::aR, 0.08f } });
    add ("PL Dual", { { ID::o1Saw, 0.7f }, { ID::o2On, 1.0f }, { ID::o2Pulse, 0.7f }, { ID::o2Octave, 1.0f }, { ID::o2Detune, 6.0f },
        { ID::fCutoff, 5500.0f }, { ID::fEnvAmt, 0.5f }, { ID::fD, 0.25f }, { ID::fS, 0.0f }, { ID::aA, 0.003f },
        { ID::aD, 0.28f }, { ID::aS, 0.0f }, { ID::aR, 0.2f }, { ID::chorus, 1.0f } });
    add ("PL Wood", { { ID::o1Pulse, 0.5f }, { ID::o1Sub, 0.5f }, { ID::fType, 0.0f }, { ID::fCutoff, 2500.0f },
        { ID::fReso, 0.3f }, { ID::fEnvAmt, 0.5f }, { ID::fD, 0.2f }, { ID::aA, 0.002f }, { ID::aD, 0.25f },
        { ID::aS, 0.0f }, { ID::aR, 0.15f } });

    // ---------------- BRASS / STRINGS ----------------
    add ("BR Classic", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 1.0f }, { ID::o2Detune, 8.0f },
        { ID::fCutoff, 2500.0f }, { ID::fReso, 0.2f }, { ID::fEnvAmt, 0.5f }, { ID::fD, 0.5f }, { ID::fS, 0.3f },
        { ID::aA, 0.02f }, { ID::aD, 0.4f }, { ID::aS, 0.7f }, { ID::aR, 0.3f }, { ID::chorus, 1.0f } });
    add ("BR Soft", { { ID::o1Saw, 0.8f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.7f }, { ID::o2Detune, 6.0f },
        { ID::fCutoff, 2000.0f }, { ID::fEnvAmt, 0.4f }, { ID::fA, 0.1f }, { ID::fD, 0.6f }, { ID::fS, 0.4f },
        { ID::aA, 0.08f }, { ID::aS, 0.8f }, { ID::aR, 0.4f }, { ID::chorus, 2.0f } });
    add ("BR Big", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 1.0f }, { ID::o2Detune, 12.0f },
        { ID::fCutoff, 3000.0f }, { ID::fReso, 0.25f }, { ID::fEnvAmt, 0.6f }, { ID::fD, 0.5f }, { ID::fS, 0.3f },
        { ID::aA, 0.03f }, { ID::aS, 0.7f }, { ID::aR, 0.3f }, { ID::chorus, 3.0f } });
    add ("BR Octave", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 1.0f }, { ID::o2Octave, 1.0f }, { ID::o2Detune, 5.0f },
        { ID::fCutoff, 3000.0f }, { ID::fEnvAmt, 0.5f }, { ID::aA, 0.02f }, { ID::aS, 0.7f }, { ID::aR, 0.3f }, { ID::chorus, 1.0f } });
    add ("ST Synth Strings", { { ID::o1Saw, 0.8f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.8f }, { ID::o2Detune, 10.0f },
        { ID::fCutoff, 2200.0f }, { ID::fEnvAmt, 0.3f }, { ID::fA, 0.5f }, { ID::aA, 0.4f }, { ID::aD, 1.0f },
        { ID::aS, 0.8f }, { ID::aR, 1.0f }, { ID::chorus, 3.0f } });
    add ("ST Sweep Strings", { { ID::o1Saw, 0.9f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.9f }, { ID::o2Detune, 9.0f },
        { ID::fCutoff, 1200.0f }, { ID::fReso, 0.3f }, { ID::fEnvAmt, 0.7f }, { ID::fA, 1.5f }, { ID::fD, 1.5f }, { ID::fS, 0.5f },
        { ID::aA, 0.5f }, { ID::aR, 1.2f }, { ID::chorus, 3.0f } });
    add ("ST Analog", { { ID::o1Saw, 0.7f }, { ID::o1Sub, 0.3f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.7f }, { ID::o2Detune, 8.0f },
        { ID::fCutoff, 2000.0f }, { ID::aA, 0.3f }, { ID::aR, 0.9f }, { ID::chorus, 2.0f } });
    add ("ST Ensemble", { { ID::o1Saw, 0.8f }, { ID::o1Pulse, 0.4f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.8f },
        { ID::o2Pulse, 0.4f }, { ID::o2Detune, 11.0f }, { ID::fCutoff, 2400.0f }, { ID::aA, 0.4f }, { ID::aR, 1.0f }, { ID::chorus, 3.0f } });

    // ---------------- ARP ----------------
    add ("AR Trance", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.7f }, { ID::o2Detune, 7.0f },
        { ID::fCutoff, 2000.0f }, { ID::fReso, 0.5f }, { ID::fEnvAmt, 0.6f }, { ID::fD, 0.2f }, { ID::fS, 0.1f },
        { ID::aA, 0.005f }, { ID::aD, 0.3f }, { ID::aS, 0.6f }, { ID::aR, 0.1f },
        { ID::arpOn, 1.0f }, { ID::arpMode, 0.0f }, { ID::arpRate, 3.0f }, { ID::arpOct, 2.0f }, { ID::arpGate, 0.5f }, { ID::chorus, 1.0f } });
    add ("AR Up", { { ID::o1Saw, 0.9f }, { ID::fCutoff, 4000.0f }, { ID::fReso, 0.3f }, { ID::fEnvAmt, 0.5f },
        { ID::fD, 0.2f }, { ID::fS, 0.0f }, { ID::aA, 0.003f }, { ID::aD, 0.25f }, { ID::aS, 0.0f }, { ID::aR, 0.1f },
        { ID::arpOn, 1.0f }, { ID::arpMode, 0.0f }, { ID::arpRate, 3.0f }, { ID::arpOct, 1.0f }, { ID::arpGate, 0.5f }, { ID::chorus, 1.0f } });
    add ("AR UpDown Oct", { { ID::o1Saw, 0.8f }, { ID::o1Pulse, 0.4f }, { ID::fCutoff, 3500.0f }, { ID::fReso, 0.4f },
        { ID::fEnvAmt, 0.5f }, { ID::fD, 0.2f }, { ID::fS, 0.0f }, { ID::arpOn, 1.0f }, { ID::arpMode, 2.0f },
        { ID::arpRate, 3.0f }, { ID::arpOct, 2.0f }, { ID::arpGate, 0.6f }, { ID::chorus, 1.0f } });
    add ("AR Random", { { ID::o1Saw, 1.0f }, { ID::fCutoff, 3000.0f }, { ID::fReso, 0.5f }, { ID::fEnvAmt, 0.6f },
        { ID::fD, 0.18f }, { ID::fS, 0.0f }, { ID::arpOn, 1.0f }, { ID::arpMode, 3.0f }, { ID::arpRate, 4.0f },
        { ID::arpOct, 2.0f }, { ID::arpGate, 0.4f }, { ID::chorus, 2.0f } });
    add ("AR Fast 32", { { ID::o1Pulse, 1.0f }, { ID::o1Pw, 0.4f }, { ID::fCutoff, 4000.0f }, { ID::fReso, 0.4f },
        { ID::fEnvAmt, 0.5f }, { ID::fD, 0.12f }, { ID::fS, 0.0f }, { ID::arpOn, 1.0f }, { ID::arpMode, 0.0f },
        { ID::arpRate, 5.0f }, { ID::arpOct, 1.0f }, { ID::arpGate, 0.4f }, { ID::chorus, 1.0f } });
    add ("AR Hold Pad", { { ID::o1Saw, 0.8f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.7f }, { ID::o2Detune, 8.0f },
        { ID::fCutoff, 2500.0f }, { ID::fEnvAmt, 0.3f }, { ID::aR, 0.6f }, { ID::arpOn, 1.0f }, { ID::arpHold, 1.0f },
        { ID::arpMode, 2.0f }, { ID::arpRate, 3.0f }, { ID::arpOct, 2.0f }, { ID::arpGate, 0.7f }, { ID::chorus, 3.0f } });

    // ---------------- FX / MISC ----------------
    add ("FX Noise Sweep", { { ID::noise, 0.8f }, { ID::o1Saw, 0.2f }, { ID::fType, 0.0f }, { ID::fCutoff, 300.0f },
        { ID::fReso, 0.6f }, { ID::fEnvAmt, 0.9f }, { ID::fA, 2.0f }, { ID::fD, 2.0f }, { ID::aA, 1.0f }, { ID::aR, 1.5f }, { ID::chorus, 2.0f } });
    add ("FX Drone", { { ID::o1Saw, 0.7f }, { ID::o1Sub, 0.5f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.7f }, { ID::o2Detune, 30.0f },
        { ID::fCutoff, 1500.0f }, { ID::fReso, 0.4f }, { ID::fLfo, 0.4f }, { ID::lfoRate, 0.2f },
        { ID::aA, 1.5f }, { ID::aS, 0.9f }, { ID::aR, 2.0f }, { ID::chorus, 3.0f } });
    add ("FX Sci-Fi", { { ID::o1Pulse, 1.0f }, { ID::o1Pwm, 0.4f }, { ID::lfoRate, 8.0f }, { ID::fCutoff, 3000.0f },
        { ID::fReso, 0.7f }, { ID::fEnvAmt, 0.5f }, { ID::fLfo, 0.5f }, { ID::lfoPitch, 0.3f }, { ID::aS, 0.7f }, { ID::aR, 0.5f }, { ID::chorus, 2.0f } });
    add ("FX Wind", { { ID::noise, 1.0f }, { ID::hpf, 2.0f }, { ID::fType, 0.0f }, { ID::fCutoff, 800.0f },
        { ID::fReso, 0.3f }, { ID::fLfo, 0.6f }, { ID::lfoRate, 0.3f }, { ID::aA, 1.5f }, { ID::aS, 0.8f }, { ID::aR, 2.0f } });
    add ("FX Dub Space", { { ID::o1Pulse, 0.8f }, { ID::o1Octave, -1.0f }, { ID::fCutoff, 1500.0f }, { ID::fReso, 0.5f },
        { ID::aA, 0.003f }, { ID::aD, 0.4f }, { ID::aS, 0.0f }, { ID::aR, 0.3f },
        { ID::dlyOn, 1.0f }, { ID::dlyNote, 2.0f }, { ID::dlyFb, 0.75f }, { ID::dlyTone, 2500.0f }, { ID::dlyMix, 0.5f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.7f }, { ID::rvbMix, 0.3f } });

    // ---------------- LEGENDS (v0.6 phase 1 : hommages aux classiques) ----------------
    // Minimoog Model D : 2 "oscs" saw legerement desaccordes, ladder 24 dB pousse par le drive
    add ("LG Mini Bass", { { ID::o1Saw, 1.0f }, { ID::o1Octave, -1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 1.0f },
        { ID::o2Octave, -1.0f }, { ID::o2Detune, 6.0f }, { ID::fCutoff, 750.0f }, { ID::fReso, 0.15f },
        { ID::fDrive, 2.5f }, { ID::fEnvAmt, 0.5f }, { ID::fD, 0.3f }, { ID::fS, 0.15f }, { ID::fKeyTrack, 0.4f },
        { ID::aA, 0.003f }, { ID::aD, 0.4f }, { ID::aS, 0.7f }, { ID::aR, 0.12f }, { ID::chorus, 0.0f },
        { ID::voiceMode, 2.0f }, { ID::notePrio, 1.0f } });   // legato low-note comme le vrai
    // Moog Taurus : pedale basse enorme, saw + sub, drive genereux
    add ("LG Taurus", { { ID::o1Saw, 0.9f }, { ID::o1Sub, 1.0f }, { ID::o1Octave, -1.0f }, { ID::o2On, 1.0f },
        { ID::o2Saw, 0.8f }, { ID::o2Octave, -1.0f }, { ID::o2Detune, 4.0f }, { ID::fCutoff, 500.0f },
        { ID::fDrive, 4.0f }, { ID::fEnvAmt, 0.35f }, { ID::fD, 0.6f }, { ID::fS, 0.3f }, { ID::glide, 0.02f },
        { ID::aA, 0.01f }, { ID::aS, 1.0f }, { ID::aR, 0.5f }, { ID::chorus, 0.0f },
        { ID::voiceMode, 1.0f }, { ID::notePrio, 1.0f } });   // mono low-note, retrigger pedale
    // ELP "Lucky Man" : lead Moog gras, gros portamento + vibrato retarde
    add ("LG Lucky Man", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.9f }, { ID::o2Detune, 8.0f },
        { ID::glide, 0.08f }, { ID::fCutoff, 2800.0f }, { ID::fReso, 0.15f }, { ID::fDrive, 2.0f },
        { ID::fEnvAmt, 0.25f }, { ID::fS, 0.5f }, { ID::lfoPitch, 0.12f }, { ID::lfoRate, 5.5f }, { ID::lfoDelay, 0.5f },
        { ID::aA, 0.01f }, { ID::aS, 0.9f }, { ID::aR, 0.35f }, { ID::chorus, 0.0f },
        { ID::dlyOn, 1.0f }, { ID::dlyNote, 2.0f }, { ID::dlyFb, 0.3f }, { ID::dlyMix, 0.15f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.6f }, { ID::rvbMix, 0.25f },
        { ID::voiceMode, 2.0f } });   // legato : le portamento chante vraiment
    // Van Halen "Jump" : OB-Xa poly saw brillant, filtre 12 dB
    add ("LG Jump", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 1.0f }, { ID::o2Detune, 10.0f },
        { ID::fType, 1.0f }, { ID::fCutoff, 5000.0f }, { ID::fReso, 0.05f }, { ID::fEnvAmt, 0.2f },
        { ID::fD, 0.5f }, { ID::fS, 0.3f }, { ID::aA, 0.004f }, { ID::aD, 0.5f }, { ID::aS, 0.75f }, { ID::aR, 0.2f },
        { ID::chorus, 0.0f }, { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.45f }, { ID::rvbMix, 0.18f } });
    // Oberheim OB-Xa : brass large, 12 dB, chorus discret
    add ("LG OB-Xa Brass", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 1.0f }, { ID::o2Detune, 14.0f },
        { ID::fType, 1.0f }, { ID::fCutoff, 2200.0f }, { ID::fReso, 0.15f }, { ID::fDrive, 1.8f },
        { ID::fEnvAmt, 0.5f }, { ID::fA, 0.02f }, { ID::fD, 0.5f }, { ID::fS, 0.45f },
        { ID::aA, 0.03f }, { ID::aS, 0.8f }, { ID::aR, 0.3f }, { ID::chorus, 1.0f } });
    // Jupiter-8 : strings PWM soyeuses (LFO lent = mouvement de pulse width)
    add ("LG JP8 PWM Strings", { { ID::o1Pulse, 0.9f }, { ID::o1Pw, 0.5f }, { ID::o1Pwm, 0.3f }, { ID::o2On, 1.0f },
        { ID::o2Pulse, 0.8f }, { ID::o2Pw, 0.45f }, { ID::o2Detune, 9.0f }, { ID::lfoRate, 0.8f },
        { ID::hpf, 1.0f }, { ID::fCutoff, 3200.0f }, { ID::fEnvAmt, 0.15f }, { ID::fA, 0.6f },
        { ID::aA, 0.5f }, { ID::aD, 1.0f }, { ID::aS, 0.85f }, { ID::aR, 1.3f }, { ID::chorus, 2.0f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.55f }, { ID::rvbMix, 0.22f } });
    // Jupiter-8 : pad chaud a sweep lent
    add ("LG JP8 Pad", { { ID::o1Saw, 0.8f }, { ID::o1Pulse, 0.3f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.7f },
        { ID::o2Detune, 12.0f }, { ID::fCutoff, 1400.0f }, { ID::fEnvAmt, 0.35f }, { ID::fA, 1.5f },
        { ID::fD, 2.0f }, { ID::fS, 0.6f }, { ID::aA, 1.2f }, { ID::aS, 0.85f }, { ID::aR, 2.2f }, { ID::chorus, 3.0f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.7f }, { ID::rvbPre, 30.0f }, { ID::rvbMix, 0.3f } });
    // Prophet-5 : brass poly sec et punchy
    add ("LG P5 Brass", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 1.0f }, { ID::o2Detune, 9.0f },
        { ID::fCutoff, 1900.0f }, { ID::fReso, 0.2f }, { ID::fDrive, 1.5f }, { ID::fEnvAmt, 0.55f },
        { ID::fD, 0.45f }, { ID::fS, 0.35f }, { ID::aA, 0.04f }, { ID::aS, 0.8f }, { ID::aR, 0.25f },
        { ID::chorus, 0.0f }, { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.4f }, { ID::rvbMix, 0.15f } });
    // CS-80 façon Vangelis : saw + montee de filtre lente, vibrato tardif, grosse reverb
    add ("LG Blade Runner", { { ID::o1Saw, 1.0f }, { ID::o2On, 1.0f }, { ID::o2Saw, 0.6f }, { ID::o2Detune, 5.0f },
        { ID::glide, 0.04f }, { ID::fCutoff, 900.0f }, { ID::fDrive, 2.0f }, { ID::fEnvAmt, 0.6f },
        { ID::fA, 1.8f }, { ID::fD, 3.0f }, { ID::fS, 0.55f }, { ID::fR, 2.0f },
        { ID::lfoPitch, 0.07f }, { ID::lfoRate, 5.0f }, { ID::lfoDelay, 1.2f },
        { ID::aA, 0.3f }, { ID::aS, 0.9f }, { ID::aR, 2.0f }, { ID::chorus, 1.0f },
        { ID::dlyOn, 1.0f }, { ID::dlyNote, 1.0f }, { ID::dlyFb, 0.35f }, { ID::dlyMix, 0.15f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.88f }, { ID::rvbDamp, 0.3f }, { ID::rvbPre, 60.0f }, { ID::rvbMix, 0.42f } });
    // Roland SH-101 : basse mono nerveuse saw + sub
    add ("LG SH-101 Bass", { { ID::o1Saw, 1.0f }, { ID::o1Sub, 0.8f }, { ID::o1Octave, -1.0f },
        { ID::fCutoff, 700.0f }, { ID::fReso, 0.35f }, { ID::fEnvAmt, 0.5f }, { ID::fD, 0.25f }, { ID::fS, 0.0f },
        { ID::glide, 0.03f }, { ID::aA, 0.002f }, { ID::aD, 0.35f }, { ID::aS, 0.25f }, { ID::aR, 0.08f },
        { ID::chorus, 0.0f }, { ID::voiceMode, 1.0f } });   // mono retrigger, derniere note
    // ARP Odyssey : sample & hold sur le filtre, bleeps aleatoires
    add ("LG ARP S&H", { { ID::o1Pulse, 1.0f }, { ID::o1Pw, 0.4f }, { ID::lfoShape, 4.0f }, { ID::lfoRate, 6.0f },
        { ID::fLfo, 0.85f }, { ID::fCutoff, 1200.0f }, { ID::fReso, 0.65f },
        { ID::aS, 0.8f }, { ID::aR, 0.3f }, { ID::chorus, 0.0f },
        { ID::dlyOn, 1.0f }, { ID::dlyNote, 5.0f }, { ID::dlyFb, 0.5f }, { ID::dlyMix, 0.35f } });

    // ---- LEGENDS 2e vague (v0.6 phase 2 : triangle, mono/legato, hard sync) ----
    // Prophet/Cars : hard sync lead — DCO1 muet sert de master, on ecoute DCO2 synchronise
    add ("LG Sync Lead", { { ID::o1Saw, 0.0f }, { ID::o2On, 1.0f }, { ID::o2Sync, 1.0f }, { ID::o2Saw, 1.0f },
        { ID::o2Octave, 1.0f }, { ID::o2Detune, 35.0f }, { ID::fCutoff, 3500.0f }, { ID::fReso, 0.2f },
        { ID::fDrive, 2.0f }, { ID::fEnvAmt, 0.3f }, { ID::fD, 0.5f }, { ID::fS, 0.4f },
        { ID::voiceMode, 2.0f }, { ID::lfoPitch, 0.08f }, { ID::lfoRate, 5.5f }, { ID::lfoDelay, 0.6f },
        { ID::aA, 0.005f }, { ID::aS, 0.85f }, { ID::aR, 0.2f }, { ID::chorus, 0.0f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.45f }, { ID::rvbMix, 0.18f } });
    // Moog : lead flute triangle, legato + glide
    add ("LG Moog Flute", { { ID::o1Saw, 0.0f }, { ID::o1Tri, 1.0f }, { ID::fCutoff, 2200.0f }, { ID::fReso, 0.1f },
        { ID::fDrive, 1.5f }, { ID::glide, 0.05f }, { ID::voiceMode, 2.0f }, { ID::notePrio, 1.0f },
        { ID::lfoPitch, 0.06f }, { ID::lfoRate, 5.0f }, { ID::lfoDelay, 0.8f },
        { ID::aA, 0.04f }, { ID::aS, 0.9f }, { ID::aR, 0.3f }, { ID::chorus, 0.0f },
        { ID::rvbOn, 1.0f }, { ID::rvbSize, 0.5f }, { ID::rvbMix, 0.2f } });
    // Moog : basse funk triangle, mono priorite basse
    add ("LG Tri Funk Bass", { { ID::o1Saw, 0.25f }, { ID::o1Tri, 0.9f }, { ID::o1Octave, -1.0f },
        { ID::fCutoff, 900.0f }, { ID::fReso, 0.3f }, { ID::fDrive, 2.5f }, { ID::fEnvAmt, 0.55f },
        { ID::fD, 0.2f }, { ID::fS, 0.05f }, { ID::voiceMode, 1.0f }, { ID::notePrio, 1.0f },
        { ID::aA, 0.002f }, { ID::aD, 0.35f }, { ID::aS, 0.4f }, { ID::aR, 0.1f }, { ID::chorus, 0.0f } });
}

void PresetManager::applyValues (const std::vector<std::pair<juce::String, float>>& vals)
{
    for (auto& kv : vals)
        if (auto* p = apvts.getParameter (kv.first))
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
}

void PresetManager::setCurrent (const juce::String& n)
{
    currentName = n;
    if (onChange) onChange();
}

void PresetManager::initPatch()
{
    auto& params = apvts.processor.getParameters();
    for (auto* base : params)
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (base))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    setCurrent ("Init");
}

void PresetManager::loadFactory (const juce::String& name)
{
    for (auto& f : factory)
        if (f.name == name)
        {
            initPatch();
            applyValues (f.values);
            setCurrent (name);
            return;
        }
}

void PresetManager::loadUser (const juce::String& name)
{
    auto file = userDir.getChildFile (name + presetExt);
    if (! file.existsAsFile())
        return;

    if (auto xml = juce::XmlDocument::parse (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            setCurrent (name);
        }
}

bool PresetManager::saveUserPreset (const juce::String& name)
{
    if (name.isEmpty())
        return false;

    auto state = apvts.copyState();
    if (auto xml = state.createXml())
    {
        auto file = userDir.getChildFile (name + presetExt);
        const bool ok = xml->writeTo (file);
        if (ok) setCurrent (name);
        return ok;
    }
    return false;
}

bool PresetManager::deleteUserPreset (const juce::String& name)
{
    return userDir.getChildFile (name + presetExt).deleteFile();
}

bool PresetManager::renameUserPreset (const juce::String& oldName, const juce::String& newName)
{
    if (newName.isEmpty() || oldName == newName)
        return false;

    auto src = userDir.getChildFile (oldName + presetExt);
    auto dst = userDir.getChildFile (newName + presetExt);
    if (! src.existsAsFile() || dst.existsAsFile())   // never clobber an existing preset
        return false;

    const bool ok = src.moveFileTo (dst);
    if (ok) setCurrent (newName);
    return ok;
}

bool PresetManager::isUserPreset (const juce::String& name) const
{
    return getUserNames().contains (name);
}

juce::StringArray PresetManager::getFactoryNames() const
{
    juce::StringArray a;
    for (auto& f : factory)
        a.add (f.name);
    return a;
}

juce::StringArray PresetManager::getUserNames() const
{
    juce::StringArray a;
    for (auto& f : userDir.findChildFiles (juce::File::findFiles, false, juce::String ("*") + presetExt))
        a.add (f.getFileNameWithoutExtension());
    a.sort (true);
    return a;
}

juce::StringArray PresetManager::getAllDisplayNames() const
{
    juce::StringArray a;
    for (auto& n : getFactoryNames()) a.add (factoryPrefix + n);
    for (auto& n : getUserNames())    a.add (n);
    return a;
}

void PresetManager::loadByDisplayName (const juce::String& displayName)
{
    if (displayName.startsWith (factoryPrefix))
        loadFactory (displayName.substring ((int) juce::String (factoryPrefix).length()));
    else
        loadUser (displayName);
}

void PresetManager::loadNext()
{
    auto all = getAllDisplayNames();
    if (all.isEmpty()) return;
    int idx = all.indexOf (currentName.startsWith (factoryPrefix) ? currentName
                                                                  : (factory.size() ? factoryPrefix + currentName : currentName));
    // resolve by matching either factory-prefixed or plain
    idx = -1;
    for (int i = 0; i < all.size(); ++i)
        if (all[i] == currentName || all[i] == factoryPrefix + currentName) { idx = i; break; }
    idx = (idx + 1) % all.size();
    loadByDisplayName (all[idx]);
}

void PresetManager::loadPrevious()
{
    auto all = getAllDisplayNames();
    if (all.isEmpty()) return;
    int idx = -1;
    for (int i = 0; i < all.size(); ++i)
        if (all[i] == currentName || all[i] == factoryPrefix + currentName) { idx = i; break; }
    idx = (idx - 1 + all.size()) % all.size();
    loadByDisplayName (all[idx]);
}

bool PresetManager::exportBank (const juce::File& dest) const
{
    juce::XmlElement bank ("UB_Poly16_Bank");
    for (auto& n : getUserNames())
    {
        auto file = userDir.getChildFile (n + presetExt);
        if (auto xml = juce::XmlDocument::parse (file))
        {
            auto* preset = bank.createNewChildElement ("PRESET");
            preset->setAttribute ("name", n);
            preset->addChildElement (new juce::XmlElement (*xml));
        }
    }
    return bank.writeTo (dest);
}

int PresetManager::importBank (const juce::File& src)
{
    auto bank = juce::XmlDocument::parse (src);
    if (bank == nullptr || ! bank->hasTagName ("UB_Poly16_Bank"))
        return 0;

    int count = 0;
    for (auto* preset : bank->getChildWithTagNameIterator ("PRESET"))
    {
        const auto name = preset->getStringAttribute ("name", "Imported");
        if (auto* state = preset->getFirstChildElement())
        {
            auto file = userDir.getChildFile (name + presetExt);
            if (state->writeTo (file))
                ++count;
        }
    }
    if (onChange) onChange();
    return count;
}
