#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include <functional>
#include <vector>

// Handles factory presets, user presets (one file each) and bank import/export.
//  - User presets:  <Documents>/UB_Poly16/Presets/<name>.ubp   (XML of the APVTS state)
//  - Banks:         <name>.ubbank                               (XML holding many presets)
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& s);

    // Combined list shown in the browser: factory first, then user presets.
    juce::StringArray getFactoryNames() const;
    juce::StringArray getUserNames() const;

    void initPatch();
    void loadFactory (const juce::String& name);
    void loadUser (const juce::String& name);
    bool saveUserPreset (const juce::String& name);   // overwrites if exists
    bool deleteUserPreset (const juce::String& name);
    bool renameUserPreset (const juce::String& oldName, const juce::String& newName);
    bool isUserPreset (const juce::String& name) const;   // true if a <name>.ubp exists

    void loadByDisplayName (const juce::String& displayName); // "FACTORY: x" / "x"
    juce::StringArray getAllDisplayNames() const;
    juce::String getCurrentName() const { return currentName; }

    // Restores the DISPLAYED name only (parameter values travel in the APVTS
    // state) — used when the host reloads the session state. May be called
    // off the message thread; the editor's timer picks the change up.
    void setCurrentName (const juce::String& n) { currentName = n; }

    void loadNext();
    void loadPrevious();

    bool exportBank (const juce::File& dest) const;   // all user presets -> one .ubbank
    int  importBank (const juce::File& src);          // returns number imported

    juce::File getUserDir() const { return userDir; }

    std::function<void()> onChange;   // GUI refresh hook

    static constexpr const char* presetExt = ".ubp";
    static constexpr const char* bankExt   = ".ubbank";
    static constexpr const char* factoryPrefix = "[F] ";

private:
    struct Factory
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> values; // actual (non-normalised) values
    };

    void buildFactory();
    void applyValues (const std::vector<std::pair<juce::String, float>>& vals);
    void setCurrent (const juce::String& n);

    juce::AudioProcessorValueTreeState& apvts;
    juce::File userDir;
    juce::String currentName;
    std::vector<Factory> factory;
};
