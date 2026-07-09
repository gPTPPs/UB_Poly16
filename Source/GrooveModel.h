#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Arpeggiator.h"
#include <array>
#include <atomic>
#include <vector>
#include <cmath>

// Owns the 16-step arp groove pattern. The authoritative copy lives as an
// "ARP_GROOVE" child of the APVTS state (so it is saved/loaded with every
// preset and host session via copyState/replaceState). Edits happen on the
// message thread (the grid UI); the audio thread only reads a lock-free
// snapshot of packed atomics, kept in sync by a ValueTree listener.
class GrooveModel : private juce::ValueTree::Listener
{
public:
    using Step = Arpeggiator::GrooveStep;

    explicit GrooveModel (juce::AudioProcessorValueTreeState& s) : apvts (s)
    {
        ensureChild();
        apvts.state.addListener (this);
        repack();
    }

    ~GrooveModel() override { apvts.state.removeListener (this); }

    // ---- audio thread ----
    void read (std::array<Step, 16>& dest) const
    {
        for (int i = 0; i < 16; ++i)
            dest[(size_t) i] = decode (packed[(size_t) i].load (std::memory_order_relaxed));
    }

    // ---- message thread (UI) ----
    Step getStep (int i) const
    {
        return decode (packed[(size_t) juce::jlimit (0, 15, i)].load (std::memory_order_relaxed));
    }

    void setStep (int i, const Step& s)
    {
        auto g  = ensureChild();
        auto st = g.getChild (juce::jlimit (0, 15, i));
        if (! st.isValid())
            return;
        st.setProperty (pOn,  s.on ? 1 : 0,          nullptr);
        st.setProperty (pAcc, (double) s.accent,     nullptr);
        st.setProperty (pTie, s.tie ? 1 : 0,         nullptr);
        st.setProperty (pRat, juce::jlimit (1, 4, s.ratchet), nullptr);
        // the listener repacks
    }

    // re-acquire + repack after a host-level state load (belt and suspenders)
    void refreshFromState() { ensureChild(); repack(); }

    // --- factory-preset helpers: write a pattern straight into the state tree
    //     (an owning instance's listener then repacks). 'steps' shorter than 16
    //     is padded with neutral steps; empty resets the whole lane to neutral.
    static void writePattern (juce::AudioProcessorValueTreeState& apvts,
                              const std::vector<Step>& steps)
    {
        auto g = apvts.state.getChildWithName (kGroove);
        if (! g.isValid())
        {
            g = juce::ValueTree (kGroove);
            for (int i = 0; i < 16; ++i)
                g.appendChild (juce::ValueTree (kStep), nullptr);
            apvts.state.appendChild (g, nullptr);
        }
        for (int i = 0; i < 16; ++i)
        {
            Step s;   // neutral default (on, accent 1, no tie, ratchet 1)
            if (i < (int) steps.size())
                s = steps[(size_t) i];
            auto st = g.getChild (i);
            if (! st.isValid()) { g.appendChild (juce::ValueTree (kStep), nullptr); st = g.getChild (i); }
            st.setProperty (pOn,  s.on ? 1 : 0,      nullptr);
            st.setProperty (pAcc, (double) s.accent, nullptr);
            st.setProperty (pTie, s.tie ? 1 : 0,     nullptr);
            st.setProperty (pRat, juce::jlimit (1, 4, s.ratchet), nullptr);
        }
    }

    static void resetPattern (juce::AudioProcessorValueTreeState& apvts)
    {
        writePattern (apvts, {});
    }

    // bumped on every repack: lets the UI cheaply poll for external changes
    juce::uint32 getVersion() const { return version.load (std::memory_order_relaxed); }

private:
    static juce::uint32 encode (const Step& s)
    {
        juce::uint32 v = 0;
        v |= s.on  ? 1u : 0u;
        v |= s.tie ? 2u : 0u;
        v |= (juce::uint32) (juce::jlimit (1, 4, s.ratchet) - 1) << 2;
        const int q = juce::jlimit (0, 255, (int) std::lround (juce::jlimit (0.0f, 1.0f, s.accent) * 255.0f));
        v |= (juce::uint32) q << 8;
        return v;
    }

    static Step decode (juce::uint32 v)
    {
        Step s;
        s.on      = (v & 1u) != 0;
        s.tie     = (v & 2u) != 0;
        s.ratchet = (int) ((v >> 2) & 3u) + 1;
        s.accent  = (float) ((v >> 8) & 0xffu) / 255.0f;
        return s;
    }

    juce::ValueTree ensureChild()
    {
        auto g = apvts.state.getChildWithName (kGroove);
        if (! g.isValid())
        {
            g = juce::ValueTree (kGroove);
            for (int i = 0; i < 16; ++i)
            {
                juce::ValueTree st (kStep);
                st.setProperty (pOn, 1, nullptr);
                st.setProperty (pAcc, 1.0, nullptr);
                st.setProperty (pTie, 0, nullptr);
                st.setProperty (pRat, 1, nullptr);
                g.appendChild (st, nullptr);
            }
            apvts.state.appendChild (g, nullptr);
        }
        return g;
    }

    void repack()
    {
        auto g = apvts.state.getChildWithName (kGroove);
        for (int i = 0; i < 16; ++i)
        {
            Step s;
            if (g.isValid() && i < g.getNumChildren())
            {
                auto st = g.getChild (i);
                s.on      = (int)    st.getProperty (pOn,  1) != 0;
                s.accent  = (float) (double) st.getProperty (pAcc, 1.0);
                s.tie     = (int)    st.getProperty (pTie, 0) != 0;
                s.ratchet = (int)    st.getProperty (pRat, 1);
            }
            packed[(size_t) i].store (encode (s), std::memory_order_relaxed);
        }
        version.fetch_add (1, std::memory_order_relaxed);
    }

    // ---- ValueTree::Listener (message thread) ----
    void valueTreePropertyChanged (juce::ValueTree& t, const juce::Identifier&) override
    {
        if (t.hasType (kStep) || t.hasType (kGroove)) repack();
    }
    void valueTreeChildAdded (juce::ValueTree& parent, juce::ValueTree& child) override
    {
        if (child.hasType (kGroove) || parent.hasType (kGroove)) repack();
    }
    void valueTreeChildRemoved (juce::ValueTree& parent, juce::ValueTree& child, int) override
    {
        if (child.hasType (kGroove) || parent.hasType (kGroove)) repack();
    }

    juce::AudioProcessorValueTreeState& apvts;
    std::array<std::atomic<juce::uint32>, 16> packed {};
    std::atomic<juce::uint32> version { 0 };

    inline static const juce::Identifier kGroove { "ARP_GROOVE" };
    inline static const juce::Identifier kStep   { "STEP" };
    inline static const juce::Identifier pOn     { "on" };
    inline static const juce::Identifier pAcc    { "acc" };
    inline static const juce::Identifier pTie    { "tie" };
    inline static const juce::Identifier pRat    { "rat" };
};
