// Developer tool: renders NF Delay 42's editor to a PNG.  Usage: NFDelay42Snapshot out.png [width] [tap] [clockMode 0/1] [peakSignalDb]
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    NFDelay42AudioProcessor proc;
    auto arg = [&] (int i, float def) { return argc > i ? (float) std::atof (argv[i]) : def; };
    auto setP = [&] (const char* id, float v) { auto* q = proc.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    setP ("tap", arg (3, 80)); setP ("setmode", arg (4, 0)); setP ("depth", 3.0f); setP ("feedback", 4.5f);
    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi;
    const float amp = std::pow (10.0f, arg (5, -8.0f) / 20.0f);
    for (int b = 0; b < 100; ++b)
    {
        for (int i = 0; i < 512; ++i) { const float v = amp * std::sin (6.2831853f * 220.0f * (float) (b * 512 + i) / 48000.0f); buf.setSample (0, i, v); buf.setSample (1, i, v); }
        proc.processBlock (buf, midi);
    }
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setResizable (false, false);
    const int w = (int) arg (2, 1700);
    ed->setSize (w, juce::roundToInt (w * 152.0f / 1700.0f));
    for (int i = 0; i < 40; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File out (argc > 1 ? argv[1] : "snapshot.png");
    out.deleteFile(); juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os);
    return 0;
}
