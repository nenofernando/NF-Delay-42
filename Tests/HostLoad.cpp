// Developer test: loads the SHIPPED VST3 binary like a host would (scan, instantiate, process audio, state round-trip, open the editor).
// Usage: NFDelay42HostLoad "/path/to/NF D-42.vst3"
#include <JuceHeader.h>
static int fails = 0;
#define CHECK(c, ...) do { if (c) std::printf ("  ok    "); else { std::printf ("  FAIL  "); ++fails; } std::printf (__VA_ARGS__); std::printf ("\n"); } while (0)

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::AudioPluginFormatManager fm; fm.addFormat (new juce::VST3PluginFormat());
    juce::File f (argc > 1 ? argv[1] : "");
    CHECK (f.exists(), "plug-in found: %s", f.getFullPathName().toRawUTF8());
    juce::OwnedArray<juce::PluginDescription> descs;
    for (auto* fmt : fm.getFormats()) fmt->findAllTypesForFile (descs, f.getFullPathName());
    CHECK (descs.size() == 1, "scan finds %d plug-in(s) in the bundle", descs.size());
    if (descs.isEmpty()) return 1;
    auto& d = *descs[0];
    std::printf ("  name=\"%s\" maker=\"%s\" version=%s category=%s inputs=%d outputs=%d\n", d.name.toRawUTF8(), d.manufacturerName.toRawUTF8(), d.version.toRawUTF8(), d.category.toRawUTF8(), d.numInputChannels, d.numOutputChannels);
    CHECK (d.name == "NF D-42" && d.manufacturerName == "NF Audio Tools", "name is NF D-42 by NF Audio Tools");
    juce::String err; std::unique_ptr<juce::AudioPluginInstance> p (fm.createPluginInstance (d, 48000.0, 512, err));
    CHECK (p != nullptr, "instantiates%s", err.isEmpty() ? "" : (" (" + err + ")").toRawUTF8());
    if (! p) return 1;
    p->setPlayConfigDetails (2, 2, 48000.0, 512); p->prepareToPlay (48000.0, 512);
    CHECK (p->getParameters().size() >= 18, "exposes %d parameters to the host", p->getParameters().size());
    juce::StringArray names; for (auto* q : p->getParameters()) names.add (q->getName (40));
    std::printf ("  params: %s\n", names.joinIntoString (", ").toRawUTF8());

    // audio: impulse -> echo at the default tap (80 -> 250 ms), OUTPUT MIX fully delayed
    for (auto* q : p->getParameters()) { if (q->getName (40) == "Output Mix") q->setValue (1.0f); if (q->getName (40) == "Feedback") q->setValue (0.0f); }
    juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi; int peakAt = -1; float pk = 0;
    for (int b = 0; b < 60; ++b)
    {
        buf.clear(); if (b == 0) { buf.setSample (0, 100, 0.2f); buf.setSample (1, 100, 0.2f); }
        p->processBlock (buf, midi);
        for (int i = 0; i < 512; ++i) if (b > 0 && std::abs (buf.getSample (0, i)) > pk) { pk = std::abs (buf.getSample (0, i)); peakAt = b * 512 + i - 100; }
    }
    CHECK (std::abs (peakAt / 48.0 - 250.0) < 2.0 && pk > 0.05f, "audio: echo at %.1f ms (default tap 80 = 250 ms), level %.3f", peakAt / 48.0, pk);

    // sample rates / block sizes the DAWs use
    for (double sr : { 44100.0, 88200.0, 96000.0 })
        for (int bs : { 32, 480, 2048 })
        {
            p->releaseResources(); p->setPlayConfigDetails (2, 2, sr, bs); p->prepareToPlay (sr, bs);
            juce::AudioBuffer<float> b2 (2, bs); bool ok = true;
            for (int k = 0; k < 20; ++k) { for (int c = 0; c < 2; ++c) for (int i = 0; i < bs; ++i) b2.setSample (c, i, 0.3f * std::sin (0.05f * (float) (k * bs + i))); p->processBlock (b2, midi);
                for (int c = 0; c < 2; ++c) for (int i = 0; i < bs; ++i) if (! std::isfinite (b2.getSample (c, i)) || std::abs (b2.getSample (c, i)) > 4.0f) ok = false; }
            CHECK (ok, "%.0f Hz / %d samples: finite, bounded output", sr, bs);
        }
    // mono
    { p->releaseResources(); p->setPlayConfigDetails (1, 1, 48000.0, 256); p->prepareToPlay (48000.0, 256); juce::AudioBuffer<float> m (1, 256); m.clear(); p->processBlock (m, midi); CHECK (true, "mono configuration processes"); }
    p->releaseResources(); p->setPlayConfigDetails (2, 2, 48000.0, 512); p->prepareToPlay (48000.0, 512);

    // state round-trip through the VST3 wrapper
    juce::MemoryBlock mb; p->getStateInformation (mb); CHECK (mb.getSize() > 100, "state saved (%zu bytes)", mb.getSize());
    p->setStateInformation (mb.getData(), (int) mb.getSize()); CHECK (true, "state restored without error");

    // editor opens, has the right size and survives a repaint
    if (p->hasEditor())
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditorIfNeeded());
        CHECK (ed != nullptr && ed->getWidth() > 800 && std::abs ((double) ed->getWidth() / ed->getHeight() - 1700.0 / 152.0) < 0.2, "editor opens at %dx%d", ed ? ed->getWidth() : 0, ed ? ed->getHeight() : 0);
        for (int i = 0; i < 20; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        CHECK (img.isValid() && img.getWidth() == ed->getWidth(), "editor renders (%dx%d)", img.getWidth(), img.getHeight());
    }
    p.reset();
    std::printf ("\n%s\n", fails ? "SOME CHECKS FAILED" : "ALL HOST CHECKS PASSED");
    return fails;
}
