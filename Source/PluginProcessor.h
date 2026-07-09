#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "SynthVoice.h"
#include "Arpeggiator.h"
#include "GrooveModel.h"
#include "PresetManager.h"
#include "Effects.h"
#include "MidiTools.h"
#include "Scope.h"

class UBAudioProcessor : public juce::AudioProcessor
{
public:
    UBAudioProcessor();
    ~UBAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "UB_Poly16"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override
    {
        const bool d = apvts.getRawParameterValue (ID::dlyOn)->load() > 0.5f;
        const bool r = apvts.getRawParameterValue (ID::rvbOn)->load() > 0.5f;
        return (d || r) ? 8.0 : 0.0;
    }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    GrooveModel groove { apvts };
    PresetManager presets { apvts };
    ChordMemory chord;
    MidiCcMap ccMap { apvts };
    ScopeBuffer scope;

private:
    double stepSecondsFromRate (int rateIndex, double bpm) const;
    double delaySecondsFromNote (int noteIndex, double bpm) const;

    juce::Synthesiser synth;
    Arpeggiator arp;
    MonoMode mono;
    std::atomic<float> lastNoteHz { 0.0f };   // shared glide source for all voices
    juce::dsp::Chorus<float> chorus;
    UBDelay delayFx;
    UBReverbFx reverbFx;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UBAudioProcessor)
};
