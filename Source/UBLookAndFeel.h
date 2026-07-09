#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// UB_Poly16 look: dark charcoal, AZUR for structure (section titles, selectors,
// borders) and MAUVE for everything that lives (knob arcs, slider fills, groove
// bars, toggles, values). "accent" = azur (structure), "active" = mauve.
namespace UBColours
{
    const juce::Colour bg       { 0xff14161a };
    const juce::Colour panel    { 0xff242a31 };
    const juce::Colour panel2   { 0xff1a1e24 };   // gradient bottom
    const juce::Colour panelHi  { 0xff39414a };   // relief top (knob/thumb body)
    const juce::Colour panelLo  { 0xff1f242a };   // relief bottom
    const juce::Colour accent   { 0xff4a9eff };   // azur  — structure
    const juce::Colour active   { 0xffb07cf0 };   // mauve — values / active
    const juce::Colour valueTx  { 0xffc9a6f5 };   // value read-outs (mauve clair)
    const juce::Colour text     { 0xffd6dde3 };
    const juce::Colour textDim  { 0xff8b949b };
    const juce::Colour track    { 0xff0d0f12 };
    const juce::Colour outline  { 0xff33414f };   // bluish border for selectors
}

class UBLookAndFeel : public juce::LookAndFeel_V4
{
public:
    UBLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, UBColours::bg);
        setColour (juce::Slider::textBoxTextColourId, UBColours::valueTx);   // values = mauve
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::backgroundColourId, UBColours::track);
        setColour (juce::ComboBox::textColourId, UBColours::text);
        setColour (juce::ComboBox::outlineColourId, UBColours::outline);     // azur-grey
        setColour (juce::ComboBox::arrowColourId, UBColours::accent);        // azur
        setColour (juce::PopupMenu::backgroundColourId, UBColours::panel);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, UBColours::accent);
        setColour (juce::PopupMenu::highlightedTextColourId, UBColours::bg);
        setColour (juce::PopupMenu::textColourId, UBColours::text);
        setColour (juce::TextButton::buttonColourId, UBColours::panelHi);
        setColour (juce::TextButton::textColourOnId, UBColours::accent);
        setColour (juce::TextButton::textColourOffId, UBColours::text);
        setColour (juce::Label::textColourId, UBColours::text);
        setColour (juce::ToggleButton::textColourId, UBColours::text);
        setColour (juce::ToggleButton::tickColourId, UBColours::active);     // mauve
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override
    {
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float lineW = juce::jmax (2.0f, radius * 0.14f);
        const float arcR  = radius - lineW * 0.5f;

        // body with soft relief (top-lit)
        auto body = bounds.reduced (lineW);
        juce::ColourGradient bodyGrad (UBColours::panelHi, body.getCentreX(), body.getY(),
                                       UBColours::panelLo, body.getCentreX(), body.getBottom(), false);
        g.setGradientFill (bodyGrad);
        g.fillEllipse (body);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawEllipse (body.reduced (0.5f), 1.0f);

        // background arc
        juce::Path back;
        back.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (UBColours::track);
        g.strokePath (back, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // value arc (mauve) + soft halo
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (UBColours::active.withAlpha (0.18f));
        g.strokePath (arc, juce::PathStrokeType (lineW * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (UBColours::active);
        g.strokePath (arc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // pointer (mauve)
        juce::Path p;
        const float pl = radius * 0.55f;
        p.addRoundedRectangle (-lineW * 0.35f, -pl, lineW * 0.7f, pl, lineW * 0.35f);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (UBColours::active);
        g.fillPath (p);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPos, float, float,
                           const juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            const float cx = x + w * 0.5f;
            const float trackW = juce::jmax (4.0f, w * 0.18f);
            juce::Rectangle<float> track (cx - trackW * 0.5f, (float) y, trackW, (float) h);
            g.setColour (UBColours::track);
            g.fillRoundedRectangle (track, trackW * 0.5f);

            juce::Rectangle<float> fill (track.getX(), sliderPos, trackW, (float) (y + h) - sliderPos);
            g.setColour (UBColours::active.withAlpha (0.16f));                 // halo
            g.fillRoundedRectangle (fill.expanded (2.0f, 0.0f), trackW);
            g.setColour (UBColours::active);                                   // fill (mauve)
            g.fillRoundedRectangle (fill, trackW * 0.5f);

            // thumb with relief
            const float thumbH = juce::jmax (8.0f, h * 0.04f) + 8.0f;
            juce::Rectangle<float> thumb (x + 2.0f, sliderPos - thumbH * 0.5f, w - 4.0f, thumbH);
            juce::ColourGradient tg (UBColours::panelHi, thumb.getX(), thumb.getY(),
                                     UBColours::panelLo, thumb.getX(), thumb.getBottom(), false);
            g.setGradientFill (tg);
            g.fillRoundedRectangle (thumb, 3.0f);
            g.setColour (juce::Colours::white.withAlpha (0.09f));
            g.drawRoundedRectangle (thumb.reduced (0.5f), 3.0f, 1.0f);
        }
        else
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, s);
        }
    }

    juce::Font getLabelFont (juce::Label&) override
    {
        return juce::Font (juce::FontOptions (12.0f));
    }
};
