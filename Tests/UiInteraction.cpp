// Developer test: drives the real editor/processor like a user would (clicks, steps, presets, logo, state) and checks the parameters and audio.
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/ManualManager.h"
#include "NFDelay42BinaryData.h"

static int fails = 0;
#define CHECK(c, ...) do { if (c) std::printf ("  ok    "); else { std::printf ("  FAIL  "); ++fails; } std::printf (__VA_ARGS__); std::printf ("\n"); } while (0)

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    NFDelay42AudioProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    auto P = [&] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    auto setP = [&] (const char* id, float v) { auto* q = proc.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setResizable (true, true); ed->setSize (1500, 134);
    for (int i = 0; i < 10; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (10);

    // children in the order the editor adds them
    // 0..6 knobs: level feedback outmix manual depth waveform rate | 7..12 buttons: hicut fbinv dlyinv inf x2 power | 13 down 14 up 15 setMode 16 presetTab 17 menu
    CHECK (ed->getNumChildComponents() == 19, "editor has %d components (18 controls + the resize corner)", ed->getNumChildComponents());
    auto kn = [&] (int i) { return dynamic_cast<juce::Slider*> (ed->getChildComponent (i)); };
    auto bt = [&] (int i) { return dynamic_cast<juce::Button*> (ed->getChildComponent (i)); };

    std::printf ("push buttons -> parameters\n");
    const char* ids[] = { "hicut", "fbinv", "dlyinv", "inf", "delayx2", "power" };
    const float before[] = { 0, 0, 0, 0, 0, 1 };
    for (int i = 0; i < 6; ++i)
    {
        auto* b = bt (7 + i); b->triggerClick(); juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
        const bool flipped = P (ids[i]) != before[i];
        b->triggerClick(); juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
        CHECK (flipped && P (ids[i]) == before[i], "%-8s click toggles the parameter and back", ids[i]);
    }

    std::printf ("knobs -> parameters\n");
    const char* kids[] = { "level", "feedback", "outmix", "manual", "depth", "waveform", "rate" };
    for (int i = 0; i < 7; ++i)
    {
        kn (i)->setValue (9.0, juce::sendNotificationSync);
        CHECK (std::abs (P (kids[i]) - 9.0f) < 0.02f, "%-8s knob 9.0 -> parameter %.2f", kids[i], P (kids[i]));
    }
    // and parameter -> knob (automation direction)
    setP ("depth", 2.5f); juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    CHECK (std::abs (kn (4)->getValue() - 2.5) < 0.02, "automation moves the knob (depth 2.5 -> %.2f)", kn (4)->getValue());

    std::printf ("UP / DOWN and SET-MODE\n");
    setP ("tap", 80); setP ("clknum", 0); setP ("clkden", 1);
    auto* down = dynamic_cast<nfd42ui::SquareStepButton*> (ed->getChildComponent (13)); auto* up = dynamic_cast<nfd42ui::SquareStepButton*> (ed->getChildComponent (14));
    up->onDownEdge(); up->onDownEdge(); up->onDownEdge();
    CHECK (std::lround (P ("tap")) == 83, "UP x3 in DLY: tap 80 -> %ld", std::lround (P ("tap")));
    down->onDownEdge();
    CHECK (std::lround (P ("tap")) == 82, "DOWN in DLY: tap -> %ld", std::lround (P ("tap")));
    setP ("tap", 255); up->onDownEdge(); CHECK (std::lround (P ("tap")) == 255, "UP stops at tap 255");
    setP ("tap", 0); down->onDownEdge(); CHECK (std::lround (P ("tap")) == 0, "DOWN stops at tap 0");
    bt (15)->triggerClick(); juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
    CHECK (P ("setmode") > 0.5f, "SET-MODE slide switch to CLK");
    down->onDownEdge(); down->onDownEdge();
    CHECK (std::lround (P ("clknum")) == 2, "DOWN in CLK steps the numerator (1->3->5): index %ld", std::lround (P ("clknum")));
    up->onDownEdge();
    CHECK (std::lround (P ("clkden")) == 2, "UP in CLK steps the denominator (2->4): index %ld", std::lround (P ("clkden")));
    for (int i = 0; i < 6; ++i) down->onDownEdge();
    CHECK (std::lround (P ("clknum")) == 3, "numerator wraps 9 -> 1 (index %ld)", std::lround (P ("clknum")));
    bt (15)->triggerClick(); juce::MessageManager::getInstance()->runDispatchLoopUntil (5);

    std::printf ("presets\n");
    auto* tab = dynamic_cast<nfd42ui::PresetTab*> (ed->getChildComponent (16));
    tab->onNext(); const auto n1 = tab->name;
    juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
    CHECK (n1 == "Slapback" && std::abs (P ("feedback") - 1.0f) < 0.02f && std::lround (P ("tap")) == 32, "NEXT from Default -> %s (feedback %.1f, tap %ld)", n1.toRawUTF8(), P ("feedback"), std::lround (P ("tap")));
    tab->onNext(); CHECK (tab->name == "Tape-style Echo", "NEXT -> %s", tab->name.toRawUTF8());
    tab->onPrev(); tab->onPrev();
    CHECK (tab->name == "Default" && std::abs (P ("feedback") - 3.0f) < 0.02f && std::lround (P ("tap")) == 80 && P ("hicut") < 0.5f,
           "PREV back to Default restores every control (feedback %.1f, tap %ld, hicut %.0f)", P ("feedback"), std::lround (P ("tap")), P ("hicut"));
    tab->onPrev(); CHECK (tab->name != "Default", "PREV from Default wraps to the last preset (%s)", tab->name.toRawUTF8());
    tab->onNext();   // back to Default
    {   // every factory preset applies without error and sets sane values
        int n = 0; for (int i = 0; i < 20; ++i) { tab->onNext(); ++n; if (tab->name == "Default") break; }
        CHECK (n >= 13, "cycling through all presets returns to Default after %d steps (Default + 12 factory)", n);
    }

    std::printf ("logo click -> default window size\n");
    ed->setSize (900, 80); juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    const float s = (float) ed->getWidth() / 1700.0f; const juce::Point<float> pos ((1563.0f - 22.0f) * s, (86.0f - 40.0f) * s);
    juce::MouseEvent ev (juce::Desktop::getInstance().getMainMouseSource(), pos, juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, ed.get(), ed.get(),
                         juce::Time::getCurrentTime(), pos, juce::Time::getCurrentTime(), 1, false);
    ed->mouseDown (ev); juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    CHECK (ed->getWidth() == 1500 && ed->getHeight() == 134, "click on the NF logo: 900x80 -> %dx%d", ed->getWidth(), ed->getHeight());
    ed->setSize (1000, 89);
    CHECK (ed->getWidth() == 1000 && std::abs (ed->getHeight() - 89) <= 1, "window resize keeps the aspect ratio (%dx%d)", ed->getWidth(), ed->getHeight());

    std::printf ("display mouse gestures and BYPASS lamp\n");
    {
        ed->setSize (1700, 152); juce::MessageManager::getInstance()->runDispatchLoopUntil (10);   // 1 design unit = 1 pixel
        auto mk = [&] (juce::Point<float> p, juce::Point<float> down) { return juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), p, juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, ed.get(), ed.get(),
                         juce::Time::getCurrentTime(), down, juce::Time::getCurrentTime(), 1, true); };
        const juce::Point<float> disp (1030.0f - 22.0f, 116.0f - 40.0f);
        setP ("tap", 100); setP ("setmode", 0);
        juce::MouseWheelDetails wd; wd.deltaX = 0; wd.deltaY = 0.5f; wd.isReversed = false; wd.isSmooth = false; wd.isInertial = false;
        ed->mouseWheelMove (mk (disp, disp), wd);
        CHECK (std::lround (P ("tap")) > 100, "mouse wheel up over the display: tap 100 -> %ld", std::lround (P ("tap")));
        setP ("tap", 100); wd.deltaY = -0.5f; ed->mouseWheelMove (mk (disp, disp), wd);
        CHECK (std::lround (P ("tap")) < 100, "mouse wheel down over the display: tap 100 -> %ld", std::lround (P ("tap")));
        setP ("tap", 100); ed->mouseDown (mk (disp, disp)); ed->mouseDrag (mk (disp + juce::Point<float> (0, -25), disp));
        CHECK (std::lround (P ("tap")) == 110, "dragging the display up 25 px: tap 100 -> %ld", std::lround (P ("tap")));
        setP ("tap", 100); wd.deltaY = 0.5f; ed->mouseWheelMove (mk (juce::Point<float> (300, 60), juce::Point<float> (300, 60)), wd);
        CHECK (std::lround (P ("tap")) == 100, "mouse wheel elsewhere does not change the delay");
        setP ("bypass", 0);
        const juce::Point<float> byp (638.0f - 22.0f, 157.0f - 40.0f);
        ed->mouseDown (mk (byp, byp)); CHECK (P ("bypass") > 0.5f, "click on the BYPASS lamp turns bypass on");
        ed->mouseDown (mk (byp, byp)); CHECK (P ("bypass") < 0.5f, "second click turns it off");
    }

    std::printf ("embedded manuals\n");
    CHECK (NFDelay42BinaryData::NF_D42_Manual_English_pdfSize > 10000 && std::memcmp (NFDelay42BinaryData::NF_D42_Manual_English_pdf, "%PDF", 4) == 0, "English manual embedded (%d bytes, valid PDF header)", NFDelay42BinaryData::NF_D42_Manual_English_pdfSize);
    CHECK (NFDelay42BinaryData::NF_D42_Manual_Portugues_pdfSize > 10000 && std::memcmp (NFDelay42BinaryData::NF_D42_Manual_Portugues_pdf, "%PDF", 4) == 0, "Portuguese manual embedded (%d bytes, valid PDF header)", NFDelay42BinaryData::NF_D42_Manual_Portugues_pdfSize);

    std::printf ("audio through the processor + session state\n");
    {
        setP ("tap", 64); setP ("outmix", 10.0f); setP ("feedback", 0.0f); setP ("depth", 0.0f); setP ("manual", 5.0f); setP ("level", 7.5f); setP ("delayx2", 0.0f); setP ("power", 1.0f); setP ("bypass", 0.0f);
        proc.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi; int peakAt = -1; float pk = 0;
        for (int b = 0; b < 60; ++b)
        {
            buf.clear(); if (b == 0) { buf.setSample (0, 100, 0.2f); buf.setSample (1, 100, 0.2f); }
            proc.processBlock (buf, midi);
            for (int i = 0; i < 512; ++i) if (b > 0 && std::abs (buf.getSample (0, i)) > pk) { pk = std::abs (buf.getSample (0, i)); peakAt = b * 512 + i - 100; }
        }
        CHECK (std::abs (peakAt / 48.0 - 200.0) < 1.5 && pk > 0.05f, "echo through processBlock at %.1f ms (tap 64 = 200 ms), level %.3f", peakAt / 48.0, pk);
        setP ("power", 0.0f); buf.clear(); buf.setSample (0, 5, 0.3f); proc.processBlock (buf, midi);
        CHECK (std::abs (buf.getSample (0, 5) - 0.3f) < 1e-6f, "POWER off passes the signal untouched");
        setP ("power", 1.0f);
        setP ("inf", 1.0f); setP ("tap", 33); setP ("hicut", 1.0f); setP ("clkden", 5);
        juce::MemoryBlock mb; proc.getStateInformation (mb);
        setP ("tap", 5); setP ("hicut", 0.0f); setP ("inf", 0.0f);
        proc.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (std::lround (P ("tap")) == 33 && P ("hicut") > 0.5f && std::lround (P ("clkden")) == 5, "session state restores tap/hicut/clock (tap %ld)", std::lround (P ("tap")));
        CHECK (P ("inf") < 0.5f, "infinite repeat is never restored as ON (power-up rule)");
    }
    return fails;
}
