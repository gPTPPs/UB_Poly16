#include "Arpeggiator.h"
#include <cmath>

void Arpeggiator::prepare (double sr)
{
    sampleRate = sr;
    reset();
}

void Arpeggiator::reset()
{
    heldOrder.clear();
    velMap.clear();
    physicallyDown.clear();
    pattern.clear();
    gL = 1;
    advancesPerCycle = 1;
    partial.fill (0);
    lastFiredStep = std::numeric_limits<long long>::min();
    freeStep = 0;
    samplesUntilNextStep = 0;
    activeNote = -1;
    samplesUntilGateOff = -1;
    ratchetHitsLeft = 0;
    samplesUntilRatchet = 0;
}

juce::uint8 Arpeggiator::velocityFor (int note) const
{
    auto it = velMap.find (note);
    return (juce::uint8) (it != velMap.end() ? it->second : 100);
}

void Arpeggiator::rebuildPattern (const Params& prm)
{
    pattern.clear();

    std::vector<int> base = heldOrder;
    if (base.empty())
        return;

    if (prm.mode != 4) // everything but "As Played" works on sorted notes
        std::sort (base.begin(), base.end());

    std::vector<int> oct;
    for (int o = 0; o < juce::jmax (1, prm.octaves); ++o)
        for (int n : base)
            oct.push_back (n + 12 * o);

    const int n = (int) oct.size();

    switch (prm.mode)
    {
        case 1: // Down
            std::reverse (oct.begin(), oct.end());
            pattern = oct;
            break;

        case 2: // Up/Down (extremes not repeated): up then back down excl. ends
            pattern = oct;
            for (int k = n - 2; k >= 1; --k)
                pattern.push_back (oct[(size_t) k]);
            break;

        case 5: // Down/Up (extremes not repeated): down then back up excl. ends
            for (int k = n - 1; k >= 0; --k)
                pattern.push_back (oct[(size_t) k]);
            for (int k = 1; k <= n - 2; ++k)
                pattern.push_back (oct[(size_t) k]);
            break;

        case 6: // Up&Down (extremes repeated)
            pattern = oct;
            for (int k = n - 1; k >= 0; --k)
                pattern.push_back (oct[(size_t) k]);
            break;

        case 7: // Converge: lo, hi, lo+1, hi-1, ...
        {
            int a = 0, b = n - 1;
            while (a <= b)
            {
                pattern.push_back (oct[(size_t) a]);
                if (a != b) pattern.push_back (oct[(size_t) b]);
                ++a; --b;
            }
            break;
        }

        case 8: // Diverge: centre outwards (reverse of converge)
        {
            std::vector<int> c;
            int a = 0, b = n - 1;
            while (a <= b)
            {
                c.push_back (oct[(size_t) a]);
                if (a != b) c.push_back (oct[(size_t) b]);
                ++a; --b;
            }
            pattern.assign (c.rbegin(), c.rend());
            break;
        }

        default: // 0 Up, 3 Random, 4 As Played all iterate this list
            pattern = oct;
            break;
    }
}

void Arpeggiator::rebuildGroove (const Params& prm)
{
    if (prm.grooveOn)
    {
        gL = juce::jlimit (1, 16, prm.grooveLen);
        for (int i = 0; i < gL; ++i)
            gSteps[(size_t) i] = prm.steps[(size_t) i];
    }
    else
    {
        gL = 1;
        gSteps[0] = GrooveStep {};   // single on-step: classic arp behaviour
    }

    int acc = 0;
    partial[0] = 0;
    for (int i = 0; i < gL; ++i)
    {
        if (! gSteps[(size_t) i].tie) ++acc;
        partial[(size_t) (i + 1)] = acc;
    }
    advancesPerCycle = juce::jmax (1, acc);
}

long long Arpeggiator::noteAdvancesBefore (long long g) const
{
    const long long L = gL;
    // floored division so it stays correct for negative ppq (count-in)
    long long cyc = g / L;
    long long rem = g - cyc * L;
    if (rem < 0) { rem += L; --cyc; }
    return cyc * advancesPerCycle + partial[(size_t) rem];
}

int Arpeggiator::sustainStepsAt (int p) const
{
    int s = 1;
    for (int k = 1; k < gL; ++k)
    {
        if (gSteps[(size_t) ((p + k) % gL)].tie) ++s;
        else break;
    }
    return s;
}

double Arpeggiator::stepStartPpq (long long g, const Params& prm) const
{
    const bool odd = (((g % 2) + 2) % 2) == 1;
    return (double) g * prm.stepBeats + (odd ? prm.swing * prm.stepBeats : 0.0);
}

long long Arpeggiator::currentStepForPpq (double ppq, const Params& prm) const
{
    const long long g0 = (long long) std::floor (ppq / juce::jmax (1.0e-9, prm.stepBeats));
    for (long long g = g0 + 1; g >= g0 - 1; --g)
        if (stepStartPpq (g, prm) <= ppq + 1.0e-9)
            return g;
    return g0 - 1;
}

void Arpeggiator::fireStep (juce::MidiBuffer& out, int i, long long g,
                            const Params& prm, int stepLenSamples)
{
    const int L = gL;
    const int p = (int) (((g % L) + L) % L);
    const GrooveStep& gs = gSteps[(size_t) p];

    if (gs.tie)
        return; // hold previous note: no retrigger, no advance, gate untouched

    if (! gs.on)
    {   // rest: silence any sounding note
        if (activeNote >= 0)
        {
            out.addEvent (juce::MidiMessage::noteOff (1, activeNote), i);
            activeNote = -1;
        }
        ratchetHitsLeft = 0;
        return;
    }

    // ON step: retrigger
    if (activeNote >= 0)
    {
        out.addEvent (juce::MidiMessage::noteOff (1, activeNote), i);
        activeNote = -1;
    }

    const int sz = (int) pattern.size();
    int idx;
    if (prm.mode == 3) // Random
        idx = rng.nextInt (sz);
    else
        idx = (int) (((noteAdvancesBefore (g) % sz) + sz) % sz);

    const int n    = juce::jlimit (0, 127, pattern[(size_t) idx]);
    const int base = velocityFor (n);
    const int vel  = juce::jlimit (1, 127, (int) std::lround (base * gs.accent));

    out.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) vel), i);
    activeNote = n;

    const int r = juce::jlimit (1, 4, gs.ratchet);
    if (r > 1)
    {
        const int subLen = juce::jmax (1, stepLenSamples / r);
        ratchetGateSamples  = juce::jmax (1, (int) (subLen * prm.gate));
        samplesUntilGateOff = ratchetGateSamples;
        ratchetInterval     = subLen;
        samplesUntilRatchet = subLen;
        ratchetHitsLeft     = r - 1;
        ratchetNote         = n;
        ratchetVel          = (juce::uint8) vel;
    }
    else
    {   // tie look-ahead: extend the gate over the following tied steps
        const int    sustain = sustainStepsAt (p);
        const double dur = stepLenSamples * ((sustain - 1) + (double) prm.gate);
        samplesUntilGateOff = juce::jmax (1, (int) dur);
        ratchetHitsLeft = 0;
    }
}

void Arpeggiator::process (juce::MidiBuffer& midi, int numSamples, const Params& prm)
{
    juce::MidiBuffer output;

    // ---- 1. consume incoming events / update held notes ----
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int  pos = meta.samplePosition;

        if (! prm.on)
        {
            output.addEvent (msg, pos);
            continue;
        }

        if (msg.isNoteOn())
        {
            const int nn = msg.getNoteNumber();
            if (prm.hold && physicallyDown.empty())
            {
                heldOrder.clear();
                velMap.clear();
            }
            physicallyDown.insert (nn);
            if (std::find (heldOrder.begin(), heldOrder.end(), nn) == heldOrder.end())
                heldOrder.push_back (nn);
            velMap[nn] = msg.getVelocity();
        }
        else if (msg.isNoteOff())
        {
            const int nn = msg.getNoteNumber();
            physicallyDown.erase (nn);
            if (! prm.hold)
            {
                heldOrder.erase (std::remove (heldOrder.begin(), heldOrder.end(), nn), heldOrder.end());
                velMap.erase (nn);
            }
        }
        else
        {
            output.addEvent (msg, pos); // pass through CC, pitch bend, aftertouch...
        }
    }

    if (! prm.on)
    {
        if (wasOn && activeNote >= 0)
        {
            output.addEvent (juce::MidiMessage::noteOff (1, activeNote), 0);
            activeNote = -1;
        }
        wasOn = false;
        midi.swapWith (output);
        return;
    }

    if (! wasOn) // just turned on -> restart the sequencer cleanly
    {
        lastFiredStep = std::numeric_limits<long long>::min();
        freeStep = 0;
        samplesUntilNextStep = 0;
        ratchetHitsLeft = 0;
    }
    wasOn = true;

    rebuildPattern (prm);
    rebuildGroove  (prm);

    if (pattern.empty())
    {
        if (activeNote >= 0)
        {
            output.addEvent (juce::MidiMessage::noteOff (1, activeNote), 0);
            activeNote = -1;
        }
        samplesUntilNextStep = 0;
        lastFiredStep = std::numeric_limits<long long>::min();
        ratchetHitsLeft = 0;
        midi.swapWith (output);
        return;
    }

    const int    stepLenSamples = juce::jmax (1, (int) (prm.stepSeconds * sampleRate));
    const double ppqPerSample    = (prm.bpm / 60.0) / sampleRate;
    const bool   lock            = prm.phaseLock;
    double       ppq             = prm.ppqStart;

    // ---- 2. step sequencer ----
    for (int i = 0; i < numSamples; ++i)
    {
        if (lock)
        {
            const long long g = currentStepForPpq (ppq, prm);
            if (g != lastFiredStep)
            {
                fireStep (output, i, g, prm, stepLenSamples);
                lastFiredStep = g;
            }
        }
        else if (samplesUntilNextStep <= 0)
        {
            fireStep (output, i, freeStep, prm, stepLenSamples);
            const bool even = (freeStep % 2) == 0;
            const double d = stepLenSamples * (even ? (1.0 + prm.swing) : (1.0 - prm.swing));
            samplesUntilNextStep = juce::jmax (1, (int) std::lround (d));
            ++freeStep;
        }

        // ratchet sub-hits
        if (ratchetHitsLeft > 0 && --samplesUntilRatchet <= 0)
        {
            if (activeNote >= 0)
                output.addEvent (juce::MidiMessage::noteOff (1, activeNote), i);
            output.addEvent (juce::MidiMessage::noteOn (1, ratchetNote, ratchetVel), i);
            activeNote = ratchetNote;
            samplesUntilGateOff = ratchetGateSamples;
            samplesUntilRatchet = ratchetInterval;
            --ratchetHitsLeft;
        }

        // gate off
        if (activeNote >= 0 && samplesUntilGateOff >= 0)
        {
            if (samplesUntilGateOff == 0)
            {
                output.addEvent (juce::MidiMessage::noteOff (1, activeNote), i);
                activeNote = -1;
            }
            --samplesUntilGateOff;
        }

        if (! lock)
            --samplesUntilNextStep;
        ppq += ppqPerSample;
    }

    midi.swapWith (output);
}
