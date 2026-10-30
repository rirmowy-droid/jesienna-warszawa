#pragma once
#include "PluginProcessor.h"

class JwLook : public juce::LookAndFeel_V4
{
public:
    JwLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool hl, bool down) override;
    static juce::Colour amber() { return juce::Colour (0xffe0a24a); }
    static juce::Colour bg() { return juce::Colour (0xff15110f); }
    static juce::Colour panel() { return juce::Colour (0xff221b17); }
};

/** One page: titled groups of knobs / lists / switches built from parameter IDs. */
class ParamPage : public juce::Component
{
public:
    struct Group { juce::String title; juce::StringArray ids; };
    ParamPage (JesiennaProcessor&, std::vector<Group>);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    struct Ctl
    {
        juce::String id; std::unique_ptr<juce::Component> comp; juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
        int kind = 0;   // 0 knob, 1 combo, 2 toggle
    };
    std::vector<Group> groups;
    std::vector<std::vector<std::unique_ptr<Ctl>>> ctls;
    std::vector<juce::Rectangle<int>> boxes;
};

class JesiennaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit JesiennaEditor (JesiennaProcessor&);
    ~JesiennaEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void updateTuner();
    JesiennaProcessor& proc;
    JwLook look;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::ComboBox presetBox;
    juce::Label status;
    std::vector<float> tbuf; int tfill = 0;
    float tunerHz = 0, inLevel = 0, outL = 0, outR = 0;
    juce::String tunerNote, chordText, barText;
    float tunerCents = 0;
};
