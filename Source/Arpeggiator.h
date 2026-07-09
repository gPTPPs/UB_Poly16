#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <limits>

// MIDI-domain arpeggiator. When enabled it captures held notes and emits a
// stepped note pattern; when disabled it passes MIDI through untouched.
//
// On top of the classic note-order modes it runs an optional 16-step "groove
// lane": per step on/off, accent (velocity multiplier), tie (tracker-style:
// holds the previous note, plays nothing, does not advance the note pointer)
// and ratchet (retrigger x2..x4 inside the step). A global swing delays the
// odd steps. When the host is playing and Arp Sync is on, step positions are
// phase-locked to the transport PPQ so the pattern is fully reproducible.
class Arpeggiator
{
public:
    struct GrooveStep
    {
        bool  on      = true;   // false = rest (silences, still advances note pointer)
        float accent  = 1.0f;   // velocity multiplier
        bool  tie     = false;  // hold previous note through this step
        int   ratchet = 1;      // 1 = single hit, 2..4 = retriggers within the step
    };

    struct Params
    {
        bool   on = false;
        int    mode = 0;        // 0 Up,1 Down,2 Up/Down,3 Random,4 As Played,
                                // 5 Down/Up,6 Up&Down,7 Converge,8 Diverge
        int    octaves = 1;
        float  gate = 0.5f;
        bool   hold = false;
        double stepSeconds = 0.25;

        // groove lane
        bool   grooveOn = false;
        int    grooveLen = 16;
        float  swing = 0.0f;    // 0..0.75 fraction of a step, delays odd steps
        std::array<GrooveStep, 16> steps {};

        // phase-lock (host transport)
        bool   phaseLock = false; // sync && host present && playing
        double ppqStart  = 0.0;   // ppq at the first sample of the block
        double bpm       = 120.0;
        double stepBeats = 0.25;  // step length in quarter notes
    };

    void prepare (double sampleRate);
    void reset();

    // Transforms 'midi' in place for a block of 'numSamples'.
    void process (juce::MidiBuffer& midi, int numSamples, const Params& prm);

private:
    void rebuildPattern (const Params& prm);
    void rebuildGroove  (const Params& prm);
    juce::uint8 velocityFor (int note) const;

    // groove helpers (operate on the compiled gSteps / gL)
    long long noteAdvancesBefore (long long globalStep) const;
    int       sustainStepsAt (int patternStep) const;
    double    stepStartPpq (long long g, const Params& prm) const;
    long long currentStepForPpq (double ppq, const Params& prm) const;

    // emits the note event(s) for global step 'g' at sample offset 'i'
    void fireStep (juce::MidiBuffer& out, int i, long long g,
                   const Params& prm, int stepLenSamples);

    double sampleRate = 44100.0;

    std::vector<int>   heldOrder;   // notes in order pressed (for As Played)
    std::map<int, int> velMap;      // note -> velocity
    std::set<int>      physicallyDown;
    std::vector<int>   pattern;     // compiled note order

    // compiled groove
    int                        gL = 1;
    std::array<GrooveStep, 16> gSteps {};
    int                        advancesPerCycle = 1;
    std::array<int, 17>        partial {};  // partial[k] = non-tie steps in [0,k)

    // stepping state
    long long lastFiredStep = std::numeric_limits<long long>::min();
    long long freeStep = 0;             // free-run step counter (no transport)
    int  samplesUntilNextStep = 0;      // free-run timing
    int  activeNote = -1;
    int  samplesUntilGateOff = -1;
    bool wasOn = false;

    // ratchet scheduler
    int         ratchetHitsLeft = 0;
    int         ratchetInterval = 0;
    int         samplesUntilRatchet = 0;
    int         ratchetGateSamples = 0;
    int         ratchetNote = -1;
    juce::uint8 ratchetVel = 100;

    juce::Random rng;
};
