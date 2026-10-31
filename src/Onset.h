#pragma once
// TOMRUM — PARASITENS ÖRA. Hittar slagen i en loop man släppt på panelen. Ren C++.
//
// Energi i korta fönster (256 sampel), positiv ökning i log-energi = ett slag.
// Tröskeln följer loopens egen dynamik, och två slag måste ligga minst 45 ms isär.

#include <algorithm>
#include <cmath>
#include <vector>

namespace tomrum
{
inline std::vector<int> detectOnsets (const float* mono, int n, double sampleRate)
{
    std::vector<int> out;
    const int hop = 256;
    if (mono == nullptr || n < hop * 8 || sampleRate <= 0) return out;
    const int frames = n / hop;
    std::vector<float> le ((size_t) frames), flux ((size_t) frames, 0.0f);
    for (int f = 0; f < frames; ++f)
    {
        double e = 0;
        for (int i = 0; i < hop; ++i) { const float x = mono[f * hop + i]; e += (double) x * x; }
        le[(size_t) f] = (float) std::log10 (e / hop + 1.0e-10);
    }
    for (int f = 1; f < frames; ++f) flux[(size_t) f] = std::max (0.0f, le[(size_t) f] - le[(size_t) f - 1]);

    double mean = 0, sq = 0;
    for (float x : flux) { mean += x; sq += (double) x * x; }
    mean /= frames;
    const double sd = std::sqrt (std::max (0.0, sq / frames - mean * mean));
    const float thr = (float) (mean + 1.2 * sd);
    const int gap = std::max (1, (int) (0.045 * sampleRate / hop));
    int last = -gap;
    for (int f = 1; f < frames - 1; ++f)
    {
        const float x = flux[(size_t) f];
        if (x > thr && x >= flux[(size_t) f - 1] && x >= flux[(size_t) f + 1] && f - last >= gap)
        {
            // fina läget: största språnget i amplitud inom fönstret
            int best = f * hop;
            float bestD = 0.0f;
            for (int i = std::max (1, (f - 1) * hop); i < std::min (n, (f + 1) * hop); ++i)
            {
                const float d = std::fabs (mono[i]) - std::fabs (mono[i - 1]);
                if (d > bestD) { bestD = d; best = i; }
            }
            out.push_back (best);
            last = f;
        }
    }
    return out;
}

// Hur många takter loopen är vid ett visst tempo (1, 2, 4 eller 8).
inline int loopBars (int lengthSamples, double sampleRate, double bpm)
{
    const double bars = lengthSamples / sampleRate * bpm / 240.0;
    int best = 1;
    double bestErr = 1e9;
    for (int b : { 1, 2, 4, 8 })
    {
        const double err = std::fabs (std::log2 (bars / b));
        if (err < bestErr) { bestErr = err; best = b; }
    }
    return best;
}
} // namespace tomrum
