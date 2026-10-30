#include "PluginEditor.h"
using namespace juce;

// ---------------- look ----------------
JwLook::JwLook()
{
    setColour (ResizableWindow::backgroundColourId, bg());
    setColour (Slider::rotarySliderFillColourId, amber());
    setColour (Slider::textBoxTextColourId, Colour (0xffd8ccc0));
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Label::textColourId, Colour (0xffbfb2a4));
    setColour (ComboBox::backgroundColourId, Colour (0xff2b221d));
    setColour (ComboBox::outlineColourId, Colour (0xff4a3a30));
    setColour (ComboBox::textColourId, Colour (0xffeee2d4));
    setColour (ComboBox::arrowColourId, amber());
    setColour (PopupMenu::backgroundColourId, Colour (0xff221b17));
    setColour (PopupMenu::highlightedBackgroundColourId, amber().withAlpha (0.35f));
    setColour (PopupMenu::textColourId, Colour (0xffeee2d4));
    setColour (TabbedButtonBar::tabTextColourId, Colour (0xffbfb2a4));
    setColour (TabbedButtonBar::frontTextColourId, amber());
    setColour (TabbedComponent::backgroundColourId, bg());
    setColour (TabbedComponent::outlineColourId, Colours::transparentBlack);
}

void JwLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider&)
{
    auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4);
    const float d = jmin (r.getWidth(), r.getHeight()); auto c = r.withSizeKeepingCentre (d, d);
    const float ang = a0 + pos * (a1 - a0);
    g.setColour (Colour (0xff2e2520)); g.fillEllipse (c.reduced (d * 0.12f));
    g.setColour (Colour (0xff4a3a30)); g.drawEllipse (c.reduced (d * 0.12f), 1.2f);
    Path bgArc; bgArc.addCentredArc (c.getCentreX(), c.getCentreY(), d * 0.46f, d * 0.46f, 0, a0, a1, true);
    g.setColour (Colour (0xff3a2f28)); g.strokePath (bgArc, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    Path arc; arc.addCentredArc (c.getCentreX(), c.getCentreY(), d * 0.46f, d * 0.46f, 0, a0, ang, true);
    g.setColour (amber()); g.strokePath (arc, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    Path ptr; ptr.addRoundedRectangle (-1.5f, -d * 0.36f, 3.0f, d * 0.18f, 1.5f);
    g.setColour (Colour (0xfff2e6d8)); g.fillPath (ptr, AffineTransform::rotation (ang).translated (c.getCentreX(), c.getCentreY()));
}

void JwLook::drawToggleButton (Graphics& g, ToggleButton& b, bool hl, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (2);
    const bool on = b.getToggleState();
    g.setColour (on ? amber().withAlpha (0.85f) : Colour (0xff2b221d)); g.fillRoundedRectangle (r, 6);
    g.setColour (hl ? amber() : Colour (0xff4a3a30)); g.drawRoundedRectangle (r, 6, 1.2f);
    g.setColour (on ? Colour (0xff1a1410) : Colour (0xffd8ccc0)); g.setFont (FontOptions (13.0f, Font::bold));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().reduced (4, 0), Justification::centred, 2);
}

// ---------------- page ----------------
ParamPage::ParamPage (JesiennaProcessor& p, std::vector<Group> gs) : groups (std::move (gs))
{
    for (auto& gr : groups)
    {
        std::vector<std::unique_ptr<Ctl>> row;
        for (auto& id : gr.ids)
        {
            auto* prm = p.apvts.getParameter (id); if (prm == nullptr) continue;
            auto c = std::make_unique<Ctl>(); c->id = id;
            if (auto* ch = dynamic_cast<AudioParameterChoice*> (prm))
            {
                auto cb = std::make_unique<ComboBox>(); cb->addItemList (ch->choices, 1);
                c->ca = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, id, *cb);
                c->comp = std::move (cb); c->kind = 1;
            }
            else if (dynamic_cast<AudioParameterBool*> (prm) != nullptr)
            {
                auto tb = std::make_unique<ToggleButton> (prm->getName (40));
                c->ba = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id, *tb);
                c->comp = std::move (tb); c->kind = 2;
            }
            else
            {
                auto s = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
                s->setTextBoxStyle (Slider::TextBoxBelow, false, 70, 16);
                c->sa = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, *s);
                s->setNumDecimalPlacesToDisplay (prm->getLabel().isNotEmpty() ? 0 : 2);
                s->setTextValueSuffix (prm->getLabel().isNotEmpty() ? " " + prm->getLabel() : "");
                s->setDoubleClickReturnValue (true, dynamic_cast<RangedAudioParameter*> (prm)->convertFrom0to1 (prm->getDefaultValue()));
                c->comp = std::move (s);
            }
            if (c->kind != 2)
            {
                // short label: drop "Group – " prefix
                auto nm = prm->getName (60); if (nm.contains (" – ")) nm = nm.fromFirstOccurrenceOf (" – ", false, false);
                c->label.setText (nm, dontSendNotification); c->label.setJustificationType (Justification::centred);
                c->label.setFont (FontOptions (12.5f)); addAndMakeVisible (c->label);
            }
            addAndMakeVisible (*c->comp);
            row.push_back (std::move (c));
        }
        ctls.push_back (std::move (row));
    }
}

void ParamPage::paint (Graphics& g)
{
    for (size_t i = 0; i < boxes.size(); ++i)
    {
        g.setColour (JwLook::panel()); g.fillRoundedRectangle (boxes[i].toFloat(), 8);
        g.setColour (JwLook::amber()); g.setFont (FontOptions (13.0f, Font::bold));
        g.drawText (groups[i].title.toUpperCase(), boxes[i].reduced (10, 4).removeFromTop (18), Justification::left);
    }
}

void ParamPage::resized()
{
    // groups flow left→right, wrapping to new rows; every group sized by its controls
    boxes.clear();
    auto area = getLocalBounds().reduced (8);
    const int knobW = 84, rowH = 112, comboW = 220;
    int x = area.getX(), y = area.getY(), lineH = 0;
    for (size_t gi = 0; gi < groups.size(); ++gi)
    {
        int combos = 0, knobs = 0; for (auto& c : ctls[gi]) (c->kind == 0 ? knobs : combos)++;
        const int width = jmin (area.getWidth(), 16 + (combos ? comboW + 8 : 0) + knobs * knobW);
        const int knobRows = combos ? jmax (1, (combos * 46 + 10) / rowH + ((combos * 46 + 10) % rowH ? 1 : 0)) : 1;
        const int perRow = jmax (1, (width - 16 - (combos ? comboW + 8 : 0)) / knobW);
        const int h = 28 + jmax (knobRows, (knobs + perRow - 1) / perRow) * rowH;
        if (x + width > area.getRight() && x > area.getX()) { x = area.getX(); y += lineH + 8; lineH = 0; }
        Rectangle<int> box (x, y, width, h); boxes.push_back (box);
        auto inner = box.reduced (8); inner.removeFromTop (22);
        auto comboCol = combos ? inner.removeFromLeft (comboW) : Rectangle<int>();
        if (combos) inner.removeFromLeft (8);
        int k = 0;
        for (auto& c : ctls[gi])
        {
            if (c->kind != 0)
            {
                auto r = comboCol.removeFromTop (46);
                if (c->kind == 1) { c->label.setBounds (r.removeFromTop (16)); c->label.setJustificationType (Justification::left); c->comp->setBounds (r.removeFromTop (26)); }
                else c->comp->setBounds (r.reduced (0, 6));
            }
            else
            {
                const int col = k % perRow, rw = k / perRow; ++k;
                Rectangle<int> r (inner.getX() + col * knobW, inner.getY() + rw * rowH, knobW, rowH);
                c->label.setBounds (r.removeFromTop (16)); c->comp->setBounds (r.reduced (2));
            }
        }
        x += width + 8; lineH = jmax (lineH, h);
    }
}

// ---------------- editor ----------------
JesiennaEditor::JesiennaEditor (JesiennaProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&look);
    using G = ParamPage::Group;
    tabs.addTab ("Gitara", JwLook::bg(), new ParamPage (p, {
        G { "Wejście", { "inCh", "inGain", "gate", "comp" } },
        G { "Kostka", { "odOn", "odDrive", "odTone", "odLevel" } },
        G { "Wzmacniacz", { "amp", "ampGain", "bass", "mid", "treble", "presence", "master" } },
        G { "Kolumna i mikrofon", { "cab", "mic", "cabMix" } },
        G { "Modulacja", { "modType", "modRate", "modDepth" } },
        G { "Delay (w tempie)", { "dlyDiv", "dlyMix", "dlyFb", "dlyTone" } },
        G { "Gitara w miksie", { "gtrLevel", "gtrPan", "gtrSpace" } } }), true);
    tabs.addTab ("Zespół", JwLook::bg(), new ParamPage (p, {
        G { "Utwór", { "bandOn", "sync", "song", "key", "style", "tempo" } },
        G { "Gra", { "keysInst", "energy", "swing" } },
        G { "Głośności", { "drums", "bassVol", "keysVol", "bandLevel", "bandSpace" } } }), true);
    tabs.addTab ("Przestrzeń i wyjście", JwLook::bg(), new ParamPage (p, {
        G { "Przestrzeń", { "room", "spaceMix", "gtrSpace", "bandSpace" } },
        G { "Barwa i taśma", { "colour", "colourMix", "tape" } },
        G { "Wyjście", { "limit", "out" } } }), true);
    tabs.setTabBarDepth (34);
    addAndMakeVisible (tabs);

    for (int i = 0; i < p.getNumPrograms(); ++i) presetBox.addItem (p.getProgramName (i), i + 1);
    presetBox.setTextWhenNothingSelected ("Brzmienia…");
    presetBox.onChange = [this] { if (presetBox.getSelectedId() > 0) proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);
    status.setJustificationType (Justification::centredLeft); status.setColour (Label::textColourId, JwLook::amber());
    addAndMakeVisible (status);
    tbuf.assign (4096, 0.0f);
    setResizable (true, true); setResizeLimits (900, 600, 1800, 1200);
    setSize (1120, 720);
    startTimerHz (20);
}

JesiennaEditor::~JesiennaEditor() { setLookAndFeel (nullptr); }

void JesiennaEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (78);
    presetBox.setBounds (280, 12, jmin (300, getWidth() - 760), 28);
    status.setBounds (280, 46, getWidth() - 760, 20);
    tabs.setBounds (r.reduced (6, 0).withTrimmedBottom (6));
}

void JesiennaEditor::paint (Graphics& g)
{
    g.fillAll (JwLook::bg());
    auto top = getLocalBounds().removeFromTop (78).reduced (12, 8);
    g.setColour (JwLook::amber()); g.setFont (FontOptions (24.0f, Font::bold));
    g.drawText ("Jesienna Warszawa", top.removeFromLeft (260).removeFromTop (30), Justification::left);
    g.setColour (Colour (0xff8f8274)); g.setFont (FontOptions (12.0f));
    g.drawText ("gitara · zespół · przestrzeń", 12, 44, 250, 16, Justification::left);

    // tuner
    auto tr = Rectangle<int> (getWidth() - 470, 10, 220, 58);
    g.setColour (JwLook::panel()); g.fillRoundedRectangle (tr.toFloat(), 8);
    g.setColour (Colour (0xffeee2d4)); g.setFont (FontOptions (22.0f, Font::bold));
    g.drawText (tunerNote.isEmpty() ? "–" : tunerNote, tr.removeFromLeft (70), Justification::centred);
    auto scale = tr.reduced (8, 18);
    g.setColour (Colour (0xff4a3a30)); g.fillRect (scale.withHeight (2).withY (scale.getCentreY()));
    g.fillRect (scale.getCentreX() - 1, scale.getY(), 2, scale.getHeight());
    if (tunerNote.isNotEmpty())
    {
        const float x = (float) scale.getCentreX() + tunerCents / 50.0f * scale.getWidth() / 2.0f;
        g.setColour (std::abs (tunerCents) < 4 ? Colour (0xff8fd18a) : JwLook::amber());
        g.fillRoundedRectangle (x - 3, (float) scale.getY() - 2, 6, (float) scale.getHeight() + 4, 3);
        g.setFont (FontOptions (11.0f)); g.setColour (Colour (0xff8f8274));
        g.drawText (String (tunerHz, 1) + " Hz", scale.translated (0, 16), Justification::centred);
    }

    // band display + meters
    auto br = Rectangle<int> (getWidth() - 240, 10, 140, 58);
    g.setColour (JwLook::panel()); g.fillRoundedRectangle (br.toFloat(), 8);
    g.setColour (Colour (0xffeee2d4)); g.setFont (FontOptions (22.0f, Font::bold));
    g.drawText (chordText.isEmpty() ? "–" : chordText, br.removeFromTop (34), Justification::centred);
    g.setColour (Colour (0xff8f8274)); g.setFont (FontOptions (12.0f)); g.drawText (barText, br, Justification::centred);

    auto mr = Rectangle<int> (getWidth() - 92, 12, 80, 54);
    auto meter = [&] (Rectangle<int> r, float lvl, const char* lab)
    {
        g.setColour (Colour (0xff2b221d)); g.fillRect (r.removeFromTop (r.getHeight() - 14));
        const float db = jlimit (-60.0f, 0.0f, Decibels::gainToDecibels (lvl + 1e-6f));
        auto bar = r.withY (12).withHeight (40); bar = bar.withTrimmedTop ((int) (bar.getHeight() * (-db / 60.0f)));
        g.setColour (db > -3 ? Colours::indianred : JwLook::amber()); g.fillRect (bar.withY (bar.getY()));
        g.setColour (Colour (0xff8f8274)); g.setFont (FontOptions (10.0f)); g.drawText (lab, r, Justification::centred);
    };
    meter (mr.removeFromLeft (24), inLevel, "IN"); mr.removeFromLeft (4);
    meter (mr.removeFromLeft (22), outL, "L"); mr.removeFromLeft (4);
    meter (mr.removeFromLeft (22), outR, "R");
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
        barText = "takt " + String (proc.band.uiBar.load() + 1) + " · " + String (proc.band.currentBpm(), 0) + " BPM";
    }
    else { chordText = {}; barText = "zespół wyłączony"; }
    status.setText (proc.statusText(), dontSendNotification);
    repaint (0, 0, getWidth(), 78);
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
    // linearise the ring buffer, decimate to ~12 kHz
    const int dec = jmax (1, (int) std::round (rate / 12000.0)); const double r = rate / dec;
    std::vector<float> x; x.reserve (tbuf.size() / (size_t) dec);
    for (int i = 0; i < (int) tbuf.size(); i += dec) x.push_back (tbuf[(size_t) ((tfill + i) % (int) tbuf.size())]);
    float pk = 0; for (auto v : x) pk = jmax (pk, std::abs (v));
    if (pk < 0.004f) { tunerNote = {}; return; }
    // YIN difference function
    const int W = (int) x.size() / 2, minLag = (int) (r / 1100), maxLag = jmin (W - 2, (int) (r / 60));
    std::vector<float> d ((size_t) maxLag + 2, 0.0f);
    float run = 0; int best = -1;
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        float s = 0; for (int i = 0; i < W; ++i) { const float q = x[(size_t) i] - x[(size_t) (i + tau)]; s += q * q; }
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
