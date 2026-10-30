// One Neural Amp Modeler profile (a captured real amp or pedal), loaded in the background and swapped in safely.
// Profiles are trained at 48 kHz; at other host rates the signal is resampled around the model.
#pragma once
#include <juce_dsp/juce_dsp.h>
#include "NAM/get_dsp.h"
#include "NAM/dsp.h"

namespace jw {

/** Streaming resampler: push input at one rate, pull as many output samples as the input allows. */
struct StreamResampler
{
    void prepare (int capacity) { buf.assign ((size_t) capacity, 0.0f); fill = 0; ip.reset(); }
    void reset() { fill = 0; ip.reset(); }
    void push (const float* x, int n)
    {
        n = juce::jmin (n, (int) buf.size() - fill);
        std::copy (x, x + n, buf.data() + fill); fill += n;
    }
    int pull (double speed, float* out, int maxOut)
    {
        int m = (int) std::floor ((fill - 4) / speed) - 1;
        m = juce::jlimit (0, maxOut, m);
        if (m <= 0) return 0;
        const int used = ip.process (speed, buf.data(), out, m);
        std::copy (buf.data() + used, buf.data() + fill, buf.data()); fill -= used;
        return m;
    }
    juce::LagrangeInterpolator ip; std::vector<float> buf; int fill = 0;
};

class NamSlot
{
public:
    static constexpr int prefill = 48;   // host samples of safety when resampling

    void prepare (double hostRate, int maxBlock)
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        hostSr = hostRate; blockMax = juce::jmax (16, maxBlock);
        setupBuffers();
        if (dsp) dsp->Reset (modelSr, block48);
    }

    /** Off the audio thread. Empty file = no model. */
    bool load (const juce::File& f)
    {
        std::unique_ptr<nam::DSP> fresh;
        double rate = 48000;
        if (f.existsAsFile())
        {
            try
            {
                fresh = nam::get_dsp (std::filesystem::path (f.getFullPathName().toStdString()));
                if (fresh)
                {
                    rate = fresh->GetExpectedSampleRate() > 0 ? fresh->GetExpectedSampleRate() : 48000.0;
                    const int b48 = (int) std::ceil (blockMax * rate / hostSr) + 16;
                    fresh->Reset (rate, b48);
                    measuredDb.store (measure (*fresh, rate, b48));
                    fresh->Reset (rate, b48);
                }
            }
            catch (...) { fresh.reset(); }
        }
        {
            const juce::SpinLock::ScopedLockType sl (lock);
            std::swap (dsp, fresh);
            modelSr = rate;
            setupBuffers();
        }
        fresh.reset();      // old model freed here, not on the audio thread
        return dsp != nullptr;
    }

    bool active() const { return dsp != nullptr; }

    /** Output level (dBFS RMS) of the loaded model for a standard plucked test phrase at about -20 dBFS RMS.
        Used to level profiles that come without a stored trim (the player's own .nam files). */
    std::atomic<float> measuredDb { -20.0f };
    static constexpr float testInputDb = -19.6f;

    static float measure (nam::DSP& m, double rate, int block)
    {
        const float notes[] = { 82.4f, 110, 146.8f, 196, 246.9f, 329.6f };
        const int N = (int) (6 * 0.6 * rate); std::vector<float> in ((size_t) N, 0.0f), out ((size_t) N, 0.0f);
        for (int k = 0; k < 6; ++k)
        {
            const int st = (int) (k * 0.6 * rate); const float f = notes[k] * (k % 2 ? 2.0f : 1.0f);
            for (int i = 0; i < (int) (0.6 * rate) && st + i < N; ++i)
            {
                const float t = (float) (i / rate), e = std::exp (-t * 3);
                in[(size_t) (st + i)] = 0.25f * e * (std::sin (6.283f * f * t) + 0.5f * std::sin (6.283f * 2 * f * t) + 0.25f * std::sin (6.283f * 3 * f * t));
            }
        }
        for (int s = 0; s < N; s += block) { float* ip = in.data() + s; float* op = out.data() + s; m.process (&ip, &op, juce::jmin (block, N - s)); }
        double e = 0; const int skip = (int) (rate * 0.3);
        for (int i = skip; i < N; ++i) e += (double) out[(size_t) i] * out[(size_t) i];
        const double rms = std::sqrt (e / juce::jmax (1, N - skip));
        return rms > 1e-7 ? (float) (20.0 * std::log10 (rms)) : -20.0f;
    }
    int latency() const { return std::abs (hostSr - modelSr) > 1 ? prefill : 0; }

    /** In place, mono, host rate. Returns false (signal untouched) if no model is ready. */
    bool process (float* x, int n)
    {
        const juce::SpinLock::ScopedTryLockType sl (lock);
        if (! sl.isLocked() || dsp == nullptr) return false;
        if (! resampling)
        {
            for (int s = 0; s < n; s += block48)
            {
                const int k = juce::jmin (block48, n - s);
                float* ip = x + s; float* op = tmp48.data();
                dsp->process (&ip, &op, k);
                std::copy (op, op + k, x + s);
            }
            return true;
        }
        inS.push (x, n);
        const int m = inS.pull (hostSr / modelSr, in48.data(), (int) in48.size());
        for (int s = 0; s < m; s += block48)
        {
            const int k = juce::jmin (block48, m - s);
            float* ip = in48.data() + s; float* op = tmp48.data();
            dsp->process (&ip, &op, k);
            outS.push (op, k);
        }
        const int got = outS.pull (modelSr / hostSr, hostTmp.data(), (int) hostTmp.size());
        for (int i = 0; i < got && outFill < (int) outFifo.size(); ++i) outFifo[(size_t) outFill++] = hostTmp[(size_t) i];
        const int take = juce::jmin (n, outFill);
        std::copy (outFifo.data(), outFifo.data() + take, x);
        std::fill (x + take, x + n, 0.0f);
        std::copy (outFifo.data() + take, outFifo.data() + outFill, outFifo.data()); outFill -= take;
        if (take < n) underruns++;
        return true;
    }

    int underruns = 0;

private:
    void setupBuffers()
    {
        resampling = std::abs (hostSr - modelSr) > 1;
        block48 = (int) std::ceil (blockMax * modelSr / hostSr) + 16;
        const int cap = (blockMax + 64) * 4;
        in48.assign ((size_t) cap * 2, 0.0f); tmp48.assign ((size_t) juce::jmax (block48, cap), 0.0f); hostTmp.assign ((size_t) cap * 2, 0.0f);
        inS.prepare (cap * 2); outS.prepare (cap * 2);
        outFifo.assign ((size_t) cap * 2, 0.0f); outFill = prefill;
    }

    juce::SpinLock lock;
    std::unique_ptr<nam::DSP> dsp;
    double hostSr = 48000, modelSr = 48000; int blockMax = 512, block48 = 528; bool resampling = false;
    StreamResampler inS, outS;
    std::vector<float> in48, tmp48, hostTmp, outFifo; int outFill = 0;
};

} // namespace jw
