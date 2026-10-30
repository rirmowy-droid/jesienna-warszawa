#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;
using namespace juce;

using jw::U;

static StringArray names (const juce::Array<jw::IrEntry>& a, const String& none)
{
    StringArray s; if (none.isNotEmpty()) s.add (none);
    for (auto& e : a) s.add (e.name);
    if (s.isEmpty()) s.add (U ("(brak plików – zainstaluj dane)"));
    return s;
}

StringArray JesiennaProcessor::ampChoices()
{
    StringArray s; for (auto& a : jw::ampModels()) s.add (U (a.name) + U (" · model"));
    for (auto& n : jw::listNam ("amp")) s.add (n.name);
    return s;
}
StringArray JesiennaProcessor::pedalChoices()
{
    StringArray s { U ("Jesienny overdrive (wbudowany)") };
    for (auto& n : jw::listNam ("pedal")) s.add (n.name);
    return s;
}

APVTS::ParameterLayout JesiennaProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    // value → text, short and readable
    auto pct = [] (float v, int) { return String (juce::roundToInt (v * 100)) + " %"; };
    auto db  = [] (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; };
    auto pan = [] (float v, int) { const int k = juce::roundToInt (v * 100); return k == 0 ? U ("środek") : (k < 0 ? "L " + String (-k) : "P " + String (k)); };
    auto bpm = [] (float v, int) { return String (juce::roundToInt (v)) + " BPM"; };
    auto ten = [] (float v, int) { return String (v * 10.0f, 1); };          // amp-style 0–10
    using Fmt = std::function<String (float, int)>;
    auto F = [&] (const char* id, const char* name, float lo, float hi, float def, Fmt fmt)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, U (name), NormalisableRange<float> (lo, hi), def,
                                                            AudioParameterFloatAttributes().withStringFromValueFunction (fmt)));
    };
    auto C = [&] (const char* id, const char* name, StringArray choices, int def) { p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, U (name), choices, def)); };
    auto B = [&] (const char* id, const char* name, bool def) { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, U (name), def)); };

    // ---- guitar ----
    C ("inCh", "Wejście gitary", U ({ "Lewy kanał (wejście 1)", "Prawy kanał (wejście 2)", "Suma L+P" }), 0);
    F ("inGain", "Wzmocnienie wejścia", -24, 24, 0, db);
    F ("gate", "Bramka szumów", -90, -30, -70, [] (float v, int) { return v <= -89 ? U ("wył.") : String (juce::roundToInt (v)) + " dB"; });
    F ("comp", "Kompresor", 0, 1, 0.2f, pct);
    B ("odOn", "Przester włączony", false);
    C ("pedal", "Kostka przesteru", pedalChoices(), 0);
    F ("odDrive", "Przester – drive", 0, 1, 0.4f, ten); F ("odTone", "Przester – ton", 0, 1, 0.5f, ten); F ("odLevel", "Przester – poziom", 0, 1, 0.5f, ten);
    C ("amp", "Wzmacniacz", ampChoices(), jw::builtInAmps);   // first captured amp by default
    F ("ampGain", "Gain", 0, 1, 0.5f, ten); F ("bass", "Bas", 0, 1, 0.5f, ten); F ("mid", "Środek", 0, 1, 0.5f, ten);
    F ("treble", "Góra", 0, 1, 0.5f, ten); F ("presence", "Presence", 0, 1, 0.5f, ten); F ("master", "Master", 0, 1, 0.6f, ten);
    C ("cab", "Kolumna", names (jw::listCabs(), U ("Bez kolumny (DI)")), 5);
    F ("cabMix", "Kolumna – miks", 0, 1, 1, pct);
    StringArray mics; for (auto* m : jw::micNames()) mics.add (U (m));
    C ("mic", "Mikrofon", mics, 1);
    B ("modOn", "Modulacja włączona", false);
    C ("modType", "Modulacja – rodzaj", U ({ "Chorus", "Wibrato", "Tremolo" }), 0);
    F ("modRate", "Modulacja – szybkość", 0, 1, 0.3f, ten); F ("modDepth", "Modulacja – głębokość", 0, 1, 0.3f, ten);
    B ("dlyOn", "Delay włączony", true);
    F ("dlyMix", "Delay – miks", 0, 1, 0.15f, pct); F ("dlyFb", "Delay – powtórzenia", 0, 0.95f, 0.3f, pct); F ("dlyTone", "Delay – barwa", 0, 1, 0.5f, ten);
    C ("dlyDiv", "Delay – podział", U ({ "ćwierćnuta", "ósemka z kropką", "ósemka", "triola ćwierćnutowa", "półnuta", "szesnastka" }), 1);
    F ("gtrLevel", "Gitara – głośność", 0, 1.5f, 0.8f, pct); F ("gtrPan", "Gitara – panorama", -1, 1, 0, pan); F ("gtrSpace", "Gitara – przestrzeń", 0, 1, 0.35f, pct);

    // ---- band ----
    B ("bandOn", "Zespół gra", false);
    B ("sync", "Synchronizuj z DAW", true);
    StringArray songNames; for (auto& s : jw::songs()) songNames.add (U (s.name));
    C ("song", "Utwór", songNames, 0);
    C ("key", "Tonacja", U ({ "Oryginalna", "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/B", "H" }), 0);
    C ("style", "Styl", U ({ "Z utworu", "Swing", "Ballada", "Prosto (rock/blues)", "Bossa" }), 0);
    C ("keysInst", "Klawisze", U ({ "Rhodes", "Fortepian" }), 0);
    F ("tempo", "Tempo (bez DAW)", 40, 240, 90, bpm);
    F ("energy", "Energia", 0, 1, 0.45f, pct); F ("swing", "Swing", 0, 1, 0.6f, pct);
    F ("drums", "Perkusja", 0, 1.5f, 0.8f, pct); F ("bassVol", "Kontrabas", 0, 1.5f, 0.8f, pct); F ("keysVol", "Klawisze – głośność", 0, 1.5f, 0.7f, pct);
    F ("bandLevel", "Zespół – głośność", 0, 1.5f, 0.7f, pct); F ("bandSpace", "Zespół – przestrzeń", 0, 1, 0.3f, pct);

    // ---- space & output ----
    C ("room", "Przestrzeń", names (jw::listRooms(), ""), 0);
    F ("spaceMix", "Przestrzeń – poziom", 0, 1, 0.5f, pct);
    C ("colour", "Barwa", names (jw::listColours(), U ("Brak")), 0);
    F ("colourMix", "Barwa – miks", 0, 1, 0.6f, pct);
    F ("tape", "Taśma", 0, 1, 0.15f, pct);
    F ("out", "Wyjście", -24, 12, 0, db);
    B ("limit", "Limiter", true);
    return { p.begin(), p.end() };
}

JesiennaProcessor::JesiennaProcessor()
    : AudioProcessor (BusesProperties().withInput ("Gitara", AudioChannelSet::stereo(), true).withOutput ("Wyjście", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "JW", createLayout())
{
    dataFolder = jw::dataDir();
    cabs = jw::listCabs(); rooms = jw::listRooms(); colours = jw::listColours();
    namAmps = jw::listNam ("amp"); namPedals = jw::listNam ("pedal");
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
    lastCab = lastRoom = lastColour = -1;   // (re)load IRs at the new rate (captured amps re-prepare themselves)
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
    if (const int a = pi ("amp") - jw::builtInAmps; a != lastNamAmp)
    {
        lastNamAmp = a; ++loadingJobs;
        const File f = a >= 0 && a < namAmps.size() ? namAmps[a].file : File();
        pool.addJob ([this, f] { rig.ampNam.load (f); --loadingJobs; });
    }
    if (const int pd = pi ("pedal") - 1; pd != lastNamPedal)
    {
        lastNamPedal = pd; ++loadingJobs;
        const File f = pd >= 0 && pd < namPedals.size() ? namPedals[pd].file : File();
        pool.addJob ([this, f] { rig.pedalNam.load (f); --loadingJobs; });
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
    if (! dataFolder.exists()) return U ("Nie znaleziono danych (IR i sampli). Uruchom instalator albo ustaw JESIENNA_DATA.");
    if (loadingJobs > 0) return U ("Wczytywanie…");
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
    if (n > gtr.getNumSamples()) for (auto* b : { &gtr, &bnd, &send, &wet, &col }) b->setSize (2, n, false, false, true);   // should not happen
    const int inChs = getTotalNumInputChannels();

    // ---- guitar input ----
    const int ch = pi ("inCh");
    float* mono = col.getWritePointer (0);   // scratch
    if (inChs == 0) FloatVectorOperations::clear (mono, n);
    else if (ch == 2 && inChs > 1) { FloatVectorOperations::copy (mono, buffer.getReadPointer (0), n); FloatVectorOperations::add (mono, buffer.getReadPointer (1), n); FloatVectorOperations::multiply (mono, 0.5f, n); }
    else FloatVectorOperations::copy (mono, buffer.getReadPointer (jmin (ch == 1 ? 1 : 0, inChs - 1)), n);

    jw::RigParams rp;
    rp.inGainDb = pf ("inGain"); rp.gateDb = pf ("gate"); rp.comp = pf ("comp");
    rp.odOn = pf ("odOn") > 0.5f; rp.pedal = pi ("pedal");
    // stored trims level the bundled profiles; the player's own files are levelled from a measurement made when loading
    auto trimOf = [] (const juce::Array<jw::NamEntry>& list, int i, const jw::NamSlot& slot, float targetDb)
    {
        if (i < 0 || i >= list.size()) return 0.0f;
        const float t = list.getReference (i).trimDb;
        return std::isnan (t) ? targetDb - slot.measuredDb.load() : t;
    };
    rp.pedalTrimDb = trimOf (namPedals, rp.pedal - 1, rig.pedalNam, jw::NamSlot::testInputDb);
    rp.ampTrimDb = trimOf (namAmps, pi ("amp") - jw::builtInAmps, rig.ampNam, -10.0f);
    rp.modOn = pf ("modOn") > 0.5f; rp.dlyOn = pf ("dlyOn") > 0.5f; rp.odDrive = pf ("odDrive"); rp.odTone = pf ("odTone"); rp.odLevel = pf ("odLevel");
    rp.amp = pi ("amp"); rp.ampGain = pf ("ampGain"); rp.bass = pf ("bass"); rp.mid = pf ("mid"); rp.treble = pf ("treble"); rp.presence = pf ("presence"); rp.master = pf ("master");
    rp.cab = pi ("cab"); rp.cabMix = rp.cab == 0 ? 0.0f : pf ("cabMix"); rp.mic = pi ("mic");
    rp.modType = pi ("modType") + 1; rp.modRate = pf ("modRate"); rp.modDepth = pf ("modDepth");
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
        { "Jesienna noc – czysty jazz", { {"amp",7},{"ampGain",.45f},{"bass",.55f},{"treble",.45f},{"cab",2},{"mic",2},{"dlyOn",1},{"dlyMix",.12f},{"gtrSpace",.4f} } },
        { "Mgła – ambient (chorus i długi delay)", { {"amp",9},{"ampGain",.4f},{"treble",.55f},{"cab",2},{"mic",3},{"modOn",1},{"modType",0},{"modRate",.2f},{"modDepth",.5f},{"dlyOn",1},{"dlyMix",.35f},{"dlyFb",.6f},{"dlyTone",.3f},{"dlyDiv",0},{"gtrSpace",.7f},{"spaceMix",.7f},{"room",22} } },
        { "Tremolo w deszczu", { {"amp",8},{"ampGain",.45f},{"cab",2},{"mic",2},{"modOn",1},{"modType",2},{"modRate",.45f},{"modDepth",.55f},{"dlyOn",1},{"dlyMix",.18f},{"gtrSpace",.6f},{"spaceMix",.6f} } },
        { "Tweed blues", { {"amp",18},{"ampGain",.5f},{"mid",.6f},{"cab",4},{"mic",1},{"dlyOn",1},{"dlyMix",.1f},{"dlyDiv",2},{"gtrSpace",.25f} } },
        { "Brytyjski crunch + Tube Screamer", { {"amp",23},{"ampGain",.5f},{"cab",9},{"mic",1},{"odOn",1},{"pedal",1},{"odDrive",.3f},{"odLevel",.6f},{"dlyOn",1},{"dlyMix",.12f},{"gtrSpace",.25f} } },
        { "Lead na solo", { {"amp",31},{"ampGain",.5f},{"mid",.65f},{"cab",21},{"mic",1},{"odOn",1},{"pedal",2},{"odDrive",.25f},{"odLevel",.6f},{"dlyOn",1},{"dlyMix",.22f},{"dlyFb",.4f},{"dlyDiv",1},{"gtrSpace",.35f} } },
        { "Ciężko – 5150", { {"amp",35},{"ampGain",.5f},{"bass",.6f},{"mid",.45f},{"cab",16},{"mic",1},{"gate",-55},{"dlyOn",0},{"gtrSpace",.15f} } },
        { "Stare radio / kaseta", { {"amp",6},{"ampGain",.5f},{"cab",0},{"mic",4},{"modOn",1},{"modType",2},{"modRate",.4f},{"modDepth",.25f},{"tape",.6f},{"colour",1},{"colourMix",.7f} } },
    };
    return p;
}

void JesiennaProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= (int) presets().size()) return;
    currentPreset = index;
    static const char* guitarIds[] = { "inGain","gate","comp","odOn","pedal","odDrive","odTone","odLevel","amp","ampGain","bass","mid","treble","presence","master",
                                       "cab","cabMix","mic","modOn","modType","modRate","modDepth","dlyOn","dlyMix","dlyFb","dlyTone","dlyDiv","gtrSpace","tape","colour" };
    for (auto* id : guitarIds)
        if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->getDefaultValue());
    for (auto& [id, v] : presets()[(size_t) index].values)
        if (auto* prm = dynamic_cast<RangedAudioParameter*> (apvts.getParameter (id)))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}
const String JesiennaProcessor::getProgramName (int index) { return index >= 0 && index < (int) presets().size() ? U (presets()[(size_t) index].name) : String(); }

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
