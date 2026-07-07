#include "PluginEditor.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

UBEditor::UBEditor (UBAudioProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);

    // ---- preset bar ----
    addAndMakeVisible (presetBox);
    presetBox.setTextWhenNothingSelected ("-- preset --");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0 && id <= presetIdToName.size())
            proc.presets.loadByDisplayName (presetIdToName[id - 1]);
    };

    isStandalone = (proc.wrapperType == juce::AudioProcessor::wrapperType_Standalone);

    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &renBtn, &delBtn, &initBtn, &importBtn, &exportBtn, &audioBtn })
        addAndMakeVisible (b);
    audioBtn.setVisible (isStandalone);   // a DAW host provides its own audio/MIDI setup

    audioBtn.onClick = [this]
    {
       #if JucePlugin_Build_Standalone
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            holder->showAudioSettingsDialog();
       #endif
    };

    prevBtn.onClick   = [this] { proc.presets.loadPrevious(); };
    nextBtn.onClick   = [this] { proc.presets.loadNext(); };
    initBtn.onClick   = [this] { proc.presets.initPatch(); };
    saveBtn.onClick   = [this] { savePresetDialog(); };
    renBtn.onClick    = [this] { renamePresetDialog(); };
    delBtn.onClick    = [this] { deletePresetDialog(); };

    exportBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Export bank", proc.presets.getUserDir(),
                                                       juce::String ("*") + PresetManager::bankExt);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f != juce::File())
                    proc.presets.exportBank (f.withFileExtension (PresetManager::bankExt));
            });
    };

    importBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Import bank", proc.presets.getUserDir(),
                                                       juce::String ("*") + PresetManager::bankExt);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f.existsAsFile())
                    proc.presets.importBank (f);
            });
    };

    // ---- sections ----
    for (auto* s : { &secDco1, &secDco2, &secFilter, &secLfo, &secAmp, &secFenv, &secArp, &secGlobal,
                     &secDelay, &secReverb, &secUni, &secChord, &secScope })
        addAndMakeVisible (s);

    secScope.addAndMakeVisible (scopeComp);
    secScope.setFill (&scopeComp);

    makeVSlider (secDco1, ID::o1Saw,   "Saw");
    makeVSlider (secDco1, ID::o1Tri,   "Tri");
    makeVSlider (secDco1, ID::o1Pulse, "Pulse");
    makeVSlider (secDco1, ID::o1Sub,   "Sub");
    secDco1.addBreak();
    makeKnob    (secDco1, ID::o1Octave, "Oct");
    makeKnob    (secDco1, ID::o1Pw,    "PW");
    makeKnob    (secDco1, ID::o1Pwm,   "PWM");

    makeToggle  (secDco2, ID::o2On,     "DCO2 On", 92);
    makeVSlider (secDco2, ID::o2Saw,    "Saw");
    makeVSlider (secDco2, ID::o2Tri,    "Tri");
    makeVSlider (secDco2, ID::o2Pulse,  "Pulse");
    makeVSlider (secDco2, ID::o2Sub,    "Sub");
    secDco2.addBreak();
    makeKnob    (secDco2, ID::o2Octave, "Oct");
    makeKnob    (secDco2, ID::o2Detune, "Detune");
    makeKnob    (secDco2, ID::o2Pw,     "PW");
    makeKnob    (secDco2, ID::o2Pwm,    "PWM");
    makeToggle  (secDco2, ID::o2Sync,   "Sync", 64);

    makeCombo   (secFilter, ID::fType,     "Type");
    makeCombo   (secFilter, ID::hpf,       "HPF");
    secFilter.addBreak();
    makeKnob    (secFilter, ID::fCutoff,   "Cutoff");
    makeKnob    (secFilter, ID::fReso,     "Reso");
    makeKnob    (secFilter, ID::fDrive,    "Drive");
    makeKnob    (secFilter, ID::fEnvAmt,   "Env Amt");
    secFilter.addBreak();
    makeKnob    (secFilter, ID::fKeyTrack, "KeyTrk");
    makeKnob    (secFilter, ID::fLfo,      "LFO Amt");
    makeKnob    (secFilter, ID::fVelAmt,   "Vel");

    makeCombo   (secLfo, ID::lfoShape, "Shape");
    secLfo.addBreak();
    makeKnob    (secLfo, ID::lfoRate,  "Rate");
    makeKnob    (secLfo, ID::lfoDelay, "Delay");
    secLfo.addBreak();
    makeKnob    (secLfo, ID::lfoPitch, "Vibrato");

    makeVSlider (secAmp, ID::aA, "A");
    makeVSlider (secAmp, ID::aD, "D");
    makeVSlider (secAmp, ID::aS, "S");
    makeVSlider (secAmp, ID::aR, "R");

    makeVSlider (secFenv, ID::fA, "A");
    makeVSlider (secFenv, ID::fD, "D");
    makeVSlider (secFenv, ID::fS, "S");
    makeVSlider (secFenv, ID::fR, "R");

    makeToggle  (secArp, ID::arpOn,   "Arp On", 84);
    makeToggle  (secArp, ID::arpHold, "Hold", 70);
    makeToggle  (secArp, ID::arpSync, "Sync DAW", 108);
    secArp.addBreak();
    makeCombo   (secArp, ID::arpMode, "Mode");
    makeCombo   (secArp, ID::arpRate, "Rate");
    secArp.addBreak();
    makeKnob    (secArp, ID::arpOct,  "Octaves");
    makeKnob    (secArp, ID::arpGate, "Gate");
    makeKnob    (secArp, ID::bpm,     "Tempo");

    makeToggle  (secDelay, ID::dlyOn,   "Delay On", 96);
    makeToggle  (secDelay, ID::dlySync, "Sync", 70);
    makeToggle  (secDelay, ID::dlyPing, "Ping-Pong", 108);
    secDelay.addBreak();
    makeCombo   (secDelay, ID::dlyNote, "Note");
    makeKnob    (secDelay, ID::dlyTime, "Time ms");
    secDelay.addBreak();
    makeKnob    (secDelay, ID::dlyFb,   "Feedback");
    makeKnob    (secDelay, ID::dlyTone, "Tone");
    makeKnob    (secDelay, ID::dlyMix,  "Mix");

    makeCombo   (secUni, ID::uniVoices, "Voices");
    secUni.addBreak();
    makeKnob    (secUni, ID::uniDetune, "Detune");
    makeKnob    (secUni, ID::uniSpread, "Spread");

    makeToggle  (secChord, ID::chordOn, "Chord On", 100);
    secChord.addBreak();
    secChord.addAndMakeVisible (chordLearnBtn);
    secChord.addCell (&chordLearnBtn, nullptr, 76, 28);
    secChord.addBreak();
    chordLearnBtn.onClick = [this] { proc.chord.startLearn(); };
    chordLabel.setJustificationType (juce::Justification::centred);
    chordLabel.setColour (juce::Label::textColourId, UBColours::textDim);
    chordLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    secChord.addAndMakeVisible (chordLabel);
    secChord.addCell (&chordLabel, nullptr, 176, 24);

    makeToggle  (secReverb, ID::rvbOn,    "Reverb On", 106);
    secReverb.addBreak();
    makeKnob    (secReverb, ID::rvbSize,  "Size");
    makeKnob    (secReverb, ID::rvbDamp,  "Damp");
    makeKnob    (secReverb, ID::rvbWidth, "Width");
    secReverb.addBreak();
    makeKnob    (secReverb, ID::rvbPre,   "PreDly");
    makeKnob    (secReverb, ID::rvbMix,   "Mix");

    makeCombo   (secGlobal, ID::chorus,     "Chorus");
    makeKnob    (secGlobal, ID::masterGain, "Master");
    secGlobal.addBreak();
    makeKnob    (secGlobal, ID::glide,      "Glide");
    makeKnob    (secGlobal, ID::pbRange,    "PB Range");
    makeVSlider (secGlobal, ID::noise,      "Noise");
    secGlobal.addBreak();
    makeCombo   (secGlobal, ID::voiceMode,  "Voice");
    makeCombo   (secGlobal, ID::notePrio,   "Priority");

    proc.presets.onChange = [this] { refreshPresetList(); };
    refreshPresetList();

    setResizable (true, true);
    setResizeLimits (1000, 740, 1800, 1300);
    setSize (1220, 960);

    learnLabel.setJustificationType (juce::Justification::centredRight);
    learnLabel.setColour (juce::Label::textColourId, UBColours::accent);
    learnLabel.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
    addAndMakeVisible (learnLabel);

    lastPresetName = proc.presets.getCurrentName();

    addMouseListener (this, true);   // right-click on any control = MIDI Learn menu
    startTimer (250);   // window fix (first tick) + chord/learn/preset UI refresh
}

void UBEditor::timerCallback()
{
    if (! windowFixed)
    {
        windowFixed = true;
        fixWindowPosition();
    }

    // ---- chord memory UI refresh ----
    const bool learning = proc.chord.isLearning();
    chordLearnBtn.setButtonText (learning ? "..." : "Learn");
    juce::String txt = learning ? "Jouez un accord..."
                                : proc.chord.toString().replace (",", " / ");
    if (! txt.containsChar ('/') && ! learning)
        txt = "--";
    if (chordLabel.getText() != txt)
        chordLabel.setText (txt, juce::dontSendNotification);

    // ---- MIDI Learn indicator ----
    const auto armed = proc.ccMap.getArmed();
    juce::String learnTxt;
    if (armed.isNotEmpty())
    {
        auto* p = proc.apvts.getParameter (armed);
        learnTxt = "MIDI Learn : envoyez un CC -> "
                   + (p != nullptr ? p->getName (32) : armed) + "...";
    }
    if (learnLabel.getText() != learnTxt)
        learnLabel.setText (learnTxt, juce::dontSendNotification);

    // ---- preset name resync (the host can setState behind our back) ----
    const auto cur = proc.presets.getCurrentName();
    if (cur != lastPresetName)
    {
        lastPresetName = cur;
        refreshPresetList();
    }
}

void UBEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        return;

    // find a control carrying a paramID property under the click
    juce::Component* c = e.eventComponent;
    juce::String pid;
    for (int hops = 0; c != nullptr && hops < 3 && pid.isEmpty(); ++hops)
    {
        pid = c->getProperties().getWithDefault ("paramID", juce::String()).toString();
        c = c->getParentComponent();
    }
    if (pid.isEmpty())
        return;

    const int cc = proc.ccMap.getCcFor (pid);
    juce::PopupMenu m;
    m.addItem (1, cc >= 0 ? "MIDI Learn (actuellement CC " + juce::String (cc) + ")" : "MIDI Learn");
    if (cc >= 0)
        m.addItem (2, "Retirer le mapping MIDI");
    m.showMenuAsync (juce::PopupMenu::Options(), [this, pid] (int r)
    {
        if (r == 1)      proc.ccMap.armLearn (pid);
        else if (r == 2) proc.ccMap.clear (pid);
    });
}

void UBEditor::fixWindowPosition()
{
    if (! isStandalone)   // never touch the host's window in a DAW
        return;

    auto* top = getTopLevelComponent();
    if (top == nullptr)
        return;

    // Standalone: use the OS-native title bar so Windows handles moving/resizing
    // across monitors (incl. different DPI) — fixes the window getting stuck.
    if (auto* rw = dynamic_cast<juce::ResizableWindow*> (top))
        if (! rw->isUsingNativeTitleBar())
            rw->setUsingNativeTitleBar (true);

    const auto wb = top->getScreenBounds();
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto total = displays.getTotalBounds (true);

    const bool onScreen = total.intersects (wb) && total.contains (wb.getCentre());
    if (! onScreen)
        if (auto* main = displays.getPrimaryDisplay())
        {
            const auto area = main->userArea;
            top->setTopLeftPosition (area.getX() + (area.getWidth()  - top->getWidth())  / 2,
                                     area.getY() + (area.getHeight() - top->getHeight()) / 2);
        }
}

UBEditor::~UBEditor()
{
    proc.presets.onChange = nullptr;
    setLookAndFeel (nullptr);
}

juce::Slider* UBEditor::makeKnob (Section& sec, const juce::String& id, const juce::String& name)
{
    auto* s = sliders.add (new juce::Slider());
    s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 14);
    s->getProperties().set ("paramID", id);   // enables right-click MIDI Learn
    const bool isInt = dynamic_cast<juce::AudioParameterInt*> (proc.apvts.getParameter (id)) != nullptr;
    sec.addAndMakeVisible (s);
    sAtts.add (new APVTS::SliderAttachment (proc.apvts, id, *s));

    // The attachment overrides the slider's text function with the parameter's,
    // so set our own AFTER it to cap decimals (2 normally, 0 for ints / large Hz values).
    {
        float end = 1.0f;
        if (auto* rr = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (id)))
            end = rr->getNormalisableRange().end;
        const int dec = isInt ? 0 : (end >= 1000.0f ? 0 : 2);
        s->textFromValueFunction = [dec] (double v) { return juce::String (v, dec); };
        s->updateText();
    }

    auto* lab = labels.add (new juce::Label ({}, name));
    lab->setJustificationType (juce::Justification::centred);
    lab->setColour (juce::Label::textColourId, UBColours::textDim);
    lab->setFont (juce::Font (juce::FontOptions (11.0f)));
    sec.addAndMakeVisible (lab);

    sec.addCell (s, lab, 66, 84);
    return s;
}

juce::Slider* UBEditor::makeVSlider (Section& sec, const juce::String& id, const juce::String& name)
{
    auto* s = sliders.add (new juce::Slider());
    s->setSliderStyle (juce::Slider::LinearVertical);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 40, 14);
    s->getProperties().set ("paramID", id);   // enables right-click MIDI Learn
    sec.addAndMakeVisible (s);
    sAtts.add (new APVTS::SliderAttachment (proc.apvts, id, *s));
    s->textFromValueFunction = [] (double v) { return juce::String (v, 2); };
    s->updateText();

    auto* lab = labels.add (new juce::Label ({}, name));
    lab->setJustificationType (juce::Justification::centred);
    lab->setColour (juce::Label::textColourId, UBColours::textDim);
    lab->setFont (juce::Font (juce::FontOptions (11.0f)));
    sec.addAndMakeVisible (lab);

    sec.addCell (s, lab, 44, 124);
    return s;
}

juce::ComboBox* UBEditor::makeCombo (Section& sec, const juce::String& id, const juce::String& name)
{
    auto* cb = combos.add (new juce::ComboBox());
    if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        cb->addItemList (cp->choices, 1);
    sec.addAndMakeVisible (cb);
    cAtts.add (new APVTS::ComboBoxAttachment (proc.apvts, id, *cb));

    auto* lab = labels.add (new juce::Label ({}, name));
    lab->setJustificationType (juce::Justification::centred);
    lab->setColour (juce::Label::textColourId, UBColours::textDim);
    lab->setFont (juce::Font (juce::FontOptions (11.0f)));
    sec.addAndMakeVisible (lab);

    sec.addCell (cb, lab, 122, 48);
    return cb;
}

juce::ToggleButton* UBEditor::makeToggle (Section& sec, const juce::String& id, const juce::String& text, int width)
{
    auto* t = toggles.add (new juce::ToggleButton (text));
    sec.addAndMakeVisible (t);
    bAtts.add (new APVTS::ButtonAttachment (proc.apvts, id, *t));
    sec.addCell (t, nullptr, width, 28);
    return t;
}

void UBEditor::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);
    presetIdToName.clear();

    auto headingFor = [] (const juce::String& n) -> juce::String
    {
        const auto pfx = n.upToFirstOccurrenceOf (" ", false, false);
        if (pfx == "BS") return "BASS";
        if (pfx == "LD") return "LEAD";
        if (pfx == "PD") return "PAD";
        if (pfx == "KY") return "KEYS";
        if (pfx == "PL") return "PLUCK";
        if (pfx == "BR") return "BRASS";
        if (pfx == "ST") return "STRINGS";
        if (pfx == "AR") return "ARP";
        if (pfx == "FX") return "FX";
        if (pfx == "LG") return "LEGENDS";
        return {};   // Init & friends live at the top, before the first heading
    };

    int id = 0;
    juce::String lastHeading;
    for (auto& n : proc.presets.getFactoryNames())
    {
        const auto h = headingFor (n);
        if (h.isNotEmpty() && h != lastHeading)
        {
            presetBox.addSectionHeading (h);
            lastHeading = h;
        }
        presetBox.addItem (n, ++id);
        presetIdToName.add (juce::String (PresetManager::factoryPrefix) + n);
    }

    const auto users = proc.presets.getUserNames();
    if (! users.isEmpty())
        presetBox.addSectionHeading ("USER");
    for (auto& n : users)
    {
        presetBox.addItem (n, ++id);
        presetIdToName.add (n);
    }

    const auto cur = proc.presets.getCurrentName();
    for (int i = 0; i < presetIdToName.size(); ++i)
        if (presetIdToName[i] == cur || presetIdToName[i] == juce::String (PresetManager::factoryPrefix) + cur)
        {
            presetBox.setSelectedId (i + 1, juce::dontSendNotification);
            break;
        }

    // Rename/Delete only apply to user presets (factory presets are read-only)
    const bool isUser = proc.presets.isUserPreset (cur);
    renBtn.setEnabled (isUser);
    delBtn.setEnabled (isUser);
}

void UBEditor::savePresetDialog()
{
    auto* aw = new juce::AlertWindow ("Save preset", "Enter a name:", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", proc.presets.getCurrentName().removeCharacters ("[]"));
    aw->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, aw] (int result)
        {
            if (result == 1)
            {
                auto name = aw->getTextEditorContents ("name").trim();
                if (name.isNotEmpty())
                {
                    proc.presets.saveUserPreset (name);
                    refreshPresetList();
                }
            }
            delete aw;
        }), false);
}

void UBEditor::renamePresetDialog()
{
    const auto oldName = proc.presets.getCurrentName();
    if (! proc.presets.isUserPreset (oldName))   // factory presets are read-only
        return;

    auto* aw = new juce::AlertWindow ("Rename preset", "New name:", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", oldName);
    aw->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, aw, oldName] (int result)
        {
            if (result == 1)
            {
                auto newName = aw->getTextEditorContents ("name").trim();
                if (newName.isNotEmpty() && newName != oldName
                    && ! proc.presets.renameUserPreset (oldName, newName))
                    juce::NativeMessageBox::showMessageBoxAsync (
                        juce::MessageBoxIconType::WarningIcon, "Rename preset",
                        "Could not rename (a preset with that name may already exist).");
                refreshPresetList();
            }
            delete aw;
        }), false);
}

void UBEditor::deletePresetDialog()
{
    const auto name = proc.presets.getCurrentName();
    if (! proc.presets.isUserPreset (name))      // factory presets are read-only
        return;

    auto* aw = new juce::AlertWindow ("Delete preset",
                                      "Delete user preset \"" + name + "\" ?\nThis cannot be undone.",
                                      juce::MessageBoxIconType::WarningIcon);
    aw->addButton ("Delete", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, aw, name] (int result)
        {
            if (result == 1)
            {
                proc.presets.deleteUserPreset (name);
                proc.presets.initPatch();        // leave a valid patch selected
                refreshPresetList();
            }
            delete aw;
        }), false);
}

void UBEditor::paint (juce::Graphics& g)
{
    g.fillAll (UBColours::bg);
}

void UBEditor::resized()
{
    auto area = getLocalBounds().reduced (8);

    // ---- preset bar ----
    auto bar = area.removeFromTop (34);
    {
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        auto item = [] (juce::Component& c, float flex, float w)
        {
            return juce::FlexItem (c).withFlex (flex).withMinWidth (w).withMargin (juce::FlexItem::Margin (0, 3, 0, 0));
        };
        fb.items.add (item (prevBtn,   0.0f, 32));
        fb.items.add (item (presetBox, 1.0f, 160));
        fb.items.add (item (nextBtn,   0.0f, 32));
        fb.items.add (item (saveBtn,   0.0f, 64));
        fb.items.add (item (renBtn,    0.0f, 48));
        fb.items.add (item (delBtn,    0.0f, 48));
        fb.items.add (item (initBtn,   0.0f, 56));
        fb.items.add (item (importBtn, 0.0f, 72));
        fb.items.add (item (exportBtn, 0.0f, 72));
        if (isStandalone)
            fb.items.add (item (audioBtn, 0.0f, 92));
        fb.items.add (item (learnLabel, 0.8f, 120));   // empty when no learn is armed
        fb.performLayout (bar);
    }

    area.removeFromTop (8);

    const int gap = 8;
    const int rowH = (area.getHeight() - 2 * gap) / 3;
    auto row1 = area.removeFromTop (rowH);
    area.removeFromTop (gap);
    auto row2 = area.removeFromTop (rowH);
    area.removeFromTop (gap);
    auto row3 = area;

    auto layoutRow = [] (juce::Rectangle<int> r, std::vector<std::pair<Section*, float>> items)
    {
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        for (auto& it : items)
            fb.items.add (juce::FlexItem (*it.first).withFlex (it.second).withMargin (juce::FlexItem::Margin (0, 4, 0, 4)));
        fb.performLayout (r);
    };

    layoutRow (row1, { { &secDco1, 1.0f }, { &secDco2, 1.5f }, { &secFilter, 1.3f }, { &secLfo, 0.8f } });
    layoutRow (row2, { { &secAmp, 0.75f }, { &secFenv, 0.75f }, { &secArp, 1.45f }, { &secGlobal, 1.05f } });
    layoutRow (row3, { { &secUni, 0.62f }, { &secChord, 0.72f }, { &secDelay, 1.25f }, { &secReverb, 0.9f },
                       { &secScope, 0.72f } });
}
