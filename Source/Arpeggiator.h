#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <map>
#include <set>

// MIDI-domain arpeggiator. When enabled it captures held notes and emits a
// stepped note pattern; when disabled it passes MIDI through untouched.
class Arpeggiator
{
public:
    struct Params
    {
        bool   on = false;
        int    mode = 0;        // 0 Up, 1 Down, 2 Up/Down, 3 Random, 4 As Played
        int    octaves = 1;
        float  gate = 0.5f;
        bool   hold = false;
        double stepSeconds = 0.25;
    };

    void prepare (double sampleRate);
    void reset();

    // Transforms 'midi' in place for a block of 'numSamples'.
    void process (juce::MidiBuffer& midi, int numSamples, const Params& prm);

private:
    void rebuildPattern (const Params& prm);
    juce::uint8 velocityFor (int note) const;

    double sampleRate = 44100.0;

    std::vector<int>      heldOrder;   // notes in order pressed (for As Played)
    std::map<int, int>    velMap;      // note -> velocity
    std::set<int>         physicallyDown;
    std::vector<int>      pattern;

    int  stepIndex = 0;
    int  samplesUntilStep = 0;
    int  activeNote = -1;
    int  samplesUntilGateOff = -1;
    bool wasOn = false;

    juce::Random rng;
};
