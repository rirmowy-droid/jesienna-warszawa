// Time-based guitar effects modelled on classic units (they cannot be captured as IR/NAM because they move in time),
// plus a reverb pedal that plays real recorded springs, plates and rooms.
#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Assets.h"

namespace jw {

inline float triLfo (float ph) { return ph < 0.5f ? ph * 4 - 1 : 3 - ph * 4; }   // -1..1

// ============================ modulation ============================
// 0 chorus (BBD, CE-2 style, stereo), 1 vibrato (pitch, Magnatone-style), 2 opto tremolo (Fender blackface),
// 3 harmonic tremolo (Fender brownface), 4 Uni-Vibe (four phase-shift stages driven by a lamp)
inline juce::StringArray modTypeNames()
{
    return { juce::String::fromUTF8 ("Chorus (jak Boss CE-2)"), juce::String::fromUTF8 ("Wibrato (jak Magnatone)"), juce::String::fromUTF8 ("Tremolo (jak Fender Deluxe)"),
             juce::String::fromUTF8 ("Tremolo harmoniczne (Brownface)"), juce::String::fromUTF8 ("Uni-Vibe") };
}

class ModFx
{
public:
    void prepare (double rate, int maxBlock)
    {
        sr = rate;
        juce::dsp::ProcessSpec mono { sr, (juce::uint32) maxBlock, 1 };
        for (auto* d : { &dl, &dr }) { d->prepare (mono); d->setMaximumDelayInSamples ((int) (sr * 0.04)); d->reset(); }
        for (auto* f : { &bbdL, &bbdR }) { f->prepare (mono); f->coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 5200.0f, 0.6f); f->reset(); }
        for (auto* f : { &lowL, &lowR }) { f->prepare (mono); f->coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 750.0f, 0.6f); f->reset(); }
        ph = 0; opto = 0; for (auto& s : ap) s = {};
    }

    void process (float* L, float* R, int n, int type, float rateKnob, float depth)
    {
        const float fs = (float) sr, two = juce::MathConstants<float>::twoPi;
        switch (type)
        {
            case 0:   // chorus: one bucket-brigade voice, triangle LFO, darker wet, left/right LFOs 90° apart
            {
                const float hz = 0.15f + rateKnob * rateKnob * 4.5f, base = 0.0065f * fs, dep = (0.0004f + depth * 0.0034f) * fs;
                for (int i = 0; i < n; ++i)
                {
                    adv (hz);
                    dl.pushSample (0, L[i]); dr.pushSample (0, R[i]);
                    const float wl = bbdL.processSample (dl.popSample (0, base + dep * (0.5f + 0.5f * triLfo (ph))));
                    const float wr = bbdR.processSample (dr.popSample (0, base + dep * (0.5f + 0.5f * triLfo (std::fmod (ph + 0.25f, 1.0f)))));
                    L[i] = 0.75f * L[i] + 0.8f * softBbd (wl); R[i] = 0.75f * R[i] + 0.8f * softBbd (wr);
                }
                break;
            }
            case 1:   // true vibrato: only the delayed (pitch-wobbling) signal is heard
            {
                const float hz = 2.5f + rateKnob * 6.0f, dep = (0.0003f + depth * 0.0028f) * fs;
                for (int i = 0; i < n; ++i)
                {
                    adv (hz);
                    const float m = 0.5f * (L[i] + R[i]); dl.pushSample (0, m);
                    L[i] = R[i] = dl.popSample (0, 2.0f + dep * (1.0f + std::sin (two * ph)));
                }
                break;
            }
            case 2:   // opto tremolo: an LFO-lit lamp on a light cell – the cell reacts fast to light, slowly to dark
            {
                const float hz = 1.5f + rateKnob * 8.5f;
                const float up = std::exp (-1.0f / (0.003f * fs)), down = std::exp (-1.0f / (0.022f * fs));
                for (int i = 0; i < n; ++i)
                {
                    adv (hz);
                    const float light = std::pow (0.5f + 0.5f * std::sin (two * ph), 1.6f);
                    opto = light > opto ? light + (opto - light) * up : light + (opto - light) * down;
                    const float g = 1.0f - depth * 0.95f * opto;
                    L[i] *= g; R[i] *= g;
                }
                break;
            }
            case 3:   // harmonic tremolo: bass and treble pulse in opposite phase
            {
                const float hz = 1.5f + rateKnob * 7.0f;
                for (int i = 0; i < n; ++i)
                {
                    adv (hz);
                    const float s = std::sin (two * ph), gl = 1.0f - depth * 0.9f * (0.5f + 0.5f * s), gh = 1.0f - depth * 0.9f * (0.5f - 0.5f * s);
                    const float ll = lowL.processSample (L[i]), lr = lowR.processSample (R[i]);
                    L[i] = ll * gl + (L[i] - ll) * gh; R[i] = lr * gl + (R[i] - lr) * gh;
                }
                break;
            }
            default:  // Uni-Vibe: four first-order phase shifters with different capacitors, swept by a lamp-shaped LFO, mixed with dry ("chorus" mode)
            {
                const float hz = 0.6f + rateKnob * 9.0f;
                static const float base[4] = { 95.0f, 280.0f, 620.0f, 1400.0f };
                for (int i = 0; i < n; ++i)
                {
                    adv (hz);
                    const float lamp = std::pow (0.5f + 0.5f * std::sin (two * ph), 2.2f);
                    const float m = 0.5f * (L[i] + R[i]);
                    float y = m;
                    if ((i & 7) == 0)
                        for (int k = 0; k < 4; ++k)
                        {
                            const float f = juce::jmin (0.45f * fs, base[k] * (1.0f + depth * 5.0f * lamp));
                            const float t = std::tan (juce::MathConstants<float>::pi * f / fs);
                            ap[(size_t) k].a = (t - 1.0f) / (t + 1.0f);
                        }
                    for (auto& s : ap) { const float o = s.a * y + s.x1 - s.a * s.y1; s.x1 = y; s.y1 = o; y = o; }
                    const float w = 0.5f * (m + y) * 1.2f;
                    L[i] = w + (L[i] - m); R[i] = w + (R[i] - m);
                }
                break;
            }
        }
    }

private:
    void adv (float hz) { ph += hz / (float) sr; if (ph >= 1) ph -= 1; }
    static float softBbd (float x) { return std::tanh (x * 1.3f) / 1.3f; }
    struct Ap { float a = 0, x1 = 0, y1 = 0; };
    double sr = 48000; float ph = 0, opto = 0;
    std::array<Ap, 4> ap {};
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> dl, dr;
    juce::dsp::IIR::Filter<float> bbdL, bbdR, lowL, lowR;
};

// ============================ delay ============================
// 0 tape echo (Roland Space Echo style), 1 analog bucket-brigade (Memory Man style), 2 clean digital ping-pong
inline juce::StringArray delayTypeNames()
{
    return { juce::String::fromUTF8 ("Taśmowe echo (jak Space Echo)"), juce::String::fromUTF8 ("Analogowe (jak Memory Man)"), juce::String::fromUTF8 ("Cyfrowe ping-pong") };
}

class DelayFx
{
public:
    void prepare (double rate, int maxBlock)
    {
        sr = rate;
        juce::dsp::ProcessSpec mono { sr, (juce::uint32) maxBlock, 1 };
        for (auto* d : { &dl, &dr }) { d->prepare (mono); d->setMaximumDelayInSamples ((int) (sr * 3.0)); d->reset(); }
        for (auto* f : { &lpL, &lpR, &hpL, &hpR }) { f->prepare (mono); f->reset(); }
        lastTone = -1; lastType = -1; smoothD = -1; wow = flut = mod = 0;
    }

    void process (float* L, float* R, int n, int type, float mix, float fb, float tone, int div, double bpm)
    {
        static const float divs[] = { 1.0f, 0.75f, 0.5f, 2.0f / 3.0f, 2.0f, 0.25f };
        const float q = (float) (60.0 / juce::jmax (30.0, bpm)) * divs[juce::jlimit (0, 5, div)];
        const float target = juce::jlimit (8.0f, (float) sr * 2.9f, q * (float) sr);
        if (smoothD < 0) smoothD = target;
        if (std::abs (tone - lastTone) > 0.004f || type != lastType)
        {
            lastTone = tone; lastType = type;
            const float lpHz = type == 0 ? 2200.0f + tone * 5000.0f : (type == 1 ? 1600.0f + tone * 3000.0f : 3000.0f + tone * 13000.0f);
            const float hpHz = type == 0 ? 160.0f : (type == 1 ? 90.0f : 40.0f);
            lpL.coefficients = lpR.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, lpHz, 0.6f);
            hpL.coefficients = hpR.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, hpHz, 0.6f);
        }
        const float glide = type == 0 ? 0.00008f : 0.0006f;           // tape slides in pitch when the tempo moves
        const float two = juce::MathConstants<float>::twoPi, fs = (float) sr;
        for (int i = 0; i < n; ++i)
        {
            smoothD += (target - smoothD) * glide;
            float d = smoothD, dR = smoothD * 1.012f;
            if (type == 0)
            {
                wow += 0.45f / fs; flut += 6.2f / fs; if (wow > 1) wow -= 1; if (flut > 1) flut -= 1;
                const float w = 1.0f + 0.0028f * std::sin (two * wow) + 0.0007f * std::sin (two * flut);
                d *= w; dR = d;
            }
            else if (type == 1)
            {
                mod += 0.6f / fs; if (mod > 1) mod -= 1;
                const float w = 1.0f + 0.0016f * std::sin (two * mod); d *= w; dR *= w;
            }
            const float eL = dl.popSample (0, d), eR = dr.popSample (0, dR);
            if (type == 2)        // clean ping-pong
            {
                dl.pushSample (0, L[i] + lpL.processSample (eR) * fb);
                dr.pushSample (0, R[i] + lpR.processSample (eL) * fb);
                L[i] += eL * mix; R[i] += eR * mix;
            }
            else                  // tape / analog: mono echo, the repeats saturate and darken
            {
                const float in = 0.5f * (L[i] + R[i]);
                const float drive = type == 0 ? 1.6f : 1.2f;
                const float loop = hpL.processSample (lpL.processSample (eL)) * fb * 1.05f;
                dl.pushSample (0, std::tanh ((in + loop) * drive) / drive);
                dr.pushSample (0, 0.0f);
                L[i] += eL * mix; R[i] += eL * mix * 0.92f;
            }
        }
    }

private:
    double sr = 48000; float lastTone = -1, smoothD = -1, wow = 0, flut = 0, mod = 0; int lastType = -1;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> dl, dr;
    juce::dsp::IIR::Filter<float> lpL, lpR, hpL, hpR;
};

// ============================ reverb pedal ============================
struct ReverbIr { const char* name; const char* file; };
inline const std::vector<ReverbIr>& reverbIrs()
{
    static const std::vector<ReverbIr> r {
        { "Sprężyna lampowa (Fisher K-10)", "rooms/sprezyna_lampowa.wav" },
        { "Sprężyna wzmacniacza", "rooms/sprezyna.mp3" },
        { "Sprężyna miękka (HG)", "rooms/sprezyna_hg.mp3" },
        { "Płyta pogłosowa", "rooms/plyta.mp3" },
        { "Mały drewniany kościół", "rooms/kosciol.mp3" },
        { "Ceglana klatka schodowa", "rooms/klatka.mp3" },
        { "Piwnica (klub)", "rooms/piwnica.mp3" },
        { "Sala balowa", "rooms/sala_balowa.mp3" },
    };
    return r;
}
inline juce::StringArray reverbNames() { juce::StringArray s; for (auto& r : reverbIrs()) s.add (juce::String::fromUTF8 (r.name)); return s; }

class ReverbPedal
{
public:
    void prepare (double rate, int maxBlock)
    {
        sr = rate;
        juce::dsp::ProcessSpec st { sr, (juce::uint32) maxBlock, 2 }, mono { sr, (juce::uint32) maxBlock, 1 };
        conv.prepare (st); conv.reset();
        wet.setSize (2, maxBlock);
        for (auto* f : { &toneL, &toneR }) { f->prepare (mono); f->reset(); }
        lastTone = -1;
    }

    /** Off the audio thread: the recorded tail is shaped by "decay" (shorter = faded out sooner). */
    void load (const juce::File& f, float decay)
    {
        juce::AudioBuffer<float> b; double rate = 48000;
        if (! readAudio (f, b, rate, 8.0)) { ready = false; return; }
        trimLeadingSilence (b);
        const float keep = 0.25f + decay * 0.75f;                       // fraction of the natural length
        const int len = juce::jmax (64, (int) (b.getNumSamples() * keep));
        juce::AudioBuffer<float> s (2, len);
        for (int c = 0; c < 2; ++c) s.copyFrom (c, 0, b, juce::jmin (c, b.getNumChannels() - 1), 0, len);
        if (b.getNumChannels() == 1)  // mono recording: let the right side start a few ms later for width
        {
            const int off = (int) (0.007 * rate);
            for (int i = len - 1; i >= off; --i) s.setSample (1, i, s.getSample (1, i - off));
            for (int i = 0; i < off; ++i) s.setSample (1, i, 0);
        }
        const float tau = (0.2f + decay * 2.5f) * (float) rate;          // extra fade for short settings
        for (int i = 0; i < len; ++i) { const float g = decay > 0.97f ? 1.0f : std::exp (-(float) i / tau) * (1.0f - (float) i / (float) len); s.setSample (0, i, s.getSample (0, i) * g); s.setSample (1, i, s.getSample (1, i) * g); }
        conv.loadImpulseResponse (std::move (s), rate, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::yes);
        ready = true;
    }

    void process (float* L, float* R, int n, float mix, float tone, float dwell)
    {
        if (! ready || mix < 0.001f) return;
        if (std::abs (tone - lastTone) > 0.004f)
        {
            lastTone = tone;
            toneL.coefficients = toneR.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 1800.0f + tone * tone * 12000.0f, 0.6f);
        }
        const float drive = 1.0f + dwell * 6.0f, norm = 1.0f / std::sqrt (drive);
        auto* wl = wet.getWritePointer (0); auto* wr = wet.getWritePointer (1);
        for (int i = 0; i < n; ++i) { wl[i] = std::tanh (L[i] * drive) * norm; wr[i] = std::tanh (R[i] * drive) * norm; }   // "dwell": how hard the springs are hit
        juce::dsp::AudioBlock<float> blk (wet); auto sb = blk.getSubBlock (0, (size_t) n);
        conv.process (juce::dsp::ProcessContextReplacing<float> (sb));
        for (int i = 0; i < n; ++i) { L[i] += toneL.processSample (wl[i]) * mix * 1.6f; R[i] += toneR.processSample (wr[i]) * mix * 1.6f; }
    }

private:
    double sr = 48000; float lastTone = -1; std::atomic<bool> ready { false };
    juce::dsp::Convolution conv;
    juce::AudioBuffer<float> wet;
    juce::dsp::IIR::Filter<float> toneL, toneR;
};

} // namespace jw
