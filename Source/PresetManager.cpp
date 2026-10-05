#include "PresetManager.h"

namespace nfd42
{
namespace
{
constexpr const char* kSignature = "NFDelay42Preset";
constexpr int kFormatVersion = 1;
constexpr juce::int64 kMaxFileBytes = 1 * 1024 * 1024;

struct Factory
{
    const char* name;
    float level, feedback; bool hicut, fbinv, dlyinv; float outmix; bool x2; int tap; float manual, depth, wave, rate; bool bypassOff;
};
// tap * 3.125 ms at MANUAL X1 in the short range (x2 doubles it). rate: 0..10 -> 0.1..10 Hz (exponential).
const Factory kFactory[] = {
    { "Slapback",            7.5f, 1.0f, true,  false, false, 3.5f, false, 32, 5.0f, 0.0f, 0.0f, 4.0f, true },
    { "Tape-style Echo",     7.5f, 4.2f, true,  false, false, 3.5f, false, 96, 5.0f, 0.4f, 0.0f, 3.0f, true },
    { "Long Echo",           7.5f, 5.0f, true,  false, false, 3.2f, true,  120, 5.0f, 0.0f, 0.0f, 4.0f, true },
    { "Double Tracking",     7.5f, 0.0f, false, false, false, 5.0f, false, 9,  5.0f, 1.2f, 0.0f, 3.2f, true },
    { "Chorus",              7.5f, 0.5f, true,  false, false, 5.0f, false, 8,  5.0f, 3.5f, 0.0f, 3.6f, true },
    { "Flanger",             7.5f, 5.0f, false, false, false, 5.0f, false, 2,  5.0f, 10.0f, 0.0f, 2.2f, true },
    { "Resonant Flange",     7.5f, 8.2f, true,  true,  false, 5.0f, false, 2,  5.0f, 10.0f, 0.0f, 1.8f, true },
    { "Through-Zero Style",  7.5f, 4.0f, false, true,  true,  5.0f, false, 3,  5.0f, 10.0f, 0.0f, 2.0f, true },
    { "Vibrato",             7.5f, 0.0f, false, false, false, 10.0f, false, 6, 5.0f, 3.0f, 0.0f, 6.4f, true },
    { "Pitch Twist",         7.5f, 0.0f, false, false, false, 10.0f, false, 80, 5.0f, 6.0f, 10.0f, 3.0f, true },
    { "Envelope Wobble",     7.5f, 3.0f, true,  false, false, 5.0f, false, 12, 5.0f, 6.0f, 5.0f, 4.0f, true },
    { "Looper (1/2 clock)",  7.5f, 0.0f, false, false, false, 5.0f, false, 128, 5.0f, 0.0f, 0.0f, 4.0f, true },
};
constexpr int kNumFactory = (int) (sizeof (kFactory) / sizeof (kFactory[0]));

void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float value)
{
    if (auto* p = apvts.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (value)); p->endChangeGesture(); }
}
}

juce::File PresetManager::getPresetsDirectory()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("NF Audio Tools").getChildFile ("NF D-42").getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

juce::String PresetManager::getCurrentPresetName (juce::AudioProcessorValueTreeState& apvts) { return apvts.state.getProperty ("presetName", "Default").toString(); }

std::vector<PresetEntry> PresetManager::listAll()
{
    std::vector<PresetEntry> out;
    out.push_back ({ "Default", -2, {} });                           // every control at its starting position
    for (int i = 0; i < kNumFactory; ++i) out.push_back ({ kFactory[i].name, i, {} });
    auto files = getPresetsDirectory().findChildFiles (juce::File::findFiles, false, "*.xml");
    files.sort();
    for (auto& f : files) out.push_back ({ f.getFileNameWithoutExtension(), -1, f });
    return out;
}

void PresetManager::restoreDefault (juce::AudioProcessorValueTreeState& apvts)
{
    apvts.state.setProperty ("presetName", "Default", nullptr);
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->paramID != "setmode") { rp->beginChangeGesture(); rp->setValueNotifyingHost (rp->getDefaultValue()); rp->endChangeGesture(); }
}

void PresetManager::applyEntry (juce::AudioProcessorValueTreeState& apvts, const PresetEntry& e)
{
    if (e.factoryIndex == -2) { restoreDefault (apvts); return; }
    if (e.factoryIndex >= 0 && e.factoryIndex < kNumFactory)
    {
        const auto& f = kFactory[e.factoryIndex];
        restoreDefault (apvts);
        apvts.state.setProperty ("presetName", f.name, nullptr);
        setParam (apvts, "level", f.level); setParam (apvts, "feedback", f.feedback);
        setParam (apvts, "hicut", f.hicut); setParam (apvts, "fbinv", f.fbinv); setParam (apvts, "dlyinv", f.dlyinv);
        setParam (apvts, "outmix", f.outmix); setParam (apvts, "delayx2", f.x2); setParam (apvts, "tap", (float) f.tap);
        setParam (apvts, "manual", f.manual); setParam (apvts, "depth", f.depth); setParam (apvts, "waveform", f.wave); setParam (apvts, "rate", f.rate);
        return;
    }
    loadPreset (apvts, e.file);
}

juce::Result PresetManager::savePreset (juce::AudioProcessorValueTreeState& apvts, const juce::File& file)
{
    apvts.state.setProperty ("presetName", file.getFileNameWithoutExtension(), nullptr);
    auto xml = apvts.copyState().createXml();
    if (xml == nullptr) return juce::Result::fail ("Could not serialise the current state.");
    xml->setAttribute ("nfdPresetSignature", kSignature); xml->setAttribute ("nfdPresetFormatVersion", kFormatVersion);
    juce::TemporaryFile temp (file);
    if (! xml->writeTo (temp.getFile())) return juce::Result::fail ("Could not write the preset file.");
    if (! temp.overwriteTargetFileWithTemporary()) return juce::Result::fail ("Could not finalise the preset file.");
    return juce::Result::ok();
}

juce::Result PresetManager::loadPreset (juce::AudioProcessorValueTreeState& apvts, const juce::File& file)
{
    if (! file.existsAsFile() || file.getSize() <= 0 || file.getSize() > kMaxFileBytes) return juce::Result::fail ("Invalid NF D-42 preset");
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || xml->getStringAttribute ("nfdPresetSignature") != kSignature || xml->getIntAttribute ("nfdPresetFormatVersion", -1) <= 0
        || ! xml->hasTagName (apvts.state.getType()))
        return juce::Result::fail ("Invalid NF D-42 preset");
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    if (auto* p = apvts.getParameter ("inf")) p->setValueNotifyingHost (0.0f);
    return juce::Result::ok();
}
}
