#include "PluginEditor.h"
#include "License/NFUpdateCheck.h"
#include "NFDelay42BinaryData.h"
#include "ManualManager.h"

using namespace nfd42ui;
using juce::Rectangle; using juce::Graphics;

// ------------------------------------------------------------------------------------------ static chassis
void NFDelay42Editor::drawChassis (Graphics& g, float pixelWidth)
{
    const float sc = pixelWidth / kW;
    g.addTransform (juce::AffineTransform::translation (-kOx, -kOy).scaled (sc));
    const float L = 22, T = 40, W = 1700, H = 152;

    // plate
    g.setGradientFill (juce::ColourGradient (kPlateTop, 0, T, kPlateBot, 0, T + H, false));
    g.fillRoundedRectangle (L, T, W, H, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawLine (L + 4, T + 1.0f, L + W - 4, T + 1.0f, 1.2f);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawLine (L + 4, T + H - 1.0f, L + W - 4, T + H - 1.0f, 1.2f);

    // rack ears: slots and screws
    for (float x : { 52.0f, 1692.0f })
        for (float y : { 60.0f, 169.0f })
        {
          g.setColour (Colour (0xfff2eff3)); g.fillRoundedRectangle (x - 17, y - 11, 34, 22, 11); g.setColour (Colour (0x66000000)); g.drawRoundedRectangle (x - 17, y - 11, 34, 22, 11, 1.0f); }
    // four identical screws, symmetric (y 78 / 150), clear of the preset tab at the top right
    for (float x : { 107.0f, 1637.0f })
        for (float y : { 78.0f, 150.0f })
        { const float r = 11.0f;
          g.setColour (Colour (0xff2a2933)); g.fillEllipse (x - r, y - r, 2 * r, 2 * r); g.setColour (Colour (0xff15151b)); g.fillEllipse (x - r * 0.4f, y - r * 0.4f, r * 0.8f, r * 0.8f);
          g.setColour (Colour (0xff6b6a78)); g.drawEllipse (x - r, y - r, 2 * r, 2 * r, 1.0f); }

    // blue sections
    auto section = [&] (float x0, float x1, Colour c) { g.setColour (c); g.fillRoundedRectangle (x0, 64, x1 - x0, 106, 16.0f);
                                                        g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawRoundedRectangle (x0 + 0.5f, 64.5f, x1 - x0 - 1, 105, 16.0f, 1.0f); };
    section (145, 702, kBlue); section (714, 1204, kBlueLight); section (1209, 1517, kBlueDeep);
    g.setColour (kPlateTop.interpolatedWith (kPlateBot, 0.45f));
    g.fillEllipse (692 - 40, 118 - 40, 80, 80); g.fillEllipse (1187 - 40, 118 - 40, 80, 80);       // dark notches around OUTPUT MIX and MANUAL

    // HEADROOM and DELAY X2 inserts
    for (auto r : { Rectangle<float> (151, 69, 63, 96), Rectangle<float> (221, 69, 62, 96) })
    { g.setColour (Colour (0xff393844)); g.fillRoundedRectangle (r, 6.0f); g.setColour (Colour (0x66000000)); g.drawRoundedRectangle (r, 6.0f, 1.0f); }

    // ---- printed labels (top row)
    auto top = [&] (const char* t, float x) { text (g, t, x, 53, 12.5f); };
    top ("DELAY X2", 248); top ("LEVEL", 332); top ("FEEDBACK", 417); top ("FILTER", 490); top ("PHASE", 571); top ("OUTPUT MIX", 693);
    top ("SET-MODE", 870); top ("MANUAL", 1185); top ("DEPTH", 1274); top ("WAVEFORM", 1363); top ("RATE", 1452);
    text (g, "DELAY-", 1022, 53, 12.5f); text (g, "ms", 1052, 53, 10.5f);
    text (g, "ENV", 1367, 65, 10.0f); text (g, "X1", 1185, 65, 10.0f, kLabel, false);
    // brackets over FEEDBACK / FILTER / PHASE
    g.setColour (kLabel); { juce::Path p; p.startNewSubPath (413, 78); p.lineTo (413, 69); p.lineTo (543, 69); p.quadraticTo (551, 69, 551, 77); g.strokePath (p, juce::PathStrokeType (1.6f)); }
    { juce::Path p; p.startNewSubPath (583, 77); p.lineTo (583, 69); p.lineTo (646, 69); p.quadraticTo (660, 69, 662, 84); g.strokePath (p, juce::PathStrokeType (1.6f)); }
    // button captions
    text (g, "HI", 491, 92, 11.0f); text (g, "CUT", 491, 104, 11.0f); text (g, "FB", 545, 92, 11.0f); text (g, "INV", 545, 104, 11.0f);
    text (g, "DLY", 598, 92, 11.0f); text (g, "INV", 598, 104, 11.0f);
    text (g, juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x9e")), 768, 96, 17.0f); text (g, "RPT", 768, 108, 11.0f);
    text (g, "BYPASS", 628, 159, 9.5f);
    // 0..10 scales and section titles
    for (float x : { 306.f, 392.f }) text (g, "0", x, 160, 9.5f);
    for (float x : { 357.f, 442.f }) text (g, "10", x, 160, 9.5f);
    text (g, "0", 1248, 160, 9.5f); text (g, "10", 1300, 160, 9.5f);
    text (g, ".1Hz", 1428, 162, 9.5f); text (g, "10", 1478, 162, 9.5f);
    text (g, "INPUT", 490, 181, 13.0f); text (g, "DELAY", 940, 181, 13.0f); text (g, "VCO-SWEEP", 1363, 181, 13.0f);
    text (g, "INPUT", 657, 178, 11.0f, kLabel, false); text (g, "DELAY", 730, 178, 11.0f, kLabel, false);
    text (g, "X.5", 1148, 178, 11.0f, kLabel, false); text (g, "X1.5", 1218, 178, 11.0f, kLabel, false);
    text (g, "DLY", 831, 72, 12.0f, Colour (0xffe8f2ff), false); text (g, "CLK", 912, 72, 12.0f, Colour (0xffe8f2ff), false);
    { g.setColour (Colour (0xffe8f2ff)); g.drawLine (851, 72, 893, 72, 1.3f); juce::Path a; a.addTriangle (849, 72, 856, 68.5f, 856, 75.5f); a.addTriangle (895, 72, 888, 68.5f, 888, 75.5f); g.fillPath (a); }
    text (g, "CLK", 1124, 101, 9.5f, Colour (0xffe8f2ff), true, false, juce::Justification::centredLeft);
    text (g, juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x9e")), 1124, 120, 13.0f, Colour (0xffe8f2ff), true, false, juce::Justification::centredLeft);
    text (g, "6kHz", 1124, 137, 9.5f, Colour (0xffe8f2ff), true, false, juce::Justification::centredLeft);
    // waveform glyphs
    { g.setColour (kLabel); juce::Path sn; sn.startNewSubPath (1331, 161); sn.cubicTo (1334, 154, 1338, 154, 1341, 161); sn.cubicTo (1344, 168, 1348, 168, 1351, 161);
      g.strokePath (sn, juce::PathStrokeType (1.6f));
      juce::Path sq; sq.startNewSubPath (1380, 167); sq.lineTo (1380, 158); sq.lineTo (1388, 158); sq.lineTo (1388, 167); sq.lineTo (1396, 167); g.strokePath (sq, juce::PathStrokeType (1.6f)); }
    // RATE vertical, HEADROOM vertical, DLY vertical
    { const char* w = "RATE"; for (int i = 0; i < 4; ++i) text (g, juce::String::charToString (w[i]), 1506, 145 + i * 0.0f - 30 + i * 10.0f + 12, 9.5f); }
    { const char* w = "HEADROOM"; for (int i = 0; i < 8; ++i) text (g, juce::String::charToString (w[i]), 199, 84 + i * 9.4f, 10.5f); }
    { const char* w = "DLY"; for (int i = 0; i < 3; ++i) text (g, juce::String::charToString (w[i]), 229, 129 + i * 12.0f, 10.5f); }
    const char* hr[] = { "0dB", "- 6", "-12", "-18", "-24" }; const float hy[] = { 80, 99, 116, 134, 151 };
    for (int i = 0; i < 5; ++i) text (g, hr[i], 173, hy[i] + 4.0f, 10.0f, kLabel, true, false, juce::Justification::centredRight);
    text (g, "X1", 237, 85, 11.5f); text (g, "X2", 237, 104, 11.5f);
    // trapezoid glyphs next to X1/X2 are drawn live (they light up)

    // ---- knob tick marks (11 dots; a bar at the centre of the knobs that have a marked middle)
    auto ticks = [&] (float cx, float cy, float R, bool bar) {
        for (int i = 0; i <= 10; ++i)
        {
            const float a = juce::degreesToRadians (-135.0f + 27.0f * i), sn = std::sin (a), co = std::cos (a);
            if (i == 5 && bar) { g.saveState(); g.addTransform (juce::AffineTransform::rotation (0.0f)); g.setColour (kLabel);
                g.fillRoundedRectangle (cx - 2.0f, cy - R - 12.0f, 4.0f, 9.0f, 1.0f); g.restoreState(); }
            else { g.setColour (kLabel); g.fillEllipse (cx + sn * (R + 9.0f) - 2.3f, cy - co * (R + 9.0f) - 2.3f, 4.6f, 4.6f); }
        } };
    ticks (332, 118, 34, false); ticks (417, 118, 34, false); ticks (692, 118, 32, true);
    ticks (1187, 119, 33, true); ticks (1275, 118, 34, true); ticks (1367, 118, 34, true); ticks (1456, 118, 34, true);
    { g.setColour (kLabel); g.fillRoundedRectangle (688, 78, 8, 3, 1); g.fillRoundedRectangle (688, 83, 8, 3, 1); }        // "=" mark over OUTPUT MIX

    // ---- brand
    g.setColour (Colour (0x55000000)); { juce::Path p; (void) p; }
    // product name: kept clear of the screws (x 107, y 78 and 150)
    text (g, "nf D-42", 42, 113, 32.0f, kLabel, true, true, juce::Justification::centredLeft);
    // brand logo in the place of the original maker's logo
    { static const juce::Image logo = juce::ImageCache::getFromMemory (NFDelay42BinaryData::nf_audio_tools_logo_png, NFDelay42BinaryData::nf_audio_tools_logo_pngSize);
      g.setImageResamplingQuality (juce::Graphics::highResamplingQuality); g.drawImage (logo, Rectangle<float> (1521.7f, 65.0f, 78.0f, 78.0f * (float) logo.getHeight() / (float) logo.getWidth())); }
    text (g, "POWER", 1547, 111, 11.5f);
    text (g, "digital", 1665, 95, 16.0f, kLabel, true, true); text (g, "delay", 1650, 111, 16.0f, kLabel, true, true); text (g, "processor", 1665, 127, 16.0f, kLabel, true, true);
}

// ------------------------------------------------------------------------------------------ editor
NFDelay42Editor::NFDelay42Editor (NFDelay42AudioProcessor& p) : AudioProcessorEditor (&p), proc (p)
   #ifdef NF_LICENSE_ENFORCE
    , licenseOverlay (p.licenseManager)
   #endif
{
    auto& v = proc.apvts;
    aLevel = std::make_unique<Slide> (v, "level", level);       aFeedback = std::make_unique<Slide> (v, "feedback", feedback);
    aOutmix = std::make_unique<Slide> (v, "outmix", outmix);   aManual = std::make_unique<Slide> (v, "manual", manual);
    aDepth = std::make_unique<Slide> (v, "depth", depth);       aWave = std::make_unique<Slide> (v, "waveform", waveform);
    aRate = std::make_unique<Slide> (v, "rate", rate);
    aHicut = std::make_unique<BtnAtt> (v, "hicut", hicut);      aFbinv = std::make_unique<BtnAtt> (v, "fbinv", fbinv);
    aDlyinv = std::make_unique<BtnAtt> (v, "dlyinv", dlyinv);   aInf = std::make_unique<BtnAtt> (v, "inf", infBtn);
    aX2 = std::make_unique<BtnAtt> (v, "delayx2", x2Btn);       aPower = std::make_unique<BtnAtt> (v, "power", power);
    aMode = std::make_unique<BtnAtt> (v, "setmode", setMode);
    for (juce::Component* c : std::initializer_list<juce::Component*> { &level, &feedback, &outmix, &manual, &depth, &waveform, &rate,
                                                                          &hicut, &fbinv, &dlyinv, &infBtn, &x2Btn, &power, &downBtn, &upBtn, &setMode, &presetTab, &menuBtn })
        addAndMakeVisible (*c);

    downBtn.onDownEdge = [this] { firstDown = downBtn.isDown(); const bool otherDown = upBtn.isDown(); if (! otherDown) { firstPressedUp = false; heldMs = 0; repeatAcc = 0; } stepOnce (false); };
    upBtn.onDownEdge   = [this] { const bool otherDown = downBtn.isDown(); if (! otherDown) { firstPressedUp = true; heldMs = 0; repeatAcc = 0; } stepOnce (true); };

    presetTab.onPrev = [this] { stepPreset (-1); };
    presetTab.onNext = [this] { stepPreset (+1); };
    presetTab.onName = [this] { showPresetMenu(); };
    menuBtn.onClick = [this] { showMainMenu(); };
    refreshPresetName();

    const int w = juce::jlimit (850, 3400, (int) proc.apvts.state.getProperty ("uiWidth", kDefaultWidth));
    setResizable (true, true);
    setResizeLimits (850, 76, 3400, 304);
    getConstrainer()->setFixedAspectRatio ((double) (kW / kH));
    setSize (w, juce::roundToInt (w * kH / kW));
   #ifdef NF_LICENSE_ENFORCE
    addChildComponent (licenseOverlay);   // last child = on top of everything
    licenseOverlay.setVisible (! proc.licenseManager.isActivated());
    licenseOverlay.onActivated = [this] { licenseOverlay.setVisible (false); };
    licenseOverlay.setLookAndFeel (&juce::LookAndFeel::getDefaultLookAndFeel());
    licenseOverlay.setBounds (getLocalBounds());
   #endif
    startTimerHz (30);
}

NFDelay42Editor::~NFDelay42Editor() { stopTimer(); }

void NFDelay42Editor::place (juce::Component& c, float cx, float cy, float w, float h)
{
    c.setBounds (juce::Rectangle<float> ((cx - kOx - w * 0.5f) * s, (cy - kOy - h * 0.5f) * s, w * s, h * s).toNearestInt());
}

void NFDelay42Editor::resized()
{
    s = (float) getWidth() / kW;
    proc.apvts.state.setProperty ("uiWidth", getWidth(), nullptr);
    chassis = juce::Image (juce::Image::ARGB, getWidth() * 2, getHeight() * 2, true);
    { Graphics g (chassis); g.setImageResamplingQuality (juce::Graphics::highResamplingQuality); drawChassis (g, (float) chassis.getWidth()); }
    place (level, 332, 118, 68, 68);    place (feedback, 417, 118, 68, 68);   place (outmix, 692, 118, 64, 64);
    place (manual, 1187, 119, 66, 66);  place (depth, 1275, 118, 68, 68);     place (waveform, 1367, 118, 68, 68);   place (rate, 1456, 118, 68, 68);
    place (hicut, 491, 137, 38, 38);    place (fbinv, 545, 137, 38, 38);      place (dlyinv, 598, 137, 38, 38);
    place (infBtn, 768, 138, 36, 36);   place (x2Btn, 257, 137, 42, 42);      place (power, 1548, 137, 42, 42);
    place (downBtn, 836, 124, 45, 45);  place (upBtn, 908, 124, 45, 45);      place (setMode, 872, 89, 30, 13);
    place (presetTab, 1562.5f, 53.5f, 157, 21); place (menuBtn, 1652.5f, 53.5f, 21, 21);
    logoRect = { 1518, 62, 84, 48 }; displayRect = { 948, 85, 165, 62 }; bypassRect = { 606, 148, 64, 18 };
   #ifdef NF_LICENSE_ENFORCE
    licenseOverlay.setBounds (getLocalBounds());
   #endif
}

void NFDelay42Editor::paint (Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (chassis, getLocalBounds().toFloat());
    g.addTransform (juce::AffineTransform::translation (-kOx, -kOy).scaled (s));
    auto& v = proc.apvts;
    auto on = [&] (const char* id) { return v.getRawParameterValue (id)->load() > 0.5f; };
    const bool powered = on ("power"), clkMode = on ("setmode");

    // HEADROOM LEDs
    const float thr[5] = { -2.0f, -6.0f, -12.0f, -18.0f, -24.0f }; const float hy[5] = { 80, 99, 116, 134, 151 };
    const Colour hc[5] = { Colour (0xffff2a2a), Colour (0xffffb020), Colour (0xff2fe05a), Colour (0xff2fe05a), Colour (0xff2fe05a) };
    auto led = [&] (float x, float y, float r, Colour c, bool lit) {
        if (lit) { g.setColour (c.withAlpha (0.25f)); g.fillEllipse (x - r * 2.0f, y - r * 2.0f, r * 4.0f, r * 4.0f); }
        g.setGradientFill (juce::ColourGradient (lit ? c.brighter (0.5f) : c.darker (0.85f), x - r * 0.4f, y - r * 0.5f, lit ? c : c.darker (0.95f), x + r, y + r, true));
        g.fillEllipse (x - r, y - r, 2 * r, 2 * r);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.drawEllipse (x - r, y - r, 2 * r, 2 * r, 1.0f); };
    for (int i = 0; i < 5; ++i) led (184.5f, hy[i], 5.6f, hc[i], powered && shownPeak > thr[i]);

    // DELAY X2: lit trapezoid for the active range
    const bool x2 = on ("delayx2");
    auto trap = [&] (float cx, float cy, float w, bool lit) { juce::Path p; p.startNewSubPath (cx - w * 0.5f, cy + 5); p.lineTo (cx - w * 0.3f, cy - 4); p.lineTo (cx + w * 0.3f, cy - 4); p.lineTo (cx + w * 0.5f, cy + 5); p.closeSubPath();
                                      g.setColour (kLabel.withAlpha (lit ? 1.0f : 0.45f)); g.strokePath (p, juce::PathStrokeType (1.5f)); };
    trap (264, 81, 24, ! x2); trap (264, 100, 15, x2);

    // display: dark red glass, 4 seven-segment digits
    g.setGradientFill (juce::ColourGradient (kDispGlass, 948, 85, kDispDark, 948, 147, false));
    g.fillRoundedRectangle (displayRect, 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (displayRect, 2.0f, 1.5f);
    int d[4] = { -1, -1, -1, -1 };
    if (powered)
    {
        if (clkMode)
        {
            const int num = 2 * getIntParam ("clknum") + 1, den = 1 << getIntParam ("clkden");
            d[0] = num; if (den >= 10) { d[2] = den / 10; d[3] = den % 10; } else d[3] = den;
        }
        else
        {
            const int ms = juce::jlimit (0, 9999, juce::roundToInt (shownDelay));
            d[3] = ms % 10; if (ms >= 10) d[2] = (ms / 10) % 10; if (ms >= 100) d[1] = (ms / 100) % 10; if (ms >= 1000) d[0] = ms / 1000;
            if (ms == 0) d[3] = 0;
        }
    }
    const float dw = 30.0f, dh = 46.0f, gap = 7.0f, x0 = displayRect.getCentreX() - (4 * dw + 3 * gap) * 0.5f, y0 = displayRect.getCentreY() - dh * 0.5f;
    for (int i = 0; i < 4; ++i) drawDigit (g, { x0 + i * (dw + gap), y0, dw, dh }, d[i], powered ? kDigitOn : Colour (0x00000000), powered ? kDigitOff : Colour (0x00000000));
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (displayRect.withHeight (displayRect.getHeight() * 0.38f), 2.0f);

    // indicator LEDs: CLK, infinity, 6 kHz, RATE, BYPASS
    const auto now = juce::Time::currentTimeMillis();
    led (1119, 97.5f, 3.0f, Colour (0xffff3a2e), powered && now < clkFlashUntil);
    led (1119, 116.5f, 3.0f, Colour (0xffff3a2e), powered && on ("inf"));
    led (1119, 133.5f, 3.0f, Colour (0xffff3a2e), powered && x2);
    led (1494, 157.5f, 4.0f, Colour (0xff3ee068), powered && proc.engine.getRateLit() && v.getRawParameterValue ("depth")->load() > 0.01f);
    led (653, 157.5f, 4.0f, Colour (0xff3ee068), powered && on ("bypass"));
}

// ------------------------------------------------------------------------------------------ interaction
int NFDelay42Editor::getIntParam (const char* id) const { return juce::roundToInt (proc.apvts.getRawParameterValue (id)->load()); }

void NFDelay42Editor::setIntParam (const char* id, int val)
{
    if (auto* p = proc.apvts.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 ((float) val)); p->endChangeGesture(); }
}

// UP / DOWN: in DLY the delay tap; in CLK the clock fraction (DOWN cycles the numerator 1,3,5,7,9; UP cycles the denominator 1..64) -- manual 2.1 K/L/M
void NFDelay42Editor::stepOnce (bool up)
{
    if (proc.apvts.getRawParameterValue ("power")->load() < 0.5f) return;
    if (proc.apvts.getRawParameterValue ("setmode")->load() > 0.5f)
    {
        if (up) setIntParam ("clkden", (getIntParam ("clkden") + 1) % 7); else setIntParam ("clknum", (getIntParam ("clknum") + 1) % 5);
    }
    else setIntParam ("tap", juce::jlimit (0, 255, getIntParam ("tap") + (up ? 1 : -1)));
}

bool NFDelay42Editor::overDisplay (juce::Point<int> p) const { return displayRect.contains (toDesign (p)); }

void NFDelay42Editor::mouseDown (const juce::MouseEvent& e)
{
    if (logoRect.contains (toDesign (e.getPosition()))) { setSize (kDefaultWidth, juce::roundToInt (kDefaultWidth * kH / kW)); return; }   // logo click: back to the default size
    if (bypassRect.contains (toDesign (e.getPosition()))) { if (auto* p = proc.apvts.getParameter ("bypass")) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); p->endChangeGesture(); } }
    dragStartTap = getIntParam ("tap");
}

// dragging the display up/down scrolls the delay tap (a plug-in convenience; the hardware only has UP / DOWN)
void NFDelay42Editor::mouseDrag (const juce::MouseEvent& e)
{
    if (! overDisplay (e.getMouseDownPosition()) || proc.apvts.getRawParameterValue ("setmode")->load() > 0.5f) return;
    setIntParam ("tap", juce::jlimit (0, 255, dragStartTap + juce::roundToInt (-e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 12.0f : 2.5f))));
}

void NFDelay42Editor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (! overDisplay (e.getPosition()) || proc.apvts.getRawParameterValue ("setmode")->load() > 0.5f) return;
    const int n = (int) std::lround (w.deltaY * (e.mods.isShiftDown() ? 40.0f : 12.0f));
    if (n != 0) setIntParam ("tap", juce::jlimit (0, 255, getIntParam ("tap") + n));
}

void NFDelay42Editor::timerCallback()
{
    // read-outs
    const float target = proc.engine.getDelayMs();
    shownDelay += 0.55f * (target - shownDelay);
    const float pk = proc.engine.getPeakDb();
    shownPeak = pk > shownPeak ? pk : shownPeak - 2.2f;      // fast attack, ~70 dB/s fall
    const uint32_t t = proc.engine.getClkTicks();
    if (t != lastTicks) { lastTicks = t; clkFlashUntil = juce::Time::currentTimeMillis() + 90; }

    // hold-to-repeat on UP / DOWN: ~1 step per second; pressing the other button as well makes it fast (manual 2.1 L/M)
    const bool d = downBtn.isDown(), u = upBtn.isDown();
    if (d || u)
    {
        heldMs += 33; repeatAcc += 33;
        if (d && u) { if (repeatAcc >= 40) { repeatAcc = 0; stepOnce (firstPressedUp); } }
        else if (repeatAcc >= 1000) { repeatAcc = 0; stepOnce (u); }
    }
    else { heldMs = 0; repeatAcc = 0; }

    repaint (juce::Rectangle<float> ((148 - kOx) * s, (60 - kOy) * s, (1530 - 148) * s, 120 * s).toNearestInt());
}

// ------------------------------------------------------------------------------------------ presets and menu
void NFDelay42Editor::refreshPresetName() { presetTab.name = nfd42::PresetManager::getCurrentPresetName (proc.apvts); presetTab.repaint(); }

void NFDelay42Editor::stepPreset (int dir)
{
    const auto all = nfd42::PresetManager::listAll();
    if (all.empty()) return;
    const auto cur = nfd42::PresetManager::getCurrentPresetName (proc.apvts);
    int idx = -1; for (int i = 0; i < (int) all.size(); ++i) if (all[(size_t) i].name == cur) { idx = i; break; }
    idx = idx < 0 ? (dir > 0 ? 0 : (int) all.size() - 1) : (idx + dir + (int) all.size()) % (int) all.size();
    nfd42::PresetManager::applyEntry (proc.apvts, all[(size_t) idx]);
    refreshPresetName();
}

void NFDelay42Editor::showPresetMenu()
{
    const auto all = nfd42::PresetManager::listAll();
    juce::PopupMenu m; int nFactory = 0; for (auto& e : all) if (e.factoryIndex >= 0) ++nFactory;
    juce::PopupMenu fac, usr;
    const auto cur = nfd42::PresetManager::getCurrentPresetName (proc.apvts);
    for (int i = 0; i < (int) all.size(); ++i)
    {
        const auto& e = all[(size_t) i];
        if (e.factoryIndex == -2) m.addItem (100 + i, "Default", true, cur == "Default");
        else (e.factoryIndex >= 0 ? fac : usr).addItem (100 + i, e.name, true, e.name == cur);
    }
    m.addSeparator(); m.addSubMenu ("Factory presets", fac);
    if ((int) all.size() > nFactory + 1) m.addSubMenu ("My presets", usr);
    m.addSeparator(); m.addItem (1, "Save preset..."); m.addItem (2, "Load preset...");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetTab), [this, all] (int r)
    {
        if (r == 0) return;
        if (r >= 100) { nfd42::PresetManager::applyEntry (proc.apvts, all[(size_t) (r - 100)]); refreshPresetName(); }
        else if (r == 1)
        {
            chooser = std::make_unique<juce::FileChooser> ("Save preset", nfd42::PresetManager::getPresetsDirectory().getChildFile ("My preset.xml"), "*.xml");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                [this] (const juce::FileChooser& fc) { auto f = fc.getResult(); if (f == juce::File()) return; nfd42::PresetManager::savePreset (proc.apvts, f.withFileExtension ("xml")); refreshPresetName(); });
        }
        else if (r == 2)
        {
            chooser = std::make_unique<juce::FileChooser> ("Load preset", nfd42::PresetManager::getPresetsDirectory(), "*.xml");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this] (const juce::FileChooser& fc) { auto f = fc.getResult(); if (f == juce::File()) return;
                    auto res = nfd42::PresetManager::loadPreset (proc.apvts, f);
                    if (res.failed()) juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "NF D-42", res.getErrorMessage());
                    refreshPresetName(); });
        }
    });
}

void NFDelay42Editor::showMainMenu()
{
    juce::PopupMenu m; m.addItem (10, "User manual (English)"); m.addItem (11, juce::String (juce::CharPointer_UTF8 ("Manual do usu\xc3\xa1rio (Portugu\xc3\xaas)"))); m.addSeparator(); m.addItem (2, "Check for updates..."); m.addItem (1, "About NF D-42");
    juce::Component::SafePointer<NFDelay42Editor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuBtn), [safe] (int r)
    {
        if (r == 2 && safe != nullptr) safe->checkForUpdates();
        if (r == 10) nfd42::ManualManager::openManual (NFDelay42BinaryData::NF_D42_Manual_English_pdf, NFDelay42BinaryData::NF_D42_Manual_English_pdfSize, "NF_D42_Manual_English.pdf");
        else if (r == 11) nfd42::ManualManager::openManual (NFDelay42BinaryData::NF_D42_Manual_Portugues_pdf, NFDelay42BinaryData::NF_D42_Manual_Portugues_pdfSize, "NF_D42_Manual_Portugues.pdf");
        else if (r == 1)
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "NF D-42",
                juce::String ("Version ") + JucePlugin_VersionString + "\nNF Audio Tools by Nenno Fernando\n\n"
                "A digital delay processor with VCO sweep, programmable clock and infinite repeat, modelled on the behaviour described "
                "in the public owner's manual of a classic rack delay.");
    });
}

// Only runs when the user clicks the menu item: one request to the NF server, off the message thread.
// The callback runs after the network round trip, so the editor may be gone by then (SafePointer).
void NFDelay42Editor::checkForUpdates()
{
    juce::Component::SafePointer<NFDelay42Editor> safe (this);
    nfupdate::checkAsync ("nf-d-42", JucePlugin_VersionString, [safe] (nfupdate::Result r)
    {
        if (safe == nullptr) return;
        const juce::String title = "NF D-42";

        if (! r.reachedServer)
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, title,
                "Could not check for updates. Please check your internet connection and try again.");
            return;
        }
        if (! r.updateAvailable)
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, title,
                juce::String ("You have the latest version (V") + JucePlugin_VersionString + ").");
            return;
        }
        const auto options = juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::InfoIcon).withTitle (title)
            .withMessage ("A new version is available: V" + r.latestVersion + " (you have V" + juce::String (JucePlugin_VersionString) + ").\n\nDownload it now?")
            .withButton ("Download").withButton ("Later");
        const auto url = r.downloadUrl;
        // AlertWindow result codes with two buttons: first = 1, second = 0.
        juce::AlertWindow::showAsync (options, [url] (int button) { if (button == 1) juce::URL (url).launchInDefaultBrowser(); });
    });
}
