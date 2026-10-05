// NF Delay 42 -- drawing helpers and the small controls of the front panel.
// Layers (house rule): the chassis (plate, blue sections, printed labels, tick marks) is a static image; every control is its own
// component drawn on top; the knob pointer, LEDs and display digits are drawn live from the parameter values.
#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace nfd42ui
{
using juce::Colour; using juce::Graphics; using juce::Rectangle;

// Colours measured on the reference photo of the hardware's panel
const Colour kPlateTop  { 0xff5b576d }, kPlateBot { 0xff45424f }, kBlue { 0xff0a74cc }, kBlueLight { 0xff1a86de }, kBlueDeep { 0xff0b6cc0 };
const Colour kDispDark  { 0xff5d0528 }, kDispGlass { 0xff720024 }, kDigitOn { 0xffff3a2e }, kDigitOff { 0xff7c0a2b };
const Colour kRedCap    { 0xfff8473e }, kPowerRed { 0xffff411c }, kWhiteCap { 0xffeeedf2 }, kButton { 0xff303146 }, kLabel { 0xfff2f1f7 };

inline juce::Font F (float h, bool bold = true, bool italic = false)
{
    int st = (bold ? juce::Font::bold : 0) | (italic ? juce::Font::italic : 0);
    return juce::Font (juce::FontOptions ("Helvetica Neue", h, st));
}

inline void text (Graphics& g, const juce::String& s, float cx, float cy, float h, Colour c = kLabel, bool bold = true, bool italic = false,
                  juce::Justification j = juce::Justification::centred)
{
    g.setColour (c); g.setFont (F (h, bold, italic));
    const float w = 260.0f;
    float x = cx - w * 0.5f;
    if (j == juce::Justification::centredLeft) x = cx; else if (j == juce::Justification::centredRight) x = cx - w;
    g.drawText (s, juce::Rectangle<float> (x, cy - h, w, 2 * h), j, false);
}

// ---------------------------------------------------------------------------------------------------- knobs
inline void drawKnobBody (Graphics& g, Rectangle<float> r, bool redCap)
{
    const auto c = r.getCentre(); const float R = r.getWidth() * 0.5f;
    // black rubber base with ridged skirt
    g.setColour (Colour (0xff0e0e12)); g.fillEllipse (r);
    for (int i = 0; i < 36; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 36.0f, s = std::sin (a), co = std::cos (a);
        g.setColour (Colour (0xff2c2c35)); g.drawLine (c.x + s * R * 0.80f, c.y - co * R * 0.80f, c.x + s * R * 0.985f, c.y - co * R * 0.985f, R * 0.045f);
    }
    g.setColour (Colour (0xff3b3b46)); g.drawEllipse (r.reduced (R * 0.012f), R * 0.03f);
    g.setGradientFill (juce::ColourGradient (Colour (0xff2b2b33), c.x - R * 0.4f, c.y - R * 0.6f, Colour (0xff09090c), c.x + R * 0.5f, c.y + R * 0.7f, false));
    g.fillEllipse (Rectangle<float> (R * 1.58f, R * 1.58f).withCentre (c));
    // cap
    const float cr = R * (redCap ? 0.66f : 0.72f); auto cap = Rectangle<float> (cr * 2, cr * 2).withCentre (c);
    g.setColour (Colour (0x66000000)); g.fillEllipse (cap.translated (0.0f, R * 0.04f).expanded (R * 0.02f));
    if (redCap) g.setGradientFill (juce::ColourGradient (Colour (0xffff7766), c.x - cr * 0.4f, c.y - cr * 0.55f, Colour (0xffd8281c), c.x + cr * 0.6f, c.y + cr * 0.8f, true));
    else        g.setGradientFill (juce::ColourGradient (Colour (0xfffefeff), c.x - cr * 0.4f, c.y - cr * 0.55f, Colour (0xffc9c7d3), c.x + cr * 0.6f, c.y + cr * 0.8f, true));
    g.fillEllipse (cap);
    g.setColour (Colour (redCap ? 0x55a01008 : 0x55808090)); g.drawEllipse (cap.reduced (0.5f), R * 0.03f);
}

class Knob : public juce::Slider
{
public:
    Knob (bool redCapIn, double defaultValue, bool snapMiddle = false) : redCap (redCapIn), snapMid (snapMiddle)
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (-2.35619f, 2.35619f, true);
        setMouseDragSensitivity (190);
        setDoubleClickReturnValue (true, defaultValue);
        setPopupDisplayEnabled (true, true, nullptr);
    }
    double snapValue (double v, DragMode) override { return (snapMid && std::abs (v - 5.0) < 0.22) ? 5.0 : v; }

    void paint (Graphics& g) override
    {
        const int px = juce::jmax (8, juce::roundToInt (getWidth() * 2.0f));
        if (! body.isValid() || body.getWidth() != px)
        {
            body = juce::Image (juce::Image::ARGB, px, px, true);
            Graphics bg (body); drawKnobBody (bg, Rectangle<float> ((float) px, (float) px), redCap);
        }
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (body, getLocalBounds().toFloat());

        const auto r = getLocalBounds().toFloat(); const auto c = r.getCentre(); const float R = r.getWidth() * 0.5f, cr = R * (redCap ? 0.66f : 0.72f);
        const float a = juce::jmap ((float) valueToProportionOfLength (getValue()), -2.35619f, 2.35619f);
        const float s = std::sin (a), co = std::cos (a);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.drawLine (c.x + s * cr * 0.12f + 0.8f, c.y - co * cr * 0.12f + 1.0f, c.x + s * cr * 0.96f + 0.8f, c.y - co * cr * 0.96f + 1.0f, R * 0.09f);
        g.setColour (redCap ? Colour (0xfffff4ee) : Colour (0xff15151c));
        g.drawLine (c.x + s * cr * 0.12f, c.y - co * cr * 0.12f, c.x + s * cr * 0.96f, c.y - co * cr * 0.96f, R * 0.085f);
        if (hasKeyboardFocus (false)) { g.setColour (juce::Colours::white.withAlpha (0.4f)); g.drawEllipse (r.reduced (1), 1.0f); }
    }
    void resized() override { body = {}; }

private:
    bool redCap, snapMid; juce::Image body;
};

// ---------------------------------------------------------------------------------------------------- push buttons
class RoundButton : public juce::Button
{
public:
    enum Kind { black, power, knobLike };
    explicit RoundButton (Kind k = black) : juce::Button (""), kind (k) { setClickingTogglesState (true); }
    void paintButton (Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.5f); const auto c = r.getCentre(); const float R = r.getWidth() * 0.5f;
        const bool on = getToggleState(), inset = on || down;
        if (kind == power)
        {
            g.setColour (Colour (0xff1a1a20)); g.fillEllipse (r);
            g.setColour (Colour (0xff55555f)); g.drawEllipse (r, 1.4f);
            auto cap = r.reduced (R * 0.16f);
            const Colour top = on ? Colour (0xffff6a3a) : Colour (0xff8a2a14), bot = on ? Colour (0xffe03210) : Colour (0xff5a1a0c);
            g.setGradientFill (juce::ColourGradient (top, c.x - R * 0.3f, c.y - R * 0.5f, bot, c.x + R * 0.4f, c.y + R * 0.8f, true));
            g.fillEllipse (cap);
            if (on) { g.setColour (Colour (0x55ff5a30)); g.drawEllipse (r.expanded (2.0f), 2.0f); }
            return;
        }
        g.setColour (Colour (0xff0c0c10)); g.fillEllipse (r);
        g.setColour (kind == knobLike ? Colour (0xffc8c8d2) : Colour (0xff4a4b60)); g.drawEllipse (r, kind == knobLike ? R * 0.1f : 1.6f);
        auto face = r.reduced (R * (kind == knobLike ? 0.22f : 0.14f));
        g.setGradientFill (juce::ColourGradient (inset ? Colour (0xff17171e) : Colour (0xff3c3d52), c.x - R * 0.3f, c.y - R * 0.6f, Colour (0xff111116), c.x + R * 0.3f, c.y + R * 0.8f, true));
        g.fillEllipse (face);
        if (on) { g.setColour (Colour (0xff59b8ff).withAlpha (0.85f)); g.drawEllipse (r.reduced (R * 0.06f), 1.6f); }
        else if (over) { g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawEllipse (face, 1.0f); }
    }
private:
    Kind kind;
};

class SquareStepButton : public juce::Button
{
public:
    explicit SquareStepButton (bool upIn) : juce::Button (""), up (upIn) {}
    std::function<void()> onDownEdge, onUpEdge;
    void mouseDown (const juce::MouseEvent& e) override { juce::Button::mouseDown (e); if (onDownEdge) onDownEdge(); }
    void mouseUp (const juce::MouseEvent& e) override { juce::Button::mouseUp (e); if (onUpEdge) onUpEdge(); }
    void paintButton (Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Colour (0xff0b0b0f)); g.fillRoundedRectangle (r, r.getWidth() * 0.08f);
        auto in = r.reduced (r.getWidth() * 0.09f);
        g.setGradientFill (juce::ColourGradient (down ? Colour (0xff1a1a21) : Colour (0xff3a3b4d), in.getX(), in.getY(), Colour (0xff17171d), in.getRight(), in.getBottom(), false));
        g.fillRoundedRectangle (in, r.getWidth() * 0.07f);
        g.setColour (Colour (0xff0b0b0f)); g.fillEllipse (in.reduced (in.getWidth() * 0.2f));
        g.setColour (Colour (0xff23232b)); g.drawEllipse (in.reduced (in.getWidth() * 0.2f), 1.0f);
        const auto c = in.getCentre(); const float h = in.getHeight() * 0.2f;
        juce::Path p; if (up) { p.addTriangle (c.x, c.y - h, c.x - h * 0.42f, c.y - h * 0.15f, c.x + h * 0.42f, c.y - h * 0.15f); }
        else { p.addTriangle (c.x, c.y + h, c.x - h * 0.42f, c.y + h * 0.15f, c.x + h * 0.42f, c.y + h * 0.15f); }
        g.setColour (juce::Colours::white.withAlpha (over ? 1.0f : 0.9f));
        g.fillPath (p); g.drawLine (c.x, c.y - h * 0.15f, c.x, c.y + h * 0.15f, h * 0.25f);
        g.drawLine (c.x, up ? c.y - h * 0.1f : c.y + h * 0.1f, c.x, up ? c.y + h : c.y - h, h * 0.22f);
    }
private:
    bool up;
};

class SlideSwitch : public juce::Button
{
public:
    SlideSwitch() : juce::Button ("") { setClickingTogglesState (true); }
    void paintButton (Graphics& g, bool, bool) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Colour (0xff0b0b10)); g.fillRoundedRectangle (r, 2.0f);
        const float w = r.getWidth() * 0.5f; auto nub = Rectangle<float> (getToggleState() ? r.getRight() - w - 1.5f : r.getX() + 1.5f, r.getY() + 1.5f, w, r.getHeight() - 3.0f);
        g.setGradientFill (juce::ColourGradient (Colour (0xff4a4b5c), nub.getX(), nub.getY(), Colour (0xff22222b), nub.getX(), nub.getBottom(), false));
        g.fillRoundedRectangle (nub, 1.5f);
        g.setColour (Colour (0xff15151c));
        for (int i = 1; i < 6; ++i) g.drawLine (nub.getX() + nub.getWidth() * i / 6.0f, nub.getY() + 1, nub.getX() + nub.getWidth() * i / 6.0f, nub.getBottom() - 1, 0.8f);
    }
};

// ---------------------------------------------------------------------------------------------------- seven-segment digit
inline void drawDigit (Graphics& g, Rectangle<float> r, int ch, Colour on, Colour off)
{
    // ch: 0..9, -1 = blank
    static const int seg[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f };   // bits a b c d e f g
    const int bits = (ch >= 0 && ch <= 9) ? seg[ch] : 0;
    const float t = r.getWidth() * 0.17f, w = r.getWidth(), h = r.getHeight(), x = r.getX(), y = r.getY(), q = t * 0.5f;
    auto horiz = [&] (float cy) { juce::Path p; p.startNewSubPath (x + q, cy); p.lineTo (x + q + t * 0.7f, cy - q); p.lineTo (x + w - q - t * 0.7f, cy - q); p.lineTo (x + w - q, cy);
                                  p.lineTo (x + w - q - t * 0.7f, cy + q); p.lineTo (x + q + t * 0.7f, cy + q); p.closeSubPath(); return p; };
    auto vert  = [&] (float cx, float y0, float y1) { juce::Path p; p.startNewSubPath (cx, y0 + q); p.lineTo (cx + q, y0 + q + t * 0.7f); p.lineTo (cx + q, y1 - q - t * 0.7f); p.lineTo (cx, y1 - q);
                                  p.lineTo (cx - q, y1 - q - t * 0.7f); p.lineTo (cx - q, y0 + q + t * 0.7f); p.closeSubPath(); return p; };
    const float xl = x + q, xr = x + w - q, ym = y + h * 0.5f, yt = y + q, yb = y + h - q;
    juce::Path s[7] = { horiz (yt), vert (xr, yt, ym), vert (xr, ym, yb), horiz (yb), vert (xl, ym, yb), vert (xl, yt, ym), horiz (ym) };
    const auto shear = juce::AffineTransform::shear (-0.10f, 0.0f).translated (0.10f * (y + h * 0.5f), 0.0f);
    for (int i = 0; i < 7; ++i)
    {
        const bool lit = (bits >> i) & 1;
        if (lit) { g.setColour (on.withAlpha (0.28f)); g.strokePath (s[i], juce::PathStrokeType (t * 0.9f), shear); }
        g.setColour (lit ? on : off); g.fillPath (s[i], shear);
    }
}

// ---------------------------------------------------------------------------------------------------- preset tab:  [<] name [>]   (157 x 21)
class PresetTab : public juce::Component
{
public:
    std::function<void()> onPrev, onNext, onName;
    juce::String name { "Default" };
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Colour (0xff25242d)); g.fillRoundedRectangle (r, r.getHeight() * 0.3f);
        g.setColour (Colour (0xff6c6a7d)); g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.3f, 1.0f);
        const float aw = r.getHeight() * 1.05f;
        g.setColour (kLabel.withAlpha (0.9f)); const auto c1 = juce::Point<float> (aw * 0.5f, r.getCentreY()), c2 = juce::Point<float> (r.getRight() - aw * 0.5f, r.getCentreY());
        const float h = r.getHeight() * 0.2f;
        g.fillPath ([&] { juce::Path p; p.addTriangle (c1.x - h * 0.6f, c1.y, c1.x + h * 0.5f, c1.y - h, c1.x + h * 0.5f, c1.y + h); return p; }());
        g.fillPath ([&] { juce::Path p; p.addTriangle (c2.x + h * 0.6f, c2.y, c2.x - h * 0.5f, c2.y - h, c2.x - h * 0.5f, c2.y + h); return p; }());
        g.setFont (F (r.getHeight() * 0.52f, true)); g.setColour (kLabel);
        g.drawText (name, juce::Rectangle<float> (aw, 0, r.getWidth() - 2 * aw, r.getHeight()), juce::Justification::centred, true);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const float aw = getHeight() * 1.05f;
        if (e.x < aw) { if (onPrev) onPrev(); } else if (e.x > getWidth() - aw) { if (onNext) onNext(); } else if (onName) onName();
    }
};

class MenuButton : public juce::Button
{
public:
    MenuButton() : juce::Button ("") {}
    void paintButton (Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Colour (0xff25242d)); g.fillRoundedRectangle (r, r.getHeight() * 0.3f);
        g.setColour (Colour (0xff6c6a7d)); g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.3f, 1.0f);
        g.setColour (kLabel.withAlpha (down ? 0.6f : over ? 1.0f : 0.85f));
        for (int i = 0; i < 3; ++i) g.fillRoundedRectangle (r.getWidth() * 0.25f, r.getHeight() * (0.3f + 0.2f * i), r.getWidth() * 0.5f, r.getHeight() * 0.075f, 1.0f);
    }
};
} // namespace nfd42ui
