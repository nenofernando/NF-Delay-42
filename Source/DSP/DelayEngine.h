// NF Delay 42 -- DSP core (plain C++17, no JUCE dependency, unit-tested in Tests/DelayEngineTests.cpp).
// Behaviour follows the public owner's manual of the digital delay it is inspired by: sections 2.1, 5 (block diagram) and 6.
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

namespace nfd
{
constexpr float kPi = 3.14159265358979f;

// All numbers below are from the manual (section 6): 256 delay taps, 800 ms (16 kHz mode) or 1600 ms (6 kHz / X2 mode)
// at MANUAL = X1, MANUAL covers 0.5..1.5 (3:1), VCO depth 0..full 3:1 sweep, LFO 0.1..10 Hz.
constexpr int   kNumTaps       = 256;
constexpr float kBaseShortSec  = 0.8f;
constexpr float kBaseLongSec   = 1.6f;
constexpr float kMultMin       = 0.5f;
constexpr float kMultMax       = 1.5f;
constexpr float kMaxDelaySec   = kBaseLongSec * kMultMax;   // 2.4 s

struct Params
{
    float levelDb    = 0.0f;   // input gain
    float feedback   = 0.0f;   // 0..1
    bool  hiCut      = false;  // 6 dB/oct low-pass (-3 dB @ 4 kHz) in the feedback path
    bool  fbInv      = false;
    bool  dlyInv     = false;
    bool  x2         = false;  // long range, 6 kHz bandwidth
    bool  bypass     = false;  // delay out of the mix, nothing written to memory
    bool  inf        = false;  // infinite repeat
    int   tap        = 0;      // 0..255
    int   clkNum     = 1;      // 1,3,5,7,9
    int   clkDen     = 2;      // 1,2,4,8,16,32,64
    float manual01   = 0.5f;   // 0..1  ->  X0.5 .. X1.5 (0.5 = X1)
    float depth01    = 0.0f;
    float wave01     = 0.0f;   // 0 sine, 0.5 envelope follower, 1 square
    float rateHz     = 1.0f;
    float mix        = 0.5f;   // 0 = direct only, 1 = delayed only
};

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float process (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
    void setLowpass (double fc, double q, double fs)
    {
        const double w = 2.0 * 3.14159265358979323846 * fc / fs, c = std::cos (w), s = std::sin (w), al = s / (2.0 * q), a0 = 1.0 + al;
        b0 = (float) (((1.0 - c) * 0.5) / a0); b1 = (float) ((1.0 - c) / a0); b2 = b0;
        a1 = (float) ((-2.0 * c) / a0); a2 = (float) ((1.0 - al) / a0);
    }
};

inline double biquadMag (const Biquad& b, double f, double fs)
{
    const double w = 2.0 * 3.14159265358979323846 * f / fs, c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
    const double nr = b.b0 + b.b1 * c1 + b.b2 * c2, ni = -(b.b1 * s1 + b.b2 * s2), dr = 1.0 + b.a1 * c1 + b.a2 * c2, di = -(b.a1 * s1 + b.a2 * s2);
    return std::sqrt ((nr * nr + ni * ni) / (dr * dr + di * di));
}

// 4th-order Butterworth low-pass (two biquads)
struct Lowpass4
{
    Biquad s1, s2;
    void set (double fc, double fs)
    {
        fc = std::min (fc, fs * 0.45);
        s1.setLowpass (fc, 0.5411961, fs);
        s2.setLowpass (fc, 1.3065630, fs);
    }
    inline float process (float x) { return s2.process (s1.process (x)); }
    void reset() { s1.reset(); s2.reset(); }
    // cutoff searched so that THIS filter is -1.5 dB at the quoted bandwidth: anti-alias + reconstruction in series = -3 dB (manual 6.1)
    void setForBandwidth (double bw, double fs)
    {
        double lo = bw, hi = std::min (bw * 2.0, fs * 0.49);
        for (int i = 0; i < 40; ++i)
        {
            const double mid = 0.5 * (lo + hi); set (mid, fs);
            const double g = biquadMag (s1, bw, fs) * biquadMag (s2, bw, fs);
            if (g > 0.8414) hi = mid; else lo = mid;   // 0.8414 = -1.5 dB
        }
        set (0.5 * (lo + hi), fs);
    }
};

struct OnePole
{
    float a = 0, z = 0;
    void setLowpass (double fc, double fs) { a = (float) (1.0 - std::exp (-2.0 * 3.14159265358979323846 * fc / fs)); }
    inline float process (float x) { z += a * (x - z); return z; }
    void reset() { z = 0; }
};

// Input stage 1: soft-knee 5:1 compression above -3 dB of the converter limit (manual 1.0 and 6.2).
inline float preLimit (float x)
{
    const float a = std::fabs (x);
    if (a < 0.2f) return x;
    const float inDb = 20.0f * std::log10 (a), thr = -3.0f, W = 6.0f, over = inDb - thr, slope = 1.0f / 5.0f - 1.0f;
    float outDb;
    if (2.0f * over < -W) return x;
    else if (2.0f * std::fabs (over) <= W) outDb = inDb + slope * (over + W * 0.5f) * (over + W * 0.5f) / (2.0f * W);
    else outDb = thr + over / 5.0f;
    return std::copysign (std::pow (10.0f, outDb / 20.0f), x);
}

// Limiter in front of the A/D: unity up to 0.8, soft saturation to 1.0 ("soft knee" instead of clipping, manual 1.0).
inline float softLimit (float x)
{
    const float a = std::fabs (x);
    if (a <= 0.8f) return x;
    return std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), x);
}

inline float hermite (float ym1, float y0, float y1, float y2, float t)
{
    const float c1 = 0.5f * (y1 - ym1), c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2, c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
    return ((c3 * t + c2) * t + c1) * t + y0;
}

class Engine
{
public:
    static constexpr int kMaxCh = 8;

    void prepare (double sampleRate, int numChannels)
    {
        fs = sampleRate;
        nCh = std::min (std::max (numChannels, 1), kMaxCh);
        size_t n = 1; while ((double) n < kMaxDelaySec * fs + 16.0) n <<= 1;
        mask = (int64_t) n - 1;
        for (int c = 0; c < kMaxCh; ++c) ring[c].assign (c < nCh ? n : 1, 0.0f);
        aSmooth   = 1.0f - std::exp (-1.0f / (0.010f * (float) fs));
        aVco      = 1.0f - std::exp (-1.0f / (0.0015f * (float) fs));
        aTap      = 1.0f - std::exp (-1.0f / (0.030f * (float) fs));
        aAtk      = 1.0f - std::exp (-1.0f / (0.003f * (float) fs));
        aRel      = 1.0f - std::exp (-1.0f / (0.150f * (float) fs));
        peakDecay = std::exp (-1.0f / (0.25f * (float) fs));
        lastX2    = -1;
        reset();
    }

    // power-up state of the hardware: memory empty, repeat off, clock phase restarted
    void reset()
    {
        for (int c = 0; c < nCh; ++c) { std::fill (ring[c].begin(), ring[c].end(), 0.0f); aa[c].reset(); rec[c].reset(); hi[c].reset(); dc[c] = 0; dcy[c] = 0; }
        w = 0; lfoPhase = 0; clkPhase = 0; env = 0; pSm = -1.0f; tapSm = -1.0f; infActive = false; gainSm = -1.0f; fbSm = -1.0f; mixSm = -1.0f;
        peak = 0; peakDbAtomic.store (-120.0f); delayMsAtomic.store (0.0f); infAtomic.store (false);
    }

    void process (float* const* io, int n, const Params& p)
    {
        // two filters in series (anti-alias + reconstruction): -1.5 dB each = -3 dB overall at the quoted bandwidth 
        if (p.x2 != lastX2) { lastX2 = p.x2; const double bw = p.x2 ? 6000.0 : 16000.0; for (int c = 0; c < nCh; ++c) { aa[c].setForBandwidth (bw, fs); rec[c].setForBandwidth (bw, fs); } }
        for (int c = 0; c < nCh; ++c) hi[c].setLowpass (4000.0, fs);

        const float base = p.x2 ? kBaseLongSec : kBaseShortSec;
        const float gainT = std::pow (10.0f, p.levelDb / 20.0f);
        if (gainSm < 0) { gainSm = gainT; fbSm = p.feedback; mixSm = p.mix; }
        if (pSm < 0) { pSm = p.manual01; tapSm = (float) p.tap; }
        const float fbSign = p.fbInv ? -1.0f : 1.0f, dlySign = p.dlyInv ? -1.0f : 1.0f;
        const float dLfo = p.rateHz / (float) fs;

        for (int i = 0; i < n; ++i)
        {
            gainSm += aSmooth * (gainT - gainSm);  fbSm += aSmooth * (p.feedback - fbSm);  mixSm += aSmooth * (p.mix - mixSm);

            // ---- input stage (+ per-channel peak for the envelope follower and the HEADROOM display)
            float x1[kMaxCh], mx = 0.0f;
            for (int c = 0; c < nCh; ++c) { x1[c] = preLimit (io[c][i] * gainSm); mx = std::max (mx, std::fabs (x1[c])); }
            peak = std::max (mx, peak * peakDecay);
            env += (mx > env ? aAtk : aRel) * (mx - env);

            // ---- VCO: LFO -> sweep position (manual p. 6: depth blends the manual control out; 10 = full 3:1 range, manual has no effect)
            lfoPhase += dLfo; if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            const float sine = 0.5f - 0.5f * std::cos (2.0f * kPi * lfoPhase), sq = lfoPhase < 0.5f ? 0.0f : 1.0f;
            const float envN = std::min (std::max ((20.0f * std::log10 (env + 1.0e-9f) + 36.0f) / 36.0f, 0.0f), 1.0f);
            const float wv = p.wave01;
            const float lfo = wv < 0.5f ? sine + (envN - sine) * (wv * 2.0f) : envN + (sq - envN) * ((wv - 0.5f) * 2.0f);
            const float pT = (1.0f - p.depth01) * p.manual01 + p.depth01 * lfo;
            pSm += aVco * (pT - pSm);
            const float mult = kMultMin + (kMultMax - kMultMin) * pSm;
            tapSm += aTap * ((float) p.tap - tapSm);
            const double Dsec = (double) tapSm / (double) kNumTaps * (double) base * (double) mult;
            const double Dsmp = std::max (Dsec * fs, 2.0);

            // ---- clock: period = memory length at the current sampling rate x fraction (manual 2.1 K, 3.6)
            const double memSec = (double) base * (double) mult;
            clkPhase += 1.0 / (memSec * ((double) p.clkNum / (double) p.clkDen) * fs);
            bool tick = false;
            if (clkPhase >= 1.0) { clkPhase -= 1.0; tick = true; ++clkTicks; }

            // ---- infinite repeat: armed immediately, captured at the first clock pulse (manual 2.1 J)
            if (! p.inf) infActive = false;
            else if (! infActive && tick)
            {
                infActive = true; loopLen = std::max (memSec * fs, 4.0); loopStart = (double) w - loopLen; loopPos = 0.0; multCap = mult;
            }

            for (int c = 0; c < nCh; ++c)
            {
                float delayed;
                if (infActive) delayed = readRing (c, loopStart + loopPos);
                else delayed = readRing (c, (double) w - Dsmp);

                // feedback tap = D/A output, before the output filter (block diagram)
                float fb = delayed * fbSign * fbSm;
                if (p.hiCut) fb = hi[c].process (fb);

                const float in = aa[c].process (x1[c]);
                float s = in + fb;
                const float hp = s - dc[c]; dc[c] += 0.0010f * hp;      // ~8 Hz DC servo inside the loop
                s = hp;
                const float wIn = p.bypass ? 0.0f : softLimit (s);
                if (! infActive) ring[c][(size_t) (w & mask)] = wIn;

                const float outDelay = p.bypass ? 0.0f : rec[c].process (delayed * dlySign);
                if (p.bypass) rec[c].reset();
                io[c][i] = x1[c] * (1.0f - mixSm) + outDelay * mixSm;
            }
            if (! infActive) ++w;
            else { loopPos += (double) (multCap / mult); if (loopPos >= loopLen) loopPos -= loopLen; }

            lastDelayMs = (float) (Dsec * 1000.0);
            lastRateLit = lfoPhase < 0.5f;
        }
        peakDbAtomic.store (20.0f * std::log10 (peak + 1.0e-9f));
        delayMsAtomic.store (lastDelayMs);
        rateLitAtomic.store (lastRateLit);
        clkTicksAtomic.store (clkTicks);
        infAtomic.store (infActive);
        lfoPhaseAtomic.store (lfoPhase);
    }

    // readouts for the UI (written once per block)
    float getPeakDb() const        { return peakDbAtomic.load(); }
    float getDelayMs() const       { return delayMsAtomic.load(); }
    bool  getRateLit() const       { return rateLitAtomic.load(); }
    uint32_t getClkTicks() const   { return clkTicksAtomic.load(); }
    bool  isInfActive() const      { return infAtomic.load(); }

private:
    inline float readRing (int c, double pos) const
    {
        const int64_t i = (int64_t) std::floor (pos);
        const float t = (float) (pos - (double) i);
        const std::vector<float>& r = ring[c];
        return hermite (r[(size_t) ((i - 1) & mask)], r[(size_t) (i & mask)], r[(size_t) ((i + 1) & mask)], r[(size_t) ((i + 2) & mask)], t);
    }

    double fs = 44100.0; int nCh = 2; int64_t mask = 0, w = 0;
    std::vector<float> ring[kMaxCh];
    Lowpass4 aa[kMaxCh], rec[kMaxCh]; OnePole hi[kMaxCh]; float dc[kMaxCh] = {}, dcy[kMaxCh] = {};
    float aSmooth = 0.01f, aVco = 0.1f, aTap = 0.001f, aAtk = 0.1f, aRel = 0.001f, peakDecay = 0.999f;
    float gainSm = -1.0f, fbSm = 0, mixSm = 0, pSm = -1.0f, tapSm = -1.0f, lfoPhase = 0, env = 0, peak = 0, multCap = 1.0f, lastDelayMs = 0;
    double clkPhase = 0, loopLen = 0, loopStart = 0, loopPos = 0; bool infActive = false, lastRateLit = false; int lastX2 = -1; uint32_t clkTicks = 0;
    std::atomic<float> peakDbAtomic { -120.0f }, delayMsAtomic { 0.0f }, lfoPhaseAtomic { 0.0f };
    std::atomic<bool> rateLitAtomic { false }, infAtomic { false };
    std::atomic<uint32_t> clkTicksAtomic { 0 };
};
} // namespace nfd
