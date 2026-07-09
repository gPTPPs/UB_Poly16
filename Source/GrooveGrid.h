#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "UBLookAndFeel.h"
#include "Parameters.h"
#include "GrooveModel.h"

// Editable 16-step arp groove lane.
//   * velocity bars  : vertical drag sets accent (0..1), implies On
//   * On/Off row     : click toggles a step (rest = silent, still advances)
//   * Tie/Ratchet row: click cycles  ·  ->  tie  ->  x2 -> x3 -> x4  ->  ·
// Columns beyond "Arp Groove Len" are dimmed; the whole grid dims when the
// groove is off (still editable, so you can dial a pattern before enabling).
class GrooveGrid : public juce::Component,
                   private juce::Timer
{
public:
    GrooveGrid (juce::AudioProcessorValueTreeState& s, GrooveModel& m)
        : apvts (s), model (m)
    {
        startTimerHz (20);
    }

    void paint (juce::Graphics& g) override
    {
        const auto geo = geom();
        const int  len = liveLen();
        const bool on  = liveOn();

        g.setColour (UBColours::track);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);

        for (int i = 0; i < 16; ++i)
        {
            const auto vel = colOf (geo.vel, i);
            const auto onC = colOf (geo.on,  i);
            const auto mod = colOf (geo.mod, i);
            const bool active = i < len;
            const auto step = model.getStep (i);

            // downbeat shading (every 4 steps)
            if (i % 4 == 0)
            {
                g.setColour (UBColours::panelHi.withAlpha (0.35f));
                g.fillRect (juce::Rectangle<int> (vel.getX(), geo.vel.getY(),
                                                  vel.getWidth(), geo.mod.getBottom() - geo.vel.getY()));
            }

            const float dim = (active ? 1.0f : 0.30f) * (on ? 1.0f : 0.55f);

            // --- velocity bar ---
            g.setColour (juce::Colour (0xff101316));
            g.fillRect (vel);
            if (step.on)
            {
                const int h = juce::roundToInt (step.accent * (float) vel.getHeight());
                auto bar = vel.withTop (vel.getBottom() - h);
                g.setColour (UBColours::active.withAlpha (dim));
                g.fillRect (bar);
            }
            else
            {
                g.setColour (UBColours::textDim.withAlpha (0.4f * dim));
                g.drawRect (vel, 1);
            }

            // --- On/Off cell ---
            g.setColour ((step.on ? UBColours::active : UBColours::panel).withAlpha (dim));
            g.fillRect (onC.reduced (1));

            // --- Tie/Ratchet cell ---
            g.setColour (UBColours::panel.withAlpha (dim));
            g.fillRect (mod.reduced (1));
            juce::String glyph = step.tie ? "~"
                               : step.ratchet > 1 ? "x" + juce::String (step.ratchet)
                               : juce::String (juce::CharPointer_UTF8 ("·"));
            g.setColour (((step.tie || step.ratchet > 1) ? UBColours::active
                                                         : UBColours::textDim).withAlpha (dim));
            g.setFont (juce::Font (juce::FontOptions (step.ratchet > 1 ? 11.0f : 13.0f)).boldened());
            g.drawText (glyph, mod, juce::Justification::centred);
        }

        // step separators
        g.setColour (UBColours::bg.withAlpha (0.6f));
        for (int i = 1; i < 16; ++i)
        {
            const int x = geo.vel.getX() + juce::roundToInt (i * colW (geo.vel));
            g.drawVerticalLine (x, (float) geo.vel.getY(), (float) geo.mod.getBottom());
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto geo = geom();
        const int i = colIndex (geo, e.position.x);
        if (i < 0) return;

        auto step = model.getStep (i);
        if (geo.mod.contains (e.position.toInt()))
        {
            dragZone = Zone::none;
            cycleMod (step);
            model.setStep (i, step);
        }
        else if (geo.on.contains (e.position.toInt()))
        {
            dragZone = Zone::none;
            step.on = ! step.on;
            model.setStep (i, step);
        }
        else if (geo.vel.contains (e.position.toInt()))
        {
            dragZone = Zone::vel;
            applyVel (i, geo, e.position.y);
        }
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragZone != Zone::vel) return;
        const auto geo = geom();
        const int i = colIndex (geo, e.position.x);
        if (i < 0) return;
        applyVel (i, geo, e.position.y);
        repaint();
    }

private:
    enum class Zone { none, vel };

    struct Geo { juce::Rectangle<int> vel, on, mod; };

    Geo geom() const
    {
        auto r = getLocalBounds().reduced (3);
        Geo g;
        g.mod = r.removeFromBottom (18);
        r.removeFromBottom (3);
        g.on  = r.removeFromBottom (14);
        r.removeFromBottom (3);
        g.vel = r;
        return g;
    }

    static float colW (const juce::Rectangle<int>& row) { return row.getWidth() / 16.0f; }

    static juce::Rectangle<int> colOf (const juce::Rectangle<int>& row, int i)
    {
        const float w = colW (row);
        const int x0 = row.getX() + juce::roundToInt (i * w);
        const int x1 = row.getX() + juce::roundToInt ((i + 1) * w);
        return { x0, row.getY(), x1 - x0, row.getHeight() };
    }

    int colIndex (const Geo& g, float x) const
    {
        const int i = (int) std::floor ((x - g.vel.getX()) / colW (g.vel));
        return (i >= 0 && i < 16) ? i : -1;
    }

    void applyVel (int i, const Geo& g, float y)
    {
        auto step = model.getStep (i);
        const float t = 1.0f - (y - (float) g.vel.getY()) / (float) juce::jmax (1, g.vel.getHeight());
        step.accent = juce::jlimit (0.0f, 1.0f, t);
        step.on = true;
        model.setStep (i, step);
    }

    static void cycleMod (GrooveModel::Step& s)
    {
        //  ·  ->  tie  ->  x2 -> x3 -> x4  ->  ·
        if      (! s.tie && s.ratchet <= 1) { s.tie = true; }
        else if (s.tie)                     { s.tie = false; s.ratchet = 2; }
        else if (s.ratchet == 2)            { s.ratchet = 3; }
        else if (s.ratchet == 3)            { s.ratchet = 4; }
        else                                { s.ratchet = 1; }
    }

    int  liveLen() const { return (int) apvts.getRawParameterValue (ID::arpGrooveLen)->load(); }
    bool liveOn()  const { return apvts.getRawParameterValue (ID::arpGrooveOn)->load() > 0.5f; }

    void timerCallback() override
    {
        const int  len = liveLen();
        const bool on  = liveOn();
        const auto v   = model.getVersion();
        if (len != cLen || on != cOn || v != cVer)
        {
            cLen = len; cOn = on; cVer = v;
            repaint();
        }
    }

    juce::AudioProcessorValueTreeState& apvts;
    GrooveModel& model;
    Zone dragZone = Zone::none;
    int  cLen = -1; bool cOn = false; juce::uint32 cVer = 0xffffffffu;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveGrid)
};
