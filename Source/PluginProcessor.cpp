#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout NFDelay42AudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto knob = [&] (const char* id, const char* name, float def) {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 10.0f, 0.01f), def)); };
    auto sw = [&] (const char* id, const char* name, bool def, bool automatable = true) {
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def, AudioParameterBoolAttributes().withAutomatable (automatable))); };

    knob ("level", "Level", 7.5f);                 // 7.5 = unity input gain
    knob ("feedback", "Feedback", 3.0f);
    sw ("hicut", "Hi Cut", false);
    sw ("fbinv", "FB Inv", false);
    sw ("dlyinv", "Dly Inv", false);
    knob ("outmix", "Output Mix", 5.0f);           // 5 = equal blend (detent)
    sw ("delayx2", "Delay X2", false);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "tap", 1 }, "Delay Tap", 0, 255, 80));
    sw ("setmode", "Set-Mode (CLK)", false, false);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "clknum", 1 }, "Clock Numerator", 0, 4, 0));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "clkden", 1 }, "Clock Denominator", 0, 6, 1));
    knob ("manual", "Manual VCO-Sweep", 5.0f);     // 5 = X1
    knob ("depth", "Depth", 0.0f);
    knob ("waveform", "Waveform", 0.0f);           // 0 sine, 5 envelope follower, 10 square
    knob ("rate", "Rate", 4.0f);
    sw ("inf", "Infinite Repeat", false);
    sw ("bypass", "Bypass", false);
    sw ("power", "Power", true);
    return { p.begin(), p.end() };
}

NFDelay42AudioProcessor::NFDelay42AudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NFDelay42", createLayout())
{
}

bool NFDelay42AudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& in = l.getMainInputChannelSet(); const auto& out = l.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void NFDelay42AudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, juce::jmax (1, getTotalNumInputChannels()));
}

nfd::Params NFDelay42AudioProcessor::readParams() const
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    nfd::Params p;
    const float lv = v ("level");
    p.levelDb  = lv < 0.05f ? -100.0f : -60.0f + 8.0f * lv;        // 7.5 -> 0 dB, 10 -> +20 dB
    p.feedback = v ("feedback") / 10.0f;
    p.hiCut = v ("hicut") > 0.5f; p.fbInv = v ("fbinv") > 0.5f; p.dlyInv = v ("dlyinv") > 0.5f;
    p.x2 = v ("delayx2") > 0.5f; p.bypass = v ("bypass") > 0.5f; p.inf = v ("inf") > 0.5f;
    p.tap = juce::jlimit (0, 255, (int) std::lround (v ("tap")));
    p.clkNum = 2 * juce::jlimit (0, 4, (int) std::lround (v ("clknum"))) + 1;
    p.clkDen = 1 << juce::jlimit (0, 6, (int) std::lround (v ("clkden")));
    p.manual01 = v ("manual") / 10.0f; p.depth01 = v ("depth") / 10.0f; p.wave01 = v ("waveform") / 10.0f;
    p.rateHz = 0.1f * std::pow (100.0f, v ("rate") / 10.0f);       // 0.1 Hz .. 10 Hz
    p.mix = v ("outmix") / 10.0f;
    return p;
}

void NFDelay42AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
   #ifdef NF_LICENSE_ENFORCE
    // Unlicensed: whatever path below runs (even "power off, signal passes"), the buffer leaves silent.
    struct LicenseMuteGuard { NFLicenseManager& lm; juce::AudioBuffer<float>& b; ~LicenseMuteGuard() { if (! lm.isActivated()) b.clear(); } } licenseGuard { licenseManager, buffer };
   #endif
    const bool powered = apvts.getRawParameterValue ("power")->load() > 0.5f;
    if (! powered) { wasPowered = false; return; }          // unit switched off: the signal passes untouched
    if (! wasPowered) { engine.reset(); wasPowered = true; } // power-up: memory empty, repeat off

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, buffer.getNumSamples());
    const int nch = juce::jmin (buffer.getNumChannels(), (int) nfd::Engine::kMaxCh);
    engine.process (buffer.getArrayOfWritePointers(), buffer.getNumSamples(), readParams());
    (void) nch;
}

juce::AudioProcessorEditor* NFDelay42AudioProcessor::createEditor() { return new NFDelay42Editor (*this); }

void NFDelay42AudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void NFDelay42AudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            if (auto* p = apvts.getParameter ("inf")) p->setValueNotifyingHost (0.0f);   // the hardware never powers up in repeat mode
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NFDelay42AudioProcessor(); }
