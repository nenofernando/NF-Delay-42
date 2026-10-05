#pragma once
#include <JuceHeader.h>
#include "DSP/DelayEngine.h"

class NFDelay42AudioProcessor : public juce::AudioProcessor
{
public:
    NFDelay42AudioProcessor();
    ~NFDelay42AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    nfd::Params readParams() const;

    juce::AudioProcessorValueTreeState apvts;
    nfd::Engine engine;      // read-outs (HEADROOM, display, LEDs) are polled by the editor

private:
    bool wasPowered = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NFDelay42AudioProcessor)
};
