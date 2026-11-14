// TOMRUM 2-motorn: g++ -std=c++17 -O2 -Wall -Wextra -Isrc tests/motor_test.cpp -o motor_test && ./motor_test
//   SAMMA      samma inställning och position ger exakt samma ljud
//   TOMRUM     mer tomrum = färre slag, och de som försvinner är de svaga
//   KARTAN     olika platser ger olika mönster, och en liten flytt ändrar lite
//   GUNG       MPC flyttar de udda 16-delarna, RAK flyttar ingenting, kicken står still
//   LÅTEN      breaken har ingen kick, introt har ingen bas
//   SPÖKE      slår aldrig samtidigt som ett annat slag
//   RIFF       basen håller sig i skalan
//   NIVÅ       inga NaN, inga toppar över 0 dBFS, tyst när transporten står
#include "Motor.h"
#include <cstdio>
#include <memory>
#include <set>

using namespace tr2;
static int failures = 0;
static void check (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what); if (! ok) ++failures; }
static constexpr double SR = 48000.0, BPM = 124.0;

struct Run { std::vector<float> L, R; std::vector<Event> ev; std::vector<double> evPpq; };

static Run render (const Engine::Controls& c, double bars, double startPpq = 0.0, bool playing = true)
{
    auto e = std::make_unique<Engine>(); e->prepare (SR);
    Run r; const int bs = 512; const int total = (int) (bars * 4.0 * 60.0 / BPM * SR);
    r.L.resize ((size_t) total); r.R.resize ((size_t) total);
    Event evs[256];
    for (int pos = 0; pos < total; pos += bs)
    {
        const int n = std::min (bs, total - pos);
        const double ppq = startPpq + (double) pos * BPM / 60.0 / SR;
        const int ne = e->process (r.L.data() + pos, r.R.data() + pos, n, playing, ppq, BPM, c, evs, 256);
        for (int i = 0; i < ne; ++i) { r.ev.push_back (evs[i]); r.evPpq.push_back (ppq + evs[i].offset * BPM / 60.0 / SR); }
    }
    return r;
}
static int count (const Run& r, int lane) { int n = 0; for (auto& e : r.ev) n += e.lane == lane; return n; }
static float peak (const Run& r) { float p = 0; for (size_t i = 0; i < r.L.size(); ++i) p = std::max ({ p, std::isfinite (r.L[i]) ? std::fabs (r.L[i]) : 99.f, std::isfinite (r.R[i]) ? std::fabs (r.R[i]) : 99.f }); return p; }

int main()
{
    Engine::Controls base; base.seed = 7; base.variation = 0.0f;

    // ---- SAMMA
    {
        const auto a = render (base, 4), b = render (base, 4);
        double d = 0; for (size_t i = 0; i < a.L.size(); ++i) d = std::max (d, (double) std::fabs (a.L[i] - b.L[i]));
        std::printf ("SAMMA    skillnad %.2e, %d slag på 4 takter\n", d, (int) a.ev.size());
        check (d == 0.0 && a.ev.size() == b.ev.size() && ! a.ev.empty(), "samma inställning ger exakt samma ljud");
    }

    // ---- TOMRUM
    {
        int prev = 1 << 30; bool mono = true; std::printf ("TOMRUM  ");
        for (float t : { 0.0f, 0.25f, 0.5f, 0.75f, 0.95f })
        {
            auto c = base; c.tomrum = t;
            const auto r = render (c, 4);
            int perc = 0; for (auto& e : r.ev) perc += e.lane != Kick;
            std::printf (" %.2f:%d", t, perc);
            mono = mono && perc <= prev; prev = perc;
        }
        std::printf ("\n");
        // de som blir kvar ska vara de starkaste: medelstyrkan ska öka när det blir tommare
        auto lo = base; lo.tomrum = 0.0f; auto hi = base; hi.tomrum = 0.7f;
        const auto a = render (lo, 4), b = render (hi, 4);
        auto meanVel = [] (const Run& r) { double s = 0; int n = 0; for (auto& e : r.ev) if (e.lane != Kick) { s += e.vel; ++n; } return n ? s / n : 0.0; };
        std::printf ("TOMRUM   medelstyrka fullt %.2f, tomt %.2f, kickar %d / %d\n", meanVel (a), meanVel (b), count (a, Kick), count (b, Kick));
        check (mono && meanVel (b) > meanVel (a) && count (a, Kick) == count (b, Kick), "mer tomrum = färre slag, de svaga går först, kicken står kvar");
    }

    // ---- KARTAN
    {
        auto patternOf = [&] (float x, float y)
        {
            auto e = std::make_unique<Engine>(); e->prepare (SR);
            auto c = base; c.mapX = x; c.mapY = y; c.tomrum = 0.15f; c.skolfarg = 0.2f;
            std::set<int> s; float w;
            for (int l = 1; l < kLanes; ++l) for (int i = 0; i < 16; ++i) if (e->previewHit (c, l, 0, i, w)) s.insert (l * 100 + i);
            return s;
        };
        auto diff = [] (const std::set<int>& a, const std::set<int>& b) { int d = 0; for (int x : a) d += ! b.count (x); for (int x : b) d += ! a.count (x); return d; };
        const auto koln = patternOf (0.0f, 0.0f), buk = patternOf (1.0f, 1.0f), near = patternOf (0.05f, 0.05f);
        std::printf ("KARTAN   KÖLN-BUKAREST skiljer %d steg, liten flytt %d steg\n", diff (koln, buk), diff (koln, near));
        check (diff (koln, buk) > 10 && diff (koln, near) * 5 <= diff (koln, buk), "kartan ändrar mönstret mjukt");
    }

    // ---- GUNG
    {
        auto offs = [&] (int typ, int lane)
        {
            auto c = base; c.gungTyp = typ; c.gung = 1.0f; c.friktion = 0.0f; c.tomrum = 0.0f;
            const auto r = render (c, 2);
            double worst = 0; int odd = 0;
            for (size_t i = 0; i < r.ev.size(); ++i)
            {
                if (lane >= 0 ? r.ev[i].lane != lane : r.ev[i].lane == Kick) continue;
                const double q = r.evPpq[i] * 4.0, d = q - std::round (q);
                worst = std::max (worst, std::fabs (d));
                if (std::fabs (d) > 0.05) ++odd;
            }
            return std::make_pair (worst, odd);
        };
        const auto rak = offs (GRak, -1), mpc = offs (GMpc, -1), kick = offs (GMpc, Kick);
        std::printf ("GUNG     RAK %.3f, MPC perc %.3f (%d flyttade), MPC kick %.3f\n", rak.first, mpc.first, mpc.second, kick.first);
        check (rak.first < 0.01 && mpc.first > 0.2 && kick.first < 0.01, "MPC flyttar, RAK flyttar inte, kicken står still");
    }

    // ---- LÅTEN
    {
        auto c = base; c.latOn = true; c.latBars = 64; c.tomrum = 0.1f;
        const auto intro = render (c, 4, 0.0), brk = render (c, 4, 32.0 * 4.0), groove = render (c, 4, 20.0 * 4.0);
        std::printf ("LÅTEN    intro: bas %d, break: kick %d, groove: kick %d bas %d stab %d\n",
                     count (intro, Bas), count (brk, Kick), count (groove, Kick), count (groove, Bas), count (groove, Stab));
        check (count (intro, Bas) == 0 && count (brk, Kick) == 0 && count (groove, Kick) >= 15, "låten har intro, break utan kick och groove");
    }

    // ---- SPÖKE
    {
        auto c = base; c.tomrum = 0.3f; c.laneDens[Spoke] = 1.0f; c.gungTyp = GRak; c.friktion = 0.0f;
        const auto r = render (c, 8);
        std::set<long> others; int clash = 0, sp = 0;
        for (size_t i = 0; i < r.ev.size(); ++i) if (r.ev[i].lane != Spoke) others.insert (std::lround (r.evPpq[i] * 4.0));
        for (size_t i = 0; i < r.ev.size(); ++i) if (r.ev[i].lane == Spoke) { ++sp; clash += (int) others.count (std::lround (r.evPpq[i] * 4.0)); }
        std::printf ("SPÖKE    %d spökslag, %d krockar\n", sp, clash);
        check (sp > 0 && clash == 0, "spöket spelar bara i tystnaden");
    }

    // ---- RIFF
    {
        auto c = base; c.riff = 1.0f; c.root = 9; c.skala = SMoll; c.tomrum = 0.0f;
        const auto r = render (c, 16);
        std::set<int> notes; bool inKey = true;
        static const bool moll[12] = { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0 };
        for (auto& e : r.ev) if (e.lane == Bas) { notes.insert (e.note); inKey = inKey && moll[((e.note - 9) % 12 + 12) % 12]; }
        std::printf ("RIFF     %d olika bastoner, alla i A-moll: %s\n", (int) notes.size(), inKey ? "ja" : "nej");
        check (notes.size() >= 3 && inKey, "basen spelar ett riff i skalan");
    }

    // ---- NIVÅ
    {
        float worst = 0; bool ok = true;
        for (int sk = 0; sk < kNumSkolor; ++sk)
            for (int g = 0; g < kNumGung; ++g)
            {
                auto c = base; c.skola = sk; c.gungTyp = g; c.tomrum = 0.0f; c.djup = 1.0f; c.kallare = 1.0f; c.kassett = 1.0f; c.riff = 1.0f;
                c.mapX = (float) g / 4.0f; c.mapY = 1.0f - c.mapX;
                const auto r = render (c, 4);
                worst = std::max (worst, peak (r));
                ok = ok && peak (r) <= 1.0f;
            }
        auto c = base; const auto quiet = render (c, 2, 0.0, false);
        std::printf ("NIVÅ     högsta topp %.2f (alla skolor och gung, allt på max), stilla: %.5f\n", worst, peak (quiet));
        check (ok && peak (quiet) < 1.0e-4f, "inga toppar, inga NaN, tyst när transporten står");
    }

    std::printf ("\n%s\n", failures ? "FEL" : "ALL OK");
    return failures ? 1 : 0;
}
