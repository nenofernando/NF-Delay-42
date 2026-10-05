#pragma once
#include <JuceHeader.h>

namespace nfd42
{
// Presets are the APVTS state (same XML as session recall) plus a signature. Factory presets are set from the table below,
// built from the application notes of the manual (echo, double tracking, flanging, resonance, vibrato, pitch twisting, looping).
struct PresetEntry { juce::String name; int factoryIndex = -1; juce::File file; };

struct PresetManager
{
    static juce::File getPresetsDirectory();
    static juce::String getCurrentPresetName (juce::AudioProcessorValueTreeState& apvts);
    static std::vector<PresetEntry> listAll();                       // factory presets first, then the user's saved ones
    static void applyEntry (juce::AudioProcessorValueTreeState& apvts, const PresetEntry& e);
    static void restoreDefault (juce::AudioProcessorValueTreeState& apvts);
    static juce::Result savePreset (juce::AudioProcessorValueTreeState& apvts, const juce::File& file);
    static juce::Result loadPreset (juce::AudioProcessorValueTreeState& apvts, const juce::File& file);
};
}
