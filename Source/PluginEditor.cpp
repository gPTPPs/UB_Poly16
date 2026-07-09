#include "PluginEditor.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

UBEditor::UBEditor (UBAudioProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);

    // ---- preset bar ----
    isStandalone = (proc.wrapperType == juce::AudioProcessor::wrapperType_Standalone);

    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &presetField, &dotsBtn })
        addAndMakeVisible (b);
    presetField.setColour (juce::TextButton::buttonColourId, UBColours::track);

    prevBtn.onClick     = [this] { proc.presets.loadPrevious(); };
    nextBtn.onClick     = [this] { proc.presets.loadNext(); };
    saveBtn.onClick     = [this] { savePresetDialog(); };
    presetField.onClick = [this] { browser.open(); };
    dotsBtn.onClick     = [this] { showManageMenu(); };

    addChildComponent (browser);
    browser.onClose  = [this] { browser.setVisible (false); grooveGrid.repaint(); };
    browser.onSave   = [this] { savePresetDialog(); };
    browser.onRename = [this] { renamePresetDialog(); };
    browser.onDelete = [this] { deletePresetDialog(); };
    browser.onInit   = [this] { proc.presets.initPatch(); };
    browser.onImport = [this] { importBankDialog(); };
    browser.onExport = [this] { exportBankDialog(); };

    // ---- sections ----
    for (auto* s : { &secDco1, &secDco2, &secFilter, &secLfo, &secAmp, &secFenv, &secPenv, &secArp, &secGlobal,
                     &secDelay, &secReverb, &secUni, &secChord, &secScope, &secGroove, &secGrooveCtl })
        addAndMakeVisible (s);

    secScope.addAndMakeVisible (scopeComp);
    secScope.setFill (&scopeComp);

    secGroove.addAndMakeVisible (grooveGrid);
    secGroove.setFill (&grooveGrid);

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
    makeKnob    (secDco2, ID::o2Coarse, "Coarse");
    makeKnob    (secDco2, ID::o2Detune, "Detune");
    makeKnob    (secDco2, ID::o2Pw,     "PW");
    makeKnob    (secDco2, ID::o2Pwm,    "PWM");
    makeToggle  (secDco2, ID::o2Sync,   "Sync", 64);
    makeKnob    (secDco2, ID::o2Ring,   "Ring");

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
    makeKnob    (secFilter, ID::atCutoff,  "AT>Cut");

    makeCombo   (secLfo, ID::lfoShape, "Shape");
    secLfo.addBreak();
    makeKnob    (secLfo, ID::lfoRate,  "Rate");
    makeKnob    (secLfo, ID::lfoDelay, "Delay");
    secLfo.addBreak();
    makeKnob    (secLfo, ID::lfoPitch, "Vibrato");
    makeKnob    (secLfo, ID::atVib,    "AT>Vib");

    makeVSlider (secAmp, ID::aA, "A");
    makeVSlider (secAmp, ID::aD, "D");
    makeVSlider (secAmp, ID::aS, "S");
    makeVSlider (secAmp, ID::aR, "R");
    secAmp.addBreak();
    makeKnob    (secAmp, ID::attackClick, "Click");

    makeVSlider (secFenv, ID::fA, "A");
    makeVSlider (secFenv, ID::fD, "D");
    makeVSlider (secFenv, ID::fS, "S");
    makeVSlider (secFenv, ID::fR, "R");

    makeVSlider (secPenv, ID::pA, "A");
    makeVSlider (secPenv, ID::pD, "D");
    makeVSlider (secPenv, ID::pS, "S");
    makeVSlider (secPenv, ID::pR, "R");
    secPenv.addBreak();
    makeKnob    (secPenv, ID::pEnvAmt,    "Amount");
    makeCombo   (secPenv, ID::pEnvTarget, "Target");

    makeToggle  (secArp, ID::arpOn,   "Arp On", 84);
    makeToggle  (secArp, ID::arpHold, "Hold", 70);
    makeToggle  (secArp, ID::arpSync, "Sync DAW", 108);
    secArp.addBreak();
    makeCombo   (secArp, ID::arpMode, "Mode");
    makeCombo   (secArp, ID::arpRate, "Rate");
    makeKnob    (secArp, ID::arpOct,  "Octaves");
    makeKnob    (secArp, ID::arpGate, "Gate");
    makeKnob    (secArp, ID::bpm,     "Tempo");

    // groove controls live in their own panel next to the step grid (below)
    makeToggle  (secGrooveCtl, ID::arpGrooveOn,  "Groove", 84);
    makeKnob    (secGrooveCtl, ID::arpSwing,      "Swing");
    makeKnob    (secGrooveCtl, ID::arpGrooveLen,  "Steps");

    makeToggle  (secDelay, ID::dlyOn,   "Delay On", 96);
    makeToggle  (secDelay, ID::dlySync, "Sync", 70);
    makeToggle  (secDelay, ID::dlyPing, "Ping-Pong", 108);
    secDelay.addBreak();
    makeCombo   (secDelay, ID::dlyNote, "Note");
    makeKnob    (secDelay, ID::dlyTime, "Time ms");
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
    setResizeLimits (1000, 860, 1800, 1400);
    setSize (1560, 988);

    learnLabel.setJustificationType (juce::Justification::centredRight);
    learnLabel.setColour (juce::Label::textColourId, UBColours::active);
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
        learnTxt = "MIDI Learn: send a CC -> "
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
    m.addItem (1, cc >= 0 ? "MIDI Learn (currently CC " + juce::String (cc) + ")" : "MIDI Learn");
    if (cc >= 0)
        m.addItem (2, "Remove MIDI mapping");
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

    // Centre the window on the display it sits on, keeping the top-left inside the
    // usable area so the title/preset bar is always reachable — even if a stale
    // saved position (e.g. from a previously oversized window) pushed it off-screen.
    auto* disp = displays.getDisplayForRect (wb, true);
    if (disp == nullptr) disp = displays.getPrimaryDisplay();
    if (disp == nullptr) return;

    const auto area = disp->userArea;
    const int x = area.getX() + juce::jmax (0, (area.getWidth()  - wb.getWidth())  / 2);
    const int y = area.getY() + juce::jmax (0, (area.getHeight() - wb.getHeight()) / 2);
    top->setTopLeftPosition (x, y);
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
    const auto cur = proc.presets.getCurrentName();

    // field shows the clean name (drop a leading 2-letter category code)
    juce::String label = cur;
    const auto tok = cur.upToFirstOccurrenceOf (" ", false, false);
    if (tok.length() == 2 && tok.containsOnly ("ABCDEFGHIJKLMNOPQRSTUVWXYZ"))
        label = cur.fromFirstOccurrenceOf (" ", false, false);
    presetField.setButtonText (label.isEmpty() ? "-- preset --" : label);

    browser.refresh();
    proc.groove.refreshFromState();   // ensure the audio-side snapshot matches the loaded tree
    grooveGrid.repaint();             // show a freshly-loaded step pattern without needing a click
}

void UBEditor::showManageMenu()
{
    const bool isUser = proc.presets.isUserPreset (proc.presets.getCurrentName());
    juce::PopupMenu m;
    m.addItem (1, "Rename...", isUser);
    m.addItem (2, "Delete",    isUser);
    m.addSeparator();
    m.addItem (3, "Init patch");
    m.addSeparator();
    m.addItem (4, "Import bank...");
    m.addItem (5, "Export bank...");
    if (isStandalone)
    {
        m.addSeparator();
        m.addItem (6, "Audio/MIDI Settings...");
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&dotsBtn),
        [this] (int r)
        {
            switch (r)
            {
                case 1: renamePresetDialog(); break;
                case 2: deletePresetDialog(); break;
                case 3: proc.presets.initPatch(); break;
                case 4: importBankDialog(); break;
                case 5: exportBankDialog(); break;
                case 6: showAudioSettings(); break;
                default: break;
            }
        });
}

void UBEditor::importBankDialog()
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
}

void UBEditor::exportBankDialog()
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
}

void UBEditor::showAudioSettings()
{
   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        holder->showAudioSettingsDialog();
   #endif
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

    // reflow zones: subtle azur frame + label bundling related panels
    for (auto& zf : zoneFrames)
    {
        auto rf = zf.second.toFloat();
        g.setColour (UBColours::accent.withAlpha (0.035f));
        g.fillRoundedRectangle (rf, 11.0f);
        g.setColour (UBColours::accent.withAlpha (0.22f));
        g.drawRoundedRectangle (rf.reduced (0.5f), 11.0f, 1.0f);
        g.setColour (UBColours::accent);
        g.setFont (juce::Font (juce::FontOptions (11.0f)).boldened());
        g.drawText (zf.first, zf.second.getX() + 13, zf.second.getY() + 3,
                    zf.second.getWidth() - 22, 14, juce::Justification::centredLeft);
    }
}

void UBEditor::resized()
{
    browser.setBounds (getLocalBounds());   // full-window overlay

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
        fb.items.add (item (prevBtn,     0.0f, 32));
        fb.items.add (item (presetField, 1.0f, 220));
        fb.items.add (item (nextBtn,     0.0f, 32));
        fb.items.add (item (saveBtn,     0.0f, 64));
        fb.items.add (item (dotsBtn,     0.0f, 40));
        fb.items.add (item (learnLabel,  0.7f, 120));   // empty when no learn is armed
        fb.performLayout (bar);
    }

    area.removeFromTop (8);

    const int gap = 8;
    const int bottomH = 170;                    // ARP & GROOVE band (ARP is now 2 rows)
    auto bottomRow = area.removeFromBottom (bottomH);
    area.removeFromBottom (gap);
    const int fxH = 166;                        // FX (Delay/Reverb/Scope) — knobs on one row each
    auto row3 = area.removeFromBottom (fxH);
    area.removeFromBottom (gap);
    const int row1H = 282;                      // oscillators row (FILTER's two knob rows are tallest)
    auto row1 = area.removeFromTop (row1H);
    area.removeFromTop (gap);
    auto row2 = area;                           // VOICE & GLOBAL row takes the remainder

    struct Zone { juce::String title; float weight; std::vector<std::pair<Section*, float>> panels; };
    zoneFrames.clear();
    const int zoneHdr = 16;

    auto layoutZones = [&] (juce::Rectangle<int> row, const std::vector<Zone>& zones)
    {
        float tw = 0.0f;
        for (auto& z : zones) tw += z.weight;
        const int zgap = 10;
        const int avail = row.getWidth() - zgap * juce::jmax (0, (int) zones.size() - 1);
        int x = row.getX();
        for (size_t zi = 0; zi < zones.size(); ++zi)
        {
            const auto& z = zones[zi];
            const int zw = (zi + 1 == zones.size()) ? (row.getRight() - x)
                                                    : (int) std::round (avail * (z.weight / tw));
            juce::Rectangle<int> zr (x, row.getY(), zw, row.getHeight());
            zoneFrames.push_back ({ z.title, zr });

            auto inner = zr;
            inner.removeFromTop (zoneHdr);
            inner = inner.reduced (5, 3);
            juce::FlexBox fb;
            fb.flexDirection = juce::FlexBox::Direction::row;
            for (auto& p : z.panels)
                fb.items.add (juce::FlexItem (*p.first).withFlex (p.second)
                                  .withMargin (juce::FlexItem::Margin (0, 3, 0, 3)));
            fb.performLayout (inner);
            x += zw + zgap;
        }
    };

    layoutZones (row1, { { "OSCILLATORS",  2.8f, { { &secDco1, 1.0f }, { &secDco2, 1.8f } } },
                         { "FILTER & MOD", 2.0f, { { &secFilter, 1.2f }, { &secLfo, 0.8f } } } });
    layoutZones (row2, { { "ENVELOPES",      2.65f, { { &secAmp, 0.85f }, { &secFenv, 0.85f }, { &secPenv, 0.95f } } },
                         { "VOICE & GLOBAL", 2.40f, { { &secUni, 0.62f }, { &secChord, 0.72f }, { &secGlobal, 1.05f } } } });
    layoutZones (row3, { { "FX", 1.0f, { { &secDelay, 1.25f }, { &secReverb, 0.9f }, { &secScope, 0.72f } } } });
    layoutZones (bottomRow, { { "ARP & GROOVE", 1.0f,
                               { { &secArp, 1.2f }, { &secGrooveCtl, 0.85f }, { &secGroove, 2.0f } } } });
}
