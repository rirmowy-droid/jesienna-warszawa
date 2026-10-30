#include "PluginEditor.h"
using jw::U;

namespace ui {

// ======================= looks of the amp heads =======================
const AmpLook& ampLook (const String& id)
{
    static const std::vector<AmpLook> looks {
        // id        tolex                panel               panelText          knob                knobCap              grille               accent
        { "black",   Colour (0xff141414), Colour (0xff111111), Colour (0xffe8e8e8), Colour (0xffc9c9c9), Colour (0xff101010), Colour (0xff8d8f8c), Colour (0xffd8263a), false, true },
        { "tweed",   Colour (0xffc9b07a), Colour (0xff5b4632), Colour (0xfff1e4c6), Colour (0xffece0c2), Colour (0xff3a2a1c), Colour (0xff6e3328), Colour (0xffe35a2a), true,  true, true },
        { "vox",     Colour (0xff181614), Colour (0xffa86a3c), Colour (0xff1c120a), Colour (0xfff0e6d0), Colour (0xff2a1c12), Colour (0xff5e4430), Colour (0xffdb3a2a), true,  false },
        { "brit",    Colour (0xff141414), Colour (0xffc8a447), Colour (0xff16120a), Colour (0xff121212), Colour (0xffd6b55a), Colour (0xffb9ad91), Colour (0xffe0e0e0), false, true },
        { "boutique",Colour (0xff1a1715), Colour (0xffe6dcc6), Colour (0xff1c1a17), Colour (0xff151515), Colour (0xffd8d0bf), Colour (0xff3b3532), Colour (0xff3fa36a), false, true },
        { "modern",  Colour (0xff111214), Colour (0xff2a2e34), Colour (0xffd2d8de), Colour (0xff17181b), Colour (0xff4aa8ff), Colour (0xff1d1e21), Colour (0xff4aa8ff), false, false },
        { "lofi",    Colour (0xffd8ccb0), Colour (0xff6d5a44), Colour (0xfff3ead6), Colour (0xffe6dcc3), Colour (0xff6d5a44), Colour (0xffbfb294), Colour (0xffff9a3c), false, false },
    };
    for (auto& l : looks) if (l.id == id) return l;
    return looks[0];
}

static const char* builtInLooks[] = { "black", "boutique", "tweed", "vox", "brit", "modern", "lofi" };

/** Georgia on Windows/macOS; on systems without it the default serif (a missing name must never reach the text shaper). */
static String serifName()
{
    static const String name = [] { auto all = Font::findAllTypefaceNames(); for (auto* n : { "Georgia", "DejaVu Serif", "Liberation Serif" }) if (all.contains (n)) return String (n); return Font::getDefaultSerifFontName(); }();
    return name;
}

// ======================= look & feel =======================
Look::Look() { refreshColours(); }

void Look::refreshColours()
{
    setColour (ResizableWindow::backgroundColourId, col::bg);
    setColour (Label::textColourId, col::text.withAlpha (0.85f));
    setColour (ComboBox::backgroundColourId, col::panel2);
    setColour (ComboBox::outlineColourId, col::line);
    setColour (ComboBox::textColourId, col::text);
    setColour (ComboBox::arrowColourId, col::amber);
    setColour (PopupMenu::backgroundColourId, col::panel.darker (0.2f));
    setColour (PopupMenu::highlightedBackgroundColourId, col::amber.withAlpha (0.3f));
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (TextButton::buttonColourId, col::panel);
    setColour (TextButton::buttonOnColourId, col::amber);
    setColour (TextButton::textColourOffId, col::dim);
    setColour (TextButton::textColourOnId, Colour (0xff1a1410));
    setColour (BubbleComponent::backgroundColourId, col::panel.withAlpha (0.95f));
    setColour (BubbleComponent::outlineColourId, col::amber);
    setColour (TooltipWindow::textColourId, col::text);
}

static void knobShadow (Graphics& g, Rectangle<float> c)
{
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.translated (0, c.getHeight() * 0.06f));
}

void Look::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const String style = s.getProperties()["style"].toString();
    auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3);
    const float d = jmin (area.getWidth(), area.getHeight());
    auto c = area.withSizeKeepingCentre (d, d);
    const float ang = a0 + pos * (a1 - a0);
    const auto cx = c.getCentreX(), cy = c.getCentreY();
    auto pointer = [&] (float r0, float r1, float thick, Colour colr)
    {
        Path p; p.addRoundedRectangle (-thick / 2, -r1, thick, r1 - r0, thick / 2);
        g.setColour (colr); g.fillPath (p, AffineTransform::rotation (ang).translated (cx, cy));
    };

    if (style == "pedal")
    {
        auto k = c.reduced (d * 0.1f);
        knobShadow (g, k);
        g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), k.getX(), k.getY(), Colour (0xff0c0c0c), k.getRight(), k.getBottom(), false));
        g.fillEllipse (k);
        g.setColour (Colour (0xff5a5a5a)); g.drawEllipse (k, 1.0f);
        g.setColour (Colours::white.withAlpha (0.12f)); g.fillEllipse (k.reduced (k.getWidth() * 0.25f).translated (-k.getWidth() * 0.08f, -k.getWidth() * 0.1f));
        pointer (d * 0.05f, d * 0.38f, jmax (2.0f, d * 0.06f), Colours::white);
        return;
    }
    if (style.startsWith ("amp-"))
    {
        const auto& L = ampLook (style.fromFirstOccurrenceOf ("amp-", false, false));
        if (L.numbers)   // 1..10 scale printed on the panel
        {
            g.setColour (L.panelText.withAlpha (0.7f)); g.setFont (FontOptions (d * 0.13f));
            for (int i = 0; i <= 10; ++i)
            {
                const float a = a0 + (a1 - a0) * i / 10.0f, r = d * 0.5f;
                auto pt = Point<float> (cx + std::sin (a) * r, cy - std::cos (a) * r);
                if (i % 2 == 0) g.drawText (String (i == 0 ? 1 : i), Rectangle<float> (d * 0.2f, d * 0.16f).withCentre (pt), Justification::centred);
                else g.fillEllipse (Rectangle<float> (2, 2).withCentre (pt));
            }
        }
        auto k = c.reduced (d * (L.numbers ? 0.2f : 0.12f));
        knobShadow (g, k);
        if (L.chickenHead)   // pointer knob: round base + long tapered pointer
        {
            g.setGradientFill (ColourGradient (L.knob.brighter (0.2f), k.getX(), k.getY(), L.knob.darker (0.5f), k.getRight(), k.getBottom(), false));
            g.fillEllipse (k.reduced (k.getWidth() * 0.12f));
            Path p; const float r = k.getWidth() / 2;
            p.startNewSubPath (-r * 0.28f, r * 0.35f); p.lineTo (-r * 0.12f, -r * 1.0f); p.quadraticTo (0, -r * 1.12f, r * 0.12f, -r * 1.0f); p.lineTo (r * 0.28f, r * 0.35f); p.closeSubPath();
            p.addEllipse (-r * 0.45f, -r * 0.2f, r * 0.9f, r * 0.9f);
            g.setColour (L.knob); g.fillPath (p, AffineTransform::rotation (ang).translated (cx, cy));
            g.setColour (L.knobCap.withAlpha (0.8f)); g.strokePath (p, PathStrokeType (1.0f), AffineTransform::rotation (ang).translated (cx, cy));
            pointer (r * 0.35f, r * 0.95f, 2.0f, L.knobCap);
            return;
        }
        if (L.id == "black")   // skirted silver knob with black cap
        {
            g.setGradientFill (ColourGradient (Colour (0xffeeeeee), k.getX(), k.getY(), Colour (0xff7a7a7a), k.getRight(), k.getBottom(), false));
            g.fillEllipse (k);
            for (int i = 0; i < 24; ++i) { const float a = i * MathConstants<float>::twoPi / 24; g.setColour (Colours::black.withAlpha (0.12f));
                g.drawLine (cx + std::sin (a) * k.getWidth() * 0.36f, cy - std::cos (a) * k.getWidth() * 0.36f, cx + std::sin (a) * k.getWidth() * 0.5f, cy - std::cos (a) * k.getWidth() * 0.5f, 1.0f); }
            auto cap = k.reduced (k.getWidth() * 0.18f);
            g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), cap.getX(), cap.getY(), Colour (0xff050505), cap.getRight(), cap.getBottom(), false));
            g.fillEllipse (cap);
            pointer (k.getWidth() * 0.12f, k.getWidth() * 0.5f, 2.2f, Colours::white);
            return;
        }
        // plain knobs (brit / boutique / modern / lofi), optional coloured cap or LED ring
        if (L.id == "modern")
        {
            Path arc; arc.addCentredArc (cx, cy, d * 0.47f, d * 0.47f, 0, a0, ang, true);
            g.setColour (L.accent.withAlpha (0.25f)); g.strokePath (arc, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (L.accent); g.strokePath (arc, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setGradientFill (ColourGradient (L.knob.brighter (0.35f), k.getX(), k.getY(), L.knob.darker (0.6f), k.getRight(), k.getBottom(), false));
        g.fillEllipse (k);
        g.setColour (Colours::black.withAlpha (0.5f)); g.drawEllipse (k, 1.0f);
        if (L.id == "brit") { auto cap = k.reduced (k.getWidth() * 0.22f); g.setGradientFill (ColourGradient (L.knobCap.brighter (0.4f), cap.getX(), cap.getY(), L.knobCap.darker (0.4f), cap.getRight(), cap.getBottom(), false)); g.fillEllipse (cap); }
        pointer (k.getWidth() * (L.id == "brit" ? 0.3f : 0.1f), k.getWidth() * 0.5f, 2.2f, L.id == "brit" ? Colour (0xff111111) : (L.id == "lofi" ? Colour (0xff4a3a2a) : Colours::white));
        return;
    }
    // default knob, in the studio's style
    auto k = c.reduced (d * 0.14f);
    Path bgArc; bgArc.addCentredArc (cx, cy, d * 0.46f, d * 0.46f, 0, a0, a1, true);
    g.setColour (col::ink.withAlpha (0.7f)); g.strokePath (bgArc, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    Path arc; arc.addCentredArc (cx, cy, d * 0.46f, d * 0.46f, 0, a0, ang, true);
    g.setColour (deco() ? col::accent2.brighter (0.2f) : col::amber); g.strokePath (arc, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    knobShadow (g, k);
    if (deco())
    {   // black lacquer with a gold ring and fluted edge
        g.setGradientFill (ColourGradient (Colour (0xff3a3632), k.getX(), k.getY(), Colour (0xff060505), k.getRight(), k.getBottom(), false));
        g.fillEllipse (k);
        for (int i = 0; i < 20; ++i) { const float a = i * MathConstants<float>::twoPi / 20; g.setColour (col::amber.withAlpha (0.35f));
            g.drawLine (cx + std::sin (a) * k.getWidth() * 0.42f, cy - std::cos (a) * k.getWidth() * 0.42f, cx + std::sin (a) * k.getWidth() * 0.5f, cy - std::cos (a) * k.getWidth() * 0.5f, 1.0f); }
        g.setColour (col::amber); g.drawEllipse (k.reduced (k.getWidth() * 0.08f), 1.2f);
        pointer (d * 0.06f, d * 0.33f, 2.5f, col::amber.brighter (0.3f));
    }
    else
    {   // brass knob with an engraved leaf on the cap
        g.setGradientFill (ColourGradient (Colour (0xfff0d58a), k.getX(), k.getY(), Colour (0xff6e4e1e), k.getRight(), k.getBottom(), false));
        g.fillEllipse (k);
        auto cap = k.reduced (k.getWidth() * 0.16f);
        g.setGradientFill (ColourGradient (Colour (0xff3a5a34), cap.getX(), cap.getY(), Colour (0xff142312), cap.getRight(), cap.getBottom(), false));
        g.fillEllipse (cap);
        g.setColour (Colour (0xff2a1c0c).withAlpha (0.6f)); g.drawEllipse (k, 1.0f);
        Path leaf; const float L = cap.getWidth() * 0.32f;
        leaf.startNewSubPath (0, -L); leaf.quadraticTo (L * 0.45f, 0, 0, L * 0.6f); leaf.quadraticTo (-L * 0.45f, 0, 0, -L); leaf.closeSubPath();
        g.setColour (col::amber.withAlpha (0.35f)); g.fillPath (leaf, AffineTransform::translation (cx, cy));
        pointer (d * 0.06f, d * 0.35f, 2.5f, Colour (0xfff5e3b0));
    }
}

void Look::drawToggleButton (Graphics& g, ToggleButton& b, bool hl, bool down)
{
    const String style = b.getProperties()["style"].toString();
    auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    if (style == "foot")   // metal footswitch
    {
        const float d = jmin (r.getWidth(), r.getHeight()) - 4;
        auto c = r.withSizeKeepingCentre (d, d);
        g.setColour (Colours::black.withAlpha (0.4f)); g.fillEllipse (c.translated (0, 2));
        g.setGradientFill (ColourGradient (Colour (0xffe9e9e9), c.getX(), c.getY(), Colour (0xff6c6c6c), c.getRight(), c.getBottom(), false));
        g.fillEllipse (c);
        auto inner = c.reduced (d * (down ? 0.2f : 0.16f));
        g.setGradientFill (ColourGradient (Colour (0xff8a8a8a), inner.getX(), inner.getY(), Colour (0xffd8d8d8), inner.getRight(), inner.getBottom(), false));
        g.fillEllipse (inner);
        g.setColour (Colours::black.withAlpha (hl ? 0.5f : 0.3f)); g.drawEllipse (c, 1.0f);
        return;
    }
    r = r.reduced (2);
    g.setColour (on ? col::amber.withAlpha (0.9f) : col::panel2); g.fillRoundedRectangle (r, r.getHeight() / 2);
    g.setColour (hl ? col::amber : col::line); g.drawRoundedRectangle (r, r.getHeight() / 2, 1.2f);
    auto dot = r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.3f);
    g.setColour (on ? Colour (0xff1a1410) : col::dim); g.fillEllipse (dot);
    g.setColour (on ? Colour (0xff1a1410) : col::text); g.setFont (FontOptions (14.0f, Font::bold));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().withTrimmedRight (8), Justification::centredLeft, 1);
}

void Look::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    const String style = box.getProperties()["style"].toString();
    auto r = Rectangle<float> (0, 0, (float) w, (float) h);
    if (style == "plate")   // nameplate on the amp: no box, just an arrow
    {
        auto c = box.findColour (ComboBox::textColourId);
        Path p; const float ax = (float) w - 14, ay = (float) h / 2;
        p.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
        g.setColour (c.withAlpha (0.8f)); g.fillPath (p);
        return;
    }
    if (style == "lcd")
    {
        g.setColour (Colour (0xff0d1410)); g.fillRoundedRectangle (r, 4);
        g.setColour (Colour (0xff2c3a30)); g.drawRoundedRectangle (r.reduced (0.5f), 4, 1.0f);
        Path p; const float ax = (float) w - 14, ay = (float) h / 2;
        p.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
        g.setColour (Colour (0xff9df2b4).withAlpha (0.7f)); g.fillPath (p);
        return;
    }
    g.setColour (box.findColour (ComboBox::backgroundColourId)); g.fillRoundedRectangle (r, 5);
    g.setColour (box.findColour (ComboBox::outlineColourId)); g.drawRoundedRectangle (r.reduced (0.5f), 5, 1.0f);
    Path p; const float ax = (float) w - 13, ay = (float) h / 2;
    p.addTriangle (ax - 4.5f, ay - 2.5f, ax + 4.5f, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (box.findColour (ComboBox::arrowColourId)); g.fillPath (p);
}

Font Look::getComboBoxFont (ComboBox& box)
{
    const String style = box.getProperties()["style"].toString();
    if (style == "plate") return Font (FontOptions (serifName(), jmin (26.0f, box.getHeight() * 0.7f), Font::bold | Font::italic));
    if (style == "lcd") return Font (FontOptions (Font::getDefaultMonospacedFontName(), jmin (17.0f, box.getHeight() * 0.55f), Font::bold));
    return Font (FontOptions (jmin (15.0f, box.getHeight() * 0.55f)));
}

void Look::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 26, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    const String style = box.getProperties()["style"].toString();
    label.setJustificationType (style == "plate" ? Justification::centred : Justification::centredLeft);
    if (style == "lcd") label.setColour (Label::textColourId, Colour (0xff9df2b4));
}

void Look::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (findColour (PopupMenu::backgroundColourId));
    g.setColour (col::line); g.drawRect (0, 0, w, h);
}

// ======================= small widgets =======================
Knob::Knob (APVTS& s, const String& id, const String& caption, const String& style)
{
    slider.getProperties().set ("style", style);
    slider.setPopupDisplayEnabled (true, true, nullptr, 1500);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    att = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
    if (auto* p = dynamic_cast<RangedAudioParameter*> (s.getParameter (id)))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    label.setText (caption, dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (FontOptions (13.0f, Font::bold));
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (slider); addAndMakeVisible (label);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromBottom (18));
    slider.setBounds (r);
}

Choice::Choice (APVTS& s, const String& id, const String& style)
{
    getProperties().set ("style", style);
    if (auto* c = dynamic_cast<AudioParameterChoice*> (s.getParameter (id))) addItemList (c->choices, 1);
    att = std::make_unique<APVTS::ComboBoxAttachment> (s, id, *this);
}

Switch::Switch (APVTS& s, const String& id, const String& text, const String& style) : ToggleButton (text)
{
    getProperties().set ("style", style);
    att = std::make_unique<APVTS::ButtonAttachment> (s, id, *this);
}

// ======================= pedal =======================
Pedal::Pedal (APVTS& s, const String& t, Colour c, const String& switchId, const String& typeId, std::vector<std::pair<String, String>> ks)
    : title (t), body (c)
{
    if (switchId.isNotEmpty()) { foot = std::make_unique<Switch> (s, switchId, String(), "foot"); addAndMakeVisible (*foot); foot->onStateChange = [this] { repaint(); }; }
    if (typeId.isNotEmpty()) { type = std::make_unique<Choice> (s, typeId); addAndMakeVisible (*type); }
    for (auto& [id, cap] : ks)
    {
        auto* k = knobs.add (new Knob (s, id, cap, "pedal"));
        k->setCaptionColour (Colours::white.withAlpha (0.9f));
        addAndMakeVisible (k);
    }
}

void Pedal::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (4);
    // shadow + enclosure
    g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r.translated (3, 5), 12);
    if (deco())
    {   // black lacquer case, the effect's colour as an inlaid panel, gold fan behind the name
        g.setGradientFill (ColourGradient (Colour (0xff2a2622), r.getX(), r.getY(), Colour (0xff070606), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 8);
        auto inlay = r.reduced (9).withTrimmedTop (r.getHeight() * 0.52f).withTrimmedBottom (2);
        g.setGradientFill (ColourGradient (body.brighter (0.15f), inlay.getX(), inlay.getY(), body.darker (0.55f), inlay.getRight(), inlay.getBottom(), false));
        g.fillRect (inlay);
        { Graphics::ScopedSaveState ss (g); g.reduceClipRegion (inlay.toNearestInt());
          decoFan (g, { inlay.getCentreX(), inlay.getBottom() + 4 }, inlay.getHeight() * 0.9f, -1.25f, 1.25f, 14, Colours::black.withAlpha (0.28f), 1.2f); }
        decoFrame (g, r.reduced (3), col::amber.withAlpha (0.85f), 8);
        g.setColour (col::amber.withAlpha (0.8f)); g.drawRect (inlay, 1.0f);
    }
    else
    {   // enamelled case with gold edging and vines in the corners
        g.setGradientFill (ColourGradient (body.brighter (0.3f), r.getX(), r.getY(), body.darker (0.5f), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 14);
        {
            Graphics::ScopedSaveState ss (g); Path clip; clip.addRoundedRectangle (r, 14); g.reduceClipRegion (clip);
            Random rnd (title.hashCode());   // enamel mottling
            for (int i = 0; i < 70; ++i) { g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.05f)); g.fillEllipse (r.getX() + rnd.nextFloat() * r.getWidth(), r.getY() + rnd.nextFloat() * r.getHeight(), 6 + rnd.nextFloat() * 18, 4 + rnd.nextFloat() * 10); }
        }
        nouveauFrame (g, r.reduced (3), Colour (0xffe2c06a), body.darker (0.4f).withSaturation (0.5f), 30);
    }
    // screws
    for (auto p : { r.getTopLeft() + Point<float> (10, 10), r.getTopRight() + Point<float> (-10, 10), r.getBottomLeft() + Point<float> (10, -10), r.getBottomRight() + Point<float> (-10, -10) })
    { g.setColour (deco() ? col::amber : Colour (0xffc9a24a)); g.fillEllipse (Rectangle<float> (5, 5).withCentre (p)); }
    // side jacks
    g.setColour (Colour (0xff2a2a2a));
    g.fillRect (Rectangle<float> (6, 16).withCentre ({ r.getX() - 1, r.getY() + 46 }));
    g.fillRect (Rectangle<float> (6, 16).withCentre ({ r.getRight() + 1, r.getY() + 46 }));
    // LED
    const bool on = foot == nullptr || foot->getToggleState();
    auto led = Rectangle<float> (10, 10).withCentre ({ r.getCentreX(), r.getBottom() - 92 });
    if (on) { g.setColour (Colour (0xffff3b30).withAlpha (0.35f)); g.fillEllipse (led.expanded (5)); }
    g.setColour (on ? Colour (0xffff4a3a) : Colour (0xff4a1a16)); g.fillEllipse (led);
    g.setColour (Colours::black.withAlpha (0.5f)); g.drawEllipse (led, 1.0f);
    // name
    auto nameArea = Rectangle<float> (r.getX() + 8, r.getBottom() - 78, r.getWidth() - 16, 22);
    const bool light = body.getPerceivedBrightness() > 0.6f;
    g.setColour (light ? Colour (0xff1a1410) : Colours::white.withAlpha (0.92f));
    g.setFont (FontOptions (serifName(), 17.0f, Font::bold | Font::italic));
    g.drawFittedText (title, nameArea.toNearestInt(), Justification::centred, 1);
}

void Pedal::resized()
{
    auto r = getLocalBounds().reduced (14, 12);
    if (type != nullptr) type->setBounds (r.removeFromTop (26).reduced (4, 0));
    if (type2 != nullptr) { r.removeFromTop (3); type2->setBounds (r.removeFromTop (24).reduced (4, 0)); }
    r.removeFromTop (4);
    auto knobRow = r.removeFromTop (r.getHeight() - 104);
    const int n = knobs.size();
    if (n > 0)
    {
        const int kw = jmin (78, knobRow.getWidth() / jmax (1, (n > 4 ? 2 : n)));
        if (n <= 4)
        {
            auto row = knobRow.withSizeKeepingCentre (kw * n, jmin (knobRow.getHeight(), kw + 16));
            for (auto* k : knobs) k->setBounds (row.removeFromLeft (kw));
        }
        else
        {
            auto top = knobRow.removeFromTop (knobRow.getHeight() / 2);
            auto a = top.withSizeKeepingCentre (kw * 2, top.getHeight()), b = knobRow.withSizeKeepingCentre (kw * (n - 2), knobRow.getHeight());
            for (int i = 0; i < n; ++i) knobs[i]->setBounds (i < 2 ? a.removeFromLeft (kw) : b.removeFromLeft (kw));
        }
    }
    if (foot != nullptr) foot->setBounds (getLocalBounds().withSizeKeepingCentre (52, 52).withY (getHeight() - 66));
}

// ======================= amp head =======================
AmpHead::AmpHead (JesiennaProcessor& p) : proc (p), model (p.apvts, "amp", "plate")
{
    addAndMakeVisible (model);
    const std::pair<const char*, const char*> ks[] = { { "ampGain", "Gain" }, { "bass", "Bas" }, { "mid", "Środek" }, { "treble", "Góra" }, { "presence", "Presence" }, { "master", "Master" } };
    for (auto& [id, cap] : ks) { auto* k = knobs.add (new Knob (p.apvts, id, U (cap))); addAndMakeVisible (k); }
    model.onChange = [this] { refresh(); };
    refresh();
}

void AmpHead::refresh()
{
    const int a = model.getSelectedItemIndex();
    if (a >= 0 && a < jw::builtInAmps) { lookId = builtInLooks[a]; static const char* cats[] = { "czysty", "czysty", "crunch", "crunch", "lead", "ciężki", "lo-fi" }; category = U ("model wbudowany · ") + U (cats[a]); }
    else if (a >= jw::builtInAmps && a - jw::builtInAmps < proc.namAmps.size())
    {
        auto& e = proc.namAmps.getReference (a - jw::builtInAmps);
        lookId = e.look; category = U ("prawdziwy wzmacniacz (profil NAM) · ") + e.style;
    }
    const auto& L = ampLook (lookId);
    for (auto* k : knobs) { k->setStyle ("amp-" + lookId); k->setCaptionColour (L.panelText); }
    model.setColour (ComboBox::textColourId, L.panelText);
    if (auto* parent = getParentComponent()) parent->repaint();
    repaint();
}

void AmpHead::paint (Graphics& g)
{
    const auto& L = ampLook (lookId);
    auto r = getLocalBounds().toFloat().reduced (4);
    // cabinet body (tolex / tweed)
    g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (r.translated (4, 6), 14);
    g.setColour (L.tolex); g.fillRoundedRectangle (r, 14);
    if (L.tweedCloth)
    {
        Graphics::ScopedSaveState ss (g); Path clip; clip.addRoundedRectangle (r, 14); g.reduceClipRegion (clip);
        for (float x = r.getX() - r.getHeight(); x < r.getRight(); x += 5) { g.setColour (Colour (0xff8a6a3a).withAlpha (0.45f)); g.drawLine (x, r.getBottom(), x + r.getHeight(), r.getY(), 1.4f); }
        for (float x = r.getX(); x < r.getRight() + r.getHeight(); x += 9) { g.setColour (Colour (0xff5a3e1e).withAlpha (0.25f)); g.drawLine (x, r.getBottom(), x - r.getHeight(), r.getY(), 1.0f); }
    }
    else
    {   // tolex grain
        Random rnd (7);
        for (int i = 0; i < 900; ++i) { g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.04f)); g.fillRect (r.getX() + rnd.nextFloat() * r.getWidth(), r.getY() + rnd.nextFloat() * r.getHeight(), 2.0f, 1.0f); }
    }
    g.setColour (Colours::black.withAlpha (0.6f)); g.drawRoundedRectangle (r, 14, 1.5f);
    // piping
    g.setColour (L.id == "tweed" ? Colour (0xff3a2a1c) : Colour (0xffcfc6b0).withAlpha (L.id == "modern" ? 0.25f : 0.55f));
    g.drawRoundedRectangle (r.reduced (8), 10, 1.4f);
    // handle on top
    auto handle = Rectangle<float> (r.getCentreX() - 70, r.getY() - 2, 140, 10);
    g.setColour (Colour (0xff0d0d0d)); g.fillRoundedRectangle (handle, 5);
    // control panel
    auto panel = r.reduced (22, 0).withTrimmedTop (r.getHeight() * 0.40f).withTrimmedBottom (20);
    g.setGradientFill (ColourGradient (L.panel.brighter (0.18f), panel.getX(), panel.getY(), L.panel.darker (0.25f), panel.getX(), panel.getBottom(), false));
    g.fillRoundedRectangle (panel, 6);
    if (L.id == "brit" || L.id == "modern")   // brushed metal
        for (float y = panel.getY() + 2; y < panel.getBottom() - 2; y += 2) { g.setColour (Colours::white.withAlpha (0.035f)); g.drawHorizontalLine ((int) y, panel.getX() + 4, panel.getRight() - 4); }
    g.setColour (Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (panel, 6, 1.2f);
    // grille / logo area above the panel
    auto grille = r.reduced (22, 0).withTrimmedTop (22).withHeight (r.getHeight() * 0.40f - 30);
    g.setColour (L.grille); g.fillRoundedRectangle (grille, 6);
    {
        Graphics::ScopedSaveState ss (g); g.reduceClipRegion (grille.toNearestInt());
        for (float x = grille.getX(); x < grille.getRight(); x += 4) { g.setColour (Colours::black.withAlpha (0.10f)); g.drawVerticalLine ((int) x, grille.getY(), grille.getBottom()); }
        for (float y = grille.getY(); y < grille.getBottom(); y += 4) { g.setColour (Colours::white.withAlpha (0.05f)); g.drawHorizontalLine ((int) y, grille.getX(), grille.getRight()); }
    }
    g.setColour (Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (grille, 6, 1.0f);
    // nameplate behind the model selector
    auto plate = model.getBounds().toFloat().expanded (6, 2);
    g.setColour (L.panel.darker (0.1f).withAlpha (0.92f)); g.fillRoundedRectangle (plate, 6);
    g.setColour (L.panelText.withAlpha (0.35f)); g.drawRoundedRectangle (plate, 6, 1.0f);
    g.setColour (L.panelText.withAlpha (0.75f)); g.setFont (FontOptions (12.0f));
    g.drawText (category, plate.translated (0, plate.getHeight() + 2).withHeight (16).toNearestInt(), Justification::centred);
    // input jack, pilot light
    g.setColour (L.panelText.withAlpha (0.8f)); g.setFont (FontOptions (11.0f, Font::bold));
    auto jack = Rectangle<float> (22, 22).withCentre ({ panel.getX() + 30, panel.getCentreY() - 4 });
    g.drawText ("INPUT", jack.translated (0, 18).expanded (14, 0).toNearestInt(), Justification::centred);
    g.setColour (Colour (0xffb0b0b0)); g.fillEllipse (jack); g.setColour (Colour (0xff111111)); g.fillEllipse (jack.reduced (6));
    auto jewel = Rectangle<float> (20, 20).withCentre ({ panel.getRight() - 30, panel.getCentreY() - 4 });
    g.setColour (L.accent.withAlpha (0.35f)); g.fillEllipse (jewel.expanded (6));
    g.setGradientFill (ColourGradient (L.accent.brighter (0.6f), jewel.getX(), jewel.getY(), L.accent.darker (0.4f), jewel.getRight(), jewel.getBottom(), false));
    g.fillEllipse (jewel);
    g.setColour (Colours::white.withAlpha (0.5f)); g.fillEllipse (jewel.reduced (6).translated (-2, -2));
    // the studio's own touch: gold inlay with a fan (Art Deco) or vines in the corners (Secession)
    if (deco())
    {
        decoFrame (g, r.reduced (4), col::amber.withAlpha (0.75f), 10);
        decoFan (g, { grille.getCentreX(), grille.getBottom() - 2 }, grille.getHeight() * 0.42f, -1.35f, 1.35f, 18, col::amber.withAlpha (0.28f), 1.0f);
    }
    else nouveauFrame (g, r.reduced (4), Colour (0xffe2c06a), Colour (0xff4f7a45), 46);
}

void AmpHead::resized()
{
    auto r = getLocalBounds().reduced (4);
    auto grille = r.reduced (22, 0).withTrimmedTop (22).withHeight ((int) (r.getHeight() * 0.40f) - 30);
    model.setBounds (grille.withSizeKeepingCentre (jmin (430, grille.getWidth() - 40), 38).translated (0, -8));
    auto panel = r.reduced (22, 0).withTrimmedTop ((int) (r.getHeight() * 0.40f)).withTrimmedBottom (20);
    auto row = panel.reduced (64, 8);
    const int kw = row.getWidth() / jmax (1, knobs.size());
    for (auto* k : knobs) k->setBounds (row.removeFromLeft (kw).reduced (2, 0));
}

// ======================= cabinet =======================
Cabinet::Cabinet (JesiennaProcessor& p) : proc (p), cab (p.apvts, "cab"), mic (p.apvts, "mic"), mix (p.apvts, "cabMix", U ("Miks kolumny"))
{
    addAndMakeVisible (cab); addAndMakeVisible (mic); addAndMakeVisible (mix);
    cab.onChange = [this] { repaint(); };
    mic.onChange = [this] { repaint(); };
}

void Cabinet::paint (Graphics& g)
{
    const String name = cab.getText();
    const bool none = cab.getSelectedItemIndex() == 0;
    auto area = box;
    card (g, getLocalBounds().toFloat().reduced (2), {}, 0.55f);
    if (none)
    {
        g.setColour (col::dim); g.setFont (FontOptions (15.0f));
        g.drawFittedText (U ("Bez kolumny – sygnał prosto ze wzmacniacza (DI)"), area.toNearestInt(), Justification::centred, 2);
        return;
    }
    // painted cabinet in the studio's style; which one depends on the speaker layout
    String file;
    const bool d = deco();
    if (name.contains (U ("4×12")))      file = d ? "deco_kol_4x12.png" : "sec_kol_lilie.png";
    else if (name.contains (U ("2×12"))) file = d ? "deco_kol_slonce.png" : "sec_kol_dama.png";
    else if (name.contains (U ("1×8")))  file = d ? "deco_kol_waska.png" : "sec_kol_roze.png";
    else                                 file = d ? "deco_kol_wachlarz.png" : "sec_kol_paw.png";
    auto cabBox = area.reduced (14, 8);
    g.setColour (Colours::black.withAlpha (0.35f)); g.fillEllipse (cabBox.withTrimmedTop (cabBox.getHeight() - 16).translated (0, 6));
    drawSprite (g, file, cabBox);

    // the microphone in front of the cabinet
    const int m = mic.getSelectedItemIndex();
    if (m <= 0) return;
    static const char* mics[] = { "", "dynamiczny", "wstegowy", "pojemnosciowy", "daleki" };
    const String micFile = String (d ? "deco_mik_" : "sec_mik_") + mics[jlimit (0, 4, m)] + ".png";
    const bool far = m == 4;
    const float h = cabBox.getHeight() * (far ? 0.42f : 0.62f);
    auto mb = far ? Rectangle<float> (h * 0.5f, h).withPosition (cabBox.getRight() - h * 0.42f, cabBox.getY() - 2)
                  : Rectangle<float> (h * 0.55f, h).withCentre ({ cabBox.getX() + cabBox.getWidth() * 0.3f, cabBox.getBottom() - h * 0.45f });
    g.setColour (Colours::black.withAlpha (0.3f)); g.fillEllipse (mb.withTrimmedTop (mb.getHeight() - 10).translated (4, 4));
    drawSprite (g, micFile, mb);
}

void Cabinet::resized()
{
    auto r = getLocalBounds();
    auto controls = r.removeFromBottom (92);
    box = r.toFloat().reduced (10, 6);
    auto left = controls.removeFromLeft (controls.getWidth() - 96);
    cab.setBounds (left.removeFromTop (40).reduced (4, 6));
    mic.setBounds (left.removeFromTop (40).reduced (4, 6));
    mix.setBounds (controls.reduced (6, 0));
}

// ======================= guitar page =======================
static Colour pedalColour (const String& look)
{
    if (look == "green") return Colour (0xff2f7d4f);
    if (look == "yellow") return Colour (0xffd9b52f);
    if (look == "orange") return Colour (0xffe0782a);
    if (look == "blue") return Colour (0xff2d5f9e);
    if (look == "black") return Colour (0xff2a2a2d);
    return Colour (0xffa4532a);   // our own: autumn rust
}

RigPage::RigPage (JesiennaProcessor& p)
    : proc (p),
      input (p.apvts, U ("Wejście"), Colour (0xff60666c), {}, {}, { { "inGain", U ("Gain") }, { "gate", U ("Bramka") }, { "comp", U ("Kompresja") } }),
      drive (p.apvts, U ("Przester"), pedalColour ({}), "odOn", "pedal", { { "odDrive", U ("Drive") }, { "odTone", U ("Ton") }, { "odLevel", U ("Poziom") } }),
      mod (p.apvts, U ("Modulacja"), Colour (0xff5b3f8c), "modOn", "modType", { { "modRate", U ("Szybkość") }, { "modDepth", U ("Głębia") } }),
      delay (p.apvts, U ("Delay"), Colour (0xff1f6f73), "dlyOn", "dlyType", { { "dlyMix", U ("Miks") }, { "dlyFb", U ("Powtórz.") }, { "dlyTone", U ("Barwa") } }),
      reverb (p.apvts, U ("Pogłos"), Colour (0xff8a7a5c), "revOn", "revType", { { "revMix", U ("Miks") }, { "revDecay", U ("Długość") }, { "revTone", U ("Barwa") }, { "revDwell", U ("Siła") } }),
      amp (p), cab (p),
      level (p.apvts, "gtrLevel", U ("Głośność")), pan (p.apvts, "gtrPan", U ("Panorama")), space (p.apvts, "gtrSpace", U ("Przestrzeń")),
      inCh (p.apvts, "inCh")
{
    delay.addSecondType (p.apvts, "dlyDiv");
    for (auto* c : std::initializer_list<Component*> { &input, &drive, &mod, &delay, &reverb, &amp, &cab, &level, &pan, &space, &inCh }) addAndMakeVisible (c);
    auto updDrive = [this]
    {
        const int i = drive.type->getSelectedItemIndex();
        const String look = i > 0 && i - 1 < proc.namPedals.size() ? proc.namPedals[i - 1].look : String();
        drive.setColour (pedalColour (look));
    };
    drive.type->onChange = updDrive; updDrive();
    auto prevAmp = amp.model.onChange;
    amp.model.onChange = [this, prevAmp] { prevAmp(); cab.lookId = amp.lookId; cab.repaint(); };
    cab.lookId = amp.lookId;
}

void RigPage::paint (Graphics& g)
{
    // the studio behind everything
    backdrop (g, getLocalBounds(), deco() ? "deco_wzmacniacze.jpg" : "secesja_sala.jpg", deco() ? 0.22f : 0.2f, 0.5f, 0.55f, this);
    // pedalboard
    auto board = Rectangle<float> (12, 8, (float) getWidth() - 24, 284);
    g.setColour (Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (board.translated (0, 5), 12);
    frosted (g, board, deco() ? 4.0f : 14.0f, this);
    g.setGradientFill (ColourGradient (col::panel.withAlpha (0.62f), board.getX(), board.getY(), col::bg.withAlpha (0.75f), board.getX(), board.getBottom(), false));
    g.fillRoundedRectangle (board, deco() ? 4.0f : 14.0f);
    for (float y = board.getY() + 30; y < board.getBottom(); y += 54) { g.setColour (Colours::black.withAlpha (0.3f)); g.fillRect (board.getX() + 10, y, board.getWidth() - 20, 6.0f); }
    if (deco()) decoFrame (g, board.reduced (2), col::amber.withAlpha (0.7f), 12);
    else nouveauFrame (g, board.reduced (2), col::amber, col::accent2, 40);
    g.setColour (col::dim); g.setFont (FontOptions (11.0f, Font::bold));
    g.drawText (U ("GITARA"), Rectangle<int> (deco() ? 22 : 62, 12, 60, 14), Justification::left);

    // patch cables: guitar → pedals → amp → cabinet
    auto cable = [&] (Point<float> a, Point<float> b, float sag)
    {
        Path p; p.startNewSubPath (a); p.cubicTo (a.translated (30, sag), b.translated (-30, sag), b);
        g.setColour (Colours::black.withAlpha (0.4f)); g.strokePath (p, PathStrokeType (7.0f), AffineTransform::translation (0, 3));
        g.setColour (Colour (0xff1b1b1b)); g.strokePath (p, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colours::white.withAlpha (0.08f)); g.strokePath (p, PathStrokeType (1.5f), AffineTransform::translation (-1, -1.5f));
    };
    auto jy = [] (Component& c) { return (float) c.getY() + 4 + 46; };
    cable ({ 24, 60 }, { (float) input.getX() + 4, jy (input) }, 30);
    cable ({ (float) input.getRight() - 4, jy (input) }, { (float) drive.getX() + 4, jy (drive) }, 26);
    cable ({ (float) drive.getRight() - 4, jy (drive) }, { (float) mod.getX() + 4, jy (mod) }, 26);
    cable ({ (float) mod.getRight() - 4, jy (mod) }, { (float) delay.getX() + 4, jy (delay) }, 26);
    cable ({ (float) delay.getRight() - 4, jy (delay) }, { (float) reverb.getX() + 4, jy (reverb) }, 26);
    // from the delay down to the amp's input (the order here is drawn like a real rig; the sound runs modulation and delay after the amp)
    const float ampIn = (float) amp.getX() + 4 + 22 + 30, ampInY = (float) amp.getY() + 4 + (amp.getHeight() - 8) * 0.40f + ((amp.getHeight() - 8) * 0.60f - 20) / 2 - 4;
    {
        Path p; Point<float> a ((float) reverb.getRight() - 4, jy (reverb)), b (ampIn, ampInY);
        p.startNewSubPath (a); p.cubicTo (a.translated (60, 30), { a.x + 40, board.getBottom() + 40 }, { (a.x + b.x) / 2 + 200, board.getBottom() + 24 });
        p.cubicTo ({ b.x - 60, board.getBottom() + 10 }, b.translated (-60, -40), b);
        g.setColour (Colours::black.withAlpha (0.4f)); g.strokePath (p, PathStrokeType (7.0f), AffineTransform::translation (0, 3));
        g.setColour (Colour (0xff1b1b1b)); g.strokePath (p, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    // speaker cable
    cable ({ (float) amp.getRight() - 30, (float) amp.getBottom() - 30 }, { (float) cab.getX() + 20, (float) cab.getY() + cab.getHeight() * 0.35f }, 40);

    // output strip
    auto strip = Rectangle<float> ((float) level.getX() - 8, (float) inCh.getY() - 30, (float) level.getWidth() + 16, (float) space.getBottom() - inCh.getY() + 40);
    card (g, strip, {}, 0.86f, this);
    g.setColour (col::amber); g.setFont (FontOptions (12.0f, Font::bold));
    g.drawText (U ("WEJŚCIE / MIKS"), strip.withHeight (26).translated (0, 6).toNearestInt(), Justification::centred);
}

void RigPage::resized()
{
    const int top = 22, ph = 256, gap = 20;
    int x = 40;
    input.setBounds (x, top, 168, ph); x += 168 + gap;
    drive.setBounds (x, top, 222, ph); x += 222 + gap;
    mod.setBounds (x, top, 222, ph); x += 222 + gap;
    delay.setBounds (x, top, 222, ph); x += 222 + gap;
    reverb.setBounds (x, top, jmin (222, getWidth() - 36 - x), ph);
    const int rowY = 312, rowH = getHeight() - rowY - 10;
    amp.setBounds (20, rowY + 10, 640, rowH - 10);
    cab.setBounds (676, rowY, 330, rowH);
    auto strip = Rectangle<int> (1030, rowY + 30, 150, rowH - 40);
    inCh.setBounds (strip.removeFromTop (30).reduced (2, 0)); strip.removeFromTop (8);
    const int kh = strip.getHeight() / 3;
    level.setBounds (strip.removeFromTop (kh).reduced (24, 2));
    pan.setBounds (strip.removeFromTop (kh).reduced (24, 2));
    space.setBounds (strip.removeFromTop (kh).reduced (24, 2));
}

// ======================= band page =======================
BandPage::BandPage (JesiennaProcessor& p)
    : proc (p), on (p.apvts, "bandOn", U ("Zespół gra")), sync (p.apvts, "sync", U ("Synchronizacja z DAW")),
      song (p.apvts, "song"), key (p.apvts, "key"), style (p.apvts, "style"), keysInst (p.apvts, "keysInst"),
      tempo (p.apvts, "tempo", U ("Tempo")), energy (p.apvts, "energy", U ("Energia")), swing (p.apvts, "swing", U ("Swing")),
      drums (p.apvts, "drums", U ("Perkusja")), bass (p.apvts, "bassVol", U ("Kontrabas")), keys (p.apvts, "keysVol", U ("Klawisze")),
      level (p.apvts, "bandLevel", U ("Zespół")), space (p.apvts, "bandSpace", U ("Przestrzeń"))
{
    for (auto* c : std::initializer_list<Component*> { &on, &sync, &song, &key, &style, &keysInst, &tempo, &energy, &swing, &drums, &bass, &keys, &level, &space }) addAndMakeVisible (c);
    keysInst.onChange = [this] { repaint(); };
    timerCallback();   // chord chart filled straight away
    startTimerHz (15);
}

void BandPage::timerCallback()
{
    const int s = song.getSelectedItemIndex(), k = key.getSelectedItemIndex();
    bool dirty = false;
    if (s != progSong || k != progKey) { progSong = s; progKey = k; prog = jw::Band::progressionFor (jmax (0, s), jmax (0, k)); dirty = true; }
    const int c = proc.apvts.getRawParameterValue ("bandOn")->load() > 0.5f ? proc.band.uiIndex.load() : -1;
    if (c != current) { current = c; dirty = true; }
    if (dirty) repaint (chart);
}

/** toUpperCase() leaves Polish letters alone in the C locale. */
static String upperPl (const String& t)
{
    return t.toUpperCase().replace (U ("ą"), U ("Ą")).replace (U ("ć"), U ("Ć")).replace (U ("ę"), U ("Ę")).replace (U ("ł"), U ("Ł")).replace (U ("ń"), U ("Ń"))
                          .replace (U ("ó"), U ("Ó")).replace (U ("ś"), U ("Ś")).replace (U ("ź"), U ("Ź")).replace (U ("ż"), U ("Ż"));
}

static String chordLabel (const jw::Chord& c)
{
    static const char* nm[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "H" };
    String q = jw::chordTypes()[(size_t) c.type].key;
    q = q.replace ("maj7s11", U ("maj7♯11")).replace ("m7b5", U ("ø")).replace ("dim7", U ("°7")).replace ("b9", U ("♭9")).replace ("#9", U ("♯9")).replace ("#11", U ("♯11"));
    return String (nm[c.root % 12]) + q;
}

void BandPage::paint (Graphics& g)
{
    backdrop (g, getLocalBounds(), deco() ? "deco_studio.jpg" : "secesja_sala.jpg", 0.24f, 0.5f, 0.5f, this);
    card (g, Rectangle<int> (12, 8, 330, 330).toFloat(), upperPl (U ("Utwór")), 0.84f, this);
    card (g, chart.expanded (10, 34).withTrimmedBottom (24).toFloat(), upperPl (U ("Akordy (bieżący podświetlony)")), 0.84f, this);
    card (g, Rectangle<int> (12, 350, getWidth() - 24, getHeight() - 360).toFloat(), upperPl (U ("Scena")), 0.7f, this);
    {   // a framed picture on the wall: the jazz club poster (Art Deco) or Mucha's "Autumn", 1896 (Secession)
        auto frame = Rectangle<float> ((float) getWidth() - 186, 8, 174, 330);
        g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (frame.translated (3, 5), deco() ? 2.0f : 10.0f);
        g.setColour (deco() ? Colour (0xff0b0907) : Colour (0xff3a2a16)); g.fillRoundedRectangle (frame, deco() ? 2.0f : 10.0f);
        drawSprite (g, deco() ? "deco_klub.jpg" : "mucha_jesien.jpg", frame.reduced (12).withTrimmedBottom (14), RectanglePlacement::centred | RectanglePlacement::fillDestination);
        if (deco()) decoFrame (g, frame.reduced (2), col::amber, 8); else nouveauFrame (g, frame.reduced (2), col::amber, col::accent2, 34);
        g.setColour (col::text.withAlpha (0.8f)); g.setFont (FontOptions (serifName(), 11.5f, Font::italic));
        g.drawText (deco() ? U ("Klub „Pod Złotym Wachlarzem”") : U ("A. Mucha, „Jesień”, 1896"), frame.withTrimmedTop (frame.getHeight() - 24).toNearestInt(), Justification::centred);
    }

    // chord chart: one chip per chord, width ~ duration
    if (! prog.empty())
    {
        auto area = chart.toFloat();
        float x = area.getX(), y = area.getY(); const float rowH = 46, unit = 92;
        for (int i = 0; i < (int) prog.size(); ++i)
        {
            const float w = jmax (46.0f, unit * prog[(size_t) i].bars) - 6;
            if (x + w > area.getRight()) { x = area.getX(); y += rowH; }
            if (y + rowH > area.getBottom() + 4) break;
            auto chip = Rectangle<float> (x, y, w, rowH - 8);
            const bool cur = i == current;
            g.setColour (cur ? (deco() ? col::accent2 : col::amber) : col::panel2.withAlpha (0.9f)); g.fillRoundedRectangle (chip, deco() ? 2.0f : 9.0f);
            g.setColour (cur ? col::amber.brighter (0.3f) : col::line); g.drawRoundedRectangle (chip, deco() ? 2.0f : 9.0f, 1.0f);
            g.setColour (cur ? (deco() ? Colours::white : Colour (0xff1a1410)) : col::text); g.setFont (FontOptions (w < 60 ? 14.0f : 17.0f, Font::bold));
            g.drawFittedText (chordLabel (prog[(size_t) i]), chip.toNearestInt().reduced (4, 0), Justification::centred, 1);
            x += w + 6;
        }
    }
    // the band on stage: painted instruments (the keys follow the chosen instrument)
    const float sy = (float) stage.getY();
    auto put = [&] (const String& file, Rectangle<float> r)
    {
        g.setColour (Colours::black.withAlpha (0.3f)); g.fillEllipse (r.withTrimmedTop (r.getHeight() - 12).translated (0, 4));
        drawSprite (g, file, r, RectanglePlacement::centred | RectanglePlacement::yBottom);
    };
    put ("inst_perkusja.png", { (float) drums.getX() - 30, sy - 14, (float) drums.getWidth() + 60, 140 });
    put ("inst_bas.png", { (float) bass.getX(), sy - 18, (float) bass.getWidth(), 144 });
    const bool piano = proc.apvts.getRawParameterValue ("keysInst")->load() > 0.5f;
    put (piano ? "inst_fortepian.png" : "inst_klawisze.png", { (float) keys.getX() - 50, sy - 10, (float) keys.getWidth() + 100, 136 });
}

void BandPage::resized()
{
    auto left = Rectangle<int> (12, 8, 330, 330).reduced (16, 34);
    on.setBounds (left.removeFromTop (34)); left.removeFromTop (6);
    sync.setBounds (left.removeFromTop (34)); left.removeFromTop (10);
    song.setBounds (left.removeFromTop (30)); left.removeFromTop (8);
    auto row = left.removeFromTop (30); key.setBounds (row.removeFromLeft (row.getWidth() / 2 - 4)); row.removeFromLeft (8); style.setBounds (row);
    left.removeFromTop (6);
    tempo.setBounds (left.withSizeKeepingCentre (110, left.getHeight()));
    chart = Rectangle<int> (366, 42, getWidth() - 366 - 212, 270);
    // stage
    auto st = Rectangle<int> (12, 350, getWidth() - 24, getHeight() - 360).reduced (20, 34);
    stage = st.withHeight (130);
    auto knobs = st.withTrimmedTop (140);
    const int cw = knobs.getWidth() / 7;
    auto place = [&] (Knob& k) { k.setBounds (knobs.removeFromLeft (cw).withSizeKeepingCentre (96, jmin (knobs.getHeight(), 112))); };
    place (drums); place (bass);
    auto kcol = knobs.removeFromLeft (cw);
    keys.setBounds (kcol.withSizeKeepingCentre (96, jmin (kcol.getHeight(), 112)).translated (0, -8));
    keysInst.setBounds (kcol.withSizeKeepingCentre (130, 26).withY (keys.getBottom() + 2));
    place (energy); place (swing); place (level); place (space);
    // instruments over the first three columns; centre the stage drawing on them
    stage = stage.withLeft (st.getX());
}

// ======================= space page =======================
SpacePage::SpacePage (JesiennaProcessor& p)
    : proc (p), room (p.apvts, "room", "lcd"), colour (p.apvts, "colour", "lcd"),
      spaceMix (p.apvts, "spaceMix", U ("Poziom")), gtrSpace (p.apvts, "gtrSpace", U ("Gitara")), bandSpace (p.apvts, "bandSpace", U ("Zespół")),
      colourMix (p.apvts, "colourMix", U ("Miks")), tape (p.apvts, "tape", U ("Taśma")), out (p.apvts, "out", U ("Wyjście")),
      limit (p.apvts, "limit", U ("Limiter"))
{
    for (auto* c : std::initializer_list<Component*> { &room, &colour, &spaceMix, &gtrSpace, &bandSpace, &colourMix, &tape, &out, &limit }) addAndMakeVisible (c);
}

void SpacePage::paint (Graphics& g)
{
    backdrop (g, getLocalBounds(), deco() ? "deco_studio.jpg" : "secesja_sala.jpg", 0.24f, 0.5f, 0.45f, this);
    auto rack = [&] (Rectangle<int> r, const String& t, const String& sub)
    {
        auto f = r.toFloat();
        g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (f.translated (3, 5), 6);
        frosted (g, f, deco() ? 3.0f : 12.0f, this);
        g.setGradientFill (ColourGradient (col::panel2.withAlpha (0.62f), f.getX(), f.getY(), col::panel.darker (0.4f).withAlpha (0.74f), f.getX(), f.getBottom(), false));
        g.fillRoundedRectangle (f, deco() ? 3.0f : 12.0f);
        for (float y = f.getY() + 3; y < f.getBottom() - 3; y += 2) { g.setColour (Colours::white.withAlpha (0.02f)); g.drawHorizontalLine ((int) y, f.getX() + 40, f.getRight() - 40); }
        // rack ears with holes
        for (auto ex : { f.getX(), f.getRight() - 30 })
        {
            g.setColour (deco() ? Colour (0xff1a1714) : Colour (0xff4a3a22)); g.fillRect (ex, f.getY(), 30.0f, f.getHeight());
            g.setColour (col::amber.withAlpha (0.6f)); g.drawRect (ex + 2, f.getY() + 2, 26.0f, f.getHeight() - 4, 1.0f);
            for (auto hy : { f.getY() + 18, f.getBottom() - 18 }) { g.setColour (Colour (0xff0e0c0a)); g.fillRoundedRectangle (ex + 8, hy - 5, 14, 10, 5); }
        }
        if (deco()) { decoFrame (g, f.reduced (34, 4), col::amber.withAlpha (0.5f), 8); decoFan (g, { f.getRight() - 46, f.getBottom() - 6 }, 26, -1.57f, 0.0f, 8, col::amber.withAlpha (0.5f)); }
        else { nouveauCorner (g, { f.getX() + 36, f.getY() + 6 }, 30, 1, 1, col::amber, col::accent2); nouveauCorner (g, { f.getRight() - 36, f.getBottom() - 6 }, 30, -1, -1, col::amber, col::accent2); }
        g.setColour (col::amber); g.setFont (FontOptions (serifName(), 18.0f, Font::bold | Font::italic));
        g.drawText (t, r.reduced (46, 10).removeFromTop (24), Justification::left);
        g.setColour (col::dim); g.setFont (FontOptions (12.0f));
        g.drawText (sub, r.reduced (46, 10).withTrimmedTop (24).removeFromTop (16), Justification::left);
    };
    rack (r1, U ("Przestrzeń"), U ("kluby, tunele, ulice, kościoły, płyta i sprężyny – prawdziwe odpowiedzi impulsowe"));
    rack (r2, U ("Barwa i taśma"), U ("mikrofon kasetowy, deck, taśma szpulowa, gramofon, telefon + wow, nasycenie i szum"));
    rack (r3, U ("Wyjście"), U ("głośność całości i limiter, żeby nic nie przesterowało"));
    // tape reels on the second unit
    auto reelArea = r2.withTrimmedLeft (r2.getWidth() - 330).withTrimmedRight (60).reduced (10, 34).toFloat();
    const float reelD = jmin (reelArea.getHeight(), reelArea.getWidth() * 0.46f);
    for (int i = 0; i < 2; ++i)
    {
        auto c = Rectangle<float> (reelD, reelD).withCentre ({ reelArea.getX() + reelArea.getWidth() * (i ? 0.75f : 0.25f), reelArea.getCentreY() });
        g.setColour (Colour (0xff2a2622)); g.fillEllipse (c);
        g.setColour (Colour (0xff6a5a46)); g.fillEllipse (c.reduced (c.getWidth() * (i ? 0.3f : 0.12f)));
        g.setColour (Colour (0xffcfc8bc)); g.fillEllipse (c.reduced (c.getWidth() * 0.36f));
        for (int s = 0; s < 3; ++s) { const float a = s * MathConstants<float>::twoPi / 3; g.setColour (Colour (0xff2a2622)); g.fillEllipse (Rectangle<float> (7, 7).withCentre (c.getCentre() + Point<float> (std::sin (a), std::cos (a)) * c.getWidth() * 0.08f)); }
    }
    // big output meters on the third unit
    auto mArea = r3.withTrimmedLeft (r3.getWidth() - 360).reduced (50, 26).toFloat();
    const float levels[] = { proc.outLevelL.load(), proc.outLevelR.load() };
    for (int i = 0; i < 2; ++i)
    {
        auto bar = mArea.removeFromTop (mArea.getHeight() / (2 - i)).reduced (0, 4);
        g.setColour (Colour (0xff0d0b09)); g.fillRoundedRectangle (bar, 3);
        const float db = jlimit (-48.0f, 0.0f, Decibels::gainToDecibels (levels[i] + 1e-6f));
        auto fill = bar.reduced (2).withWidth ((bar.getWidth() - 4) * (db + 48) / 48);
        g.setGradientFill (ColourGradient (Colour (0xff3fae5a), fill.getX(), 0, Colour (0xffe0a24a), bar.getX() + bar.getWidth() * 0.8f, 0, false));
        g.fillRoundedRectangle (fill, 2);
        g.setColour (col::dim); g.setFont (FontOptions (11.0f)); g.drawText (i ? "R" : "L", bar.withX (bar.getX() - 16).withWidth (14).toNearestInt(), Justification::centred);
    }
}

void SpacePage::resized()
{
    const int w = getWidth() - 40;
    r1 = { 20, 14, w, 220 }; r2 = { 20, 250, w, 220 }; r3 = { 20, 486, w, getHeight() - 500 };
    auto lay = [] (Rectangle<int> r, Choice& c, std::initializer_list<Knob*> ks)
    {
        auto inner = r.reduced (46, 10).withTrimmedTop (48);
        c.setBounds (inner.removeFromLeft (380).withSizeKeepingCentre (380, 40));
        inner.removeFromLeft (30);
        for (auto* k : ks) k->setBounds (inner.removeFromLeft (110).withSizeKeepingCentre (100, jmin (inner.getHeight(), 120)));
    };
    lay (r1, room, { &spaceMix, &gtrSpace, &bandSpace });
    lay (r2, colour, { &colourMix, &tape });
    auto inner = r3.reduced (46, 10).withTrimmedTop (44);
    limit.setBounds (inner.removeFromLeft (180).withSizeKeepingCentre (160, 36));
    inner.removeFromLeft (30);
    out.setBounds (inner.removeFromLeft (110).withSizeKeepingCentre (100, jmin (inner.getHeight(), 120)));
}

} // namespace ui

// ======================= editor =======================
using namespace juce;

void JesiennaEditor::Header::paint (Graphics& g)
{
    auto& e = *ed;
    auto r = getLocalBounds();
    ui::backdrop (g, r, ui::deco() ? "deco_studio.jpg" : "secesja_sala.jpg", 0.5f, 0.5f, ui::deco() ? 0.3f : 0.18f);
    g.setGradientFill (ColourGradient (ui::col::bg.withAlpha (0.85f), 0, 0, ui::col::bg.withAlpha (0.0f), 520, 0, false));
    g.fillRect (r.withWidth (560));
    g.setColour (ui::col::amber.withAlpha (0.6f)); g.drawHorizontalLine (r.getBottom() - 1, 0, (float) r.getWidth());
    if (ui::deco()) ui::decoFan (g, { 400.0f, (float) r.getBottom() }, 70, -1.4f, 1.4f, 16, ui::col::amber.withAlpha (0.35f));
    else ui::nouveauCorner (g, { 4.0f, 4.0f }, 56, 1, 1, ui::col::amber, ui::col::accent2);
    g.setColour (ui::col::amber); g.setFont (FontOptions (ui::serifName(), 30.0f, Font::bold | Font::italic));
    g.drawText ("Jesienna Warszawa", ui::deco() ? 18 : 40, 10, 360, 36, Justification::left);
    g.setColour (ui::col::dim); g.setFont (FontOptions (12.0f));
    g.drawText (jw::U ("gitara · wzmacniacze · zespół · przestrzeń"), ui::deco() ? 20 : 42, 46, 340, 16, Justification::left);

    // tuner
    auto tr = Rectangle<int> (W - 500, 10, 230, 62);
    g.setColour (ui::col::panel.withAlpha (0.85f)); g.fillRoundedRectangle (tr.toFloat(), 8);
    g.setColour (ui::col::text); g.setFont (FontOptions (24.0f, Font::bold));
    g.drawText (e.tunerNote.isEmpty() ? jw::U ("–") : e.tunerNote, tr.removeFromLeft (70), Justification::centred);
    auto scale = tr.reduced (8, 20);
    g.setColour (ui::col::line); g.fillRect (scale.withHeight (2).withY (scale.getCentreY()));
    g.fillRect (scale.getCentreX() - 1, scale.getY(), 2, scale.getHeight());
    if (e.tunerNote.isNotEmpty())
    {
        const float x = (float) scale.getCentreX() + e.tunerCents / 50.0f * scale.getWidth() / 2.0f;
        g.setColour (std::abs (e.tunerCents) < 4 ? ui::col::green : ui::col::amber);
        g.fillRoundedRectangle (x - 3, (float) scale.getY() - 2, 6, (float) scale.getHeight() + 4, 3);
        g.setFont (FontOptions (11.0f)); g.setColour (ui::col::dim);
        g.drawText (String (e.tunerHz, 1) + " Hz", scale.translated (0, 17), Justification::centred);
    }
    else { g.setColour (ui::col::dim); g.setFont (FontOptions (11.0f)); g.drawText (jw::U ("stroik – zagraj strunę"), scale.translated (0, 17), Justification::centred); }

    // chord display
    auto br = Rectangle<int> (W - 260, 10, 150, 62);
    g.setColour (ui::col::panel.withAlpha (0.85f)); g.fillRoundedRectangle (br.toFloat(), 8);
    g.setColour (ui::col::text); g.setFont (FontOptions (24.0f, Font::bold));
    g.drawText (e.chordText.isEmpty() ? jw::U ("–") : e.chordText, br.removeFromTop (38), Justification::centred);
    g.setColour (ui::col::dim); g.setFont (FontOptions (11.5f)); g.drawText (e.barText, br, Justification::centred);

    // meters
    auto mr = Rectangle<int> (W - 96, 12, 84, 58);
    auto meter = [&] (Rectangle<int> m, float lvl, const char* lab)
    {
        auto bar = m.removeFromTop (m.getHeight() - 14).toFloat();
        g.setColour (Colour (0xff0d0b09)); g.fillRoundedRectangle (bar, 2);
        const float db = jlimit (-60.0f, 0.0f, Decibels::gainToDecibels (lvl + 1e-6f));
        auto fill = bar.reduced (2).withTrimmedTop ((bar.getHeight() - 4) * (-db / 60.0f));
        g.setColour (db > -3 ? Colours::indianred : (db > -12 ? ui::col::amber : Colour (0xff3fae5a))); g.fillRoundedRectangle (fill, 1);
        g.setColour (ui::col::dim); g.setFont (FontOptions (10.0f)); g.drawText (lab, m, Justification::centred);
    };
    meter (mr.removeFromLeft (26), e.inLevel, "IN"); mr.removeFromLeft (4);
    meter (mr.removeFromLeft (24), e.outL, "L"); mr.removeFromLeft (4);
    meter (mr.removeFromLeft (24), e.outR, "R");
}

JesiennaEditor::JesiennaEditor (JesiennaProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&look);
    header.ed = this;
    addAndMakeVisible (content);
    content.addAndMakeVisible (header);
    rig = std::make_unique<ui::RigPage> (p);
    band = std::make_unique<ui::BandPage> (p);
    space = std::make_unique<ui::SpacePage> (p);
    for (auto* c : std::initializer_list<Component*> { rig.get(), band.get(), space.get() }) content.addChildComponent (c);
    const char* names[] = { "Gitara", "Zespół", "Przestrzeń i wyjście" };
    for (int i = 0; i < 3; ++i)
    {
        auto* b = tabButtons.add (new TextButton (jw::U (names[i])));
        b->setClickingTogglesState (false);
        b->onClick = [this, i] { showTab (i); };
        content.addAndMakeVisible (b);
    }
    for (int i = 0; i < p.getNumPrograms(); ++i) presetBox.addItem (p.getProgramName (i), i + 1);
    presetBox.setTextWhenNothingSelected (jw::U ("Gotowe brzmienia…"));
    presetBox.onChange = [this] { if (presetBox.getSelectedId() > 0) proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    content.addAndMakeVisible (presetBox);
    status.setJustificationType (Justification::centredLeft); status.setColour (Label::textColourId, ui::col::amber);
    status.setFont (FontOptions (12.0f));
    content.addAndMakeVisible (status);
    tbuf.assign (4096, 0.0f);

    // studio style: Art Deco (with a colour) or Secession; remembered in the project and as the default for new windows
    decoButton.setButtonText (jw::U ("Art déco")); nouveauButton.setButtonText (jw::U ("Secesja"));
    decoButton.onClick = [this] { setTheme (0, ui::theme().colour); };
    nouveauButton.onClick = [this] { setTheme (1, ui::theme().colour); };
    paletteBox.addItemList (ui::decoPaletteNames(), 1);
    paletteBox.onChange = [this] { if (paletteBox.getSelectedItemIndex() >= 0) setTheme (0, paletteBox.getSelectedItemIndex()); };
    for (auto* c : std::initializer_list<Component*> { &decoButton, &nouveauButton, &paletteBox }) content.addAndMakeVisible (c);

    content.setBounds (0, 0, W, H);
    header.setBounds (0, 0, W, 82);
    for (int i = 0; i < 3; ++i) tabButtons[i]->setBounds (16 + i * 190, 88, 184, 32);
    presetBox.setBounds (390, 16, 300, 30);
    decoButton.setBounds (W - 430, 90, 100, 28); nouveauButton.setBounds (W - 326, 90, 100, 28); paletteBox.setBounds (W - 218, 90, 200, 28);
    status.setBounds (390, 50, 340, 20);
    for (auto* c : std::initializer_list<Component*> { rig.get(), band.get(), space.get() }) c->setBounds (0, 126, W, H - 126);
    showTab (0);
    {
        const auto& st = proc.apvts.state;
        auto props = themeDefaults();
        const int style = st.hasProperty ("uiStyle") ? (int) st["uiStyle"] : props->getIntValue ("uiStyle", 0);
        const int colour = st.hasProperty ("uiColour") ? (int) st["uiColour"] : props->getIntValue ("uiColour", 0);
        setTheme (style, colour);
    }

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) W / H);
    setResizeLimits (W * 2 / 3, H * 2 / 3, W * 2, H * 2);
    setSize (W, H);
    startTimerHz (20);
}

JesiennaEditor::~JesiennaEditor() { setLookAndFeel (nullptr); }

std::unique_ptr<PropertiesFile> JesiennaEditor::themeDefaults()
{
    PropertiesFile::Options o; o.applicationName = "ustawienia"; o.folderName = "Jesienna Warszawa"; o.filenameSuffix = ".xml";
    o.osxLibrarySubFolder = "Application Support";
    return std::make_unique<PropertiesFile> (o);
}

void JesiennaEditor::setTheme (int style, int colour)
{
    ui::theme().style = jlimit (0, 1, style);
    ui::theme().colour = jlimit (0, (int) ui::decoPalettes().size() - 1, colour);
    ui::applyThemeColours();
    look.refreshColours();
    proc.apvts.state.setProperty ("uiStyle", ui::theme().style, nullptr);
    proc.apvts.state.setProperty ("uiColour", ui::theme().colour, nullptr);
    if (auto props = themeDefaults()) { props->setValue ("uiStyle", ui::theme().style); props->setValue ("uiColour", ui::theme().colour); props->saveIfNeeded(); }
    decoButton.setToggleState (ui::deco(), dontSendNotification); nouveauButton.setToggleState (! ui::deco(), dontSendNotification);
    paletteBox.setSelectedItemIndex (ui::theme().colour, dontSendNotification);
    paletteBox.setVisible (ui::deco());
    status.setColour (Label::textColourId, ui::col::amber);
    for (auto* c : std::initializer_list<Component*> { rig.get(), band.get(), space.get() }) c->resized();
    rig->amp.refresh();
    sendLookAndFeelChange();
    repaint();
}

void JesiennaEditor::showTab (int i)
{
    tab = i;
    rig->setVisible (i == 0); band->setVisible (i == 1); space->setVisible (i == 2);
    for (int k = 0; k < tabButtons.size(); ++k) tabButtons[k]->setToggleState (k == i, dontSendNotification);
}

void JesiennaEditor::resized()
{
    const float s = (float) getWidth() / W;
    content.setTransform (AffineTransform::scale (s));
}

void JesiennaEditor::paint (Graphics& g)
{
    g.fillAll (ui::col::bg);
    g.setColour (ui::col::line);
    g.drawHorizontalLine ((int) (123 * getWidth() / (float) W), 0, (float) getWidth());
}

void JesiennaEditor::timerCallback()
{
    inLevel = jmax (inLevel * 0.85f, proc.rig.inputLevel.load());
    outL = jmax (outL * 0.85f, proc.outLevelL.load()); outR = jmax (outR * 0.85f, proc.outLevelR.load());
    updateTuner();
    static const char* nm[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "H" };
    const int root = proc.band.uiChordRoot.load();
    if (root >= 0 && proc.apvts.getRawParameterValue ("bandOn")->load() > 0.5f)
    {
        chordText = String (nm[root % 12]) + jw::chordTypes()[(size_t) proc.band.uiChordType.load()].key;
        barText = "takt " + String (proc.band.uiBar.load() + 1) + jw::U (" · ") + String (proc.band.currentBpm(), 0) + " BPM";
    }
    else { chordText = {}; barText = jw::U ("zespół wyłączony"); }
    status.setText (proc.statusText(), dontSendNotification);
    header.repaint (W - 520, 0, 520, 82);
    if (tab == 2) space->repaint();
}

void JesiennaEditor::updateTuner()
{
    auto& fifo = proc.rig.tunerFifo;
    int s1, z1, s2, z2; const int ready = fifo.getNumReady();
    if (ready > 0)
    {
        fifo.prepareToRead (ready, s1, z1, s2, z2);
        auto push = [&] (int s, int z) { for (int i = 0; i < z; ++i) { tbuf[(size_t) tfill] = proc.rig.tunerBuf[(size_t) (s + i)]; tfill = (tfill + 1) % (int) tbuf.size(); } };
        push (s1, z1); push (s2, z2); fifo.finishedRead (z1 + z2);
    }
    const double rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000;
    const int dec = jmax (1, (int) std::round (rate / 12000.0)); const double r = rate / dec;
    std::vector<float> x; x.reserve (tbuf.size() / (size_t) dec);
    for (int i = 0; i < (int) tbuf.size(); i += dec) x.push_back (tbuf[(size_t) ((tfill + i) % (int) tbuf.size())]);
    float pk = 0; for (auto v : x) pk = jmax (pk, std::abs (v));
    if (pk < 0.004f) { tunerNote = {}; return; }
    const int Wn = (int) x.size() / 2, minLag = (int) (r / 1100), maxLag = jmin (Wn - 2, (int) (r / 60));
    std::vector<float> d ((size_t) maxLag + 2, 0.0f);
    float run = 0; int best = -1;
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        float s = 0; for (int i = 0; i < Wn; ++i) { const float q = x[(size_t) i] - x[(size_t) (i + tau)]; s += q * q; }
        run += s; d[(size_t) tau] = s * tau / jmax (1e-9f, run);
    }
    for (int tau = jmax (2, minLag); tau < maxLag; ++tau)
        if (d[(size_t) tau] < 0.15f) { while (tau + 1 < maxLag && d[(size_t) tau + 1] < d[(size_t) tau]) ++tau; best = tau; break; }
    if (best < 0) { tunerNote = {}; return; }
    const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
    const float shift = (a - 2 * b + c) != 0 ? 0.5f * (a - c) / (a - 2 * b + c) : 0.0f;
    const float hz = (float) (r / (best + shift));
    tunerHz = tunerHz > 0 && std::abs (hz - tunerHz) / tunerHz < 0.03f ? tunerHz * 0.7f + hz * 0.3f : hz;
    const float midi = 69 + 12 * std::log2 (tunerHz / 440.0f);
    const int m = (int) std::round (midi); tunerCents = (midi - (float) m) * 100;
    static const char* nm[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "H" };
    tunerNote = String (nm[(m % 12 + 12) % 12]) + String (m / 12 - 1);
}
