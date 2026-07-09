#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UBLookAndFeel.h"
#include <vector>

// Full-window preset browser overlay: category column, live search, per-preset
// favourites (★) and management actions. Opens over the synth; the editor wires
// the footer/manage callbacks to its existing dialogs.
class PresetBrowser : public juce::Component
{
public:
    std::function<void()> onClose, onSave, onRename, onDelete, onInit, onImport, onExport;

    explicit PresetBrowser (UBAudioProcessor& p) : proc (p)
    {
        catModel  = std::make_unique<CatModel>  (*this);
        presModel = std::make_unique<PresModel> (*this);

        addAndMakeVisible (catList);
        catList.setModel (catModel.get());
        catList.setRowHeight (30);
        catList.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);

        addAndMakeVisible (presList);
        presList.setModel (presModel.get());
        presList.setRowHeight (30);
        presList.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);

        addAndMakeVisible (search);
        search.setTextToShowWhenEmpty ("Search presets...", UBColours::textDim);
        search.setColour (juce::TextEditor::backgroundColourId, UBColours::track);
        search.setColour (juce::TextEditor::outlineColourId, UBColours::outline);
        search.setColour (juce::TextEditor::textColourId, UBColours::text);
        search.setColour (juce::TextEditor::highlightColourId, UBColours::accent.withAlpha (0.3f));
        search.onTextChange = [this] { query = search.getText().trim().toLowerCase(); rebuild(); };

        auto setupBtn = [this] (juce::TextButton& b, const juce::String& t)
        { b.setButtonText (t); addAndMakeVisible (b); };
        setupBtn (closeBtn, "X"); setupBtn (initBtn, "Init"); setupBtn (importBtn, "Import");
        setupBtn (exportBtn, "Export"); setupBtn (renBtn, "Rename"); setupBtn (delBtn, "Delete");
        setupBtn (saveBtn, "Save"); setupBtn (okBtn, "OK");
        okBtn.setColour (juce::TextButton::buttonColourId, UBColours::accent);
        okBtn.setColour (juce::TextButton::textColourOffId, UBColours::bg);

        closeBtn.onClick  = [this] { if (onClose)  onClose(); };
        okBtn.onClick     = [this] { if (onClose)  onClose(); };
        initBtn.onClick   = [this] { if (onInit)   onInit(); };
        importBtn.onClick = [this] { if (onImport) onImport(); };
        exportBtn.onClick = [this] { if (onExport) onExport(); };
        renBtn.onClick    = [this] { if (onRename) onRename(); };
        delBtn.onClick    = [this] { if (onDelete) onDelete(); };
        saveBtn.onClick   = [this] { if (onSave)   onSave(); };

        setWantsKeyboardFocus (true);
    }

    void open()
    {
        setVisible (true);
        toFront (true);
        refresh();
        search.grabKeyboardFocus();
    }

    // rebuild everything from the current preset set + favourites
    void refresh()
    {
        entries.clear();
        for (auto& d : proc.presets.getAllDisplayNames())
        {
            Entry e; e.display = d;
            split (d, e.cat, e.clean);
            entries.push_back (e);
        }
        rebuild();

        const bool isUser = proc.presets.isUserPreset (proc.presets.getCurrentName());
        renBtn.setEnabled (isUser);
        delBtn.setEnabled (isUser);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.55f));   // backdrop
        auto pf = panel().toFloat();
        juce::ColourGradient pg (juce::Colour (0xff1c2128), pf.getX(), pf.getY(),
                                 juce::Colour (0xff161a20), pf.getX(), pf.getBottom(), false);
        g.setGradientFill (pg);
        g.fillRoundedRectangle (pf, 14.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (pf.reduced (0.5f), 14.0f, 1.0f);

        // header / footer separators + title
        auto p = panel();
        g.setColour (UBColours::outline);
        g.drawHorizontalLine (p.getY() + kHead, (float) p.getX(), (float) p.getRight());
        g.drawHorizontalLine (p.getBottom() - kFoot, (float) p.getX(), (float) p.getRight());
        g.drawVerticalLine (p.getX() + kCatW, (float) (p.getY() + kHead), (float) (p.getBottom() - kFoot));
        g.setColour (UBColours::accent);
        g.setFont (juce::Font (juce::FontOptions (12.0f)).boldened());
        g.drawText ("PRESETS", p.getX() + 16, p.getY(), 120, kHead, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto p = panel();
        search.setBounds (p.getX() + 130, p.getY() + 10, juce::jmin (320, kCatW + 180), kHead - 20);
        closeBtn.setBounds (p.getRight() - 42, p.getY() + 10, 30, kHead - 20);

        catList.setBounds  (p.getX() + 8, p.getY() + kHead + 6, kCatW - 12, p.getHeight() - kHead - kFoot - 12);
        presList.setBounds (p.getX() + kCatW + 8, p.getY() + kHead + 6,
                            p.getWidth() - kCatW - 16, p.getHeight() - kHead - kFoot - 12);

        auto foot = juce::Rectangle<int> (p.getX(), p.getBottom() - kFoot, p.getWidth(), kFoot).reduced (14, 12);
        auto place = [&foot] (juce::TextButton& b, int w, bool right)
        { b.setBounds (right ? foot.removeFromRight (w) : foot.removeFromLeft (w)); foot.removeFromLeft (0); };
        initBtn.setBounds   (foot.removeFromLeft (56));  foot.removeFromLeft (6);
        importBtn.setBounds (foot.removeFromLeft (70));  foot.removeFromLeft (6);
        exportBtn.setBounds (foot.removeFromLeft (70));
        okBtn.setBounds     (foot.removeFromRight (58)); foot.removeFromRight (12);
        saveBtn.setBounds   (foot.removeFromRight (66)); foot.removeFromRight (6);
        delBtn.setBounds    (foot.removeFromRight (64)); foot.removeFromRight (6);
        renBtn.setBounds    (foot.removeFromRight (68));
        juce::ignoreUnused (place);
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey) { if (onClose) onClose(); return true; }
        return false;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! panel().contains (e.getPosition()))   // click on the backdrop closes
            if (onClose) onClose();
    }

private:
    struct Entry { juce::String display, clean, cat; };
    struct CatRow { juce::String key, label; int count; };

    static constexpr int kHead = 54, kFoot = 56, kCatW = 196;

    juce::Rectangle<int> panel() const
    {
        const int w = juce::jmin (900, getWidth()  - 60);
        const int h = juce::jmin (560, getHeight() - 60);
        return { (getWidth() - w) / 2, (getHeight() - h) / 2, w, h };
    }

    static juce::String labelForCode (const juce::String& c)
    {
        if (c == "BS") return "Bass";    if (c == "LD") return "Lead";   if (c == "PD") return "Pad";
        if (c == "KY") return "Keys";    if (c == "PL") return "Pluck";  if (c == "BR") return "Brass";
        if (c == "ST") return "Strings"; if (c == "AR") return "Arp";    if (c == "FX") return "FX";
        if (c == "PC") return "Perc";    if (c == "LG") return "Legends";
        return {};
    }

    static void split (const juce::String& display, juce::String& cat, juce::String& clean)
    {
        const juce::String fp (PresetManager::factoryPrefix);
        if (! display.startsWith (fp)) { cat = "User"; clean = display; return; }
        const auto core = display.substring (fp.length());
        const auto code = core.upToFirstOccurrenceOf (" ", false, false);
        const auto lab  = labelForCode (code);
        if (lab.isEmpty()) { cat = "Other"; clean = core; }
        else               { cat = lab;     clean = core.fromFirstOccurrenceOf (" ", false, false); }
    }

    juce::String curDisplay() const
    {
        const auto cur = proc.presets.getCurrentName();
        return proc.presets.isUserPreset (cur) ? cur : juce::String (PresetManager::factoryPrefix) + cur;
    }

    void rebuild()
    {
        // category rows (favorites + all + present categories in canonical order)
        int favN = 0; for (auto& e : entries) if (proc.presets.isFavorite (e.display)) ++favN;
        catRows.clear();
        catRows.push_back ({ "\1fav", "Favorites",  favN });
        catRows.push_back ({ "\1all", "All", (int) entries.size() });
        const char* order[] = { "Bass","Lead","Pad","Keys","Pluck","Brass","Strings","Arp","FX","Perc","Legends","Other","User" };
        for (auto* lab : order)
        {
            int n = 0; for (auto& e : entries) if (e.cat == lab) ++n;
            if (n > 0) catRows.push_back ({ lab, lab, n });
        }
        if (sel.isEmpty()) sel = "\1all";

        // filtered preset view
        view.clear();
        for (int i = 0; i < (int) entries.size(); ++i)
        {
            const auto& e = entries[(size_t) i];
            const bool qOk = query.isEmpty() || e.clean.toLowerCase().contains (query);
            const bool catOk = query.isNotEmpty()          // a search spans every category
                             || sel == "\1all"
                             || (sel == "\1fav" ? proc.presets.isFavorite (e.display) : e.cat == sel);
            if (catOk && qOk) view.push_back (i);
        }
        catList.updateContent();
        presList.updateContent();
        repaint();
    }

    // ---- category list model ----
    struct CatModel : juce::ListBoxModel
    {
        PresetBrowser& b;
        explicit CatModel (PresetBrowser& o) : b (o) {}
        int getNumRows() override { return (int) b.catRows.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override
        {
            if (row < 0 || row >= (int) b.catRows.size()) return;
            const auto& c = b.catRows[(size_t) row];
            const bool on = (c.key == b.sel);
            if (on)
            {
                g.setColour (UBColours::accent.withAlpha (0.12f));
                g.fillRect (0, 0, w, h);
                g.setColour (UBColours::accent);
                g.fillRect (0, 0, 3, h);
            }
            const bool fav = (c.key == "\1fav");
            g.setColour (on ? UBColours::accent : UBColours::textDim);
            g.setFont (juce::Font (juce::FontOptions (13.0f)));
            juce::String lab = (fav ? juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x85")) + " " : "") + c.label;
            g.drawText (lab, 12, 0, w - 46, h, juce::Justification::centredLeft);
            g.setColour (UBColours::textDim);
            g.setFont (juce::Font (juce::FontOptions (11.0f)));
            g.drawText (juce::String (c.count), w - 40, 0, 32, h, juce::Justification::centredRight);
        }
        void listBoxItemClicked (int row, const juce::MouseEvent&) override
        {
            if (row >= 0 && row < (int) b.catRows.size()) { b.sel = b.catRows[(size_t) row].key; b.rebuild(); }
        }
    };

    // ---- preset list model ----
    struct PresModel : juce::ListBoxModel
    {
        PresetBrowser& b;
        explicit PresModel (PresetBrowser& o) : b (o) {}
        int getNumRows() override { return (int) b.view.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override
        {
            if (row < 0 || row >= (int) b.view.size()) return;
            const auto& e = b.entries[(size_t) b.view[(size_t) row]];
            const bool cur = (e.display == b.curDisplay());
            if (cur)
            {
                g.setColour (UBColours::active.withAlpha (0.14f));
                g.fillRoundedRectangle (2.0f, 1.0f, (float) w - 4, (float) h - 2, 5.0f);
                g.setColour (UBColours::active);
                g.fillRect (2, 1, 3, h - 2);
            }
            const bool fav = b.proc.presets.isFavorite (e.display);
            g.setColour (fav ? UBColours::active : juce::Colour (0xff4a525c));
            g.setFont (juce::Font (juce::FontOptions (15.0f)));
            g.drawText (juce::String (juce::CharPointer_UTF8 (fav ? "\xe2\x98\x85" : "\xe2\x98\x86")),
                        10, 0, 22, h, juce::Justification::centred);
            g.setColour (cur ? UBColours::valueTx : UBColours::text);
            g.setFont (juce::Font (juce::FontOptions (14.0f)));
            g.drawText (e.clean, 40, 0, w - 40 - 78, h, juce::Justification::centredLeft);
            // category tag
            juce::Rectangle<int> tag (w - 74, h / 2 - 9, 64, 18);
            g.setColour (UBColours::outline);
            g.drawRoundedRectangle (tag.toFloat(), 9.0f, 1.0f);
            g.setColour (UBColours::textDim);
            g.setFont (juce::Font (juce::FontOptions (10.0f)));
            g.drawText (e.cat.toUpperCase(), tag, juce::Justification::centred);
        }
        void listBoxItemClicked (int row, const juce::MouseEvent& e) override
        {
            if (row < 0 || row >= (int) b.view.size()) return;
            const auto& en = b.entries[(size_t) b.view[(size_t) row]];
            if (e.x < 34)   b.proc.presets.toggleFavorite (en.display);   // star zone
            else            b.proc.presets.loadByDisplayName (en.display);
        }
        void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
        {
            if (row < 0 || row >= (int) b.view.size()) return;
            b.proc.presets.loadByDisplayName (b.entries[(size_t) b.view[(size_t) row]].display);
            if (b.onClose) b.onClose();   // double-click = load + close
        }
    };

    UBAudioProcessor& proc;
    std::vector<Entry>  entries;
    std::vector<CatRow> catRows;
    std::vector<int>    view;
    juce::String sel, query;

    juce::ListBox catList, presList;
    std::unique_ptr<CatModel>  catModel;
    std::unique_ptr<PresModel> presModel;
    juce::TextEditor search;
    juce::TextButton closeBtn, initBtn, importBtn, exportBtn, renBtn, delBtn, saveBtn, okBtn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};
