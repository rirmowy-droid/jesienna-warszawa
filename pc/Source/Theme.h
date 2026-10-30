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

/** Comic-book panel fill: flat colour with a Ben-Day dot screen (replaces any glassy, blurred look). */
inline Image dotTile()
{
    static Image tile = []
    {
        Image t (Image::ARGB, 8, 8, true);
        Graphics g (t); g.setColour (Colours::white); g.fillEllipse (2.2f, 2.2f, 3.6f, 3.6f);
        return t;
    }();
    return tile;
}
inline void frosted (Graphics& g, Rectangle<float> r, float corner, const void*)
{
    Graphics::ScopedSaveState ss (g);
    Path clip; clip.addRoundedRectangle (r, corner); g.reduceClipRegion (clip);
    g.setColour (col::panel.withAlpha (0.55f)); g.fillRect (r);
    g.setTiledImageFill (dotTile(), 0, 0, 0.06f); g.fillRect (r);
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

/** Translucent card in the current style, with an optional title. */
inline void card (Graphics& g, Rectangle<float> r, const String& title, float alpha = 0.84f, const void* glassOwner = nullptr)
{
    const float corner = deco() ? 3.0f : 14.0f;
    g.setColour (col::ink); g.fillRoundedRectangle (r.translated (5, 5), corner);       // hard comic shadow
    if (glassOwner != nullptr) { frosted (g, r, corner, glassOwner); alpha *= 0.72f; }
    g.setColour (col::panel.withAlpha (alpha)); g.fillRoundedRectangle (r, corner);
    g.setColour (col::ink); g.drawRoundedRectangle (r, corner, 2.5f);
    const float vine = jmin (44.0f, r.getHeight() * 0.3f, r.getWidth() * 0.18f);
    if (deco()) decoFrame (g, r.reduced (3), col::amber.withAlpha (0.8f));
    else nouveauFrame (g, r.reduced (3), col::amber, col::accent2, vine);
    if (title.isNotEmpty())
    {
        g.setColour (col::amber); g.setFont (FontOptions (13.0f, Font::bold));
        g.drawText (title, r.reduced (deco() ? 22.0f : vine + 12.0f, 10).removeFromTop (16).toNearestInt(), Justification::left);
    }
}

} // namespace ui
