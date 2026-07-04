#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Flat "dark slate + orange accent" look for UB_Poly16.
namespace UBColours
{
    const juce::Colour bg       { 0xff1f2327 };
    const juce::Colour panel    { 0xff2a2f35 };
    const juce::Colour panelHi  { 0xff333a41 };
    const juce::Colour accent   { 0xffb07cf0 };   // mauve
    const juce::Colour text     { 0xffd6dde3 };
    const juce::Colour textDim  { 0xff8b949b };
    const juce::Colour track    { 0xff14171a };
}

class UBLookAndFeel : public juce::LookAndFeel_V4
{
public:
    UBLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, UBColours::bg);
        setColour (juce::Slider::textBoxTextColourId, UBColours::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::backgroundColourId, UBColours::track);
        setColour (juce::ComboBox::textColourId, UBColours::text);
        setColour (juce::ComboBox::outlineColourId, UBColours::panelHi);
        setColour (juce::ComboBox::arrowColourId, UBColours::accent);
        setColour (juce::PopupMenu::backgroundColourId, UBColours::panel);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, UBColours::accent);
        setColour (juce::PopupMenu::textColourId, UBColours::text);
        setColour (juce::TextButton::buttonColourId, UBColours::panelHi);
        setColour (juce::TextButton::textColourOnId, UBColours::accent);
        setColour (juce::TextButton::textColourOffId, UBColours::text);
        setColour (juce::Label::textColourId, UBColours::text);
        setColour (juce::ToggleButton::textColourId, UBColours::text);
        setColour (juce::ToggleButton::tickColourId, UBColours::accent);
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
        const float arcR = radius - lineW * 0.5f;

        // body
        g.setColour (UBColours::panelHi);
        g.fillEllipse (bounds.reduced (lineW));

        // background arc
        juce::Path back;
        back.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (UBColours::track);
        g.strokePath (back, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // value arc
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (UBColours::accent);
        g.strokePath (arc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // pointer
        juce::Path p;
        const float pl = radius * 0.55f;
        p.addRoundedRectangle (-lineW * 0.35f, -pl, lineW * 0.7f, pl, lineW * 0.35f);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (UBColours::text);
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

            g.setColour (UBColours::accent);
            juce::Rectangle<float> fill (track.getX(), sliderPos, trackW, (float) (y + h) - sliderPos);
            g.fillRoundedRectangle (fill, trackW * 0.5f);

            // thumb
            const float thumbH = juce::jmax (8.0f, h * 0.04f) + 8.0f;
            juce::Rectangle<float> thumb (x + 2.0f, sliderPos - thumbH * 0.5f, w - 4.0f, thumbH);
            g.setColour (UBColours::text);
            g.fillRoundedRectangle (thumb, 3.0f);
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
