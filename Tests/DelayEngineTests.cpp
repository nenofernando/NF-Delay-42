// Offline tests of the NF Delay 42 DSP core against the numbers in the manual. Build:
//   c++ -std=c++17 -O2 Tests/DelayEngineTests.cpp -o /tmp/nfd_tests && /tmp/nfd_tests
#include "../Source/DSP/DelayEngine.h"
#include <cstdio>
#include <cstdlib>

using namespace nfd;
static int fails = 0;
#define CHECK(cond, ...) do { if (cond) std::printf ("  ok    "); else { std::printf ("  FAIL  "); ++fails; } std::printf (__VA_ARGS__); std::printf ("\n"); } while (0)

static const double FS = 48000.0;

// run mono-in-stereo for n samples; input generator -> output vector (left)
template <class Gen>
static std::vector<float> run (Engine& e, Params p, int n, Gen gen, int block = 256)
{
    std::vector<float> out ((size_t) n);
    std::vector<float> l ((size_t) block), r ((size_t) block);
    for (int pos = 0; pos < n; pos += block)
    {
        const int m = std::min (block, n - pos);
        for (int i = 0; i < m; ++i) l[i] = r[i] = gen (pos + i);
        float* io[2] = { l.data(), r.data() };
        e.process (io, m, p);
        for (int i = 0; i < m; ++i) out[(size_t) (pos + i)] = l[i];
    }
    return out;
}
static int peakIndex (const std::vector<float>& v, int from)
{
    int bi = from; float bm = 0; for (size_t i = (size_t) from; i < v.size(); ++i) if (std::fabs (v[i]) > bm) { bm = std::fabs (v[i]); bi = (int) i; } return bi;
}

int main()
{
    // ---- 1. tap -> delay time (800 ms range at MANUAL = X1, 256 taps, ~3 ms step)
    std::printf ("delay time\n");
    for (int tap : { 32, 128, 255 })
    {
        Engine e; e.prepare (FS, 2);
        Params p; p.tap = tap; p.mix = 1.0f; p.levelDb = 0.0f; p.depth01 = 0; p.manual01 = 0.5f;
        auto o = run (e, p, (int) (FS * 1.2), [] (int i) { return i == 100 ? 0.1f : 0.0f; });
        const int pk = peakIndex (o, 200) - 100;
        const double ms = pk * 1000.0 / FS, expect = tap / 256.0 * 800.0;
        CHECK (std::fabs (ms - expect) < 1.0, "tap %3d: %.2f ms (manual says %.2f ms, +-1 ms)", tap, ms, expect);
    }
    { // X2: double range
        Engine e; e.prepare (FS, 2);
        Params p; p.tap = 128; p.x2 = true; p.mix = 1.0f;
        auto o = run (e, p, (int) (FS * 2.0), [] (int i) { return i == 100 ? 0.1f : 0.0f; });
        const double ms = (peakIndex (o, 200) - 100) * 1000.0 / FS;
        CHECK (std::fabs (ms - 800.0) < 2.0, "X2 tap 128: %.2f ms (expected 800)", ms);
    }
    { // MANUAL X0.5 / X1.5 -> 400 / 1200 ms at full tap (manual p.28)
        for (auto mv : { std::make_pair (0.0f, 400.0 * 255 / 256), std::make_pair (1.0f, 1200.0 * 255 / 256) })
        {
            Engine e; e.prepare (FS, 2);
            Params p; p.tap = 255; p.manual01 = mv.first; p.mix = 1.0f;
            auto o = run (e, p, (int) (FS * 1.6), [] (int i) { return i == 100 ? 0.1f : 0.0f; });
            const double ms = (peakIndex (o, 200) - 100) * 1000.0 / FS;
            CHECK (std::fabs (ms - mv.second) < 2.0, "MANUAL %.1f: %.2f ms (expected %.2f)", mv.first, ms, mv.second);
        }
    }

    // ---- 2. direct path and mix
    std::printf ("mix / direct\n");
    {
        Engine e; e.prepare (FS, 2);
        Params p; p.tap = 0; p.mix = 0.0f;
        auto o = run (e, p, 4800, [] (int i) { return 0.25f * std::sin (2 * kPi * 440.0f * i / 48000.0f); });
        float mx = 0; for (int i = 2400; i < 4800; ++i) mx = std::max (mx, std::fabs (o[i]));
        CHECK (std::fabs (mx - 0.25f) < 0.005f, "mix 0 passes direct at unity (peak %.4f)", mx);
    }

    // ---- 3. feedback
    std::printf ("feedback\n");
    {
        Engine e; e.prepare (FS, 2);
        Params p; p.tap = 64; p.mix = 1.0f; p.feedback = 0.5f; // 200 ms
        auto o = run (e, p, (int) (FS * 1.0), [] (int i) { return i == 100 ? 0.2f : 0.0f; });
        const int a = peakIndex (std::vector<float> (o.begin(), o.begin() + (int) (FS * 0.3)), 200);
        float a1 = std::fabs (o[a]); float a2 = 0; for (int i = a + 8000; i < a + 11200; ++i) a2 = std::max (a2, std::fabs (o[i]));
        CHECK (std::fabs (a2 / a1 - 0.5f) < 0.07f, "second echo / first echo = %.3f (feedback 0.5)", a2 / a1);
        CHECK (a2 > 0, "echoes repeat");
    }
    {   // FB INV flips the polarity of the second echo
        Engine e1, e2; e1.prepare (FS, 2); e2.prepare (FS, 2);
        Params p; p.tap = 64; p.mix = 1.0f; p.feedback = 0.5f; Params q = p; q.fbInv = true;
        auto gen = [] (int i) { return i == 100 ? 0.2f : 0.0f; };
        auto o1 = run (e1, p, (int) (FS * 0.6), gen), o2 = run (e2, q, (int) (FS * 0.6), gen);
        const int a = peakIndex (o1, 200) + 9600; const int b = peakIndex (o1, a - 400);
        (void) b;
        float s1 = 0, s2 = 0; for (int i = a - 300; i < a + 300; ++i) { s1 += o1[i]; s2 += o2[i]; }
        CHECK (s1 * s2 < 0, "FB INV: second echo polarity flips (%.4f vs %.4f)", s1, s2);
    }
    {   // runaway feedback stays bounded (limiter in the loop)
        Engine e; e.prepare (FS, 2);
        Params p; p.tap = 20; p.mix = 1.0f; p.feedback = 1.0f; p.levelDb = 6.0f;
        srand (1); auto o = run (e, p, (int) (FS * 6.0), [] (int) { return 0.5f * ((float) rand() / RAND_MAX * 2 - 1); });
        float mx = 0; for (float v : o) mx = std::max (mx, std::fabs (v));
        CHECK (mx < 2.0f && std::isfinite (mx), "feedback 10 with noise stays bounded (peak %.3f)", mx);
    }

    // ---- 4. infinite repeat: capture at clock pulse, loop length = memory, input ignored afterwards
    std::printf ("infinite repeat\n");
    {
        Engine e; e.prepare (FS, 2);
        Params p; p.tap = 64; p.mix = 1.0f; p.clkNum = 1; p.clkDen = 64; // short clock so it captures quickly
        auto gen = [] (int i) { return (i > 2000 && i < 2400) ? 0.3f * std::sin (2 * kPi * 1000.0f * i / 48000.0f) : 0.0f; };
        run (e, p, (int) (FS * 0.3), gen);                // sound enters
        p.inf = true;
        auto o = run (e, p, (int) (FS * 4.0), [] (int) { return 0.0f; });
        CHECK (e.isInfActive(), "repeat engaged at a clock pulse");
        // memory = 800 ms at X1 : burst must come back every 0.8 s
        std::vector<int> peaks; for (int i = 1; i < (int) o.size() - 1; ++i) if (std::fabs (o[i]) > 0.08f && (peaks.empty() || i - peaks.back() > 6000)) peaks.push_back (i);
        bool periodic = peaks.size() >= 3; double per = 0;
        if (periodic) { per = (peaks[2] - peaks[1]) / FS; periodic = std::fabs (per - 0.8) < 0.01; }
        CHECK (periodic, "loop period %.3f s (memory 0.800 s at MANUAL X1), %zu repeats in 4 s", per, peaks.size());
        p.inf = false;
        auto o2 = run (e, p, (int) (FS * 1.5), [] (int) { return 0.0f; });
        CHECK (! e.isInfActive(), "repeat released");
    }

    // ---- 5. VCO sweep: depth 10 sweeps delay over a 3:1 range regardless of MANUAL; rate; waveform
    std::printf ("vco sweep\n");
    {
        Engine e; e.prepare (FS, 2);
        Params p; p.tap = 128; p.depth01 = 1.0f; p.rateHz = 2.0f; p.wave01 = 0.0f; p.manual01 = 0.9f;
        float mn = 1e9f, mx = 0; std::vector<float> l (256), r (256);
        for (int b = 0; b < (int) (FS * 1.5 / 256); ++b) { std::fill (l.begin(), l.end(), 0.0f); r = l; float* io[2] = { l.data(), r.data() }; e.process (io, 256, p); if (b > 20) { mn = std::min (mn, e.getDelayMs()); mx = std::max (mx, e.getDelayMs()); } }
        CHECK (std::fabs (mx / mn - 3.0f) < 0.25f, "depth 10 sweep ratio %.2f : 1 (manual: 3:1)", mx / mn);
        p.depth01 = 0.0f; mn = 1e9f; mx = 0;
        for (int b = 0; b < 400; ++b) { std::fill (l.begin(), l.end(), 0.0f); r = l; float* io[2] = { l.data(), r.data() }; e.process (io, 256, p); if (b > 100) { mn = std::min (mn, e.getDelayMs()); mx = std::max (mx, e.getDelayMs()); } }
        CHECK (mx - mn < 0.5f, "depth 0: delay constant (%.3f..%.3f ms)", mn, mx);
    }
    { // square wave jumps between the two extremes
        Engine e; e.prepare (FS, 2);
        Params p; p.tap = 128; p.depth01 = 1.0f; p.rateHz = 1.0f; p.wave01 = 1.0f;
        std::vector<float> l (64), r (64); std::vector<float> seen;
        for (int b = 0; b < (int) (FS * 2.2 / 64); ++b) { std::fill (l.begin(), l.end(), 0.0f); r = l; float* io[2] = { l.data(), r.data() }; e.process (io, 64, p); if (b > 40) seen.push_back (e.getDelayMs()); }
        int mid = 0; float lo = *std::min_element (seen.begin(), seen.end()), hi = *std::max_element (seen.begin(), seen.end());
        for (float v : seen) if (v > lo + 0.1f * (hi - lo) && v < hi - 0.1f * (hi - lo)) ++mid;
        CHECK (hi / lo > 2.8f && mid < (int) seen.size() / 40, "square: two states %.1f / %.1f ms, few samples in between (%d)", lo, hi, mid);
    }

    // ---- 6. clock period = memory x fraction (manual 3.6 / 2.1 K): 1/2 at X1 16 kHz => 400 ms
    std::printf ("clock\n");
    {
        Engine e; e.prepare (FS, 2); Params p; p.clkNum = 1; p.clkDen = 2;
        std::vector<float> l (256), r (256); uint32_t t0 = 0;
        for (int b = 0; b < (int) (FS * 4.1 / 256); ++b) { std::fill (l.begin(), l.end(), 0.0f); r = l; float* io[2] = { l.data(), r.data() }; e.process (io, 256, p); if (b == 20) t0 = e.getClkTicks(); }
        const uint32_t ticks = e.getClkTicks() - t0; const double sec = ((int) (FS * 4.1 / 256) - 21) * 256 / FS;
        CHECK (std::fabs (sec / ticks - 0.4) < 0.02, "clock period %.3f s (expected 0.400 s)", sec / ticks);
    }

    // ---- 7. input stage: 5:1 above -3 dB, soft ceiling
    std::printf ("input limiter\n");
    {
        const float a = preLimit (0.5f), b = preLimit (4.0f);   // -6 dB untouched ; +12 dB in
        CHECK (std::fabs (a - 0.5f) < 1e-3f, "-6 dB passes unchanged (%.4f)", a);
        CHECK (std::fabs (20 * std::log10 (b)) < 1.0f, "+12 dB in -> about 0 dB (%.2f dB)", 20 * std::log10 (b));
        CHECK (softLimit (10.0f) <= 1.0001f, "soft limiter ceiling %.4f", softLimit (10.0f));
    }

    // ---- 8. bypass: delayed signal removed from the mix
    std::printf ("bypass\n");
    {
        Engine e; e.prepare (FS, 2);
        Params p; p.tap = 64; p.mix = 1.0f; p.bypass = true;
        auto o = run (e, p, (int) (FS * 0.6), [] (int i) { return i == 100 ? 0.3f : 0.0f; });
        float mx = 0; for (int i = 200; i < (int) o.size(); ++i) mx = std::max (mx, std::fabs (o[i]));
        CHECK (mx < 1e-4f, "bypass: no delayed audio (peak %.6f)", mx);
    }

    // ---- 9. frequency response: 10 Hz..16 kHz +-0.5/-3 dB in X1; 6 kHz in X2
    std::printf ("bandwidth\n");
    auto gainAt = [] (double f, bool x2) {
        Engine e; e.prepare (FS, 2); Params p; p.tap = 0; p.mix = 1.0f; p.x2 = x2; p.levelDb = -12.0f;
        auto o = run (e, p, 24000, [f] (int i) { return 0.5f * std::sin (2 * kPi * (float) (f * i / 48000.0)); });
        float mx = 0; for (int i = 12000; i < 24000; ++i) mx = std::max (mx, std::fabs (o[i])); return 20 * std::log10 (mx / (0.5f * std::pow (10.0f, -12.0f / 20.0f))); };
    (void) gainAt;
    {   // delayed path only: tap small -> compare mix=1 and mix=0 spectra via sine amplitude at the filtered output
        auto g = [] (double f, bool x2) {
            Engine e; e.prepare (FS, 2); Params p; p.tap = 2; p.mix = 1.0f; p.x2 = x2; p.levelDb = -12.0f;
            auto o = run (e, p, 30000, [f] (int i) { return 0.5f * std::sin (2 * kPi * (float) (f * i / 48000.0)); });
            float mx = 0; for (int i = 20000; i < 30000; ++i) mx = std::max (mx, std::fabs (o[i]));
            return 20 * std::log10 (mx / (0.5f * std::pow (10.0f, -12.0f / 20.0f))); };
        const double g1k = g (1000, false), g16k = g (16000, false), g6k_x2 = g (6000, true), g1k_x2 = g (1000, true);
        CHECK (std::fabs (g1k) < 0.6, "X1 1 kHz: %.2f dB", g1k);
        CHECK (g16k < -2.0 && g16k > -4.2, "X1 16 kHz: %.2f dB (manual: -3 dB at 16 kHz)", g16k);
        CHECK (std::fabs (g1k_x2) < 0.6 && g6k_x2 < -2.0 && g6k_x2 > -4.2, "X2 1 kHz %.2f dB, 6 kHz %.2f dB", g1k_x2, g6k_x2);
    }

    std::printf ("\n%s (%d failure%s)\n", fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED", fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
