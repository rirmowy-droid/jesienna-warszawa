// Studio themes: Art Deco (with a choice of colour, so pink does not have to rule) and Secession (Art Nouveau).
// Painted backdrops and props live in data/ui; Art Deco pictures are recoloured on the fly.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Assets.h"

namespace ui {

using namespace juce;

namespace col {
    inline Colour bg { 0xff15110f }, panel { 0xff221b17 }, panel2 { 0xff2b221d }, line { 0xff4a3a30 },
                  amber { 0xffe0a24a }, text { 0xffeee2d4 }, dim { 0xff8f8274 }, green { 0xff8fd18a },
                  accent2 { 0xffd9577a }, ink { 0xff120c0a };
}

struct ThemeState { int style = 0; int colour = 0; };    // style: 0 Art Deco, 1 Secession
inline ThemeState& theme() { static ThemeState t; return t; }
inline bool deco() { return theme().style == 0; }

struct DecoPalette { const char* name; float hue, sat, val; uint32 accent, bg, panel; bool keep; };
inline const std::vector<DecoPalette>& decoPalettes()
{
    static const std::vector<DecoPalette> p {
        { "Róż i złoto",        0.95f, 1.0f, 1.0f,  0xffd9577a, 0xff150b10, 0xff22141a, true  },
        { "Bordo i złoto",      0.985f, 1.05f, 0.62f, 0xffa0283a, 0xff130909, 0xff211012, false },
        { "Szmaragd i złoto",   0.43f, 0.85f, 0.66f, 0xff2f8a64, 0xff07110d, 0xff0f1d17, false },
        { "Szafir i mosiądz",   0.62f, 0.85f, 0.70f, 0xff3a62b5, 0xff090c16, 0xff121828, false },
        { "Czerń i złoto",      0.08f, 0.10f, 0.40f, 0xffc89a48, 0xff0c0a08, 0xff181410, false },
        { "Bursztyn (jesień)",  0.07f, 1.0f, 0.85f,  0xffd2782e, 0xff130c07, 0xff22160e, false },
    };
    return p;
}
inline StringArray decoPaletteNames() { StringArray s; for (auto& p : decoPalettes()) s.add (String::fromUTF8 (p.name)); return s; }

inline void applyThemeColours()
{
    if (deco())
    {
        const auto& p = decoPalettes()[(size_t) jlimit (0, (int) decoPalettes().size() - 1, theme().colour)];
        col::accent2 = Colour (p.accent); col::bg = Colour (p.bg); col::panel = Colour (p.panel);
        col::panel2 = col::panel.brighter (0.12f); col::amber = Colour (0xffd9aa50);
        col::line = Colour (0xff6b5432);
        col::text = Colour (0xfff3e8d6); col::dim = Colour (0xffa89884); col::ink = Colour (0xff0b0807);
    }
    else
    {
        col::bg = Colour (0xff17110b); col::panel = Colour (0xff261c12); col::panel2 = Colour (0xff33261a);
        col::amber = Colour (0xffdcb560); col::accent2 = Colour (0xff5f8f55); col::line = Colour (0xff7a5f36);
        col::text = Colour (0xfff5e9cf); col::dim = Colour (0xffb39f80); col::ink = Colour (0xff1a120b);
    }
}

// ---------------- pictures ----------------
inline Image loadArt (const String& file)
{
    static std::map<String, Image> cache;
    if (auto it = cache.find (file); it != cache.end()) return it->second;
    Image im;
    auto f = jw::dataDir().getChildFile ("ui").getChildFile (file);
    if (f.existsAsFile()) im = ImageFileFormat::loadFrom (f);
    if (im.isValid() && im.getFormat() != Image::ARGB) im = im.convertedToFormat (Image::ARGB);
    cache[file] = im;
    return im;
}

/** Art Deco pictures with their pink/red lacquer moved to the chosen colour (gold, black and lamplight stay). */
inline Image art (const String& file)
{
    static std::map<String, Image> cache;
    // painted props in pink lacquer: follow the Art Deco colour; in the Secession studio the stage instruments turn to green enamel
    static const DecoPalette greenEnamel { "", 0.36f, 0.75f, 0.62f, 0, 0, 0, false };
    const bool inst = file.startsWith ("inst");
    const bool recolour = deco() ? ((file.startsWith ("deco") || inst) && ! decoPalettes()[(size_t) theme().colour].keep) : inst;
    const String key = file + (recolour ? "#" + String (deco() ? theme().colour : 99) : String());
    if (auto it = cache.find (key); it != cache.end()) return it->second;
    Image src = loadArt (file);
    if (! recolour || ! src.isValid()) { cache[key] = src; return src; }
    const auto& p = deco() ? decoPalettes()[(size_t) theme().colour] : greenEnamel;
    Image out = src.createCopy();
    Image::BitmapData bd (out, Image::BitmapData::readWrite);
    auto smooth = [] (float e0, float e1, float x) { const float t = jlimit (0.0f, 1.0f, (x - e0) / (e1 - e0)); return t * t * (3 - 2 * t); };
    for (int y = 0; y < bd.height; ++y)
        for (int x = 0; x < bd.width; ++x)
        {
            auto c = bd.getPixelColour (x, y);
            const float r = c.getFloatRed(), g = c.getFloatGreen(), b = c.getFloatBlue();
            const float mx = jmax (r, g, b), mn = jmin (r, g, b), d = mx - mn;
            if (d < 0.04f) continue;
            float h = mx == r ? std::fmod ((g - b) / d, 6.0f) : (mx == g ? (b - r) / d + 2 : (r - g) / d + 4);
            h /= 6; if (h < 0) h += 1;
            const float s = d / mx;
            // pinks, magentas and lacquer reds: hue above ~0.80 or below ~0.03; oranges and golds are left alone
            const float wh = h > 0.5f ? smooth (0.78f, 0.84f, h) : 1.0f - smooth (0.025f, 0.055f, h);
            const float w = wh * smooth (0.14f, 0.28f, s);
            if (w < 0.01f) continue;
            auto n = Colour::fromHSV (p.hue, jlimit (0.0f, 1.0f, s * p.sat), jlimit (0.0f, 1.0f, mx * p.val), c.getFloatAlpha());
            bd.setPixelColour (x, y, c.interpolatedWith (n, w));
        }
    cache[key] = out;
    return out;
}

/** Fill an area with a picture, cropped to cover it (anchor 0..1 picks which part survives), then darkened for legibility. */
/** The backdrop most recently painted (per component), so translucent cards can show it frosted behind them. */
struct BackdropInfo { String file; Rectangle<float> placed; float darken = 0; const void* owner = nullptr; };
inline BackdropInfo& lastBackdrop() { static BackdropInfo b; return b; }

inline void backdrop (Graphics& g, Rectangle<int> area, const String& file, float darken, float anchorX = 0.5f, float anchorY = 0.5f, const void* owner = nullptr)
{
    auto im = art (file);
    g.setColour (col::bg); g.fillRect (area);
    if (! im.isValid()) return;
    const float sc = jmax ((float) area.getWidth() / (float) im.getWidth(), (float) area.getHeight() / (float) im.getHeight());
    const float w = (float) im.getWidth() * sc, h = (float) im.getHeight() * sc;
    const float x = (float) area.getX() - (w - (float) area.getWidth()) * anchorX, y = (float) area.getY() - (h - (float) area.getHeight()) * anchorY;
    Graphics::ScopedSaveState ss (g); g.reduceClipRegion (area);
    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    g.setOpacity (1.0f);
    g.drawImage (im, Rectangle<float> (x, y, w, h), RectanglePlacement::stretchToFit);
    g.setColour (col::bg.withAlpha (darken)); g.fillRect (area);
    lastBackdrop() = { file, { x, y, w, h }, darken, owner };
}

/** Frosted glass: the soft copy of the current backdrop, clipped to r. */
inline void frosted (Graphics& g, Rectangle<float> r, float corner, const void* owner)
{
    auto& b = lastBackdrop();
    if (b.owner != owner || b.file.isEmpty()) return;
    auto im = art (b.file.upToLastOccurrenceOf (".", false, false) + "_mat.jpg");
    if (! im.isValid()) return;
    Graphics::ScopedSaveState ss (g);
    Path clip; clip.addRoundedRectangle (r, corner); g.reduceClipRegion (clip);
    g.setImageResamplingQuality (Graphics::highResamplingQuality); g.setOpacity (1.0f);
    g.drawImage (im, b.placed, RectanglePlacement::stretchToFit);
}

inline void drawSprite (Graphics& g, const String& file, Rectangle<float> box, RectanglePlacement place = RectanglePlacement::centred)
{
    auto im = art (file); if (! im.isValid()) return;
    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    g.setOpacity (1.0f);
    g.drawImage (im, box, place);
}

// ---------------- ornaments ----------------
/** Art Deco fan / sunburst. */
inline void decoFan (Graphics& g, Point<float> c, float r, float a0, float a1, int rays, Colour col, float thick = 1.2f)
{
    for (int i = 0; i <= rays; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / (float) rays;
        g.setColour (col.withMultipliedAlpha (i % 2 ? 0.55f : 1.0f));
        g.drawLine (c.x, c.y, c.x + std::sin (a) * r, c.y - std::cos (a) * r, thick);
    }
    Path arc; arc.addCentredArc (c.x, c.y, r * 0.35f, r * 0.35f, 0, a0, a1, true);
    g.setColour (col); g.strokePath (arc, PathStrokeType (thick));
    Path arc2; arc2.addCentredArc (c.x, c.y, r, r, 0, a0, a1, true); g.strokePath (arc2, PathStrokeType (thick));
}

/** Art Deco frame: double gold line with stepped corners. */
inline void decoFrame (Graphics& g, Rectangle<float> r, Colour gold, float step = 10)
{
    auto stepped = [&] (Rectangle<float> b, float s)
    {
        Path p;
        p.startNewSubPath (b.getX() + s * 2, b.getY());
        p.lineTo (b.getRight() - s * 2, b.getY()); p.lineTo (b.getRight() - s * 2, b.getY() + s); p.lineTo (b.getRight() - s, b.getY() + s); p.lineTo (b.getRight() - s, b.getY() + s * 2); p.lineTo (b.getRight(), b.getY() + s * 2);
        p.lineTo (b.getRight(), b.getBottom() - s * 2); p.lineTo (b.getRight() - s, b.getBottom() - s * 2); p.lineTo (b.getRight() - s, b.getBottom() - s); p.lineTo (b.getRight() - s * 2, b.getBottom() - s); p.lineTo (b.getRight() - s * 2, b.getBottom());
        p.lineTo (b.getX() + s * 2, b.getBottom()); p.lineTo (b.getX() + s * 2, b.getBottom() - s); p.lineTo (b.getX() + s, b.getBottom() - s); p.lineTo (b.getX() + s, b.getBottom() - s * 2); p.lineTo (b.getX(), b.getBottom() - s * 2);
        p.lineTo (b.getX(), b.getY() + s * 2); p.lineTo (b.getX() + s, b.getY() + s * 2); p.lineTo (b.getX() + s, b.getY() + s); p.lineTo (b.getX() + s * 2, b.getY() + s); p.closeSubPath();
        return p;
    };
    g.setColour (gold); g.strokePath (stepped (r, step * 0.6f), PathStrokeType (1.4f));
    g.setColour (gold.withMultipliedAlpha (0.55f)); g.strokePath (stepped (r.reduced (4), step * 0.45f), PathStrokeType (0.8f));
}

/** Art Nouveau corner: a whiplash stem with a curled tip and two leaves. dir: +1/-1 for x and y. */
inline void nouveauCorner (Graphics& g, Point<float> c, float s, float dx, float dy, Colour gold, Colour leaf)
{
    auto P = [&] (float x, float y) { return Point<float> (c.x + dx * x * s, c.y + dy * y * s); };
    Path stem; stem.startNewSubPath (P (0, 1.0f));
    stem.cubicTo (P (0.02f, 0.45f), P (0.2f, 0.12f), P (0.55f, 0.06f));
    stem.cubicTo (P (0.8f, 0.02f), P (0.95f, 0.18f), P (0.82f, 0.3f));
    stem.cubicTo (P (0.72f, 0.38f), P (0.6f, 0.28f), P (0.68f, 0.2f));
    g.setColour (gold); g.strokePath (stem, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    Path stem2; stem2.startNewSubPath (P (1.0f, 0)); stem2.cubicTo (P (0.5f, 0.02f), P (0.16f, 0.2f), P (0.08f, 0.55f));
    g.setColour (gold.withMultipliedAlpha (0.7f)); g.strokePath (stem2, PathStrokeType (1.1f, PathStrokeType::curved, PathStrokeType::rounded));
    auto leafAt = [&] (Point<float> a, Point<float> b, float bulge)
    {
        const auto m = (a + b) * 0.5f; const auto n = Point<float> (-(b.y - a.y), b.x - a.x) * bulge;
        Path l; l.startNewSubPath (a); l.quadraticTo (m + n, b); l.quadraticTo (m - n, a); l.closeSubPath();
        g.setColour (leaf.withAlpha (0.85f)); g.fillPath (l);
        g.setColour (gold.withMultipliedAlpha (0.8f)); g.strokePath (l, PathStrokeType (0.8f));
    };
    leafAt (P (0.1f, 0.42f), P (0.34f, 0.3f), 0.35f);
    leafAt (P (0.3f, 0.12f), P (0.42f, 0.32f), 0.3f);
    g.setColour (gold); g.fillEllipse (Rectangle<float> (s * 0.07f, s * 0.07f).withCentre (P (0.68f, 0.2f)));
}

/** Rounded Secession frame with corner vines. */
inline void nouveauFrame (Graphics& g, Rectangle<float> r, Colour gold, Colour leaf, float corner = 44)
{
    g.setColour (gold.withMultipliedAlpha (0.8f)); g.drawRoundedRectangle (r, 14, 1.3f);
    g.setColour (gold.withMultipliedAlpha (0.4f)); g.drawRoundedRectangle (r.reduced (5), 11, 0.8f);
    nouveauCorner (g, r.getTopLeft().translated (3, 3), corner, 1, 1, gold, leaf);
    nouveauCorner (g, r.getTopRight().translated (-3, 3), corner, -1, 1, gold, leaf);
    nouveauCorner (g, r.getBottomLeft().translated (3, -3), corner, 1, -1, gold, leaf);
    nouveauCorner (g, r.getBottomRight().translated (-3, -3), corner, -1, -1, gold, leaf);
}


// ---------------- patterns ----------------
// Art Deco
inline void decoChevrons (Graphics& g, Rectangle<float> r, Colour c, float step = 12, float thick = 1.2f)
{
    Graphics::ScopedSaveState ss (g); g.reduceClipRegion (r.toNearestInt()); g.setColour (c);
    for (float y = r.getY() - step; y < r.getBottom() + step; y += step)
    {
        Path p; p.startNewSubPath (r.getX() - step, y);
        for (float x = r.getX() - step; x < r.getRight() + step; x += step) { p.lineTo (x + step * 0.5f, y - step * 0.5f); p.lineTo (x + step, y); }
        g.strokePath (p, PathStrokeType (thick));
    }
}
inline void decoDiamonds (Graphics& g, Rectangle<float> r, Colour c, float step = 14)
{
    Graphics::ScopedSaveState ss (g); g.reduceClipRegion (r.toNearestInt()); g.setColour (c);
    for (float d = -r.getHeight(); d < r.getWidth() + r.getHeight(); d += step)
    {
        g.drawLine (r.getX() + d, r.getY(), r.getX() + d - r.getHeight(), r.getBottom(), 0.9f);
        g.drawLine (r.getX() + d - r.getHeight(), r.getY(), r.getX() + d, r.getBottom(), 0.9f);
    }
    for (float y = r.getY(); y < r.getBottom(); y += step) for (float x = r.getX() + (std::fmod ((y - r.getY()) / step, 2.0f) < 1 ? 0 : step * 0.5f); x < r.getRight(); x += step)
        g.fillEllipse (Rectangle<float> (2.4f, 2.4f).withCentre ({ x, y }));
}
inline void decoScallops (Graphics& g, Rectangle<float> r, Colour c, float rad = 10)
{
    Graphics::ScopedSaveState ss (g); g.reduceClipRegion (r.toNearestInt()); g.setColour (c);
    int row = 0;
    for (float y = r.getBottom(); y > r.getY() - rad; y -= rad * 0.75f, ++row)
        for (float x = r.getX() + (row % 2 ? rad : 0); x < r.getRight() + rad; x += rad * 2)
        {
            Path a; a.addCentredArc (x, y, rad, rad, 0, -MathConstants<float>::halfPi, MathConstants<float>::halfPi, true);
            g.strokePath (a, PathStrokeType (1.0f));
            Path a2; a2.addCentredArc (x, y, rad * 0.55f, rad * 0.55f, 0, -MathConstants<float>::halfPi, MathConstants<float>::halfPi, true);
            g.strokePath (a2, PathStrokeType (0.7f));
        }
}
inline void decoWaves (Graphics& g, Rectangle<float> r, Colour c, float step = 9)      // guilloché: concentric arcs
{
    Graphics::ScopedSaveState ss (g); g.reduceClipRegion (r.toNearestInt()); g.setColour (c);
    for (int k = 0; k < 3; ++k)
    {
        const Point<float> ctr (r.getX() + r.getWidth() * (0.2f + 0.3f * k), r.getBottom() + r.getHeight() * 0.2f);
        for (float rad = step; rad < r.getWidth() * 1.2f; rad += step) g.drawEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (ctr), 0.7f);
    }
}
inline void decoSteps (Graphics& g, Rectangle<float> r, Colour c, int steps = 5)   // ziggurat
{
    g.setColour (c);
    for (int i = 0; i < steps; ++i)
    {
        const float w = r.getWidth() * (1.0f - (float) i / steps), h = r.getHeight() / steps;
        g.drawRect (Rectangle<float> (w, h).withCentre ({ r.getCentreX(), r.getBottom() - h * (i + 0.5f) }), 1.0f);
    }
}
inline void decoBorderDentils (Graphics& g, Rectangle<float> strip, Colour c, float w = 6)
{
    g.setColour (c);
    for (float x = strip.getX(); x < strip.getRight(); x += w * 2) g.fillRect (x, strip.getY(), w, strip.getHeight());
}

// Secession
inline void flower (Graphics& g, Point<float> c, float r, Colour petal, Colour centre, int petals = 5, float rot = 0)
{
    for (int i = 0; i < petals; ++i)
    {
        const float a = rot + i * MathConstants<float>::twoPi / petals;
        Path p; p.startNewSubPath (0, 0); p.cubicTo (r * 0.6f, -r * 0.35f, r * 1.0f, -r * 0.2f, r, 0); p.cubicTo (r * 1.0f, r * 0.2f, r * 0.6f, r * 0.35f, 0, 0);
        g.setColour (petal); g.fillPath (p, AffineTransform::rotation (a).translated (c));
        g.setColour (petal.darker (0.5f)); g.strokePath (p, PathStrokeType (0.7f), AffineTransform::rotation (a).translated (c));
    }
    g.setColour (centre); g.fillEllipse (Rectangle<float> (r * 0.45f, r * 0.45f).withCentre (c));
}
inline void lily (Graphics& g, Point<float> base, float h, Colour petal, Colour stem)
{
    Path s; s.startNewSubPath (base); s.cubicTo (base.translated (-h * 0.1f, -h * 0.3f), base.translated (h * 0.12f, -h * 0.55f), base.translated (0, -h * 0.7f));
    g.setColour (stem); g.strokePath (s, PathStrokeType (1.4f));
    const auto top = base.translated (0, -h * 0.7f);
    for (int i = -1; i <= 1; ++i)
    {
        Path p; p.startNewSubPath (top); p.quadraticTo (top.translated (i * h * 0.22f - h * 0.05f, -h * 0.18f), top.translated (i * h * 0.16f, -h * 0.32f));
        p.quadraticTo (top.translated (i * h * 0.22f + h * 0.05f, -h * 0.12f), top); p.closeSubPath();
        g.setColour (petal); g.fillPath (p); g.setColour (petal.darker (0.4f)); g.strokePath (p, PathStrokeType (0.7f));
    }
    Path leaf; leaf.startNewSubPath (base.translated (0, -h * 0.25f)); leaf.quadraticTo (base.translated (h * 0.3f, -h * 0.35f), base.translated (h * 0.32f, -h * 0.5f));
    leaf.quadraticTo (base.translated (h * 0.1f, -h * 0.4f), base.translated (0, -h * 0.25f));
    g.setColour (stem); g.fillPath (leaf);
}
inline void peacockEye (Graphics& g, Point<float> c, float r, float angle = 0)
{
    auto t = AffineTransform::rotation (angle).translated (c);
    for (int i = 0; i < 22; ++i)    // barbs
    {
        const float a = -MathConstants<float>::pi * 0.95f + i * MathConstants<float>::pi * 1.9f / 21;
        Path b; b.startNewSubPath (0, r * 0.2f); b.quadraticTo (std::sin (a) * r * 0.8f, r * 0.2f - std::cos (a) * r * 0.6f, std::sin (a) * r * 1.25f, -std::cos (a) * r * 1.15f);
        g.setColour (Colour (0xff6f9a3a).withAlpha (0.8f)); g.strokePath (b, PathStrokeType (0.8f), t);
    }
    auto e = [&] (float rx, float ry, float dy, Colour col) { Path p; p.addEllipse (-rx, -ry + dy, rx * 2, ry * 2); g.setColour (col); g.fillPath (p, t); };
    e (r * 0.62f, r * 0.75f, -r * 0.1f, Colour (0xffc9a93a));
    e (r * 0.48f, r * 0.6f, -r * 0.05f, Colour (0xff2f8a7a));
    e (r * 0.32f, r * 0.42f, 0, Colour (0xff1f3f8a));
    e (r * 0.16f, r * 0.22f, r * 0.05f, Colour (0xff0d1430));
}
inline void whiplash (Graphics& g, Rectangle<float> r, Colour c, int n = 3)   // repeating curls (echoes)
{
    Graphics::ScopedSaveState ss (g); g.reduceClipRegion (r.toNearestInt());
    for (int k = 0; k < n; ++k)
    {
        const float s = r.getHeight() * (0.9f - 0.2f * k), x0 = r.getX() + k * r.getWidth() / n * 0.9f, y0 = r.getBottom();
        Path p; p.startNewSubPath (x0, y0);
        p.cubicTo (x0 + s * 0.2f, y0 - s * 0.8f, x0 + s * 0.9f, y0 - s * 0.9f, x0 + s * 0.8f, y0 - s * 0.45f);
        p.cubicTo (x0 + s * 0.72f, y0 - s * 0.2f, x0 + s * 0.45f, y0 - s * 0.32f, x0 + s * 0.55f, y0 - s * 0.5f);
        g.setColour (c.withMultipliedAlpha (1.0f - 0.25f * k)); g.strokePath (p, PathStrokeType (1.6f - 0.3f * k, PathStrokeType::curved, PathStrokeType::rounded));
    }
}
inline void stainedGlass (Graphics& g, Rectangle<float> r, int seed, std::initializer_list<uint32> jewels, Colour lead, float alpha = 1.0f)
{
    Graphics::ScopedSaveState ss (g); g.reduceClipRegion (r.toNearestInt());
    Random rnd (seed); std::vector<uint32> pal (jewels);
    // irregular leaded panes: a jittered grid split into triangles
    const int cols = 4, rows = 3; std::vector<Point<float>> pts;
    for (int y = 0; y <= rows; ++y) for (int x = 0; x <= cols; ++x)
    {
        float px = r.getX() + r.getWidth() * x / cols, py = r.getY() + r.getHeight() * y / rows;
        if (x > 0 && x < cols) px += (rnd.nextFloat() - 0.5f) * r.getWidth() / cols * 0.6f;
        if (y > 0 && y < rows) py += (rnd.nextFloat() - 0.5f) * r.getHeight() / rows * 0.6f;
        pts.push_back ({ px, py });
    }
    auto P = [&] (int x, int y) { return pts[(size_t) (y * (cols + 1) + x)]; };
    for (int y = 0; y < rows; ++y) for (int x = 0; x < cols; ++x)
        for (int t = 0; t < 2; ++t)
        {
            Path p;
            if ((x + y) % 2 == 0) { if (t == 0) p.addTriangle (P (x, y), P (x + 1, y), P (x + 1, y + 1)); else p.addTriangle (P (x, y), P (x + 1, y + 1), P (x, y + 1)); }
            else { if (t == 0) p.addTriangle (P (x, y), P (x + 1, y), P (x, y + 1)); else p.addTriangle (P (x + 1, y), P (x + 1, y + 1), P (x, y + 1)); }
            const Colour c = Colour (pal[(size_t) rnd.nextInt ((int) pal.size())]).withMultipliedAlpha (alpha);
            g.setGradientFill (ColourGradient (c.brighter (0.35f), p.getBounds().getX(), p.getBounds().getY(), c.darker (0.25f), p.getBounds().getRight(), p.getBounds().getBottom(), false));
            g.fillPath (p); g.setColour (lead.withMultipliedAlpha (alpha)); g.strokePath (p, PathStrokeType (1.6f, PathStrokeType::mitered));
        }
}
inline void vineBorder (Graphics& g, Rectangle<float> r, Colour stem, Colour leaf, Colour bloom, float amp = 4, float period = 34)
{
    auto run = [&] (Point<float> a, Point<float> b)
    {
        const float len = a.getDistanceFrom (b); if (len < 10) return;
        const auto dir = (b - a) / len; const Point<float> nrm (-dir.y, dir.x);
        Path p; p.startNewSubPath (a);
        for (float t = 0; t <= len; t += 2) p.lineTo (a + dir * t + nrm * (amp * std::sin (t / period * MathConstants<float>::twoPi)));
        g.setColour (stem); g.strokePath (p, PathStrokeType (1.1f));
        int k = 0;
        for (float t = period * 0.25f; t < len; t += period * 0.5f, ++k)
        {
            const float side = (k % 2 ? 1.0f : -1.0f);
            const auto at = a + dir * t + nrm * (amp * side);
            if (k % 4 == 3) { flower (g, at, amp * 1.4f, bloom, Colour (0xfff2d27a), 5, t * 0.1f); continue; }
            Path l; const auto tip = at + nrm * (side * amp * 2.2f) + dir * (amp * 1.2f);
            l.startNewSubPath (at); l.quadraticTo (at + nrm * (side * amp * 1.6f) - dir * amp, tip); l.quadraticTo (at + dir * (amp * 1.6f), at); l.closeSubPath();
            g.setColour (leaf); g.fillPath (l);
        }
    };
    run ({ r.getCentreX() + 40, r.getY() }, r.getTopRight().translated (-30, 0));      // the title sits on the left of the top edge
    run (r.getBottomLeft().translated (30, 0), r.getBottomRight().translated (-30, 0));
}

/** Translucent card in the current style, with an optional title. */
inline void card (Graphics& g, Rectangle<float> r, const String& title, float alpha = 0.84f, const void* glassOwner = nullptr)
{
    const float corner = deco() ? 3.0f : 14.0f;
    g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (2, 4), corner);
    if (glassOwner != nullptr) { frosted (g, r, corner, glassOwner); alpha *= 0.72f; }
    g.setColour (col::panel.withAlpha (alpha)); g.fillRoundedRectangle (r, corner);
    const float vine = jmin (44.0f, r.getHeight() * 0.3f, r.getWidth() * 0.18f);
    if (deco())
    {
        decoFrame (g, r.reduced (3), col::amber.withAlpha (0.8f));
        if (r.getWidth() > 220)
        {   // a band of chevrons under the top edge and fans in the lower corners
            decoChevrons (g, Rectangle<float> (r.getRight() - 150, r.getY() + 8, 120, 10), col::accent2.withAlpha (0.55f), 8, 1.0f);
            decoFan (g, { r.getX() + 8, r.getBottom() - 8 }, 26, 0.0f, MathConstants<float>::halfPi, 7, col::amber.withAlpha (0.45f), 0.9f);
            decoFan (g, { r.getRight() - 8, r.getBottom() - 8 }, 26, -MathConstants<float>::halfPi, 0.0f, 7, col::amber.withAlpha (0.45f), 0.9f);
        }
    }
    else
    {
        nouveauFrame (g, r.reduced (3), col::amber, col::accent2, vine);
        if (r.getWidth() > 220) vineBorder (g, r.reduced (8, 6), col::amber.withAlpha (0.55f), col::accent2.withAlpha (0.75f), Colour (0xffc8553a), 3.5f, 40);
    }
    if (r.getWidth() > 500 && r.getHeight() > 200)
    {   // large panels carry a faint emblem, like a watermark in the lacquer / glass
        Graphics::ScopedSaveState ss (g); g.reduceClipRegion (r.reduced (6).toNearestInt());
        if (deco())
        {
            decoFan (g, { r.getCentreX(), r.getBottom() - 6 }, r.getHeight() * 0.85f, -1.45f, 1.45f, 40, col::amber.withAlpha (0.09f), 1.2f);
            decoSteps (g, Rectangle<float> (r.getHeight() * 0.9f, r.getHeight() * 0.35f).withCentre ({ r.getCentreX(), r.getBottom() - r.getHeight() * 0.2f }), col::accent2.withAlpha (0.18f), 6);
        }
        else
        {
            const auto c = Point<float> (r.getCentreX(), r.getCentreY() + 10); const float rad = r.getHeight() * 0.42f;
            for (int i = 0; i < 12; ++i)
            {
                const float a = i * MathConstants<float>::twoPi / 12;
                Path petal; petal.startNewSubPath (c); petal.quadraticTo (c + Point<float> (std::sin (a - 0.2f), -std::cos (a - 0.2f)) * rad * 1.1f, c + Point<float> (std::sin (a), -std::cos (a)) * rad);
                petal.quadraticTo (c + Point<float> (std::sin (a + 0.2f), -std::cos (a + 0.2f)) * rad * 1.1f, c);
                g.setColour (col::amber.withAlpha (0.1f)); g.strokePath (petal, PathStrokeType (1.2f));
            }
            g.setColour (col::amber.withAlpha (0.1f)); g.drawEllipse (Rectangle<float> (rad * 2.2f, rad * 2.2f).withCentre (c), 1.2f);
            g.drawEllipse (Rectangle<float> (rad * 0.5f, rad * 0.5f).withCentre (c), 1.2f);
        }
    }
    if (title.isNotEmpty())
    {
        g.setColour (col::amber); g.setFont (FontOptions (13.0f, Font::bold));
        g.drawText (title, r.reduced (deco() ? 22.0f : vine + 12.0f, 10).removeFromTop (16).toNearestInt(), Justification::left);
    }
}

} // namespace ui
