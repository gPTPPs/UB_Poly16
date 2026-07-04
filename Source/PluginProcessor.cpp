#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace { constexpr int kNumVoices = 16; }

UBAudioProcessor::UBAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    for (int i = 0; i < kNumVoices; ++i)
    {
        auto* v = new UBVoice (apvts);
        v->setGlideSource (&lastNoteHz);
        synth.addVoice (v);
    }

    synth.addSound (new UBSound());
    synth.setNoteStealingEnabled (true);

    // legato: retune the voice that is playing synthNote, without retriggering
    mono.onLegatoChange = [this] (int synthNote, int newNote)
    {
        for (int i = 0; i < synth.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<UBVoice*> (synth.getVoice (i)))
                if (v->isVoiceActive() && v->getCurrentlyPlayingNote() == synthNote)
                    v->legatoChange (newNote);
    };
}

void UBAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<UBVoice*> (synth.getVoice (i)))
            v->prepare (sampleRate, samplesPerBlock);

    arp.prepare (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    chorus.prepare (spec);
    chorus.reset();

    delayFx.prepare (sampleRate, samplesPerBlock);
    reverbFx.prepare (sampleRate, samplesPerBlock);
}

bool UBAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

double UBAudioProcessor::stepSecondsFromRate (int rateIndex, double bpm) const
{
    const double beat = 60.0 / juce::jmax (1.0, bpm); // seconds per quarter note
    switch (rateIndex)
    {
        case 0: return beat;            // 1/4
        case 1: return beat * 0.5;      // 1/8
        case 2: return beat / 3.0;      // 1/8T
        case 3: return beat * 0.25;     // 1/16
        case 4: return beat / 6.0;      // 1/16T
        case 5: return beat * 0.125;    // 1/32
        default: return beat * 0.25;
    }
}

double UBAudioProcessor::delaySecondsFromNote (int noteIndex, double bpm) const
{
    const double beat = 60.0 / juce::jmax (1.0, bpm);
    switch (noteIndex)
    {
        case 0: return beat;            // 1/4
        case 1: return beat * 1.5;      // 1/4.
        case 2: return beat * 0.5;      // 1/8
        case 3: return beat * 0.75;     // 1/8.
        case 4: return beat / 3.0;      // 1/8T
        case 5: return beat * 0.25;     // 1/16
        case 6: return beat * 0.375;    // 1/16.
        case 7: return beat / 6.0;      // 1/16T
        default: return beat * 0.75;
    }
}

void UBAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // ---- tempo: follow host transport when "Sync DAW" is on, else the Tempo knob ----
    double bpm = apvts.getRawParameterValue (ID::bpm)->load();
    const bool syncToHost = apvts.getRawParameterValue (ID::arpSync)->load() > 0.5f;
    if (syncToHost)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto hostBpm = pos->getBpm())
                    bpm = *hostBpm;

    // ---- MIDI CC mappings (MIDI Learn) + chord memory ----
    ccMap.process (midi);
    chord.process (midi, apvts.getRawParameterValue (ID::chordOn)->load() > 0.5f);

    // ---- arpeggiator (MIDI domain) ----
    Arpeggiator::Params ap;
    ap.on      = apvts.getRawParameterValue (ID::arpOn)->load() > 0.5f;
    ap.mode    = (int) apvts.getRawParameterValue (ID::arpMode)->load();
    ap.octaves = (int) apvts.getRawParameterValue (ID::arpOct)->load();
    ap.gate    = apvts.getRawParameterValue (ID::arpGate)->load();
    ap.hold    = apvts.getRawParameterValue (ID::arpHold)->load() > 0.5f;
    ap.stepSeconds = stepSecondsFromRate ((int) apvts.getRawParameterValue (ID::arpRate)->load(), bpm);
    arp.process (midi, buffer.getNumSamples(), ap);

    // ---- mono / legato (one sounding note, chosen by priority) ----
    mono.process (midi,
                  (int) apvts.getRawParameterValue (ID::voiceMode)->load(),
                  (int) apvts.getRawParameterValue (ID::notePrio)->load());

    // ---- synth ----
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    // ---- chorus (Juno-style) ----
    const int chMode = (int) apvts.getRawParameterValue (ID::chorus)->load();
    if (chMode > 0)
    {
        switch (chMode)
        {
            case 1: chorus.setRate (0.6f);  chorus.setDepth (0.25f); chorus.setCentreDelay (7.0f); chorus.setFeedback (0.0f); chorus.setMix (0.5f); break; // I
            case 2: chorus.setRate (0.9f);  chorus.setDepth (0.40f); chorus.setCentreDelay (7.0f); chorus.setFeedback (0.0f); chorus.setMix (0.5f); break; // II
            case 3: chorus.setRate (1.3f);  chorus.setDepth (0.55f); chorus.setCentreDelay (8.0f); chorus.setFeedback (0.0f); chorus.setMix (0.5f); break; // I+II
            default: break;
        }

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
    }

    // ---- delay ----
    {
        UBDelay::Params dp;
        dp.on       = apvts.getRawParameterValue (ID::dlyOn)->load() > 0.5f;
        dp.pingPong = apvts.getRawParameterValue (ID::dlyPing)->load() > 0.5f;
        dp.feedback = apvts.getRawParameterValue (ID::dlyFb)->load();
        dp.toneHz   = apvts.getRawParameterValue (ID::dlyTone)->load();
        dp.mix      = apvts.getRawParameterValue (ID::dlyMix)->load();
        const bool noteSync = apvts.getRawParameterValue (ID::dlySync)->load() > 0.5f;
        dp.timeSec  = noteSync
            ? (float) delaySecondsFromNote ((int) apvts.getRawParameterValue (ID::dlyNote)->load(), bpm)
            : apvts.getRawParameterValue (ID::dlyTime)->load() * 0.001f;
        delayFx.process (buffer, dp);
    }

    // ---- reverb ----
    {
        UBReverbFx::Params rp;
        rp.on    = apvts.getRawParameterValue (ID::rvbOn)->load() > 0.5f;
        rp.size  = apvts.getRawParameterValue (ID::rvbSize)->load();
        rp.damp  = apvts.getRawParameterValue (ID::rvbDamp)->load();
        rp.width = apvts.getRawParameterValue (ID::rvbWidth)->load();
        rp.preMs = apvts.getRawParameterValue (ID::rvbPre)->load();
        rp.mix   = apvts.getRawParameterValue (ID::rvbMix)->load();
        reverbFx.process (buffer, rp);
    }

    // ---- master gain ----
    buffer.applyGain (apvts.getRawParameterValue (ID::masterGain)->load());

    // ---- feed the oscilloscope ----
    scope.push (buffer.getReadPointer (0),
                buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr,
                buffer.getNumSamples());
}

juce::AudioProcessorEditor* UBAudioProcessor::createEditor()
{
    return new UBEditor (*this);
}

void UBAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); auto xml = state.createXml())
    {
        xml->setAttribute ("chordIntervals", chord.toString());
        xml->setAttribute ("midiMap", ccMap.toString());
        copyXmlToBinary (*xml, destData);
    }
}

void UBAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            chord.fromString (xml->getStringAttribute ("chordIntervals"));
            ccMap.fromString (xml->getStringAttribute ("midiMap"));
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UBAudioProcessor();
}
