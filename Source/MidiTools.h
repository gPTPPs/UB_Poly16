#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <set>

// Chord memory: learn a chord (intervals relative to its lowest note), then
// every played key triggers the whole chord transposed. Runs in the MIDI
// domain BEFORE the arpeggiator, so chord + arp = arpeggiated chords.
// The learned chord is saved with the host/standalone session state
// (not with presets — it's a performance setting).
class ChordMemory
{
public:
    void startLearn()
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        learnSet.clear();
        heldCount = 0;
        learning = true;
    }

    bool isLearning() const { return learning; }

    void process (juce::MidiBuffer& midi, bool on)
    {
        if (learning)
        {
            const juce::SpinLock::ScopedLockType sl (lock);
            for (const auto meta : midi)
            {
                const auto m = meta.getMessage();
                if (m.isNoteOn())
                {
                    learnSet.insert (m.getNoteNumber());
                    ++heldCount;
                }
                else if (m.isNoteOff() && heldCount > 0 && --heldCount == 0 && ! learnSet.empty())
                {
                    const int root = *learnSet.begin();   // std::set -> lowest note
                    count = 0;
                    for (int n : learnSet)
                        if (count < maxIntervals)
                            intervals[count++] = n - root;
                    learning = false;
                }
            }
            return;   // pass the chord through unmodified while learning
        }

        const juce::SpinLock::ScopedLockType sl (lock);

        if (! on || count <= 1)
        {
            // if we were expanding notes, release everything still sounding
            if (active)
            {
                for (int n = 0; n < 128; ++n)
                    if (refs[n] > 0)
                    {
                        midi.addEvent (juce::MidiMessage::noteOff (lastChannel, n), 0);
                        refs[n] = 0;
                    }
                active = false;
            }
            return;
        }
        active = true;

        juce::MidiBuffer out;
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn() || m.isNoteOff())
            {
                lastChannel = m.getChannel();
                for (int k = 0; k < count; ++k)
                {
                    const int n = m.getNoteNumber() + intervals[k];
                    if (n < 0 || n > 127)
                        continue;
                    if (m.isNoteOn())
                    {
                        if (refs[n]++ == 0)   // refcount: shared notes trigger once
                            out.addEvent (juce::MidiMessage::noteOn (m.getChannel(), n, m.getVelocity()),
                                          meta.samplePosition);
                    }
                    else if (refs[n] > 0 && --refs[n] == 0)
                        out.addEvent (juce::MidiMessage::noteOff (m.getChannel(), n), meta.samplePosition);
                }
            }
            else
                out.addEvent (m, meta.samplePosition);
        }
        midi.swapWith (out);
    }

    juce::String toString() const
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        juce::StringArray a;
        for (int k = 0; k < count; ++k)
            a.add (juce::String (intervals[k]));
        return a.joinIntoString (",");
    }

    void fromString (const juce::String& s)
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        count = 0;
        for (auto& tok : juce::StringArray::fromTokens (s, ",", {}))
            if (tok.trim().isNotEmpty() && count < maxIntervals)
                intervals[count++] = tok.getIntValue();
    }

private:
    static constexpr int maxIntervals = 16;
    mutable juce::SpinLock lock;
    int  intervals[maxIntervals] { 0 };
    int  count = 0;
    int  refs[128] {};
    int  lastChannel = 1;
    bool active = false;
    std::set<int> learnSet;
    int  heldCount = 0;
    std::atomic<bool> learning { false };
};

// Mono / legato note handler. Runs in the MIDI domain AFTER the arpeggiator,
// just before the synth: keeps a stack of held keys and lets only ONE note
// sound, chosen by priority (last / low / high).
//  - Mono   = multi-trigger: every note change sends noteOff + noteOn (envelopes restart)
//  - Legato = single-trigger: while keys overlap, the running voice is just
//    retuned via onLegatoChange (envelopes keep going, glide applies)
// In legato the synth only ever saw ONE noteOn ("synthNote"), so the final
// noteOff must target that note — the handler tracks it separately.
class MonoMode
{
public:
    // (synthNote, newNote): the processor retunes the voice playing synthNote
    std::function<void (int, int)> onLegatoChange;

    void process (juce::MidiBuffer& midi, int mode /*0 Poly, 1 Mono, 2 Legato*/, int priority)
    {
        prio = priority;

        if (mode != lastMode)
        {
            // mode switched mid-performance: release cleanly and start over
            if (synthNote >= 0)
                midi.addEvent (juce::MidiMessage::noteOff (channel, synthNote), 0);
            heldCount = 0;
            sounding = synthNote = -1;
            lastMode = mode;
        }
        if (mode == 0)
            return;

        juce::MidiBuffer out;
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())       handleOn  (out, m, meta.samplePosition, mode);
            else if (m.isNoteOff()) handleOff (out, m, meta.samplePosition, mode);
            else                    out.addEvent (m, meta.samplePosition);
        }
        midi.swapWith (out);
    }

private:
    struct Held { int note; juce::uint8 vel; int seq; };
    static constexpr int maxHeld = 32;
    Held held[maxHeld];
    int  heldCount = 0, seqCounter = 0;
    int  prio = 0, lastMode = 0;
    int  sounding = -1;    // pitch currently heard
    int  synthNote = -1;   // note the synth was told (differs from sounding in legato)
    int  channel = 1;

    int pickIndex() const
    {
        int best = 0;
        for (int i = 1; i < heldCount; ++i)
            if (prio == 0 ? held[i].seq  > held[best].seq
              : prio == 1 ? held[i].note < held[best].note
                          : held[i].note > held[best].note)
                best = i;
        return best;
    }

    void handleOn (juce::MidiBuffer& out, const juce::MidiMessage& m, int pos, int mode)
    {
        channel = m.getChannel();
        if (heldCount < maxHeld)
            held[heldCount++] = { m.getNoteNumber(), m.getVelocity(), ++seqCounter };

        const int bi   = pickIndex();
        const int want = held[bi].note;
        if (want == sounding)
            return;   // lower-priority key: remembered but silent

        if (sounding < 0)
        {
            out.addEvent (juce::MidiMessage::noteOn (channel, want, held[bi].vel), pos);
            sounding = synthNote = want;
        }
        else if (mode == 1)   // mono: retrigger
        {
            out.addEvent (juce::MidiMessage::noteOff (channel, synthNote), pos);
            out.addEvent (juce::MidiMessage::noteOn (channel, want, held[bi].vel), pos);
            sounding = synthNote = want;
        }
        else                  // legato: retune the running voice
        {
            if (onLegatoChange) onLegatoChange (synthNote, want);
            sounding = want;
        }
    }

    void handleOff (juce::MidiBuffer& out, const juce::MidiMessage& m, int pos, int mode)
    {
        const int n = m.getNoteNumber();
        for (int i = 0; i < heldCount; ++i)
            if (held[i].note == n)
            {
                for (int j = i; j < heldCount - 1; ++j)
                    held[j] = held[j + 1];
                --heldCount;
                break;
            }

        if (n != sounding)
            return;   // that key never reached the synth: swallow its noteOff

        if (heldCount == 0)
        {
            out.addEvent (juce::MidiMessage::noteOff (channel, synthNote), pos);
            sounding = synthNote = -1;
            return;
        }

        // another key is still held: fall back to it (note memory)
        const int bi   = pickIndex();
        const int want = held[bi].note;
        if (mode == 1)
        {
            out.addEvent (juce::MidiMessage::noteOff (channel, synthNote), pos);
            out.addEvent (juce::MidiMessage::noteOn (channel, want, held[bi].vel), pos);
            sounding = synthNote = want;
        }
        else
        {
            if (onLegatoChange) onLegatoChange (synthNote, want);
            sounding = want;
        }
    }
};

// MIDI CC -> parameter mapping with learn ("right-click a knob, move a CC").
// One CC per parameter; mappings persist with the session state.
class MidiCcMap
{
public:
    explicit MidiCcMap (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

    void armLearn (const juce::String& paramID)
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        armed = paramID;
    }

    void clear (const juce::String& paramID)
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        eraseParam (paramID);
    }

    int getCcFor (const juce::String& paramID) const
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        for (auto& kv : map)
            if (kv.second == paramID)
                return kv.first;
        return -1;
    }

    // paramID waiting for a CC, empty when idle — drives the editor's
    // "MIDI Learn..." indicator
    juce::String getArmed() const
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        return armed;
    }

    void process (const juce::MidiBuffer& midi)
    {
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (! m.isController())
                continue;

            const int cc = m.getControllerNumber();
            juce::String target;
            {
                const juce::SpinLock::ScopedLockType sl (lock);
                if (armed.isNotEmpty())
                {
                    eraseParam (armed);   // re-learn moves the param to the new CC
                    map[cc] = armed;
                    armed.clear();
                }
                if (auto found = map.find (cc); found != map.end())
                    target = found->second;
            }
            if (target.isNotEmpty())
                if (auto* p = apvts.getParameter (target))
                    p->setValueNotifyingHost ((float) m.getControllerValue() / 127.0f);
        }
    }

    juce::String toString() const
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        juce::StringArray a;
        for (auto& kv : map)
            a.add (juce::String (kv.first) + "=" + kv.second);
        return a.joinIntoString (";");
    }

    void fromString (const juce::String& s)
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        map.clear();
        for (auto& tok : juce::StringArray::fromTokens (s, ";", {}))
        {
            const int eq = tok.indexOfChar ('=');
            if (eq > 0)
                map[tok.substring (0, eq).getIntValue()] = tok.substring (eq + 1);
        }
    }

private:
    void eraseParam (const juce::String& paramID)
    {
        for (auto it = map.begin(); it != map.end();)
            it = (it->second == paramID) ? map.erase (it) : std::next (it);
    }

    juce::AudioProcessorValueTreeState& apvts;
    mutable juce::SpinLock lock;
    std::map<int, juce::String> map;
    juce::String armed;
};
