#pragma once
#include "PluginProcessor.h"

namespace ui {

using namespace juce;
using APVTS = AudioProcessorValueTreeState;

namespace col {
    const Colour bg { 0xff15110f }, panel { 0xff221b17 }, panel2 { 0xff2b221d }, line { 0xff4a3a30 },
                 amber { 0xffe0a24a }, text { 0xffeee2d4 }, dim { 0xff8f8274 }, green { 0xff8fd18a };
}

/** Visual style of an amp head, chosen per amp model. */
struct AmpLook
{
    String id; Colour tolex, panel, panelText, knob, knobCap, grille, accent; bool chickenHead = false, numbers = false, tweedCloth = false;
};
const AmpLook& ampLook (const String& id);

class Look : public LookAndFeel_V4
{
public:
    Look();
    void drawRotarySlider (Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, Slider&) override;
    void drawToggleButton (Graphics&, ToggleButton&, bool hl, bool down) override;
    void drawComboBox (Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, ComboBox&) override;
    Font getComboBoxFont (ComboBox&) override;
    void positionComboBoxText (ComboBox&, Label&) override;
    void drawPopupMenuBackground (Graphics&, int w, int h) override;
};

/** A knob with its caption and parameter attachment. */
struct Knob : public Component
{
    Knob (APVTS&, const String& id, const String& caption, const String& style = "rack");
    void resized() override;
    void setStyle (const String& s) { slider.getProperties().set ("style", s); repaint(); }
    void setCaptionColour (Colour c) { label.setColour (Label::textColourId, c); }
    Slider slider { Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox };
    Label label;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

struct Choice : public ComboBox
{
    Choice (APVTS&, const String& id, const String& style = {});
    std::unique_ptr<APVTS::ComboBoxAttachment> att;
};

struct Switch : public ToggleButton
{
    Switch (APVTS&, const String& id, const String& text, const String& style = {});
    std::unique_ptr<APVTS::ButtonAttachment> att;
};

/** Stompbox: coloured enclosure, knobs, type selector, footswitch and LED. */
class Pedal : public Component
{
public:
    Pedal (APVTS&, const String& title, Colour colour, const String& switchId, const String& typeId, std::vector<std::pair<String, String>> knobs);
    void paint (Graphics&) override;
    void resized() override;
    void setColour (Colour c) { body = c; repaint(); }
    String title; Colour body;
    std::unique_ptr<Switch> foot;
    std::unique_ptr<Choice> type, type2;
    void addSecondType (APVTS& s, const String& id) { type2 = std::make_unique<Choice> (s, id); addAndMakeVisible (*type2); }
    OwnedArray<Knob> knobs;
};

class AmpHead : public Component
{
public:
    AmpHead (JesiennaProcessor&);
    void paint (Graphics&) override;
    void resized() override;
    void refresh();
    JesiennaProcessor& proc;
    Choice model;
    OwnedArray<Knob> knobs;
    String lookId;
    String category;
};

class Cabinet : public Component
{
public:
    Cabinet (JesiennaProcessor&);
    void paint (Graphics&) override;
    void resized() override;
    JesiennaProcessor& proc;
    Choice cab, mic;
    Knob mix;
    String lookId = "black";
    Rectangle<float> box;
};

class RigPage : public Component
{
public:
    RigPage (JesiennaProcessor&);
    void paint (Graphics&) override;
    void resized() override;
    JesiennaProcessor& proc;
    Pedal input, drive, mod, delay, reverb;
    AmpHead amp;
    Cabinet cab;
    Knob level, pan, space;
    Choice inCh;
};

class BandPage : public Component, private Timer
{
public:
    BandPage (JesiennaProcessor&);
    void paint (Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    JesiennaProcessor& proc;
    Switch on, sync;
    Choice song, key, style, keysInst;
    Knob tempo, energy, swing, drums, bass, keys, level, space;
    std::vector<jw::Chord> prog; int progSong = -1, progKey = -1, current = -1;
    Rectangle<int> chart, stage;
};

class SpacePage : public Component
{
public:
    SpacePage (JesiennaProcessor&);
    void paint (Graphics&) override;
    void resized() override;
    JesiennaProcessor& proc;
    Choice room, colour;
    Knob spaceMix, gtrSpace, bandSpace, colourMix, tape, out;
    Switch limit;
    Rectangle<int> r1, r2, r3;
};

} // namespace ui

class JesiennaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit JesiennaEditor (JesiennaProcessor&);
    ~JesiennaEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int W = 1200, H = 800;
    void showTab (int i);
private:
    void timerCallback() override;
    void updateTuner();
    JesiennaProcessor& proc;
    ui::Look look;
    juce::Component content;
    std::unique_ptr<ui::RigPage> rig;
    std::unique_ptr<ui::BandPage> band;
    std::unique_ptr<ui::SpacePage> space;
    juce::OwnedArray<juce::TextButton> tabButtons;
    juce::ComboBox presetBox;
    juce::Label status;
    std::vector<float> tbuf; int tfill = 0;
    float tunerHz = 0, inLevel = 0, outL = 0, outR = 0, tunerCents = 0;
    juce::String tunerNote, chordText, barText;
    int tab = 0;
    struct Header : juce::Component { JesiennaEditor* ed = nullptr; void paint (juce::Graphics&) override; } header;
};
