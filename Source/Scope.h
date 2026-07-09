#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "UBLookAndFeel.h"

// Lock-free mono ring buffer: the audio thread pushes the master output,
// the scope component reads the freshest window at paint time. Index races
// only ever cost a visually torn frame, which is fine for a display.
class ScopeBuffer
{
public:
    static constexpr int size = 8192;   // power of two

    void push (const float* l, const float* r, int n)
    {
        int w = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            data[w] = 0.5f * (l[i] + (r != nullptr ? r[i] : l[i]));
            w = (w + 1) & (size - 1);
        }
        writePos.store (w, std::memory_order_release);
    }

    void read (float* dest, int n) const
    {
        const int w = writePos.load (std::memory_order_acquire);
        const int start = (w - n) & (size - 1);
        for (int i = 0; i < n; ++i)
            dest[i] = data[(start + i) & (size - 1)];
    }

private:
    float data[size] {};
    std::atomic<int> writePos { 0 };
};

// 30 fps oscilloscope with a rising-zero-crossing trigger for a stable trace.
class ScopeComponent : public juce::Component,
                       private juce::Timer
{
public:
    explicit ScopeComponent (ScopeBuffer& b) : buf (b)
    {
        setInterceptsMouseClicks (false, false);
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (UBColours::bg);
        g.fillRoundedRectangle (r, 6.0f);

        constexpr int N = 2048;
        float tmp[N];
        buf.read (tmp, N);

        // trigger on the first rising zero crossing in the older half
        int trig = 0;
        for (int i = 1; i < N / 2; ++i)
            if (tmp[i - 1] <= 0.0f && tmp[i] > 0.0f) { trig = i; break; }

        const float midY = r.getCentreY();
        g.setColour (UBColours::textDim.withAlpha (0.3f));
        g.drawHorizontalLine ((int) midY, r.getX(), r.getRight());

        constexpr int win = N / 2;
        juce::Path p;
        for (int i = 0; i < win; ++i)
        {
            const float x = r.getX() + r.getWidth() * (float) i / (float) (win - 1);
            const float y = midY - juce::jlimit (-1.0f, 1.0f, tmp[trig + i]) * r.getHeight() * 0.48f;
            if (i == 0) p.startNewSubPath (x, y);
            else        p.lineTo (x, y);
        }
        g.setColour (UBColours::active);
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }

private:
    void timerCallback() override { repaint(); }

    ScopeBuffer& buf;
};
