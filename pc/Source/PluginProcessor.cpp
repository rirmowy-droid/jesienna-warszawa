#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;
using namespace juce;

static StringArray names (const juce::Array<jw::IrEntry>& a, const String& none)
{
    StringArray s; if (none.isNotEmpty()) s.add (none);
    for (auto& e : a) s.add (e.name);
    if (s.isEmpty()) s.add ("(brak plików – zainstaluj dane)");
    return s;
}

APVTS::ParameterLayout JesiennaProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto F = [&] (const char* id, const char* name, float lo, float hi, float def, const char* unit = "", float skewMid = -1)
    {
        NormalisableRange<float> r (lo, hi, 0.0f); if (skewMid > lo && skewMid < hi) r.setSkewForCentre (skewMid);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto C = [&] (const char* id, const char* name, StringArray choices, int def) { p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, def)); };
    auto B = [&] (const char* id, const char* name, bool def) { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };

    // ---- guitar ----
    C ("inCh", "Wejście gitary", { "Lewy kanał (wejście 1)", "Prawy kanał (wejście 2)", "Suma L+P" }, 0);
    F ("inGain", "Wzmocnienie wejścia", -24, 24, 0, "dB");
    F ("gate", "Bramka szumów", -90, -30, -70, "dB");
    F ("comp", "Kompresor", 0, 1, 0.2f);
    B ("odOn", "Przester (kostka) wł.", false);
    F ("odDrive", "Przester – gain", 0, 1, 0.4f); F ("odTone", "Przester – barwa", 0, 1, 0.5f); F ("odLevel", "Przester – poziom", 0, 1, 0.5f);
    StringArray amps; for (auto& a : jw::ampModels()) amps.add (a.name);
    C ("amp", "Wzmacniacz", amps, 1);
    F ("ampGain", "Gain", 0, 1, 0.35f); F ("bass", "Bas", 0, 1, 0.5f); F ("mid", "Środek", 0, 1, 0.5f);
    F ("treble", "Góra", 0, 1, 0.5f); F ("presence", "Presence", 0, 1, 0.5f); F ("master", "Master wzm.", 0, 1, 0.6f);
    C ("cab", "Kolumna (IR)", names (jw::listCabs(), "Bez kolumny (DI)"), 1);
    F ("cabMix", "Kolumna – miks", 0, 1, 1);
    StringArray mics; for (auto* m : jw::micNames()) mics.add (m);
    C ("mic", "Mikrofon", mics, 1);
    C ("modType", "Modulacja", { "Brak", "Chorus", "Wibrato", "Tremolo" }, 0);
    F ("modRate", "Modulacja – szybkość", 0, 1, 0.3f); F ("modDepth", "Modulacja – głębokość", 0, 1, 0.3f);
    F ("dlyMix", "Delay – miks", 0, 1, 0.15f); F ("dlyFb", "Delay – powtórzenia", 0, 0.95f, 0.3f); F ("dlyTone", "Delay – barwa", 0, 1, 0.5f);
    C ("dlyDiv", "Delay – podział", { "ćwierćnuta", "ósemka z kropką", "ósemka", "ćwierćnuta triola", "półnuta", "szesnastka" }, 1);
    F ("gtrLevel", "Gitara – głośność", 0, 1.5f, 0.8f); F ("gtrPan", "Gitara – panorama", -1, 1, 0); F ("gtrSpace", "Gitara – przestrzeń", 0, 1, 0.35f);

    // ---- band ----
    B ("bandOn", "Zespół gra", false);
    B ("sync", "Synchronizuj z DAW", true);
    StringArray songNames; for (auto& s : jw::songs()) songNames.add (s.name);
    C ("song", "Utwór", songNames, 0);
    C ("key", "Tonacja", { "Oryginalna", "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/B", "H" }, 0);
    C ("style", "Styl", { "Z utworu", "Swing", "Ballada", "Prosto (rock/blues)", "Bossa" }, 0);
    C ("keysInst", "Klawisze", { "Rhodes", "Fortepian" }, 0);
    F ("tempo", "Tempo (bez DAW)", 40, 240, 90, "BPM");
    F ("energy", "Energia", 0, 1, 0.45f); F ("swing", "Swing", 0, 1, 0.6f);
    F ("drums", "Perkusja", 0, 1.5f, 0.8f); F ("bassVol", "Kontrabas", 0, 1.5f, 0.8f); F ("keysVol", "Klawisze – głośność", 0, 1.5f, 0.7f);
    F ("bandLevel", "Zespół – głośność", 0, 1.5f, 0.7f); F ("bandSpace", "Zespół – przestrzeń", 0, 1, 0.3f);

    // ---- space & output ----
    C ("room", "Przestrzeń", names (jw::listRooms(), ""), 0);
    F ("spaceMix", "Przestrzeń – poziom", 0, 1, 0.5f);
    C ("colour", "Barwa (mikrofon/kaseta)", names (jw::listColours(), "Brak"), 0);
    F ("colourMix", "Barwa – miks", 0, 1, 0.6f);
    F ("tape", "Taśma (wow, nasycenie, szum)", 0, 1, 0.15f);
    F ("out", "Wyjście", -24, 12, 0, "dB");
    B ("limit", "Limiter", true);
    return { p.begin(), p.end() };
}

JesiennaProcessor::JesiennaProcessor()
    : AudioProcessor (BusesProperties().withInput ("Gitara", AudioChannelSet::stereo(), true).withOutput ("Wyjście", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "JW", createLayout())
{
    dataFolder = jw::dataDir();
    cabs = jw::listCabs(); rooms = jw::listRooms(); colours = jw::listColours();
    band.isStandalone = wrapperType == wrapperType_Standalone;
    startTimerHz (10);
}

JesiennaProcessor::~JesiennaProcessor() { stopTimer(); pool.removeAllJobs (true, 10000); }

bool JesiennaProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != AudioChannelSet::stereo()) return false;
    const auto in = l.getMainInputChannelSet();
    return in == AudioChannelSet::mono() || in == AudioChannelSet::stereo();
}

void JesiennaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate; maxBlock = samplesPerBlock;
    rig.prepare (sr, samplesPerBlock);
    band.prepare (sr);
    for (auto* b : { &gtr, &bnd, &send, &wet, &col }) b->setSize (2, samplesPerBlock);
    dsp::ProcessSpec spec { sr, (uint32) samplesPerBlock, 2 };
    space.prepare (spec); colour.prepare (spec); limiter.prepare (spec);
    limiter.setThreshold (-0.5f); limiter.setRelease (120.0f);
    wowL.prepare ({ sr, (uint32) samplesPerBlock, 1 }); wowR.prepare ({ sr, (uint32) samplesPerBlock, 1 });
    auto lp = dsp::IIR::Coefficients<float>::makeLowPass (sr, 11000);
    tapeLpL.coefficients = lp; tapeLpR.coefficients = lp;
    hissHp.coefficients = dsp::IIR::Coefficients<float>::makeHighPass (sr, 2500);
    tapeLpL.reset(); tapeLpR.reset(); hissHp.reset();
    setLatencySamples (rig.latencySamples());
    lastCab = lastRoom = lastColour = -1;   // (re)load IRs at the new rate
    timerCallback();
}

void JesiennaProcessor::loadIr (dsp::Convolution& conv, const File& f, double maxSec, bool stereo)
{
    ++loadingJobs;
    pool.addJob ([this, &conv, f, maxSec, stereo]
    {
        AudioBuffer<float> b; double rate = 48000;
        if (jw::readAudio (f, b, rate, maxSec))
        {
            jw::trimLeadingSilence (b);
            // gentle fade-out so cut-off tails do not click
            const int fade = jmin (b.getNumSamples() / 4, (int) (0.05 * rate));
            for (int c = 0; c < b.getNumChannels(); ++c) b.applyGainRamp (c, b.getNumSamples() - fade, fade, 1.0f, 0.0f);
            if (stereo && b.getNumChannels() == 1) { b.setSize (2, b.getNumSamples(), true); b.copyFrom (1, 0, b, 0, 0, b.getNumSamples()); }
            if (! stereo && b.getNumChannels() > 1) b.setSize (1, b.getNumSamples(), true);
            conv.loadImpulseResponse (std::move (b), rate, stereo ? dsp::Convolution::Stereo::yes : dsp::Convolution::Stereo::no,
                                      dsp::Convolution::Trim::no, dsp::Convolution::Normalise::yes);
        }
        --loadingJobs;
    });
}

void JesiennaProcessor::timerCallback()
{
    if (! dataFolder.exists()) return;
    if (const int c = pi ("cab"); c != lastCab)
    {
        lastCab = c; ++loadingJobs;
        const File f = c >= 1 && c - 1 < cabs.size() ? cabs[c - 1].file : File();
        pool.addJob ([this, f] { rig.loadCab (f); --loadingJobs; });
    }
    if (const int r = pi ("room"); r != lastRoom && r < rooms.size()) { lastRoom = r; loadIr (space, rooms[r].file, 6.0, true); }
    if (const int c = pi ("colour"); c != lastColour) { lastColour = c; if (c >= 1 && c - 1 < colours.size()) loadIr (colour, colours[c - 1].file, 1.0, true); }

    const bool bandWanted = pf ("bandOn") > 0.5f;
    if (bandWanted && (! kitRequested || pi ("keysInst") != lastKeys))
    {
        const bool needKit = ! kitRequested; kitRequested = true; lastKeys = pi ("keysInst");
        const String keysName = lastKeys == 1 ? "fortepian" : "rhodes";
        ++loadingJobs;
        pool.addJob ([this, needKit, keysName]
        {
            if (needKit) { kit = jw::loadInstrument ("zestaw"); bass = jw::loadInstrument ("kontrabas", 28, 55, 6.0); }
            auto k = jw::loadInstrument (keysName, 48, 80, 8.0);
            band.setInstruments (kit, bass, k);
            keys = k; --loadingJobs;
        });
    }
}

String JesiennaProcessor::statusText() const
{
    if (! dataFolder.exists()) return "Nie znaleziono danych (IR i sampli). Uruchom instalator albo ustaw JESIENNA_DATA.";
    if (loadingJobs > 0) return "Wczytywanie…";
    return {};
}

void JesiennaProcessor::tape (float* L, float* R, int n)
{
    const float amt = pf ("tape");
    if (amt < 0.005f) return;
    const float depth = (float) sr * (0.0004f + amt * 0.0022f), fl = (float) sr * 0.00012f * amt;
    const float drive = 1.0f + amt * 2.5f, hiss = amt * 0.0025f;
    for (int i = 0; i < n; ++i)
    {
        wowPh += 0.55f / (float) sr; flutPh += 6.3f / (float) sr; if (wowPh > 1) wowPh -= 1; if (flutPh > 1) flutPh -= 1;
        const float d = 8 + depth * (0.5f + 0.5f * std::sin (MathConstants<float>::twoPi * wowPh)) + fl * std::sin (MathConstants<float>::twoPi * flutPh);
        wowL.pushSample (0, L[i]); wowR.pushSample (0, R[i]);
        float l = wowL.popSample (0, d), r = wowR.popSample (0, d);
        l = std::tanh (l * drive) / drive * (1 + amt * 0.4f); r = std::tanh (r * drive) / drive * (1 + amt * 0.4f);
        const float h = hissHp.processSample (noise.nextFloat() * 2 - 1) * hiss;
        L[i] = tapeLpL.processSample (l) + h; R[i] = tapeLpR.processSample (r) + h;
    }
}

void JesiennaProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    if (n > gtr.getNumSamples()) for (auto* b : { &gtr, &bnd, &send, &wet, &col }) b->setSize (2, n, false, false, true);
    const int inChs = getTotalNumInputChannels();

    // ---- guitar input ----
    const int ch = pi ("inCh");
    float* mono = col.getWritePointer (0);   // scratch
    if (inChs == 0) FloatVectorOperations::clear (mono, n);
    else if (ch == 2 && inChs > 1) { FloatVectorOperations::copy (mono, buffer.getReadPointer (0), n); FloatVectorOperations::add (mono, buffer.getReadPointer (1), n); FloatVectorOperations::multiply (mono, 0.5f, n); }
    else FloatVectorOperations::copy (mono, buffer.getReadPointer (jmin (ch == 1 ? 1 : 0, inChs - 1)), n);

    jw::RigParams rp;
    rp.inGainDb = pf ("inGain"); rp.gateDb = pf ("gate"); rp.comp = pf ("comp");
    rp.odOn = pf ("odOn") > 0.5f; rp.odDrive = pf ("odDrive"); rp.odTone = pf ("odTone"); rp.odLevel = pf ("odLevel");
    rp.amp = pi ("amp"); rp.ampGain = pf ("ampGain"); rp.bass = pf ("bass"); rp.mid = pf ("mid"); rp.treble = pf ("treble"); rp.presence = pf ("presence"); rp.master = pf ("master");
    rp.cab = pi ("cab"); rp.cabMix = rp.cab == 0 ? 0.0f : pf ("cabMix"); rp.mic = pi ("mic");
    rp.modType = pi ("modType"); rp.modRate = pf ("modRate"); rp.modDepth = pf ("modDepth");
    rp.dlyMix = pf ("dlyMix"); rp.dlyFb = pf ("dlyFb"); rp.dlyTone = pf ("dlyTone"); rp.dlyDiv = pi ("dlyDiv");
    rp.level = pf ("gtrLevel"); rp.pan = pf ("gtrPan");
    double bpm = pf ("tempo");
    auto* ph = getPlayHead(); Optional<AudioPlayHead::PositionInfo> pos; if (ph != nullptr) pos = ph->getPosition();
    if (pos.hasValue() && pos->getBpm().hasValue() && pf ("sync") > 0.5f) bpm = *pos->getBpm();
    rp.bpm = bpm;
    rig.process (mono, gtr.getWritePointer (0), gtr.getWritePointer (1), n, rp);

    // ---- band ----
    bnd.clear (0, n);
    jw::Band::Params bp;
    bp.on = pf ("bandOn") > 0.5f; bp.sync = pf ("sync") > 0.5f; bp.song = pi ("song"); bp.key = pi ("key"); bp.style = pi ("style") - 1;
    bp.tempo = pf ("tempo"); bp.energy = pf ("energy"); bp.swing = pf ("swing");
    bp.drums = pf ("drums"); bp.bass = pf ("bassVol"); bp.keys = pf ("keysVol"); bp.level = pf ("bandLevel");
    band.process (bnd.getWritePointer (0), bnd.getWritePointer (1), n, pos.hasValue() ? &*pos : nullptr, bp);

    // ---- space send (predelay via silence-trim is not needed: rooms are short-ish) ----
    const float gs = pf ("gtrSpace"), bs = pf ("bandSpace"), sm = pf ("spaceMix");
    for (int c = 0; c < 2; ++c)
    {
        send.clear (c, 0, n); send.addFrom (c, 0, gtr, c, 0, n, gs); send.addFrom (c, 0, bnd, c, 0, n, bs);
    }
    {
        dsp::AudioBlock<float> blk (send.getArrayOfWritePointers(), 2, (size_t) n);
        space.process (dsp::ProcessContextReplacing<float> (blk));
    }
    for (int c = 0; c < 2; ++c)
    {
        float* o = buffer.getWritePointer (c);
        FloatVectorOperations::copy (o, gtr.getReadPointer (c), n);
        FloatVectorOperations::add (o, bnd.getReadPointer (c), n);
        FloatVectorOperations::addWithMultiply (o, send.getReadPointer (c), sm * 0.9f, n);
    }
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);

    // ---- colour IR (mic / cassette / radio) ----
    if (pi ("colour") > 0)
    {
        const float m = pf ("colourMix");
        for (int c = 0; c < 2; ++c) col.copyFrom (c, 0, buffer, c, 0, n);
        dsp::AudioBlock<float> blk (col.getArrayOfWritePointers(), 2, (size_t) n);
        colour.process (dsp::ProcessContextReplacing<float> (blk));
        for (int c = 0; c < 2; ++c) { buffer.applyGain (c, 0, n, 1 - m); buffer.addFrom (c, 0, col, c, 0, n, m); }
    }
    tape (buffer.getWritePointer (0), buffer.getWritePointer (1), n);

    buffer.applyGain (0, n, Decibels::decibelsToGain (pf ("out")));
    if (pf ("limit") > 0.5f)
    {
        dsp::AudioBlock<float> blk (buffer.getArrayOfWritePointers(), 2, (size_t) n);
        limiter.process (dsp::ProcessContextReplacing<float> (blk));
    }
    outLevelL = jmax (outLevelL.load() * 0.8f, buffer.getMagnitude (0, 0, n));
    outLevelR = jmax (outLevelR.load() * 0.8f, buffer.getMagnitude (1, 0, n));
}

// ---------------- presets ----------------
const std::vector<JesiennaProcessor::Preset>& JesiennaProcessor::presets()
{
    static const std::vector<Preset> p {
        { "Jesienna noc – czysty jazz", { {"amp",1},{"ampGain",.3f},{"bass",.55f},{"treble",.4f},{"cab",1},{"mic",2},{"modType",0},{"dlyMix",.12f},{"dlyDiv",1},{"room",0},{"gtrSpace",.4f},{"tape",.15f},{"odOn",0} } },
        { "Mgła – ambient (chorus + długi delay)", { {"amp",0},{"ampGain",.25f},{"treble",.55f},{"cab",2},{"mic",3},{"modType",1},{"modRate",.2f},{"modDepth",.5f},{"dlyMix",.35f},{"dlyFb",.6f},{"dlyTone",.3f},{"dlyDiv",0},{"gtrSpace",.7f},{"spaceMix",.7f},{"room",22},{"odOn",0} } },
        { "Tweed blues", { {"amp",2},{"ampGain",.55f},{"mid",.6f},{"cab",4},{"mic",1},{"modType",0},{"dlyMix",.1f},{"dlyDiv",2},{"gtrSpace",.25f},{"odOn",0} } },
        { "Brytyjski crunch + kostka", { {"amp",3},{"ampGain",.45f},{"cab",9},{"mic",1},{"odOn",1},{"odDrive",.35f},{"odLevel",.6f},{"dlyMix",.12f},{"gtrSpace",.25f} } },
        { "Lead na solo", { {"amp",4},{"ampGain",.6f},{"mid",.65f},{"cab",21},{"mic",1},{"odOn",1},{"odDrive",.2f},{"odLevel",.7f},{"dlyMix",.22f},{"dlyFb",.4f},{"dlyDiv",1},{"gtrSpace",.35f} } },
        { "Ciężko – nowoczesny high-gain", { {"amp",5},{"ampGain",.55f},{"bass",.6f},{"mid",.4f},{"cab",16},{"mic",1},{"odOn",1},{"odDrive",.1f},{"odLevel",.8f},{"gate",-55},{"dlyMix",.05f},{"gtrSpace",.15f} } },
        { "Stare radio / kaseta", { {"amp",6},{"ampGain",.5f},{"cab",0},{"mic",4},{"modType",3},{"modRate",.4f},{"modDepth",.25f},{"tape",.6f},{"colour",1},{"colourMix",.7f} } },
        { "Tremolo w deszczu (surf/noir)", { {"amp",0},{"ampGain",.3f},{"cab",2},{"mic",2},{"modType",3},{"modRate",.45f},{"modDepth",.55f},{"dlyMix",.18f},{"gtrSpace",.6f},{"spaceMix",.6f} } },
    };
    return p;
}

void JesiennaProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= (int) presets().size()) return;
    currentPreset = index;
    for (auto& [id, v] : presets()[(size_t) index].values)
        if (auto* prm = dynamic_cast<RangedAudioParameter*> (apvts.getParameter (id)))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}
const String JesiennaProcessor::getProgramName (int index) { return index >= 0 && index < (int) presets().size() ? presets()[(size_t) index].name : String(); }

void JesiennaProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}
void JesiennaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* JesiennaProcessor::createEditor() { return new JesiennaEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new JesiennaProcessor(); }
