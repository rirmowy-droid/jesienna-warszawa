#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "GuitarRig.h"
#include "Band.h"

class JesiennaProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    JesiennaProcessor();
    ~JesiennaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jesienna Warszawa"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    jw::GuitarRig rig;
    jw::Band band;
    juce::Array<jw::IrEntry> cabs, rooms, colours;
    juce::Array<jw::NamEntry> namAmps, namPedals;
    static juce::StringArray ampChoices(), pedalChoices();
    juce::File dataFolder;
    std::atomic<float> outLevelL { 0 }, outLevelR { 0 }, gainReduction { 0 };
    std::atomic<int> loadingJobs { 0 };
    juce::String statusText() const;

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();

private:
    void timerCallback() override;
    float pf (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    int pi (const char* id) const { return (int) std::round (apvts.getRawParameterValue (id)->load()); }

    void loadIr (juce::dsp::Convolution& conv, const juce::File& f, double maxSec, bool stereo);
    void tape (float* L, float* R, int n);

    double sr = 48000; int maxBlock = 512;
    juce::AudioBuffer<float> gtr, bnd, send, wet, col;
    juce::dsp::Convolution space, colour;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wowL { 9600 }, wowR { 9600 };
    juce::dsp::IIR::Filter<float> tapeLpL, tapeLpR, hissHp;
    float wowPh = 0, flutPh = 0; juce::Random noise;
    juce::ThreadPool pool { 2 };
    int lastCab = -1, lastRoom = -1, lastColour = -1, lastKeys = -1, lastNamAmp = -99, lastNamPedal = -99, lastRevType = -1, revSettle = 0; float lastRevDecay = -1; bool kitRequested = false;
    std::shared_ptr<jw::Instrument> kit, bass, keys;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JesiennaProcessor)
};
