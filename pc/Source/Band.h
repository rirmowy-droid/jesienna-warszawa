// The backing band for jam sessions: drums, double bass and keys playing a chord progression,
// locked sample-accurately to the DAW's tempo, bar and meter (or to its own clock in the standalone app).
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <random>
#include "Assets.h"

namespace jw {

// ---------------- harmony ----------------
struct ChordType { const char* key; std::vector<int> iv, guide, col; };
inline const std::vector<ChordType>& chordTypes()
{
    static const std::vector<ChordType> t {
        {"maj7s11",{0,4,7,11,14,18},{4,11},{14,18}}, {"maj9",{0,4,7,11,14},{4,11},{14,21}}, {"maj7",{0,4,7,11},{4,11},{14,9}},
        {"mmaj7",{0,3,7,11},{3,11},{14,9}}, {"m7b5",{0,3,6,10},{3,10},{14,17}}, {"dim7",{0,3,6,9},{3,9},{14,11}},
        {"m11",{0,3,7,10,14,17},{3,10},{14,17}}, {"m9",{0,3,7,10,14},{3,10},{14,17}}, {"m7",{0,3,7,10},{3,10},{14,17}}, {"m6",{0,3,7,9},{3,9},{14,11}},
        {"7alt",{0,4,10,13,15,20},{4,10},{13,20}}, {"7b9",{0,4,7,10,13},{4,10},{13,20}}, {"7#9",{0,4,7,10,15},{4,10},{15,20}},
        {"7#11",{0,4,7,10,14,18},{4,10},{14,18}}, {"13",{0,4,7,10,14,21},{4,10},{21,14}}, {"9sus",{0,5,7,10,14},{5,10},{14,21}},
        {"9",{0,4,7,10,14},{4,10},{14,21}}, {"7",{0,4,7,10},{4,10},{14,21}}, {"6",{0,4,7,9},{4,9},{14,7}}, {"m",{0,3,7},{3,7},{14,10}}, {"",{0,4,7},{4,7},{14,9}},
    };
    return t;
}
struct Chord { int root = 0; int type = 0; float bars = 1; };

inline int qualityToType (juce::String q)
{
    q = q.replace ("♯", "#").replace ("♭", "b").replace ("ø", "m7b5").replace ("°", "dim").replace ("Δ", "maj7").replace ("-", "m").replace ("(maj7)", "maj7");
    if (q.startsWith ("ma7") || q.startsWith ("M7")) q = "maj7" + q.substring (3);
    static const std::vector<std::pair<const char*, const char*>> alias {
        {"maj7#11","maj7s11"},{"maj9","maj9"},{"maj7","maj7"},{"mmaj7","mmaj7"},{"m7b5","m7b5"},{"dim7","dim7"},{"dim","dim7"},
        {"m11","m11"},{"m9","m9"},{"m7","m7"},{"m6","m6"},{"7alt","7alt"},{"7b9","7b9"},{"7#9","7#9"},{"7#11","7#11"},{"13","13"},
        {"9sus","9sus"},{"7sus","9sus"},{"sus","9sus"},{"9","9"},{"7","7"},{"69","6"},{"6","6"},{"m","m"} };
    for (auto& [a, k] : alias)
        if (q.startsWith (a))
            for (int i = 0; i < (int) chordTypes().size(); ++i) if (juce::String (chordTypes()[(size_t) i].key) == k) return i;
    return (int) chordTypes().size() - 1;   // plain major triad
}
inline std::vector<Chord> parseProgression (const juce::String& text)
{
    std::vector<Chord> out;
    for (auto tok : juce::StringArray::fromTokens (text.replace ("|", " "), " ", ""))
    {
        tok = tok.trim(); if (tok.isEmpty()) continue;
        auto sym = tok.upToFirstOccurrenceOf (":", false, false), dur = tok.fromFirstOccurrenceOf (":", false, false);
        float bars = dur.isNotEmpty() ? juce::jmax (0.25f, dur.replace (",", ".").getFloatValue()) : 1.0f;
        if (sym.startsWith ("%")) { if (! out.empty()) { auto c = out.back(); c.bars = bars; out.push_back (c); } continue; }
        static const int roots[] = { 9, 11, 0, 2, 4, 5, 7, 11 };   // A B C D E F G H
        const juce::juce_wchar l = sym[0]; if (l < 'A' || l > 'H') continue;
        int root = roots[l - 'A'], k = 1;
        if (sym.length() > 1 && (sym[1] == '#' || sym[1] == 0x266F)) { root++; k = 2; }
        else if (sym.length() > 1 && (sym[1] == 'b' || sym[1] == 0x266D)) { root--; k = 2; }
        out.push_back ({ (root + 12) % 12, qualityToType (sym.substring (k)), bars });
    }
    return out;
}

struct Song { const char* name; int key; const char* meter; float tempo; const char* chords; int style; };   // style: 0 swing, 1 ballad, 2 straight, 3 bossa
inline const std::vector<Song>& songs()
{
    static const std::vector<Song> s {
        {"Nocna mgła (ballada)", 2, "4/4", 64, "Dm9 Gm9 Ebmaj7#11 A7b9", 1},
        {"Praga nocą", 2, "4/4", 70, "Gm9 C9sus Fmaj9 Bbmaj7#11 Em7b5 A7b9 Dm9:2", 1},
        {"Frygijski dryf", 2, "4/4", 60, "Dm11:2 Ebmaj7#11:2", 1},
        {"Deszcz na Pradze (6/8)", 2, "6/8", 62, "Dm9 Fmaj9 Bbmaj7#11 A7b9", 1},
        {"Autumn Leaves (g-moll)", 7, "4/4", 120, "Cm7 F7 Bbmaj7 Ebmaj7 Am7b5 D7b9 Gm7:2 Cm7 F7 Bbmaj7 Ebmaj7 Am7b5 D7b9 Gm7:2 Am7b5 D7b9 Gm7:2 Cm7 F7 Bbmaj7 Ebmaj7 Am7b5 D7b9 Gm7:.5 C7:.5 Fm7:.5 Bb7:.5 Am7b5 D7b9 Gm7:2", 0},
        {"Blue Bossa", 0, "4/4", 130, "Cm7:2 Fm7:2 Dm7b5 G7b9 Cm7:2 Ebm7 Ab7 Dbmaj7:2 Dm7b5 G7b9 Cm7 Dm7b5:.5 G7b9:.5", 3},
        {"So What (modalnie)", 2, "4/4", 130, "Dm11:16 Ebm11:8 Dm11:8", 0},
        {"Take Five – vamp (5/4)", 3, "5/4", 160, "Ebm7 Bbm7 Ebm7 Bbm7", 0},
        {"All the Things You Are", 8, "4/4", 140, "Fm7 Bbm7 Eb7 Abmaj7 Dbmaj7 G7 Cmaj7:2 Cm7 Fm7 Bb7 Ebmaj7 Abmaj7 D7 Gmaj7:2 Am7 D7 Gmaj7:2 F#m7b5 B7 Emaj7 C7alt Fm7 Bbm7 Eb7 Abmaj7 Dbmaj7 Dbm7 Cm7 Bdim7 Bbm7 Eb7 Abmaj7:2", 0},
        {"Summertime (uproszczone)", 9, "4/4", 76, "Am6:4 Dm6:2 Am6:2 Bm7b5 E7b9 Am6 Dm6 Am6 E7b9 Am6:2", 1},
        {"Rhythm Changes (A)", 10, "4/4", 180, "Bbmaj7:.5 G7:.5 Cm7:.5 F7:.5 Dm7:.5 G7:.5 Cm7:.5 F7:.5 Fm7:.5 Bb7:.5 Ebmaj7:.5 Ab7:.5 Dm7:.5 G7:.5 Cm7:.5 F7:.5", 0},
        {"Jazz blues w F", 5, "4/4", 126, "F7 Bb7 F7 Cm7:.5 F7:.5 Bb7 Bdim7 F7 D7b9 Gm7 C7 F7:.5 D7:.5 Gm7:.5 C7:.5", 0},
        {"Molowy blues w C", 0, "4/4", 100, "Cm7:4 Fm7:2 Cm7:2 Ab7 G7alt Cm7 G7alt", 0},
        {"Wolny blues 12/8 w G", 7, "12/8", 58, "G7 C7 G7 G7 C7 C7 G7 E7 Am7 D7 G7 D7", 1},
        {"Blues shuffle w E (12/8)", 4, "12/8", 76, "E7 A7 E7 E7 A7 A7 E7 E7 B7 A7 E7 B7", 2},
        {"Prosty rock-blues w A (4/4)", 9, "4/4", 110, "A7 A7 A7 A7 D7 D7 A7 A7 E7 D7 A7 E7", 2},
    };
    return s;
}

// ---------------- samples ----------------
struct Sample { juce::AudioBuffer<float> buf; double sr = 44100; int onset = 0; float gain = 1; };
using SamplePtr = std::shared_ptr<Sample>;

struct Instrument
{
    std::map<int, std::map<juce::String, std::vector<SamplePtr>>> notes;   // midi -> layer -> round robins
    std::map<juce::String, std::vector<SamplePtr>> hits;                    // percussion name prefix -> round robins
    bool empty() const { return notes.empty() && hits.empty(); }
};

inline SamplePtr loadSample (const juce::File& f, double maxSec)
{
    auto s = std::make_shared<Sample>();
    if (! readAudio (f, s->buf, s->sr, maxSec)) return {};
    float pk = 0; for (int c = 0; c < s->buf.getNumChannels(); ++c) pk = juce::jmax (pk, s->buf.getMagnitude (c, 0, s->buf.getNumSamples()));
    for (int i = 0; i < s->buf.getNumSamples(); ++i) if (std::abs (s->buf.getSample (0, i)) > pk * 0.04f) { s->onset = juce::jmax (0, i - 40); break; }
    const int n = juce::jmin (s->buf.getNumSamples() - s->onset, (int) (0.8 * s->sr));
    double e = 0; for (int i = 0; i < n; ++i) { auto v = s->buf.getSample (0, s->onset + i); e += v * v; }
    const float rms = (float) std::sqrt (e / juce::jmax (1, n)) + 1e-6f;
    s->gain = rms;   // turned into a correction once the whole layer is known
    return s;
}

/** Loads one instrument from data/sample/<key> using data/sample/manifest.json (only the given range of notes). */
inline std::shared_ptr<Instrument> loadInstrument (const juce::String& key, int lo = 0, int hi = 127, double maxSec = 8.0)
{
    auto inst = std::make_shared<Instrument>();
    auto dir = dataDir().getChildFile ("sample");
    auto man = juce::JSON::parse (dir.getChildFile ("manifest.json"));
    auto entry = man[juce::Identifier (key)];
    if (entry.isVoid()) return inst;
    auto folder = dir.getChildFile (key);
    if (entry["type"].toString() == "percussion")
    {
        if (auto* arr = entry["sounds"].getArray())
            for (auto& v : *arr)
            {
                auto name = v.toString().upToLastOccurrenceOf ("_", false, false);    // ride_pp_3.mp3 -> ride_pp
                if (auto s = loadSample (folder.getChildFile (v.toString()), 6.0)) { s->gain = 1; inst->hits[name].push_back (s); }
            }
        return inst;
    }
    if (auto* layers = entry["layers"].getDynamicObject())
        for (auto& layer : layers->getProperties())
        {
            std::vector<SamplePtr> all;
            if (auto* notes = layer.value.getDynamicObject())
                for (auto& note : notes->getProperties())
                {
                    const int m = note.name.toString().getIntValue();
                    if (m < lo - 4 || m > hi + 4) continue;
                    if (auto* files = note.value.getArray())
                        for (auto& fv : *files)
                            if (auto s = loadSample (folder.getChildFile (fv.toString()), maxSec)) { inst->notes[m][layer.name.toString()].push_back (s); all.push_back (s); }
                }
            // even out loudness between notes of the layer (85 %), as in the phone version
            double mean = 0; for (auto& s : all) mean += 20 * std::log10 (s->gain); mean /= juce::jmax<size_t> (1, all.size());
            for (auto& s : all) s->gain = std::pow (10.0f, (float) ((mean - 20 * std::log10 (s->gain)) * 0.85 / 20.0));
        }
    // layers sit a few dB apart regardless of how they were recorded
    for (auto& [m, ls] : inst->notes) for (auto& [l, v] : ls) for (auto& s : v) s->gain *= l == "p" ? 0.55f : (l == "mf" ? 0.75f : 1.0f);
    return inst;
}

// ---------------- voices ----------------
struct Voice
{
    SamplePtr s; double pos = 0, rate = 1; float gain = 0, panL = 1, panR = 1; int startIn = 0; int releaseIn = -1; float env = 1, relCoef = 0.999f; bool releasing = false; bool active = false;
};

class Band
{
public:
    struct Params
    {
        bool on = true, sync = true; int song = 0, key = 0, style = -1, keysInst = 0;
        float tempo = 100, energy = 0.5f, level = 0.8f, drums = 0.8f, bass = 0.8f, keys = 0.7f, swing = 0.6f;
    };

    void prepare (double sr) { sampleRate = sr; for (auto& v : voices) v.active = false; }

    void setInstruments (std::shared_ptr<Instrument> kit, std::shared_ptr<Instrument> bass, std::shared_ptr<Instrument> keys)
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        kitI = std::move (kit); bassI = std::move (bass); keysI = std::move (keys);
    }

    std::atomic<int> uiBar { 0 }, uiChordRoot { -1 }, uiChordType { 0 }, uiIndex { -1 };

    /** The song's chords, transposed to the chosen key (0 = original). */
    static std::vector<Chord> progressionFor (int song, int key)
    {
        const auto& s = songs()[(size_t) juce::jlimit (0, (int) songs().size() - 1, song)];
        auto prog = parseProgression (s.chords);
        const int shift = key <= 0 ? 0 : ((key - 1) - s.key + 12) % 12;
        for (auto& c : prog) c.root = (c.root + shift) % 12;
        return prog;
    }

    void process (float* L, float* R, int n, const juce::AudioPlayHead::PositionInfo* host, const Params& p)
    {
        const juce::SpinLock::ScopedTryLockType sl (lock);
        if (! sl.isLocked()) return;
        if (! p.on) { render (L, R, n, p); return; }

        // ---- song & harmony (reparse only when the song changes) ----
        if (p.song != curSong || p.key != curKey)
        {
            curSong = p.song; curKey = p.key;
            const auto& s = songs()[(size_t) juce::jlimit (0, (int) songs().size() - 1, p.song)];
            prog = progressionFor (p.song, p.key);
            total = 0; for (auto& c : prog) total += c.bars;
            songMeter = s.meter; songTempo = s.tempo; songStyle = s.style;
        }
        const int style = p.style >= 0 ? p.style : songStyle;

        // ---- time base: the host's bars/beats when playing and synced, else our own clock ----
        double bpm = p.tempo > 0 ? p.tempo : songTempo; int num = 4, den = 4; double ppq = ownPpq, barStartPpq = -1; bool rolling = true;
        juce::String meterTxt = songMeter; num = meterTxt.upToFirstOccurrenceOf ("/", false, false).getIntValue(); den = meterTxt.fromFirstOccurrenceOf ("/", false, false).getIntValue();
        if (p.sync && host != nullptr && host->getIsPlaying())
        {
            if (auto b = host->getBpm()) bpm = *b;
            if (auto ts = host->getTimeSignature()) { num = ts->numerator; den = ts->denominator; }
            ppq = host->getPpqPosition().orFallback (0.0);
            barStartPpq = host->getPpqPositionOfLastBarStart().orFallback (0.0);
            if (! wasHostPlaying) { for (auto& v : voices) v.releaseIn = 0; }
            wasHostPlaying = true;
        }
        else if (p.sync && host != nullptr && ! host->getIsPlaying() && host->getBpm().hasValue() && ! isStandalone)
        {
            rolling = false; wasHostPlaying = false;        // DAW stopped → band waits
        }
        else { wasHostPlaying = false; ppq = ownPpq; barStartPpq = -1; }
        if (! rolling) { render (L, R, n, p); return; }

        const double barQuarters = num * 4.0 / juce::jmax (1, den);
        const int barEighths = juce::jmax (2, (int) std::round (barQuarters * 2));
        groups.clear();
        if (den == 8 && num % 3 == 0) for (int i = 0; i < num / 3; ++i) groups.push_back (3);
        else if (den == 8) { int left = num; while (left > 3) { groups.push_back (2); left -= 2; } groups.push_back (left); }
        else for (int i = 0; i < barEighths / 2; ++i) groups.push_back (2);
        if (barStartPpq < 0) barStartPpq = std::floor (ppq / barQuarters) * barQuarters;
        lastBpm = bpm;
        const double spq = sampleRate * 60.0 / bpm, eighthS = spq / 2;
        const double e0 = ppq * 2, e1 = (ppq + n / spq) * 2;
        for (double e = std::ceil (e0 - 1e-9); e < e1; e += 1)
        {
            const int off = (int) ((e - e0) * eighthS);
            const double inBar = e - barStartPpq * 2;
            const long long barIdx = (long long) std::floor (barStartPpq / barQuarters + std::floor (inBar / barEighths));
            int pos = (int) std::llround (std::fmod (std::fmod (inBar, barEighths) + barEighths, barEighths));
            step (barIdx, pos, barEighths, off, eighthS, p, style);
        }
        if (! (p.sync && host != nullptr && host->getIsPlaying())) ownPpq += n / spq;
        render (L, R, n, p);
    }

    bool isStandalone = false;
    double currentBpm() const { return lastBpm; }
    void resetClock() { ownPpq = 0; }

private:
    // --- where are we? ---
    int beatOf (int pos, int& sub, int& glen) const
    {
        int acc = 0; for (int b = 0; b < (int) groups.size(); ++b) { if (pos < acc + groups[(size_t) b]) { sub = pos - acc; glen = groups[(size_t) b]; return b; } acc += groups[(size_t) b]; }
        sub = 0; glen = 2; return 0;
    }
    const Chord& chordAt (double barsFromStart, int* indexOut = nullptr) const
    {
        static const Chord none {};
        if (prog.empty()) return none;
        double t = std::fmod (barsFromStart, (double) total); if (t < 0) t += total;
        double acc = 0;
        for (int i = 0; i < (int) prog.size(); ++i) { if (t < acc + prog[(size_t) i].bars - 1e-6) { if (indexOut) *indexOut = i; return prog[(size_t) i]; } acc += prog[(size_t) i].bars; }
        if (indexOut) *indexOut = (int) prog.size() - 1;
        return prog.back();
    }

    float rnd (float a, float b) { return a + (b - a) * dist (rng); }
    bool chance (float pr) { return dist (rng) < pr; }

    void step (long long bar, int pos, int barLen, int offset, double eighthS, const Params& p, int style)
    {
        int sub, glen; const int beat = beatOf (pos, sub, glen); const int nBeats = (int) groups.size();
        const bool onBeat = sub == 0, lastBeat = beat == nBeats - 1;
        const double barPos = (double) bar + (double) pos / barLen;
        int idx = 0; const Chord& c = chordAt (barPos, &idx);
        int idxPrev = -1; if (pos > 0 || bar > 0) chordAt ((double) bar + (double) (pos - 1) / barLen, &idxPrev);
        const bool change = idxPrev != idx || (pos == 0 && bar == 0);
        if (pos == 0) uiBar = (int) bar;
        if (change) { uiChordRoot = c.root; uiChordType = c.type; uiIndex = idx; }
        const float E = p.energy;
        // swing: second eighth of a duple beat comes late
        const int swingOff = (glen == 2 && sub == 1 && style != 2 && style != 3) ? (int) (eighthS * (0.12 + 0.2 * p.swing)) : 0;
        const int t = offset + swingOff + (int) (rnd (-0.0025f, 0.0025f) * (float) sampleRate);
        const bool endOfLoop = std::fmod (barPos + 1.0 / barLen, (double) total) < 1.0 / barLen + 1e-6;   // last eighth before the form repeats

        // ---------------- drums ----------------
        if (kitI && ! kitI->empty() && p.drums > 0.01f)
        {
            const float dg = p.drums;
            if (style == 2)   // straight: 8th hi-hat, kick 1 & 3, snare 2 & 4
            {
                hit ("hihat_zamk_p", t, dg * (onBeat ? 0.35f : 0.25f) * (0.7f + E * 0.5f), 0.25f);
                if (onBeat && (beat % 2 == 0)) hit ("stopa_f", t, dg * 0.6f * (0.7f + E * 0.4f), 0);
                if (onBeat && (beat % 2 == 1)) hit (E > 0.6f ? "werbel_f" : "werbel_mp", t, dg * 0.55f, -0.05f);
                if (! onBeat && E > 0.65f && chance (0.15f)) hit ("stopa_p", t, dg * 0.35f, 0);
            }
            else if (style == 3)   // bossa: rim pattern, soft kick, closed hat 8ths
            {
                hit ("hihat_zamk_p", t, dg * 0.2f, 0.25f);
                static const int rim[] = { 0, 3, 6 }; for (int r : rim) if (pos == r) hit ("werbel_obrecz", t, dg * 0.35f, -0.1f);
                if (onBeat && (beat % 2 == 0)) hit ("stopa_p", t, dg * 0.45f, 0);
                if (pos == barLen - 1) hit ("stopa_p", t, dg * 0.3f, 0);
            }
            else   // swing / ballad: ride spang-a-lang, hi-hat foot on 2 & 4, feathered kick, cross-stick, ghosts
            {
                const bool ballad = style == 1;
                const bool triple = glen == 3;
                bool ride = onBeat || (triple ? sub == 2 : (sub == 1 && beat % 2 == 1 && chance (0.85f)));
                if (ride) hit (E > 0.65f && chance (0.4f) ? "ride_mp" : "ride_pp", t, dg * (onBeat ? (beat % 2 ? 0.45f : 0.38f) : 0.28f) * (ballad ? 0.75f : 1.0f) * (0.75f + E * 0.4f), 0.2f);
                if (onBeat && beat % 2 == 1) hit ("hihat_noga", t, dg * 0.4f, 0.25f);
                if (onBeat && chance (0.2f + E * 0.35f)) hit ("stopa_p", t, dg * (0.22f + E * 0.15f), 0);
                if (onBeat && lastBeat && E < 0.55f && chance (0.7f)) hit ("werbel_obrecz", t, dg * 0.3f, -0.1f);
                if (! onBeat && E > 0.45f && chance (0.08f + E * 0.3f)) hit ("werbel_ghost", t, dg * 0.2f, -0.1f);
                if (! onBeat && E > 0.75f && chance (0.08f)) hit ("stopa_f", t, dg * 0.3f, 0);
                if (endOfLoop && E > 0.3f) { hit ("werbel_mp", t, dg * 0.35f, -0.1f); }
            }
        }

        // ---------------- bass ----------------
        if (bassI && ! bassI->empty() && p.bass > 0.01f && onBeat)
        {
            const bool walk = style == 0 ? true : (style == 1 ? E > 0.55f : false);
            int m = -1; float len = (float) glen;
            const int root = bassNote (c.root);
            if (style == 2) { m = root + (beat % 2 == 1 && chance (0.4f) ? 7 : 0); len = (float) glen * 0.9f; }
            else if (style == 3) { m = beat % 2 == 0 ? root : bassNote ((c.root + 7) % 12); len = (float) glen * 1.6f; }
            else if (change || beat == 0) m = root;
            else
            {
                int nIdx = 0; const Chord& nx = chordAt (barPos + (double) glen / barLen, &nIdx);
                if (nIdx != idx && walk) m = bassNote (nx.root) + (chance (0.5f) ? -1 : 1);                    // chromatic approach
                else if (walk) { const auto& ct = chordTypes()[(size_t) c.type].iv; m = root + ct[(size_t) juce::jlimit (0, (int) ct.size() - 1, (int) rnd (1, (float) ct.size()))] % 12; if (m > 52) m -= 12; }
                else if (beat * 2 == nBeats || (nBeats % 2 == 1 && beat == nBeats / 2 + 1)) m = bassNote ((c.root + 7) % 12);
                if (! walk) len = (float) glen * 2;
            }
            if (m > 0) note (*bassI, m, t, len * eighthS * 0.92, p.bass * (0.55f + E * 0.35f) * (beat == 0 ? 1.0f : 0.9f), E > 0.6f ? "mf" : "p", 0.0f, 0.06f);
        }

        // ---------------- keys ----------------
        if (keysI && ! keysI->empty() && p.keys > 0.01f)
        {
            float pr = 0;
            if (change) pr = 1;
            else if (onBeat) pr = 0.05f + E * 0.3f;
            else if (lastBeat && sub == glen - 1) pr = 0.1f + E * 0.25f;
            if (style == 2 && onBeat) pr = beat % 2 == 0 ? 0.9f : 0.3f;
            if (chance (pr))
            {
                const Chord& cc = (! onBeat && lastBeat) ? chordAt (barPos + 1.0 / barLen) : c;
                const auto vo = voicing (cc, E);
                const double dur = (change ? barLen : glen * 2) * eighthS * (E > 0.6f && ! change ? 0.4 : 0.95);
                for (size_t i = 0; i < vo.size(); ++i)
                    note (*keysI, vo[i], t + (int) (i * rnd (0, 0.004f) * sampleRate), dur, p.keys * (0.32f + E * 0.25f) * (change ? 1.0f : 0.85f), E > 0.65f ? "mf" : "p", -0.15f + 0.1f * (float) i, 0.25f);
            }
        }
    }

    static int bassNote (int pc) { int m = 36 + ((pc - 36) % 12 + 12) % 12; return m > 47 ? m - 12 : m; }
    std::vector<int> voicing (const Chord& c, float E)
    {
        const auto& ct = chordTypes()[(size_t) c.type];
        std::vector<int> ivs { ct.guide[0], ct.guide[1] };
        if (E > 0.4f) { ivs.push_back (ct.col[0]); ivs.push_back (ct.col[1]); } else { ivs.push_back (7); ivs.push_back (14); }
        double centre = 0; for (int m : prevVoicing) centre += m; centre /= juce::jmax<size_t> (1, prevVoicing.size());
        std::vector<int> out;
        for (int iv : ivs)
        {
            const int pcl = (c.root + iv) % 12; int best = -1;
            for (int m = 50; m <= 72; ++m) if (m % 12 == pcl && (best < 0 || std::abs (m - centre) < std::abs (best - centre))) best = m;
            if (best > 0 && std::find (out.begin(), out.end(), best) == out.end()) out.push_back (best);
        }
        std::sort (out.begin(), out.end()); prevVoicing = out; return out;
    }

    void hit (const char* name, int offset, float gain, float pan)
    {
        auto it = kitI->hits.find (name); if (it == kitI->hits.end() || it->second.empty()) return;
        auto s = it->second[(size_t) (rng() % it->second.size())];
        start (s, 1.0, offset, -1, gain * rnd (0.9f, 1.05f), pan, 0.08f);
    }
    void note (Instrument& in, int m, int offset, double durSamples, float vel, const juce::String& layer, float pan, float releaseSec)
    {
        if (in.notes.empty()) return;
        auto it = in.notes.lower_bound (m); int root;
        if (it == in.notes.end()) root = in.notes.rbegin()->first;
        else if (it == in.notes.begin()) root = it->first;
        else { auto pv = std::prev (it); root = (m - pv->first <= it->first - m) ? pv->first : it->first; }
        auto& layers = in.notes[root];
        auto lit = layers.find (layer); if (lit == layers.end()) lit = layers.begin();
        if (lit->second.empty()) return;
        auto s = lit->second[(size_t) (rng() % lit->second.size())];
        const double rate = std::pow (2.0, (m - root) / 12.0);
        start (s, rate, offset, (int) durSamples, vel * s->gain, pan, releaseSec);
    }
    void start (SamplePtr s, double rate, int offset, int dur, float gain, float pan, float releaseSec)
    {
        Voice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr) { v = &voices[(size_t) (rng() % voices.size())]; }
        v->s = std::move (s); v->pos = v->s->onset; v->rate = rate * v->s->sr / sampleRate; v->gain = gain;
        v->panL = std::cos ((pan + 1) * juce::MathConstants<float>::pi / 4) * 1.414f; v->panR = std::sin ((pan + 1) * juce::MathConstants<float>::pi / 4) * 1.414f;
        v->startIn = juce::jmax (0, offset); v->releaseIn = dur < 0 ? -1 : v->startIn + dur; v->env = 1; v->releasing = false;
        v->relCoef = std::exp (-1.0f / (juce::jmax (0.01f, releaseSec) * (float) sampleRate)); v->active = true;
    }
    void render (float* L, float* R, int n, const Params& p)
    {
        const float lvl = p.level;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            const auto& b = v.s->buf; const int len = b.getNumSamples(), ch = b.getNumChannels();
            const float* d0 = b.getReadPointer (0); const float* d1 = b.getReadPointer (ch > 1 ? 1 : 0);
            for (int i = 0; i < n; ++i)
            {
                if (v.startIn > 0) { --v.startIn; if (v.releaseIn > 0) --v.releaseIn; continue; }
                if (v.releaseIn == 0) v.releasing = true;
                if (v.releaseIn > 0) --v.releaseIn;
                if (v.releasing) { v.env *= v.relCoef; if (v.env < 0.0005f) { v.active = false; break; } }
                const int ip = (int) v.pos; if (ip + 1 >= len) { v.active = false; break; }
                const float fr = (float) (v.pos - ip);
                const float a = d0[ip] + (d0[ip + 1] - d0[ip]) * fr, c = d1[ip] + (d1[ip + 1] - d1[ip]) * fr;
                const float g = v.gain * v.env * lvl;
                L[i] += a * g * v.panL; R[i] += c * g * v.panR;
                v.pos += v.rate;
            }
        }
    }

    double sampleRate = 48000, ownPpq = 0, lastBpm = 100; bool wasHostPlaying = false;
    juce::SpinLock lock;
    std::shared_ptr<Instrument> kitI, bassI, keysI;
    std::array<Voice, 96> voices;
    std::vector<Chord> prog; float total = 1; int curSong = -1, curKey = -1, songStyle = 0; float songTempo = 100; juce::String songMeter = "4/4";
    std::vector<int> groups { 2, 2, 2, 2 }, prevVoicing { 53, 57, 60, 64 };
    std::mt19937 rng { 1234 }; std::uniform_real_distribution<float> dist { 0.0f, 1.0f };
};

} // namespace jw
