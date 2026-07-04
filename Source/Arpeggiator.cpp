#include "Arpeggiator.h"

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
    stepIndex = 0;
    samplesUntilStep = 0;
    activeNote = -1;
    samplesUntilGateOff = -1;
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

    switch (prm.mode)
    {
        case 1: // Down
            std::reverse (oct.begin(), oct.end());
            pattern = oct;
            break;
        case 2: // Up/Down (no duplicated extremes)
        {
            pattern = oct;
            for (int k = (int) oct.size() - 2; k >= 1; --k)
                pattern.push_back (oct[(size_t) k]);
            break;
        }
        default: // Up, Random, As Played all iterate this list; Random picks randomly
            pattern = oct;
            break;
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
            const int n = msg.getNoteNumber();
            if (prm.hold && physicallyDown.empty())
            {
                heldOrder.clear();
                velMap.clear();
            }
            physicallyDown.insert (n);
            if (std::find (heldOrder.begin(), heldOrder.end(), n) == heldOrder.end())
                heldOrder.push_back (n);
            velMap[n] = msg.getVelocity();
        }
        else if (msg.isNoteOff())
        {
            const int n = msg.getNoteNumber();
            physicallyDown.erase (n);
            if (! prm.hold)
            {
                heldOrder.erase (std::remove (heldOrder.begin(), heldOrder.end(), n), heldOrder.end());
                velMap.erase (n);
            }
        }
        else
        {
            // pass through CC, pitch bend, aftertouch...
            output.addEvent (msg, pos);
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
    wasOn = true;

    rebuildPattern (prm);

    if (pattern.empty())
    {
        if (activeNote >= 0)
        {
            output.addEvent (juce::MidiMessage::noteOff (1, activeNote), 0);
            activeNote = -1;
        }
        samplesUntilStep = 0;
        midi.swapWith (output);
        return;
    }

    const int stepSamples = juce::jmax (1, (int) (prm.stepSeconds * sampleRate));

    // ---- 2. step sequencer ----
    for (int i = 0; i < numSamples; ++i)
    {
        if (samplesUntilStep <= 0)
        {
            if (activeNote >= 0)
            {
                output.addEvent (juce::MidiMessage::noteOff (1, activeNote), i);
                activeNote = -1;
            }

            int idx;
            if (prm.mode == 3) // Random
                idx = rng.nextInt ((int) pattern.size());
            else
                idx = stepIndex % (int) pattern.size();

            const int n = juce::jlimit (0, 127, pattern[(size_t) idx]);
            output.addEvent (juce::MidiMessage::noteOn (1, n, velocityFor (n)), i);
            activeNote = n;
            samplesUntilGateOff = juce::jmax (1, (int) (stepSamples * prm.gate));
            ++stepIndex;
            samplesUntilStep = stepSamples;
        }

        if (activeNote >= 0 && samplesUntilGateOff >= 0)
        {
            if (samplesUntilGateOff == 0)
            {
                output.addEvent (juce::MidiMessage::noteOff (1, activeNote), i);
                activeNote = -1;
            }
            --samplesUntilGateOff;
        }

        --samplesUntilStep;
    }

    midi.swapWith (output);
}
