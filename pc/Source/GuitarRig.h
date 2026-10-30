// Guitar chain: gate → compressor → overdrive pedal → amp (oversampled preamp, tone stack, power amp)
// → cabinet IR → microphone character → modulation → tempo-synced delay. Mono in, stereo out.
#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Assets.h"
#include "Effects.h"
#include "NamSlot.h"

namespace jw {

using Filter = juce::dsp::IIR::Filter<float>;
using Coefs  = juce::dsp::IIR::Coefficients<float>;
inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }

struct AmpModel
{
    const char* name;
    float preHp;        // Hz, tightness before the preamp
    float gainMin, gainMax;  // dB range of the gain knob
    float bias;         // asymmetry (even harmonics, "tube" warmth)
    float interLp;      // Hz, smoothing between gain stages
    float bassF, midF, trebleF, midQ;
    float power;        // power-amp drive
    float sag;          // how much loud playing compresses the amp
    float outTrim;      // dB
    int   shape;        // 0 tube, 1 hard-ish, 2 transistor
};

inline const std::array<AmpModel, 7>& ampModels()
{
    static const std::array<AmpModel, 7> m {{
        { "Czysty amerykański (Twin)",        80,  -6, 18, 0.05f, 9000, 100,  500, 3000, 0.7f, 0.4f, 0.10f, -6, 0 },
        { "Jazzowe kombo (ciepłe, ciemne)",   70,  -8, 14, 0.08f, 5500, 140,  450, 2200, 0.6f, 0.3f, 0.05f, -1, 0 },
        { "Tweed na lampach (brudny blues)",  90,   0, 30, 0.18f, 7000, 120,  700, 2800, 0.8f, 1.0f, 0.30f, -11, 0 },
        { "Brytyjski crunch (AC/Plexi)",     110,   4, 34, 0.12f, 7500, 110,  800, 3200, 0.9f, 0.9f, 0.20f, -6.5f, 0 },
        { "Brytyjski lead",                  140,  10, 42, 0.10f, 6500, 100,  750, 3000, 0.9f, 0.8f, 0.15f, -8, 1 },
        { "Nowoczesny high-gain",            180,  16, 50, 0.06f, 6000,  90,  650, 3400, 1.1f, 0.7f, 0.08f, -6, 1 },
        { "Tranzystorowy lo-fi (radio)",     250,   6, 36, 0.00f, 4000, 300, 1200, 2500, 0.7f, 0.6f, 0.00f, -8.5f, 2 },
    }};
    return m;
}

struct MicModel { const char* name; };
inline const std::array<const char*, 5>& micNames()
{
    static const std::array<const char*, 5> n {{ "Bez zmian", "Dynamiczny (jak SM57)", "Wstęgowy (ciepły)", "Pojemnościowy (powietrze)", "Z daleka (pokój)" }};
    return n;
}

struct RigParams
{
    float inGainDb = 0, gateDb = -70, comp = 0.2f;
    bool  odOn = false; int pedal = 0; float odDrive = 0.4f, odTone = 0.5f, odLevel = 0.5f, pedalTrimDb = 0;
    int   amp = 1; float ampTrimDb = 0;   // amp >= builtInAmps: a captured (NAM) amp
    bool  modOn = false, dlyOn = true; float ampGain = 0.35f, bass = 0.5f, mid = 0.5f, treble = 0.5f, presence = 0.5f, master = 0.6f;
    int   cab = 0; float cabMix = 1.0f; int mic = 1;
    int   modType = 0; float modRate = 0.3f, modDepth = 0.3f;
    float dlyMix = 0.15f, dlyFb = 0.3f, dlyTone = 0.5f; int dlyDiv = 1, dlyType = 0; double bpm = 120;
    bool  revOn = false; float revMix = 0.25f, revTone = 0.6f, revDwell = 0.3f;
    float level = 0.8f, pan = 0;
};

/** Fixed integer delay, used to keep the plugin's latency identical whichever amp/pedal is chosen. */
struct Pad
{
    void prepare (int maxLen) { buf.assign ((size_t) juce::jmax (1, maxLen + 1), 0.0f); w = 0; }
    void process (float* x, int n, int d)
    {
        if (d <= 0) return;
        const int L = (int) buf.size();
        for (int i = 0; i < n; ++i) { buf[(size_t) w] = x[i]; int r = w - d; if (r < 0) r += L; x[i] = buf[(size_t) r]; if (++w >= L) w = 0; }
    }
    std::vector<float> buf; int w = 0;
};

constexpr int builtInAmps = 7;

class GuitarRig
{
public:
    NamSlot ampNam, pedalNam;
    ReverbPedal reverb;

    void prepare (double sr, int maxBlock)
    {
        sampleRate = sr; blockSize = maxBlock;
        juce::dsp::ProcessSpec mono { sr, (juce::uint32) maxBlock, 1 };
        oversampler.initProcessing ((size_t) maxBlock);
        juce::dsp::ProcessSpec osSpec { sr * 4, (juce::uint32) maxBlock * 4, 1 };
        for (auto* f : { &preHp, &interLp, &coupling }) { f->prepare (osSpec); f->reset(); }
        for (auto* f : { &odHp, &odLp, &odTilt, &bassF, &midF, &trebleF, &presF, &mic1, &mic2, &mic3 }) { f->prepare (mono); f->reset(); }
        comp.prepare (mono); comp.setAttack (8); comp.setRelease (120);
        cabConv.prepare (mono); cabConv.reset();
        early.prepare (mono); early.setMaximumDelayInSamples ((int) (sr * 0.05));
        mono1.setSize (1, maxBlock); dryBuf.setSize (1, maxBlock); work.setSize (2, maxBlock);
        lastAmp = -1; lastMic = -1; gateEnv = 0; gateGain = 0; sag = 0;
        ampNam.prepare (sr, maxBlock); pedalNam.prepare (sr, maxBlock);
        modFx.prepare (sr, maxBlock); delayFx.prepare (sr, maxBlock); reverb.prepare (sr, maxBlock);
        namLat = std::abs (sr - 48000.0) > 1 ? NamSlot::prefill : 0;
        osLat = (int) std::ceil (oversampler.getLatencyInSamples());
        padPedal.prepare (namLat + 4); padAmp.prepare (juce::jmax (osLat, namLat) + 4);
        updateFilters ({}, true);
    }

    int latencySamples() const { return namLat + juce::jmax (osLat, namLat); }

    /** Load a cabinet IR (call off the audio thread). index < 0 = no cabinet. */
    void loadCab (const juce::File& f)
    {
        juce::AudioBuffer<float> b; double sr = 48000;
        if (f.existsAsFile() && readAudio (f, b, sr, 1.0))
        {
            juce::AudioBuffer<float> monoIr (1, b.getNumSamples()); monoIr.copyFrom (0, 0, b, 0, 0, b.getNumSamples());
            cabConv.loadImpulseResponse (std::move (monoIr), sr, juce::dsp::Convolution::Stereo::no,
                                         juce::dsp::Convolution::Trim::yes, juce::dsp::Convolution::Normalise::yes);
            hasCab = true;
        }
        else hasCab = false;
    }

    // tuner tap: last input samples (read from the editor)
    juce::AbstractFifo tunerFifo { 8192 };
    std::array<float, 8192> tunerBuf {};
    std::atomic<float> inputLevel { 0 };

    void process (const float* in, float* outL, float* outR, int n, const RigParams& p)
    {
        updateFilters (p, false);
        auto* x = mono1.getWritePointer (0);
        const float inG = dbToGain (p.inGainDb);
        float peak = 0;
        for (int i = 0; i < n; ++i) { x[i] = in[i] * inG; peak = juce::jmax (peak, std::abs (x[i])); }
        inputLevel.store (juce::jmax (peak, inputLevel.load() * 0.9f));
        pushTuner (x, n);

        // noise gate (smooth, so it does not chop sustain)
        const float thr = dbToGain (p.gateDb), att = std::exp (-1.0f / (0.001f * (float) sampleRate)), rel = std::exp (-1.0f / (0.08f * (float) sampleRate));
        for (int i = 0; i < n; ++i)
        {
            const float a = std::abs (x[i]); gateEnv = a > gateEnv ? a : gateEnv * 0.9995f + a * 0.0005f;
            const float target = (p.gateDb <= -89.0f || gateEnv > thr) ? 1.0f : 0.0f;
            gateGain = target > gateGain ? target + (gateGain - target) * att : target + (gateGain - target) * rel;
            x[i] *= gateGain;
        }

        juce::dsp::AudioBlock<float> mb (mono1); auto blk = mb.getSubBlock (0, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (blk);
        comp.setThreshold (-6.0f - p.comp * 30.0f); comp.setRatio (1.0f + p.comp * 7.0f);
        if (p.comp > 0.01f) { comp.process (ctx); for (int i = 0; i < n; ++i) x[i] *= dbToGain (p.comp * 8.0f); }

        // overdrive pedal: clipped mid-hump on top of the dry signal, then tone and level
        bool pedalDone = false;
        if (p.odOn && p.pedal > 0)
        {
            // captured pedal: drive = input level, level = output; tone = gentle tilt after it
            const float gIn = dbToGain ((p.odDrive - 0.4f) * 24.0f);
            for (int i = 0; i < n; ++i) x[i] *= gIn;
            if (pedalNam.process (x, n))
            {
                const float gOut = dbToGain (p.pedalTrimDb + (p.odLevel - 0.5f) * 24.0f) / juce::jmax (0.25f, gIn * 0.5f + 0.5f);
                for (int i = 0; i < n; ++i) x[i] = odTilt.processSample (x[i]) * gOut;
                pedalDone = true;
            }
            else for (int i = 0; i < n; ++i) x[i] /= gIn;
        }
        if (! pedalDone) padPedal.process (x, n, namLat);
        if (p.odOn && ! pedalDone)
        {
            const float k = 1.0f + p.odDrive * 60.0f;
            for (int i = 0; i < n; ++i)
            {
                const float h = odHp.processSample (x[i]);
                const float d = std::tanh (h * k) / std::sqrt (k) * 2.0f;
                x[i] = odLp.processSample (x[i] * 0.6f + d) * (0.4f + p.odLevel * 1.6f);
            }
        }

        if (p.amp >= builtInAmps && ampNam.active())
            processCapturedAmp (x, n, p);
        else
            processBuiltInAmp (blk, x, n, p);

        // cabinet + microphone
        if (hasCab && p.cabMix > 0.001f)
        {
            auto* dry = dryBuf.getWritePointer (0); std::copy (x, x + n, dry);
            cabConv.process (ctx);
            for (int i = 0; i < n; ++i) x[i] = x[i] * p.cabMix * 1.4f + dry[i] * (1.0f - p.cabMix) * 0.25f;
        }
        afterAmp (x, n, outL, outR, p);
    }

private:
    void processCapturedAmp (float* x, int n, const RigParams& p)
    {
        const float gIn = dbToGain ((p.ampGain - 0.5f) * 30.0f);
        for (int i = 0; i < n; ++i) x[i] *= gIn;
        if (! ampNam.process (x, n)) return;
        padAmp.process (x, n, juce::jmax (osLat, namLat) - namLat);
        const float out = dbToGain (p.ampTrimDb) * (0.2f + p.master * 1.6f) / juce::jmax (0.3f, std::sqrt (gIn));
        for (int i = 0; i < n; ++i)
            x[i] = presF.processSample (trebleF.processSample (midF.processSample (bassF.processSample (x[i])))) * out;
    }

    void processBuiltInAmp (juce::dsp::AudioBlock<float>& blk, float* x, int n, const RigParams& p)
    {
        // ---- amp: oversampled preamp (two gain stages) ----
        const auto& m = ampModels()[(size_t) (p.amp >= builtInAmps ? 1 : juce::jlimit (0, builtInAmps - 1, p.amp))];
        const float g1 = dbToGain (juce::jmap (p.ampGain, m.gainMin, m.gainMax));
        auto up = oversampler.processSamplesUp (blk);
        auto* o = up.getChannelPointer (0); const int on = (int) up.getNumSamples();
        const float b = m.bias, nb = shape (b, m.shape);
        for (int i = 0; i < on; ++i)
        {
            float v = preHp.processSample (o[i]) * g1;
            v = shape (v + b, m.shape) - nb;                        // stage 1 (asymmetric = even harmonics)
            v = interLp.processSample (v);
            v = coupling.processSample (v) * (1.0f + p.ampGain * 3.0f);
            v = shape (v, m.shape);                                 // stage 2
            o[i] = v;
        }
        oversampler.processSamplesDown (blk);

        // tone stack + power amp with sag + presence + master
        for (int i = 0; i < n; ++i)
        {
            float v = trebleF.processSample (midF.processSample (bassF.processSample (x[i])));
            const float env = std::abs (v); sag = env > sag ? sag + (env - sag) * 0.01f : sag * 0.9995f;
            const float pg = m.power * (1.0f - m.sag * juce::jmin (1.0f, sag));
            v = std::tanh (v * (0.5f + pg)) / (0.5f + pg * 0.5f);
            v = presF.processSample (v);
            x[i] = v * dbToGain (m.outTrim) * (0.2f + p.master * 1.6f);
        }
        padAmp.process (x, n, juce::jmax (osLat, namLat) - osLat);
    }

    void afterAmp (float* x, int n, float* outL, float* outR, const RigParams& p)
    {
        for (int i = 0; i < n; ++i) x[i] = mic3.processSample (mic2.processSample (mic1.processSample (x[i])));
        if (p.mic == 4)   // "from a distance": two short early reflections
            for (int i = 0; i < n; ++i)
            {
                early.pushSample (0, x[i]);
                x[i] = x[i] * 0.7f + early.popSample (0, (float) (sampleRate * 0.009), false) * 0.35f + early.popSample (0, (float) (sampleRate * 0.017), true) * 0.25f;
            }

        // ---- stereo: modulation, delay, level, pan ----
        auto* L = work.getWritePointer (0); auto* R = work.getWritePointer (1);
        for (int i = 0; i < n; ++i) L[i] = R[i] = x[i];
        if (p.modOn) modFx.process (L, R, n, p.modType, p.modRate, p.modDepth);
        if (p.dlyOn && p.dlyMix > 0.001f) delayFx.process (L, R, n, p.dlyType, p.dlyMix, p.dlyFb, p.dlyTone, p.dlyDiv, p.bpm);
        if (p.revOn) reverb.process (L, R, n, p.revMix, p.revTone, p.revDwell);
        const float gl = p.level * std::cos ((p.pan + 1) * juce::MathConstants<float>::pi / 4) * 1.414f;
        const float gr = p.level * std::sin ((p.pan + 1) * juce::MathConstants<float>::pi / 4) * 1.414f;
        for (int i = 0; i < n; ++i) { outL[i] = L[i] * gl; outR[i] = R[i] * gr; }
    }

    static float shape (float v, int s)
    {
        if (s == 0) return v > 0 ? std::tanh (v) : std::tanh (v * 0.85f) / 0.85f * 0.9f;     // tube-ish, softer on one side
        if (s == 1) return v / std::pow (1.0f + std::pow (std::abs (v), 2.5f), 0.4f);       // tighter, more saturated
        const float c = juce::jlimit (-1.0f, 1.0f, v * 1.2f); return c - (std::abs (v) < 0.05f ? v * 0.5f : 0);   // transistor: harder, a little crossover grit
    }
    void pushTuner (const float* x, int n)
    {
        int s1, z1, s2, z2; tunerFifo.prepareToWrite (n, s1, z1, s2, z2);
        if (z1 + z2 < n) { tunerFifo.reset(); tunerFifo.prepareToWrite (n, s1, z1, s2, z2); }
        for (int i = 0; i < z1; ++i) tunerBuf[(size_t) (s1 + i)] = x[i];
        for (int i = 0; i < z2; ++i) tunerBuf[(size_t) (s2 + i)] = x[z1 + i];
        tunerFifo.finishedWrite (z1 + z2);
    }
    void updateFilters (const RigParams& p, bool force)
    {
        const bool captured = p.amp >= builtInAmps;
        static const AmpModel neutral { "", 80, 0, 0, 0, 8000, 120, 650, 3200, 0.7f, 0, 0, 0, 0 };
        const auto& m = captured ? neutral : ampModels()[(size_t) juce::jlimit (0, 6, p.amp)];
        const double os = sampleRate * 4;
        const double key = p.amp * 1e7 + p.mic * 1e6 + p.bass * 1e5 + p.mid * 1e4 + p.treble * 1e3 + p.presence * 100 + p.odTone * 10;
        if (! force && key == lastKey) return;
        lastKey = key;
        if (force || p.amp != lastAmp)
        {
            preHp.coefficients = Coefs::makeHighPass (os, m.preHp, 0.7f);
            interLp.coefficients = Coefs::makeLowPass (os, m.interLp, 0.7f);
            coupling.coefficients = Coefs::makeHighPass (os, 40.0f, 0.7f);
            lastAmp = p.amp;
        }
        bassF.coefficients   = Coefs::makeLowShelf  (sampleRate, m.bassF,   0.7f, dbToGain ((p.bass   - 0.5f) * 24));
        midF.coefficients    = Coefs::makePeakFilter(sampleRate, m.midF,    m.midQ, dbToGain ((p.mid - 0.5f) * 20));
        trebleF.coefficients = Coefs::makeHighShelf (sampleRate, m.trebleF, 0.7f, dbToGain ((p.treble - 0.5f) * 24));
        presF.coefficients   = Coefs::makeHighShelf (sampleRate, 5000.0f,   0.7f, dbToGain ((p.presence - 0.5f) * 14));
        odHp.coefficients = Coefs::makeHighPass (sampleRate, 720.0f);
        odLp.coefficients = Coefs::makeLowPass (sampleRate, 900.0f + p.odTone * 5000.0f);
        odTilt.coefficients = Coefs::makeHighShelf (sampleRate, 1800.0f, 0.7f, dbToGain ((p.odTone - 0.5f) * 14));
        switch (p.mic)
        {
            case 1:  mic1.coefficients = Coefs::makeHighPass (sampleRate, 90.0f); mic2.coefficients = Coefs::makePeakFilter (sampleRate, 5500.0f, 1.0f, dbToGain (4)); mic3.coefficients = Coefs::makeLowShelf (sampleRate, 200.0f, 0.7f, dbToGain (-2)); break;
            case 2:  mic1.coefficients = Coefs::makeHighShelf (sampleRate, 6000.0f, 0.7f, dbToGain (-6)); mic2.coefficients = Coefs::makeLowShelf (sampleRate, 150.0f, 0.7f, dbToGain (2.5f)); mic3.coefficients = Coefs::makePeakFilter (sampleRate, 3000.0f, 0.8f, dbToGain (-2)); break;
            case 3:  mic1.coefficients = Coefs::makeHighShelf (sampleRate, 9000.0f, 0.7f, dbToGain (3)); mic2.coefficients = Coefs::makePeakFilter (sampleRate, 3000.0f, 0.8f, dbToGain (1)); mic3.coefficients = Coefs::makeHighPass (sampleRate, 40.0f); break;
            case 4:  mic1.coefficients = Coefs::makeLowPass (sampleRate, 7000.0f); mic2.coefficients = Coefs::makeLowShelf (sampleRate, 200.0f, 0.7f, dbToGain (-3)); mic3.coefficients = Coefs::makePeakFilter (sampleRate, 800.0f, 0.7f, dbToGain (1.5f)); break;
            default: mic1.coefficients = mic2.coefficients = mic3.coefficients = Coefs::makeAllPass (sampleRate, 1000.0f); break;
        }
    }

    double sampleRate = 48000; int blockSize = 512;
    juce::dsp::Oversampling<float> oversampler { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    Filter preHp, interLp, coupling, odHp, odLp, odTilt, bassF, midF, trebleF, presF, mic1, mic2, mic3;
    juce::dsp::Compressor<float> comp;
    ModFx modFx; DelayFx delayFx;
    juce::dsp::Convolution cabConv;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> early;
    juce::AudioBuffer<float> mono1, dryBuf, work;
    std::atomic<bool> hasCab { false };
    Pad padPedal, padAmp; int osLat = 0, namLat = 0;
    double lastKey = -1; float gateEnv = 0, gateGain = 0, sag = 0; int lastAmp = -1, lastMic = -1;
};

} // namespace jw
