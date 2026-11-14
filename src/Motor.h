#pragma once
// TOMRUM 2 — MOTORN. Ren C++ utan JUCE.
//
//  KARTAN     en ruta 3 x 3. Åt sidan: KÖLN (rakt, rent) - BERLIN (dub) - BUKAREST (rullande).
//             Uppåt: RAK - RULL - BRUTEN. Varje plats har en vikt per steg och spår. Mellan platserna
//             blandas vikterna mjukt, så mönstren glider över i varandra när man drar.
//  TOMRUM     hur tomt. Ett steg slår bara om vikten klarar tröskeln, så de svaga stegen försvinner först.
//  SKOLA      regelboken (Skolor.h) färgar kartan, ger ljudet, spårens längd (polyrytm) och friktion.
//  GUNG       typ av sväng: RAK, MPC, SLÄP, BUKAREST, OTAKT. Plus friktion: varje spår släpar sina ms.
//  SPÖKE      spelar bara i tystnaden.
//  LÅTEN      arrangerar själv över 64 takter (eller 128): intro, in- och utgångar, break utan kick, fill.
//  VARIATION  var fjärde takt flyttas vikterna lite, alltid inom kartan.
// Allt räknas ur host-positionen: samma takt låter likadant varje gång.

#include "Skolor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace tr2
{
using tomrum::Skola;
using tomrum::Range;
using tomrum::kLanes;
using tomrum::skolor;
using tomrum::kNumSkolor;
enum : int { Kick = 0, Bas, Klick, Hat, Tra, Skrap, Stab, Spoke };

constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float clamp01 (float x) noexcept { return x < 0 ? 0 : (x > 1 ? 1 : x); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }
inline float mtof (float n) noexcept { return 440.0f * std::pow (2.0f, (n - 69.0f) / 12.0f); }
inline float pick (const Range& r, float t) noexcept { return lerp (r.lo, r.hi, clamp01 (t)); }
inline long fdiv (long a, long b) noexcept { return a >= 0 ? a / b : -((-a + b - 1) / b); }
inline int  wrap (long a, int n) noexcept { return (int) (((a % n) + n) % n); }

inline uint32_t hash3 (uint32_t a, uint32_t b, uint32_t c) noexcept
{
    uint32_t x = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA6Bu ^ (c + 0x165667B1u) * 0xC2B2AE35u;
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}
inline float h01 (uint32_t a, uint32_t b, uint32_t c) noexcept { return (float) (hash3 (a, b, c) >> 8) * (1.0f / 16777216.0f); }

struct Rng
{
    uint32_t s;
    explicit Rng (uint32_t seed) : s (seed * 2654435761u + 0x6a09e667u) { if (! s) s = 1; }
    uint32_t next() noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() noexcept { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
    float bi() noexcept { return uni() * 2.0f - 1.0f; }
};

inline float digit (char c) noexcept
{
    if (c == 'X') return 1.0f;
    if (c >= '0' && c <= '9') return (float) (c - '0') * 0.1f;
    return 0.0f;
}

// ---------------------------------------------------------------- KARTAN
// [x][y][spår]: x = KÖLN, BERLIN, BUKAREST. y = RAK, RULL, BRUTEN. Siffra = vikt (9 = viktigast), X = alltid.
inline const char* mapNode (int x, int y, int lane) noexcept
{
    static const char* m[3][3][kLanes] = {
        {   // KÖLN: rakt, rent, klick och offbeat-hattar
            { "X000X000X000X000", "0005000600050003", "0000800000008000", "0090009000900090", "0000000300000030", "0303030303030303", "0000000000000500", "0002000200020002" },
            { "X000X000X000X000", "0060030000600305", "0000800300008000", "3393339333933393", "0004000000400003", "0500050005000500", "0000006000000000", "0300003003000030" },
            { "X000X000X000X030", "0700003007000040", "0000900400009040", "0393039303930393", "0060000400600004", "0030300003003000", "0000700000000060", "3000030030000300" } },
        {   // BERLIN: dub, glest, ackord på offbeats, mull
            { "X000X000X000X000", "0030003000300030", "0000500000005000", "0080008000800080", "0000000000000000", "0000003000000030", "0070000000700000", "0000000200000002" },
            { "X000X000X000X000", "0040004000400640", "0000600000006003", "2080208020802080", "0000030000000300", "0003000000030000", "0070007000000070", "0200000002000000" },
            { "X000X000X000X000", "0600004006000400", "0000700000007040", "0080308000803080", "0000400000004000", "0300000003000030", "0080000700800000", "0020000000200000" } },
        {   // BUKAREST: rullande perc, svängig bas, skrap i 16-delar
            { "X000X000X000X000", "0070050000700500", "0000800000008000", "0090039000900390", "0040000400400004", "3535353535353535", "0000000000000000", "0003000300030003" },
            { "X000X000X000X000", "0700500307005030", "0000800300008003", "4494449444944494", "0050040000500400", "5353535353535353", "0000000500000000", "0300300003003000" },
            { "X000X000X000X003", "0800503008005070", "0000900500009050", "3593359335933593", "0060300600603006", "5355535353555353", "0000000600000006", "3003003030030030" } },
    };
    return m[std::clamp (x, 0, 2)][std::clamp (y, 0, 2)][std::clamp (lane, 0, kLanes - 1)];
}

// Vikten för spår l, steg i (0..15) på kartplats (mx, my) i 0..1. Bilinjär mellan de fyra närmaste platserna.
inline float mapWeight (float mx, float my, int lane, int i) noexcept
{
    const float fx = clamp01 (mx) * 2.0f, fy = clamp01 (my) * 2.0f;
    const int x0 = std::min (1, (int) fx), y0 = std::min (1, (int) fy);
    const float tx = fx - (float) x0, ty = fy - (float) y0;
    i &= 15;
    const float a = digit (mapNode (x0, y0, lane)[i]),     b = digit (mapNode (x0 + 1, y0, lane)[i]);
    const float c = digit (mapNode (x0, y0 + 1, lane)[i]), d = digit (mapNode (x0 + 1, y0 + 1, lane)[i]);
    return lerp (lerp (a, b, tx), lerp (c, d, tx), ty);
}

// ---------------------------------------------------------------- GUNG
enum GungTyp : int { GRak = 0, GMpc, GSlap, GBukarest, GOtakt, kNumGung };
inline const char* gungName (int g) noexcept
{
    static const char* n[kNumGung] = { "RAK", "MPC", u8"SLÄP", "BUKAREST", "OTAKT" };
    return n[std::clamp (g, 0, kNumGung - 1)];
}

// Förskjutning i andel av en 16-del för steg k (inte kicken). amt 0..1.
inline float gungOffset (int typ, long k, float amt, const char* parasite) noexcept
{
    const int p = wrap (k, 4);   // läge i taktslaget
    switch (typ)
    {
        case GMpc:      return (p & 1) ? amt * 0.36f : 0.0f;                                   // 50 % -> ca 68 %
        case GSlap:   { static const float s[4] { 0.0f, 0.22f, 0.12f, 0.30f }; return s[p] * amt; }   // allt lutar bakåt
        case GBukarest: { static const float s[4] { 0.0f, 0.26f, -0.06f, 0.20f }; return s[p] * amt; } // rull: trean trycker på
        case GOtakt:
        {
            const char c = parasite ? parasite[wrap (k, 16)] : '0';
            const float o = (c >= '0' && c <= '9') ? (float) (c - '0') * 0.04f : (c >= 'a' && c <= 'c') ? -(float) (c - 'a' + 1) * 0.03f : 0.0f;
            return o * amt * 1.6f;
        }
        default: return 0.0f;
    }
}

// ---------------------------------------------------------------- LÅTEN
// Ett varv = 64 takter (eller 128 om lång). Varje del säger vilka spår som spelar och hur tätt.
struct Del { const char* name; int fromBar; uint8_t lanes; float dens; bool kick; };
inline const Del* lat (int& n) noexcept
{
    // bitar: 0 kick 1 bas 2 klick 3 hatt 4 trä 5 skrap 6 stab 7 spöke
    static const Del d[] = {
        { "INTRO",   0,  0b00001101, -0.15f, true  },
        { "IN",      8,  0b00101111, -0.05f, true  },
        { "GROOVE", 16,  0b11111111,  0.00f, true  },
        { "BREAK",  32,  0b11011100, -0.10f, false },
        { "TILLBAKA",36, 0b11111111,  0.05f, true  },
        { "TOPP",   48,  0b11111111,  0.15f, true  },
        { "UT",     56,  0b10101111, -0.10f, true  },
    };
    n = (int) (sizeof (d) / sizeof (d[0]));
    return d;
}
inline int delIndex (long bar, int lenBars) noexcept
{
    int n = 0; const Del* d = lat (n);
    const int b = (int) ((wrap (bar, lenBars) * 64) / lenBars);
    int idx = 0;
    for (int i = 0; i < n; ++i) if (b >= d[i].fromBar) idx = i;
    return idx;
}

// ---------------------------------------------------------------- filter
struct Svf
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1.4f;
    void set (float fc, float q, float sr) noexcept
    {
        fc = std::clamp (fc, 10.0f, sr * 0.45f);
        const float g = std::tan (kPi * fc / sr);
        k = 1.0f / std::max (0.3f, q); a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void tick (float v0, float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = v0 - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
    float lp (float x) noexcept { float l, b, h; tick (x, l, b, h); return l; }
    float bp (float x) noexcept { float l, b, h; tick (x, l, b, h); return b; }
    float hp (float x) noexcept { float l, b, h; tick (x, l, b, h); return h; }
    void reset() noexcept { ic1 = ic2 = 0; }
};

// ---------------------------------------------------------------- ljudet (slumpas inom skolans ramar)
struct Sound
{
    float kickPitch = 48, kickDecay = 0.4f, kickClick = 0.3f, basDecay = 0.3f;
    float hatLen = 0.03f, hatTone = 0.6f, traHz = 900, traDecay = 0.05f, stabCut = 1000, ceiling = 16000;
    float delayFb = 0.5f, delayMix = 0.3f, pump = 0.3f, ringmod = 0, crush = 0, crackle = 0, noiseBed = 0, dub = 0.1f;
};
inline Sound buildSound (const Skola& s, uint32_t seed)
{
    Rng r (seed * 7u + 3u);
    Sound d;
    d.kickPitch = pick (s.kickPitch, r.uni()); d.kickDecay = pick (s.kickDecay, r.uni()); d.kickClick = pick (s.kickClick, r.uni());
    d.basDecay = pick (s.basDecay, r.uni());
    d.hatLen = pick (s.hatLen, r.uni()); d.hatTone = pick (s.hatTone, r.uni());
    d.traHz = pick (s.traHz, r.uni()); d.traDecay = pick (s.traDecay, r.uni());
    d.stabCut = pick (s.stabCut, r.uni()); d.ceiling = pick (s.ceiling, r.uni());
    d.delayFb = pick (s.delayFb, r.uni()); d.delayMix = pick (s.delayMix, r.uni());
    d.pump = pick (s.pump, r.uni()); d.ringmod = pick (s.ringmod, r.uni()); d.crush = pick (s.crush, r.uni());
    d.crackle = pick (s.crackle, r.uni()); d.noiseBed = pick (s.noiseBed, r.uni()); d.dub = pick (s.dub, r.uni());
    return d;
}

// Tonarter för bas och stab: steg i skalan (halvtoner)
enum Skala : int { SMoll = 0, SDorisk, SFrygisk, SMollPent, kNumSkalor };
inline const char* skalaName (int s) noexcept
{
    static const char* n[kNumSkalor] = { "MOLL", "DORISK", "FRYGISK", "PENTA" };
    return n[std::clamp (s, 0, kNumSkalor - 1)];
}
inline int skalaSteg (int skala, int deg) noexcept
{
    static const int sc[kNumSkalor][7] = { { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 9, 10 }, { 0, 1, 3, 5, 7, 8, 10 }, { 0, 3, 5, 7, 10, 12, 15 } };
    const int s = std::clamp (skala, 0, kNumSkalor - 1);
    const int oct = deg >= 0 ? deg / 7 : -((-deg + 6) / 7);
    return sc[s][wrap (deg, 7)] + 12 * oct;
}

struct Event { int offset; int lane; float vel; bool dub; int note; };

// ---------------------------------------------------------------- motorn
class Engine
{
public:
    static constexpr int kMaxEvents = 192;

    struct Controls
    {
        int skola = 0;
        uint32_t seed = 1;                  // ljudet
        uint32_t laneSeed[kLanes] { 1, 1, 1, 1, 1, 1, 1, 1 };   // mönstret per spår (LÅS behåller dem vid slump)
        float mapX = 0.5f, mapY = 0.3f;     // KARTAN
        float tomrum = 0.35f;               // 0 = fullt, 1 = nästan tomt
        float laneDens[kLanes] { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };   // 0.5 = som kartan
        float skolfarg = 0.4f;              // hur mycket skolans egna mönster färgar kartan
        float poly = 1.0f;                  // 0 = alla spår 16 steg, 1 = skolans längder (polyrytm)
        int gungTyp = GMpc;
        float gung = 0.5f, friktion = 1.0f;
        float variation = 0.4f;
        bool latOn = false; int latBars = 64;
        int root = 9, skala = SMoll; float riff = 0.3f;
        float djup = 0.4f;                  // DUB-kast + källare
        float kallare = 0.3f;               // rum
        float kassett = 0.0f;               // band: svaj och knaster
        float rota = 0.0f;                  // RUTTNA: tak och bredd ruttnar med tiden
        float pumpAmt = -1.0f;              // -1 = skolans
        float level[kLanes] { 1, 1, 1, 1, 1, 1, 1, 1 };
        bool mute[kLanes] {};
        float out = 1.0f;
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        dlL.assign ((size_t) (sr * 3.0f) + 8, 0.0f); dlR.assign (dlL.size(), 0.0f);
        static const int roomLen[6] { 1031, 1327, 1523, 1777, 211, 347 };
        for (int i = 0; i < 6; ++i) room[(size_t) i].assign ((size_t) std::max (8, (int) (roomLen[i] * sr / 44100.0f)), 0.0f);
        tape.assign ((size_t) (sr * 0.03f) + 8, 0.0f); tapeR.assign (tape.size(), 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (dlL.begin(), dlL.end(), 0.0f); std::fill (dlR.begin(), dlR.end(), 0.0f); dw = 0;
        for (auto& r : room) std::fill (r.begin(), r.end(), 0.0f);
        for (auto& p : roomPos) p = 0;
        std::fill (tape.begin(), tape.end(), 0.0f); std::fill (tapeR.begin(), tapeR.end(), 0.0f); tw = 0;
        for (auto& v : voice) v = Voice {};
        for (auto& f : ceil) f.reset();
        dHpL.reset(); dHpR.reset(); dLpL.reset(); dLpR.reset(); bedLp.reset(); rHp[0].reset(); rHp[1].reset();
        rotStart = -1.0; outEnv = 0; pumpEnv = 0; bedGain = 0; crackleImp = 0; tapePh = 0; roomLp[0] = roomLp[1] = 0;
        cachedSkola = -1;
    }

    // Renderar n sampel (skriver, adderar inte). Fyller events för MIDI ut. Returnerar antal.
    int process (float* L, float* R, int n, bool playing, double ppq, double bpm, const Controls& c, Event* out, int maxOut)
    {
        updateCache (c);
        const Skola& s = skolor()[cachedSkola];
        const Sound& d = snd;
        bpm = std::clamp (bpm, 40.0, 300.0);
        const double ppqPerSample = bpm / 60.0 / sr;
        int nOut = 0, numPending = 0;

        if (playing)
        {
            if (rotStart < 0 || ppq < rotStart) rotStart = ppq;
            const double end = ppq + n * ppqPerSample;
            const long first = (long) std::floor (ppq * 4.0) - 3;   // titta bakåt: slag kan ligga sent
            const long last = (long) std::floor (end * 4.0);
            for (long k = first; k <= last; ++k)
            {
                bool hit[kLanes] {}; float vel[kLanes] {};
                const long bar = fdiv (k, 16);
                const Del* del = nullptr; int nd = 0; const Del* dl = lat (nd);
                if (c.latOn) del = &dl[delIndex (bar, std::max (16, c.latBars))];
                const bool fill = wrap (bar, 8) == 7;          // sista takten i varje 8: fill
                bool anyPerc = false;
                for (int l = 0; l < kLanes; ++l)
                {
                    if (l == Spoke) continue;
                    if (del && ! ((del->lanes >> l) & 1)) continue;
                    if (del && l == Kick && ! del->kick) continue;
                    float dens = laneDensity (c, l) + (del ? del->dens : 0.0f);
                    if (fill && (l == Klick || l == Hat || l == Skrap || l == Tra)) dens += 0.25f * c.variation + 0.05f;
                    if (stepHit (c, s, l, k, dens, vel[l])) { hit[l] = true; if (l != Kick) anyPerc = true; }
                }
                // fill: kicken tystnar på sista slaget före en ny 16-takters fras
                if (fill && wrap (bar, 16) == 15 && wrap (k, 16) >= 12) hit[Kick] = false;
                for (int l = 0; l < kLanes; ++l)
                    if (hit[l] && l != Kick && s.lane[l].notOnKick && hit[Kick]) hit[l] = false;
                if (! anyPerc && ! hit[Kick] && (! del || ((del->lanes >> Spoke) & 1)))
                    if (stepHit (c, s, Spoke, k, laneDensity (c, Spoke), vel[Spoke])) hit[Spoke] = true;

                for (int l = 0; l < kLanes; ++l)
                {
                    if (! hit[l] || c.mute[l]) continue;
                    double t = (double) k * 0.25;
                    if (l != Kick)
                    {
                        t += gungOffset (c.gungTyp, k, c.gung, s.parasite) * 0.25;
                        const auto& lr = s.lane[l];
                        const float ms = c.friktion * lerp (lr.fricLo, lr.fricHi, h01 ((uint32_t) k, (uint32_t) l, 0xF71Cu));
                        t += ms * 0.001 * bpm / 60.0;
                    }
                    if (t < ppq || t >= end) continue;
                    const int off = std::clamp ((int) ((t - ppq) / ppqPerSample), 0, n - 1);
                    const float dubP = c.djup * (0.3f + 1.4f * d.dub) * (l == Stab ? 1.8f : (l == Klick || l == Tra ? 1.0f : 0.5f));
                    const bool dub = l != Kick && l != Bas && h01 ((uint32_t) k, (uint32_t) l, 0xD0Bu ^ c.laneSeed[l]) < dubP;
                    if (numPending < kMaxEvents) pending[numPending++] = { off, l, vel[l], dub, noteFor (c, s, l, k) };
                }
            }
        }
        else rotStart = -1.0;
        std::sort (pending, pending + numPending, [] (const Event& a, const Event& b) { return a.offset < b.offset; });
        lastDel = c.latOn && playing ? delIndex (fdiv ((long) std::floor (ppq * 4.0), 16), std::max (16, c.latBars)) : -1;

        // ---- RUTTNA: taket sjunker och bredden smalnar med tiden
        float rot = 0.0f;
        if (playing && rotStart >= 0) rot = (float) std::min (1.0, (ppq - rotStart) / 4.0 / (7.0 * bpm / 4.0)) * c.rota;
        const float ceilHz = std::clamp (d.ceiling, 300.0f, 20000.0f) * std::pow (2.0f, -3.2f * rot);
        const float width = 1.0f - 0.7f * rot;
        for (auto& f : ceil) f.set (ceilHz, 0.75f, sr);
        const float beatSec = (float) (60.0 / bpm);
        const float dTime = std::min ((float) dlL.size() - 4, s.delayBeats * beatSec * sr);
        dHpL.set (300, 0.7f, sr); dHpR.set (300, 0.7f, sr); dLpL.set (2200, 0.7f, sr); dLpR.set (2200, 0.7f, sr);
        bedLp.set (2500, 0.6f, sr); rHp[0].set (180, 0.7f, sr); rHp[1].set (180, 0.7f, sr);
        const float pumpRel = std::exp (-1.0f / (beatSec * 0.45f * sr));
        const float pump = c.pumpAmt >= 0 ? c.pumpAmt : d.pump;
        const float roomFb = 0.72f + 0.2f * c.kallare, roomA = 1.0f - std::exp (-kTwoPi * 3800.0f / sr);
        const float tapeDepth = c.kassett * 0.0018f * sr;

        int e = 0;
        for (int i = 0; i < n; ++i)
        {
            while (e < numPending && pending[e].offset <= i)
            {
                const auto& ev = pending[e++];
                trigger (ev, s, d);
                if (ev.lane == Kick) pumpEnv = 1.0f;
                if (nOut < maxOut) out[nOut++] = ev;
            }
            float kick = 0, send = 0, sL = 0, sR = 0, roomIn = 0;
            for (int l = 0; l < kLanes; ++l)
            {
                float v = renderVoice (l, s, d) * c.level[l];
                if (l == Kick) { kick += v; continue; }
                if (((l >= Klick && l <= Skrap) || l == Spoke) && d.ringmod > 0.001f)
                { rmPh += 310.0f / sr; if (rmPh > 1) rmPh -= 1; v = lerp (v, v * std::sin (kTwoPi * rmPh) * 1.6f, d.ringmod); }
                const float pan = panOf (l) * width;
                sL += v * (1.0f - pan); sR += v * (1.0f + pan);
                send += v * voice[l].throwEnv;
                voice[l].throwEnv *= 0.9996f;
                roomIn += v * (l == Bas ? 0.0f : 1.0f);
            }
            bedGain += ((playing ? 1.0f : 0.0f) - bedGain) * 0.0005f;   // bäddarna tystnar när transporten står
            if (d.crackle > 0.001f || c.kassett > 0.001f)
            {
                const float amt = std::max (d.crackle, c.kassett * 0.5f) * bedGain;
                if (noise.uni() < 0.0024f * amt) crackleImp = noise.bi() * amt;
                sL += crackleImp * 0.5f; sR += crackleImp * 0.4f; crackleImp *= 0.6f;
            }
            if (d.noiseBed > 0.001f) { const float nb = bedLp.lp (noise.bi()) * d.noiseBed * 0.25f * bedGain; sL += nb; sR += nb * 0.9f; }
            if (d.crush > 0.001f)
            {
                const float hold = 1.0f + d.crush * 18.0f;
                if ((crushCnt += 1.0f) >= hold) { crushCnt = 0; crushL = sL; crushR = sR; }
                const float q = std::pow (2.0f, 12.0f - 8.0f * d.crush);
                sL = lerp (sL, std::round (crushL * q) / q, d.crush); sR = lerp (sR, std::round (crushR * q) / q, d.crush);
            }
            pumpEnv *= pumpRel;
            const float duck = 1.0f - pump * 0.8f * pumpEnv;
            sL *= duck; sR *= duck;

            // DUB: de kastade slagen går in i källarens eko
            int r = dw - (int) dTime; while (r < 0) r += (int) dlL.size();
            const float dl = dlL[(size_t) r], dr = dlR[(size_t) r];
            dlL[(size_t) dw] = std::tanh (send * 1.2f + dLpL.lp (dHpL.hp (dr)) * d.delayFb);
            dlR[(size_t) dw] = std::tanh (dLpR.lp (dHpR.hp (dl)) * d.delayFb);
            if (++dw >= (int) dlL.size()) dw = 0;
            const float dubMix = d.delayMix * (0.6f + 1.4f * c.djup);
            sL += dl * dubMix * duck; sR += dr * dubMix * duck;

            // KÄLLARE: litet betongrum (fyra kamfilter + två allpass), bas och kick hålls torra
            if (c.kallare > 0.001f)
            {
                const float x = rHp[0].hp ((roomIn + send * 0.5f) * 0.25f);
                float acc = 0;
                for (int k = 0; k < 4; ++k)
                {
                    auto& b = room[(size_t) k]; int& p = roomPos[k];
                    const float y = b[(size_t) p];
                    roomLp[k & 1] += roomA * (y - roomLp[k & 1]);
                    b[(size_t) p] = x + roomLp[k & 1] * roomFb;
                    if (++p >= (int) b.size()) p = 0;
                    acc += y;
                }
                for (int k = 4; k < 6; ++k)
                {
                    auto& b = room[(size_t) k]; int& p = roomPos[k];
                    const float z = b[(size_t) p], y = -0.6f * acc + z;
                    b[(size_t) p] = acc + 0.6f * y; if (++p >= (int) b.size()) p = 0;
                    acc = y;
                }
                const float rw = c.kallare * 0.55f * duck;
                sL += acc * rw; sR += (acc * 0.85f + roomLp[1] * 0.15f) * rw;
            }

            float Lx = kick + sL, Rx = kick + sR;
            // KASSETT: svaj (wow) på hela bussen
            if (c.kassett > 0.001f)
            {
                tape[(size_t) tw] = Lx; tapeR[(size_t) tw] = Rx;
                tapePh += 0.55f / sr; if (tapePh > 1) tapePh -= 1;
                const float dly = 2.0f + tapeDepth * (1.0f + std::sin (kTwoPi * tapePh) + 0.3f * std::sin (kTwoPi * tapePh * 7.3f));
                float rp = (float) tw - dly; while (rp < 0) rp += (float) tape.size();
                const int i0 = (int) rp; const float fr = rp - (float) i0; const int i1 = (i0 + 1) % (int) tape.size();
                const float wl = tape[(size_t) i0] + fr * (tape[(size_t) i1] - tape[(size_t) i0]);
                const float wr = tapeR[(size_t) i0] + fr * (tapeR[(size_t) i1] - tapeR[(size_t) i0]);
                if (++tw >= (int) tape.size()) tw = 0;
                Lx = lerp (Lx, wl, std::min (1.0f, c.kassett * 2.0f)); Rx = lerp (Rx, wr, std::min (1.0f, c.kassett * 2.0f));
            }
            Lx = ceil[0].lp (Lx); Rx = ceil[1].lp (Rx);
            Lx = std::tanh (Lx * c.out * 0.95f) * 0.95f; Rx = std::tanh (Rx * c.out * 0.95f) * 0.95f;
            if (! std::isfinite (Lx) || ! std::isfinite (Rx)) { reset(); Lx = Rx = 0; }
            L[i] = Lx; R[i] = Rx;
            const float a = std::max (std::fabs (Lx), std::fabs (Rx));
            outEnv = a > outEnv ? a : outEnv * 0.9993f;
        }
        rotShown = rot;
        return nOut;
    }

    // ---- för panelen
    float rotShown = 0.0f;
    int lastDel = -1;
    // Vikten (0..1) och om steget slår, för att rita mönstret: spår l, steg i i takten 'bar'
    bool previewHit (const Controls& c, int l, long bar, int i, float& w) noexcept
    {
        updateCache (c);
        const Skola& s = skolor()[cachedSkola];
        float v = 0;
        const long k = bar * 16 + i;
        w = weightAt (c, s, l, k);
        return stepHit (c, s, l, k, laneDensity (c, l), v);
    }
    int laneLength (const Controls& c, int l) noexcept { updateCache (c); return laneLen (c, l); }

private:
    struct Voice
    {
        float amp = 0, coef = 0.99f, ph = 0, ph2 = 0, freq = 100, pitchEnv = 0, pitchCoef = 0.99f, click = 0;
        float throwEnv = 0, vel = 1, glideFrom = 0, glide = 0, body = 0, bodyCoef = 0.99f;
        Svf f1, f2;
        float chordPh[4] {}, chordF[4] {};
        float metPh[6] {};
    };

    float sr = 48000;
    Sound snd;
    int cachedSkola = -1; uint32_t cachedSeed = 0;
    int schoolVariant[kLanes] {};
    Voice voice[kLanes];
    Event pending[kMaxEvents];
    double rotStart = -1.0;
    std::vector<float> dlL, dlR, tape, tapeR;
    std::array<std::vector<float>, 6> room;
    int roomPos[6] {}; float roomLp[2] {};
    int dw = 0, tw = 0;
    Svf ceil[2], dHpL, dHpR, dLpL, dLpR, bedLp, rHp[2];
    float bedGain = 0, outEnv = 0, pumpEnv = 0, rmPh = 0, crackleImp = 0, crushL = 0, crushR = 0, crushCnt = 0, tapePh = 0;
    Rng noise { 7 };

    void updateCache (const Controls& c) noexcept
    {
        const int sk = std::clamp (c.skola, 0, kNumSkolor - 1);
        uint32_t key = c.seed * 2654435761u;
        for (int l = 0; l < kLanes; ++l) key = hash3 (key, c.laneSeed[l], (uint32_t) l);
        if (sk == cachedSkola && key == cachedSeed) return;
        cachedSkola = sk; cachedSeed = key;
        const Skola& s = skolor()[sk];
        snd = buildSound (s, c.seed);
        for (int l = 0; l < kLanes; ++l)
        {
            int nv = 0; while (nv < 3 && s.lane[l].pattern[nv] != nullptr) ++nv;
            schoolVariant[l] = nv > 0 ? (int) (h01 (c.laneSeed[l], (uint32_t) l, 0x5C01u) * (float) nv) % nv : -1;
        }
    }

    static float panOf (int l) noexcept
    {
        static const float p[kLanes] = { 0, 0, -0.25f, 0.3f, -0.35f, 0.4f, 0.0f, -0.45f };
        return p[l];
    }

    const char* schoolPattern (const Skola& s, int l) const noexcept
    {
        const int v = schoolVariant[l];
        return v >= 0 ? s.lane[l].pattern[v] : nullptr;
    }

    int laneLen (const Controls& c, int l) const noexcept
    {
        if (c.poly < 0.5f || l == Kick) return 16;
        const char* p = schoolPattern (skolor()[cachedSkola], l);
        return p ? std::clamp ((int) std::strlen (p), 3, 32) : 16;
    }

    float laneDensity (const Controls& c, int l) const noexcept
    {
        const float g = 1.0f - clamp01 (c.tomrum);
        if (l == Kick) return g > 0.04f ? 1.0f : 0.0f;
        return std::clamp (g * (0.3f + 1.4f * c.laneDens[l]), 0.0f, 1.2f);
    }

    float weightAt (const Controls& c, const Skola& s, int l, long k) const noexcept
    {
        const int len = laneLen (c, l);
        const int idx = wrap (k, len);
        float w = mapWeight (c.mapX, c.mapY, l, idx);
        if (const char* p = schoolPattern (s, l))
        {
            const int pl = (int) std::strlen (p);
            w = lerp (w, digit (p[idx % pl]), clamp01 (c.skolfarg));
        }
        // VARIATION: var fjärde takt flyttas vikterna lite (aldrig för kicken på X)
        if (c.variation > 0.001f && w < 1.0f)
        {
            const uint32_t block = (uint32_t) fdiv (k, 64);
            w += (h01 (block, (uint32_t) (l * 131 + idx), c.laneSeed[l] ^ 0xA11Au) - 0.5f) * 0.3f * c.variation;
        }
        // mönstrets eget frö: varje spår får sin egen lilla förskjutning av vikterna (samma varje takt)
        if (w < 1.0f)
        {
            w += (h01 ((uint32_t) idx, (uint32_t) l, c.laneSeed[l] ^ 0x5EEDu) - 0.5f) * 0.22f;
        }
        return w;
    }

    bool stepHit (const Controls& c, const Skola& s, int l, long k, float dens, float& vel) const noexcept
    {
        if (dens <= 0.0f) return false;
        const float w = weightAt (c, s, l, k);
        if (w <= 0.02f) return false;
        if (w < 1.0f - dens + 0.05f) return false;
        vel = w >= 1.0f ? 1.0f : 0.5f + 0.5f * clamp01 (w);
        return true;
    }

    // RIFF: basen och staben går på skalsteg. Låg riff = grundton, hög = små figurer med kvinter och oktaver.
    int noteFor (const Controls& c, const Skola& s, int l, long k) const noexcept
    {
        const int base = 36 + wrap (c.root, 12);   // C2..B2
        auto riffDeg = [&] (uint32_t salt) -> int
        {
            const long bar = fdiv (k, 16);
            const uint32_t phrase = (uint32_t) fdiv (bar, 4);
            const uint32_t rs = c.laneSeed[salt == 0xBA55u ? Bas : Stab];
            if (h01 ((uint32_t) wrap (k, 16), phrase, rs ^ salt) >= c.riff) return 0;
            static const int degs[8] = { 0, 4, 7, -3, 2, 0, 5, -7 };   // grund, kvint, oktav, under ...
            return degs[(int) (h01 ((uint32_t) wrap (k, 16), phrase * 7u, rs ^ salt ^ 0x77u) * 7.999f)];
        };
        switch (l)
        {
            case Kick:  return 36;
            case Bas:
            {
                int deg = riffDeg (0xBA55u);
                if (s.basDropEvery > 0 && wrap (k, s.basDropEvery) == s.basDropEvery - 1 && c.riff < 0.2f) deg = -3;
                return base + skalaSteg (c.skala, deg);
            }
            case Klick: return 37;
            case Hat:   return 42;
            case Tra:   return 76;
            case Skrap: return 70;
            case Stab:  return base + 12 + skalaSteg (c.skala, riffDeg (0x57ABu) == 0 ? 0 : 3);
            default:    return 75;
        }
    }

    void trigger (const Event& ev, const Skola& s, const Sound& d) noexcept
    {
        auto& v = voice[ev.lane];
        v.vel = ev.vel;
        if (ev.dub) v.throwEnv = 1.0f;
        switch (ev.lane)
        {
            case Kick:
                // två steg: snabbt klick-svep och en rund kropp som sjunker en halvton
                v.amp = 0.9f * ev.vel; v.coef = std::exp (-6.9f / (d.kickDecay * sr));
                v.pitchEnv = 1; v.pitchCoef = std::exp (-1.0f / (0.03f * sr)); v.freq = d.kickPitch; v.ph = 0; v.click = d.kickClick;
                v.body = 1; v.bodyCoef = std::exp (-1.0f / (0.25f * sr));
                break;
            case Bas:
            {
                const float f = mtof ((float) ev.note);
                v.glideFrom = v.amp > 0.05f ? v.freq : f; v.glide = v.amp > 0.05f ? 1.0f : 0.0f;
                v.amp = 0.55f * ev.vel; v.coef = std::exp (-6.9f / (d.basDecay * sr)); v.freq = f;
                v.pitchEnv = 0.06f; v.pitchCoef = std::exp (-1.0f / (0.012f * sr));
                break;
            }
            case Klick:
                v.amp = 0.5f * ev.vel; v.coef = std::exp (-6.9f / (0.004f * sr)); v.f1.set (lerp (2500, 6000, d.hatTone), 1.5f, sr);
                break;
            case Hat:
                v.amp = 0.3f * ev.vel; v.coef = std::exp (-6.9f / (std::max (0.001f, d.hatLen) * sr));
                v.f1.set (lerp (6000, 10000, d.hatTone), 0.8f, sr); v.freq = lerp (7000, 11000, d.hatTone); v.ph = 0;
                break;
            case Tra:
                v.amp = 0.5f * ev.vel; v.coef = std::exp (-6.9f / (d.traDecay * sr)); v.freq = d.traHz * (1.0f + 0.03f * (ev.vel - 0.75f)); v.ph = v.ph2 = 0;
                break;
            case Skrap:
                v.amp = 0.35f * ev.vel; v.coef = std::exp (-6.9f / (0.06f * sr)); v.f1.set (lerp (1500, 4000, d.hatTone), 2.5f, sr);
                break;
            case Stab:
            {
                v.amp = 0.26f * ev.vel; v.coef = std::exp (-6.9f / (0.5f * sr));
                static const int ch[4][4] = { { 0, 3, 7, 14 }, { 0, 3, 7, 10 }, { 0, 4, 7, 11 }, { 0, 7, 12, 19 } };
                const int cc = std::clamp (s.chord, 0, 3);
                for (int k = 0; k < 4; ++k) { v.chordF[k] = mtof ((float) (ev.note + ch[cc][k])) * (1.0f + 0.002f * (float) (k - 1)); v.chordPh[k] = 0.25f * (float) k; }
                v.f1.set (d.stabCut, 1.2f, sr);
                break;
            }
            default:
                v.amp = 0.2f * ev.vel; v.coef = std::exp (-6.9f / (0.03f * sr)); v.f1.set (1800, 3.0f, sr);
                break;
        }
    }

    float renderVoice (int l, const Skola& s, const Sound&) noexcept
    {
        auto& v = voice[l];
        if (v.amp < 1.0e-5f) return 0.0f;
        float y = 0.0f;
        switch (l)
        {
            case Kick:
            {
                const float f = v.freq * (1.0f + 3.0f * v.pitchEnv * v.pitchEnv) * (1.0f - 0.06f * (1.0f - v.body));
                v.ph += f / sr; if (v.ph >= 1) v.ph -= 1;
                y = std::sin (kTwoPi * v.ph) * v.amp + noise.bi() * v.click * v.amp * 0.4f * v.pitchEnv * v.pitchEnv;
                y = std::tanh (y * 1.6f);
                v.pitchEnv *= v.pitchCoef; v.body *= v.bodyCoef;
                break;
            }
            case Bas:
            {
                float f = v.freq;
                if (v.glide > 0.001f) { f = v.freq + (v.glideFrom - v.freq) * v.glide; v.glide *= 0.9985f; }
                v.ph += f * (1.0f + v.pitchEnv) / sr; if (v.ph >= 1) v.ph -= 1;
                const float sn = std::sin (kTwoPi * v.ph);
                y = std::tanh (sn * 1.3f) * 0.85f * v.amp; v.pitchEnv *= v.pitchCoef;
                break;
            }
            case Klick: y = v.f1.bp (noise.bi()) * v.amp * 2.0f; break;
            case Hat:
                if (s.hatSine) { v.ph += v.freq / sr; if (v.ph >= 1) v.ph -= 1; y = std::sin (kTwoPi * v.ph) * v.amp; }
                else
                {
                    // metallisk: sex fyrkanter (som 808) blandade med brus
                    static const float ratio[6] { 1.0f, 1.4471f, 1.6170f, 1.9265f, 2.5028f, 2.6637f };
                    float m = 0;
                    for (int k = 0; k < 6; ++k) { v.metPh[k] += 0.05f * v.freq * ratio[k] / sr; if (v.metPh[k] >= 1) v.metPh[k] -= 1; m += v.metPh[k] < 0.5f ? 1.0f : -1.0f; }
                    y = v.f1.hp (m * 0.12f + noise.bi() * 0.6f) * v.amp;
                }
                break;
            case Tra:
            {
                v.ph += v.freq / sr; if (v.ph >= 1) v.ph -= 1;
                v.ph2 += v.freq * 2.76f / sr; if (v.ph2 >= 1) v.ph2 -= 1;
                y = (std::sin (kTwoPi * v.ph) + 0.35f * std::sin (kTwoPi * v.ph2)) * v.amp;
                break;
            }
            case Skrap:
            {
                const float cr = noise.uni() < 0.08f ? noise.bi() : 0.0f;
                y = v.f1.bp (cr * 3.0f + noise.bi() * 0.15f) * v.amp * 2.5f;
                break;
            }
            case Stab:
            {
                float x = 0;
                for (int k = 0; k < 4; ++k) { v.chordPh[k] += v.chordF[k] / sr; if (v.chordPh[k] >= 1) v.chordPh[k] -= 1; x += 2 * v.chordPh[k] - 1; }
                y = v.f1.lp (x * 0.25f) * v.amp;
                break;
            }
            default: y = v.f1.bp ((noise.uni() < 0.12f ? noise.bi() : 0.0f) * 3.0f) * v.amp * 2.0f; break;
        }
        v.amp *= v.coef;
        return y;
    }
};

} // namespace tr2
