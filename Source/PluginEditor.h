#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/PanelParts.h"
#include "PresetManager.h"

// Front panel drawn in the coordinate space of the reference photo (panel = x 22..1722, y 40..192 -> 1700 x 152).
class NFDelay42Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NFDelay42Editor (NFDelay42AudioProcessor&);
    ~NFDelay42Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static constexpr float kW = 1700.0f, kH = 152.0f, kOx = 22.0f, kOy = 40.0f;
    static void drawChassis (juce::Graphics& g, float pixelWidth);

private:
    using Slide = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BtnAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void place (juce::Component& c, float cx, float cy, float w, float h);
    void stepOnce (bool up);
    void stepPreset (int dir);
    void showPresetMenu();
    void showMainMenu();
    void refreshPresetName();
    void setIntParam (const char* id, int v);
    int  getIntParam (const char* id) const;
    juce::Point<float> toDesign (juce::Point<int> p) const { return { p.x / s + kOx, p.y / s + kOy }; }
    bool overDisplay (juce::Point<int> p) const;

    NFDelay42AudioProcessor& proc;
    float s = 1.0f;
    juce::Image chassis;

    nfd42ui::Knob level { false, 7.5 }, feedback { false, 3.0 }, outmix { false, 5.0, true }, manual { true, 5.0, true },
                  depth { true, 0.0 }, waveform { true, 0.0, true }, rate { true, 4.0 };
    nfd42ui::RoundButton hicut, fbinv, dlyinv, infBtn, x2Btn { nfd42ui::RoundButton::knobLike }, power { nfd42ui::RoundButton::power };
    nfd42ui::SquareStepButton downBtn { false }, upBtn { true };
    nfd42ui::SlideSwitch setMode;
    nfd42ui::PresetTab presetTab;
    nfd42ui::MenuButton menuBtn;
    std::unique_ptr<juce::FileChooser> chooser;

    std::unique_ptr<Slide> aLevel, aFeedback, aOutmix, aManual, aDepth, aWave, aRate;
    std::unique_ptr<BtnAtt> aHicut, aFbinv, aDlyinv, aInf, aX2, aPower, aMode;

    // live read-outs
    float shownDelay = 0.0f, shownPeak = -120.0f; uint32_t lastTicks = 0; juce::int64 clkFlashUntil = 0;
    bool firstDown = false, firstPressedUp = false; int heldMs = 0, repeatAcc = 0; int dragStartTap = 0;
    juce::Rectangle<float> displayRect, bypassRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NFDelay42Editor)
};
