// TOMRUM: motortest. Slumpen ska alltid hålla sig inom regelboken.
#include "../src/Tomrum.h"
#include <cstdio>
#include <cstring>
using namespace tomrum;
static int fails = 0;
#define CHECK(c, ...) do { if (! (c)) { ++fails; std::printf ("FEL: " __VA_ARGS__); std::printf ("\n"); } } while (0)

int main()
{
    // 1) GENETIK följer reglerna, 8 skolor x 400 frön
    for (int s = 0; s < kNumSkolor; ++s)
    {
        const Skola& sk = skolor()[s];
        int rangeFail = 0;
        for (uint32_t seed = 1; seed <= 400; ++seed)
        {
            Genome g = mutate (Genome {}, s, seed);
            for (int l = 0; l < kLanes; ++l)
            {
                const auto& r = sk.lane[l];
                const auto& lg = g.lane[l];
                if (! r.pattern[0]) continue;
                // slag bara där regeln tillåter
                bool okVariant = false;
                for (int v = 0; v < 3 && r.pattern[v]; ++v)
                {
                    const char* p = r.pattern[v];
                    if ((int) std::strlen (p) != lg.len) continue;
                    bool ok = true;
                    for (int i = 0; i < lg.len; ++i)
                    {
                        const float w = stepWeight (p[i]);
                        if (lg.on (i) && w <= 0) ok = false;
                        if (w >= 1 && ! lg.on (i)) ok = false;
                    }
                    okVariant |= ok;
                }
                CHECK (okVariant, "%s %s frö %u: slag utanför regeln", sk.name, laneName (l), seed);
                int c = 0; for (int i = 0; i < lg.len; ++i) c += lg.on (i);
                if (c < r.minHits || c > r.maxHits) ++rangeFail;
            }
            // ljudet inom skolans ramar
            CHECK (g.snd.kickPitch >= sk.kickPitch.lo - 0.01f && g.snd.kickPitch <= sk.kickPitch.hi + 0.01f, "%s kickton", sk.name);
            CHECK (g.snd.ceiling >= sk.ceiling.lo - 1 && g.snd.ceiling <= sk.ceiling.hi + 1, "%s tak", sk.name);
        }
        // min/max kan i sällsynta fall inte nås med 24 försök: tillåt högst 1 %
        CHECK (rangeFail <= 400 * kLanes / 100, "%s: %d spår utanför min/max", sk.name, rangeFail);
        std::printf ("%-14s ok (min/max-miss %d av %d)\n", sk.name, rangeFail, 400 * kLanes);
    }

    // 2) Mutera bara ett spår: övriga orörda, kicken orörd
    {
        Genome a = mutate (Genome {}, 2, 11);
        Genome b = mutate (a, 2, 999, 1u << Hat, false);
        for (int l = 0; l < kLanes; ++l)
            if (l != Hat) CHECK (a.lane[l].hits == b.lane[l].hits, "spår %d ändrades fast det var låst", l);
        CHECK (a.snd.ceiling == b.snd.ceiling, "ljudet ändrades");
    }

    // 3) Rendera: deterministiskt, inga NaN, inget klipp, kicken på rutnätet, inga slag på kicken där förbjudet
    const double sr = 48000;
    for (int s = 0; s < kNumSkolor; ++s)
    {
        const Skola& sk = skolor()[s];
        Engine e1, e2; e1.prepare (sr); e2.prepare (sr);
        Genome g = mutate (Genome {}, s, 1234);
        Parasite p; p.fromSchool (sk);
        e1.setGenome (g); e2.setGenome (g); e1.setParasite (p); e2.setParasite (p);
        Engine::Controls c;
        const double bpm = sk.bpm;
        const int blk = 512;
        std::vector<float> L (blk), R (blk), L2 (blk), R2 (blk);
        Event ev[256];
        double ppq = 0; float peak = 0; double sumsq = 0; long nS = 0; bool nan = false, diff = false;
        int kicksOff = 0, kicks = 0, onKick = 0, events = 0, silentBlocks = 0;
        const int blocks = (int) (sr * 30 / blk);
        for (int b = 0; b < blocks; ++b)
        {
            const int n = e1.process (L.data(), R.data(), blk, true, ppq, bpm, c, ev, 256);
            e2.process (L2.data(), R2.data(), blk, true, ppq, bpm, c, nullptr, 0);
            events += n;
            // kickar per 16-del i blocket
            for (int i = 0; i < n; ++i)
            {
                const double t = ppq + ev[i].offset * bpm / 60.0 / sr;
                const double frac = t * 4 - std::floor (t * 4 + 0.5);
                if (ev[i].lane == Kick) { ++kicks; if (std::fabs (frac) * 0.25 * 60.0 / bpm * 1000 > 1.5) ++kicksOff; }
            }
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < n; ++j)
                    if (ev[i].lane == Kick && ev[j].lane != Kick && ev[j].lane != Bas && sk.lane[ev[j].lane].notOnKick && ev[i].offset == ev[j].offset) ++onKick;
            float bp = 0;
            for (int i = 0; i < blk; ++i)
            {
                if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) nan = true;
                if (L[i] != L2[i]) diff = true;
                peak = std::max (peak, std::max (std::fabs (L[i]), std::fabs (R[i])));
                bp = std::max (bp, std::fabs (L[i]));
                sumsq += L[i] * L[i]; ++nS;
            }
            if (bp < 0.001f) ++silentBlocks;
            ppq += blk * bpm / 60.0 / sr;
        }
        const float rms = (float) std::sqrt (sumsq / nS);
        CHECK (! nan, "%s NaN", sk.name);
        CHECK (! diff, "%s inte deterministisk", sk.name);
        CHECK (peak < 0.99f, "%s klipper %.3f", sk.name, peak);
        CHECK (rms > 0.01f, "%s för tyst %.4f", sk.name, rms);
        CHECK (kicks > 20, "%s för få kickar %d", sk.name, kicks);
        CHECK (kicksOff == 0, "%s kick utanför rutnätet %d", sk.name, kicksOff);
        CHECK (onKick == 0, "%s slag på kicken trots förbud %d", sk.name, onKick);
        std::printf ("%-14s %3d bpm  kickar %3d  slag %4d  topp %.2f  rms %.3f  tysta block %d\n", sk.name, sk.bpm, kicks, events, peak, rms, silentBlocks);
    }

    // 4) Spola: samma position ger samma slag
    {
        Engine a, b; a.prepare (sr); b.prepare (sr);
        Genome g = mutate (Genome {}, 0, 77); a.setGenome (g); b.setGenome (g);
        Engine::Controls c; c.rot = 0;
        std::vector<float> L (512), R (512); Event ea[128], eb[128];
        double ppq = 0; for (int i = 0; i < 400; ++i) { a.process (L.data(), R.data(), 512, true, ppq, 120, c, ea, 128); ppq += 512 * 2.0 / sr; }
        const int na = a.process (L.data(), R.data(), 512, true, ppq, 120, c, ea, 128);
        const int nb = b.process (L.data(), R.data(), 512, true, ppq, 120, c, eb, 128);
        CHECK (na == nb, "spolning: %d mot %d slag", na, nb);
        for (int i = 0; i < std::min (na, nb); ++i) CHECK (ea[i].offset == eb[i].offset && ea[i].lane == eb[i].lane, "spolning olika");
    }

    // 5) Parasit ur onsets
    {
        std::vector<int> on; const int len = 48000 * 2, s16 = len / 16;
        for (int k = 0; k < 16; ++k) on.push_back (k * s16 + ((k & 1) ? s16 / 5 : 0));
        Parasite p; CHECK (extractParasite (on, len, 16, p), "parasit misslyckades");
        CHECK (p.off[1] > 0.15f && std::fabs (p.off[0]) < 0.02f, "parasit fel %.3f %.3f", p.off[0], p.off[1]);
    }

    std::printf (fails ? "\n%d FEL\n" : "\nALLT GRÖNT\n", fails);
    return fails ? 1 : 0;
}
