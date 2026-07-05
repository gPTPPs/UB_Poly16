#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UBLookAndFeel.h"
#include "Scope.h"

// A titled panel that lays its child controls out in centred rows.
// Rows come from explicit addBreak() calls (plus width wrapping as fallback);
// each row is centred horizontally, cells are centred vertically within their
// row, and the whole block is centred vertically inside the panel.
class Section : public juce::Component
{
public:
    explicit Section (const juce::String& t) : title (t) {}

    void addCell (juce::Component* main, juce::Label* lab, int w, int h)
    {
        cells.push_back ({ main, lab, w, h });
    }

    void addBreak()                   { cells.push_back ({ nullptr, nullptr, 0, 0 }); }
    void setFill (juce::Component* c) { fillComp = c; }   // one component fills the panel

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (UBColours::panel);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (UBColours::accent);
        g.fillRoundedRectangle (r.removeFromTop (22.0f).reduced (1.0f), 7.0f);
        g.setColour (UBColours::bg);
        g.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
        g.drawText (title, getLocalBounds().removeFromTop (22), juce::Justification::centred);
    }

    void resized() override
    {
        if (fillComp != nullptr)
        {
            fillComp->setBounds (8, 28, getWidth() - 16, getHeight() - 36);
            return;
        }

        const int pad = 8, gapX = 6, gapY = 6, top = 26;
        const int availW = juce::jmax (10, getWidth() - 2 * pad);

        // group cells into rows (explicit breaks, width wrap as fallback)
        struct Row { std::vector<const Cell*> items; int w = 0, h = 0; };
        std::vector<Row> rows (1);
        for (auto& c : cells)
        {
            if (c.main == nullptr)
            {
                if (! rows.back().items.empty())
                    rows.emplace_back();
                continue;
            }
            auto* r = &rows.back();
            if (! r->items.empty() && r->w + gapX + c.w > availW)
            {
                rows.emplace_back();
                r = &rows.back();
            }
            r->w += (r->items.empty() ? 0 : gapX) + c.w;
            r->h  = juce::jmax (r->h, c.h);
            r->items.push_back (&c);
        }

        int totalH = 0, numRows = 0;
        for (auto& r : rows)
            if (! r.items.empty()) { totalH += r.h; ++numRows; }
        totalH += gapY * juce::jmax (0, numRows - 1);

        int y = top + juce::jmax (0, (getHeight() - top - pad - totalH) / 2);
        for (auto& r : rows)
        {
            if (r.items.empty())
                continue;
            int x = pad + juce::jmax (0, (availW - r.w) / 2);
            for (auto* c : r.items)
            {
                juce::Rectangle<int> cell (x, y + (r.h - c->h) / 2, c->w, c->h);
                if (c->label != nullptr)
                    c->label->setBounds (cell.removeFromBottom (16));
                c->main->setBounds (cell);
                x += c->w + gapX;
            }
            y += r.h + gapY;
        }
    }

private:
    struct Cell { juce::Component* main; juce::Label* label; int w, h; };
    juce::String title;
    std::vector<Cell> cells;
    juce::Component* fillComp = nullptr;
};

class UBEditor : public juce::AudioProcessorEditor,
                 private juce::Timer
{
public:
    explicit UBEditor (UBAudioProcessor&);
    ~UBEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;   // periodic: window fix (once) + chord UI refresh
    void mouseDown (const juce::MouseEvent&) override;   // right-click = MIDI Learn

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    // control factories (register attachment + label, return the component)
    juce::Slider* makeKnob (Section&, const juce::String& id, const juce::String& name);
    juce::Slider* makeVSlider (Section&, const juce::String& id, const juce::String& name);
    juce::ComboBox* makeCombo (Section&, const juce::String& id, const juce::String& name);
    juce::ToggleButton* makeToggle (Section&, const juce::String& id, const juce::String& text, int width = 120);

    void refreshPresetList();
    void savePresetDialog();
    void fixWindowPosition();

    UBAudioProcessor& proc;
    UBLookAndFeel lnf;

    // preset bar
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "Save" },
                     initBtn { "Init" }, importBtn { "Import" }, exportBtn { "Export" },
                     audioBtn { "Audio/MIDI" };
    bool isStandalone = false;

    // sections
    Section secDco1 { "DCO 1" }, secDco2 { "DCO 2" }, secFilter { "FILTER" }, secLfo { "LFO" },
            secAmp { "AMP ENV" }, secFenv { "FILTER ENV" }, secArp { "ARP" }, secGlobal { "GLOBAL" },
            secDelay { "DELAY" }, secReverb { "REVERB" }, secUni { "UNISON" }, secChord { "CHORD" },
            secScope { "SCOPE" };

    // chord memory UI
    juce::TextButton chordLearnBtn { "Learn" };
    juce::Label chordLabel;
    bool windowFixed = false;

    // MIDI Learn indicator + host-driven preset name changes (both polled
    // by the timer: setStateInformation may run off the message thread)
    juce::Label learnLabel;
    juce::String lastPresetName;

    // oscilloscope + categorised preset list
    ScopeComponent scopeComp { proc.scope };
    juce::StringArray presetIdToName;   // ComboBox item id-1 -> display name

    // owned controls + attachments
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::ComboBox> combos;
    juce::OwnedArray<juce::ToggleButton> toggles;
    juce::OwnedArray<juce::Label> labels;
    juce::OwnedArray<APVTS::SliderAttachment> sAtts;
    juce::OwnedArray<APVTS::ComboBoxAttachment> cAtts;
    juce::OwnedArray<APVTS::ButtonAttachment> bAtts;

    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UBEditor)
};
