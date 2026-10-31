#pragma once
// TOMRUM — MOTORN. Ren C++ utan JUCE.
//
//  GENETIK    bygger ett genom (mönster + ljud) ur skolans regelbok. Aldrig fri slump.
//  MINUS      tar bort slag, de svaga först. Kicken rörs aldrig.
//  SPÖKE      spelar bara i tystnaden: där inget annat spår slår.
//  FRIKTION   varje spår släpar sina millisekunder bakom rutnätet. Kicken ligger rakt.
//  PARASIT    otakten: skolans mall, eller den som sugits ur en loop man släppt på panelen.
//  RUTTNA     inget låter likadant länge: ett lågpass och stereobredden ruttnar takt för takt.
//  DUB        enstaka slag kastas ner i källaren (ekot), resten står torrt.
//  VARIATION  var fjärde takt vrids svaga steg, alltid inom regelboken.
// Allt räknas ur host-positionen: spolar man hamnar samma slag på samma ställe.

#include "Skolor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace tomrum
{
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;
constexpr int kMaxLen = 32;

inline float clamp01 (float x) noexcept { return x < 0 ? 0 : (x > 1 ? 1 : x); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }
inline float mtof (float n) noexcept { return 440.0f * std::pow (2.0f, (n - 69.0f) / 12.0f); }
inline float pick (const Range& r, float t) noexcept { return lerp (r.lo, r.hi, clamp01 (t)); }

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
};

inline float stepWeight (char c) noexcept
{
    if (c == 'X') return 1.0f;
    if (c >= '0' && c <= '9') return (float) (c - '0') * 0.1f;
    return 0.0f;
}

// ---------------------------------------------------------------- genomet
struct LaneGenome
{
    int len = 16;
    uint32_t hits = 0;              // bit per steg
    float weight[kMaxLen] {};       // regelbokens sannolikhet, för MINUS och VARIATION
    bool on (int i) const noexcept { return (hits >> i) & 1u; }
};

struct Sound
{
    float kickPitch = 48, kickDecay = 0.4f, kickClick = 0.3f;
    float basDecay = 0.3f;
    float hatLen = 0.03f, hatTone = 0.6f;
    float traHz = 900, traDecay = 0.05f;
    float stabCut = 1000, ceiling = 16000;
    float delayFb = 0.5f, delayMix = 0.3f;
    float pump = 0.3f, ringmod = 0, crush = 0, crackle = 0, noiseBed = 0;
    float swing = 0, minus = 0.2f, dub = 0.1f;
};

struct Genome
{
    int skola = 0;
    uint32_t seed = 1;
    LaneGenome lane[kLanes];
    Sound snd;
};

// Bygger ett spårs mönster ur regeln. Försöker tills min/max-reglerna håller.
inline LaneGenome buildLane (const LaneRule& r, Rng& rng)
{
    int nVar = 0;
    while (nVar < 3 && r.pattern[nVar] != nullptr) ++nVar;
    LaneGenome best;
    if (nVar == 0) return best;
    const char* pat = r.pattern[(int) (rng.uni() * (float) nVar) % nVar];
    const int len = std::clamp ((int) std::strlen (pat), 1, kMaxLen);
    int bestErr = 1 << 30;
    for (int attempt = 0; attempt < 24; ++attempt)
    {
        LaneGenome g; g.len = len;
        bool prev = false; int count = 0;
        for (int i = 0; i < len; ++i)
        {
            const float w = stepWeight (pat[i]);
            g.weight[i] = w;
            float p = w;
            if (w < 1.0f && prev) p = clamp01 (p * r.follow);
            const bool hit = w >= 1.0f || (w > 0 && rng.uni() < p);
            if (hit) { g.hits |= 1u << i; ++count; }
            prev = hit;
        }
        const int err = count < r.minHits ? r.minHits - count : (count > r.maxHits ? count - r.maxHits : 0);
        if (err < bestErr) { bestErr = err; best = g; }
        if (err == 0) break;
    }
    return best;
}

inline Sound buildSound (const Skola& s, Rng& r)
{
    Sound d;
    d.kickPitch = pick (s.kickPitch, r.uni()); d.kickDecay = pick (s.kickDecay, r.uni()); d.kickClick = pick (s.kickClick, r.uni());
    d.basDecay = pick (s.basDecay, r.uni());
    d.hatLen = pick (s.hatLen, r.uni()); d.hatTone = pick (s.hatTone, r.uni());
    d.traHz = pick (s.traHz, r.uni()); d.traDecay = pick (s.traDecay, r.uni());
    d.stabCut = pick (s.stabCut, r.uni()); d.ceiling = pick (s.ceiling, r.uni());
    d.delayFb = pick (s.delayFb, r.uni()); d.delayMix = pick (s.delayMix, r.uni());
    d.pump = pick (s.pump, r.uni()); d.ringmod = pick (s.ringmod, r.uni()); d.crush = pick (s.crush, r.uni());
    d.crackle = pick (s.crackle, r.uni()); d.noiseBed = pick (s.noiseBed, r.uni());
    d.swing = pick (s.swing, r.uni()); d.minus = pick (s.minus, r.uni()); d.dub = pick (s.dub, r.uni());
    return d;
}

// GENETIK: ett helt genom. laneMask = vilka spår som byggs om (resten behålls), sound = bygg om ljudet.
inline Genome mutate (const Genome& from, int skola, uint32_t seed, uint32_t laneMask = 0xFFu, bool sound = true)
{
    Genome g = from;
    const bool newSchool = skola != from.skola;
    g.skola = std::clamp (skola, 0, kNumSkolor - 1);
    g.seed = seed;
    const Skola& s = skolor()[g.skola];
    Rng rng (seed);
    for (int l = 0; l < kLanes; ++l)
    {
        Rng lr (seed * 31u + (uint32_t) l * 977u);
        if (newSchool || (laneMask >> l) & 1u) g.lane[l] = buildLane (s.lane[l], lr);
    }
    if (sound || newSchool) g.snd = buildSound (s, rng);
    return g;
}

// PARASIT: otakt per 16-del, andel av en 16-del (+ = sent)
struct Parasite
{
    float off[16] {};
    void fromSchool (const Skola& s) noexcept
    {
        for (int i = 0; i < 16; ++i)
        {
            const char c = s.parasite[i];
            off[i] = (c >= '0' && c <= '9') ? (float) (c - '0') * 0.04f : (c >= 'a' && c <= 'c') ? -(float) (c - 'a' + 1) * 0.03f : 0.0f;
        }
    }
};

// Sugs ur en loop: transienternas läge mot 16-delsrutnätet (onsets i sampel, loopens längd, antal 16-delar)
inline bool extractParasite (const std::vector<int>& onsets, int lengthSamples, int sixteenths, Parasite& out)
{
    if (onsets.size() < 6 || sixteenths < 4 || lengthSamples <= 0) return false;
    const double s16 = (double) lengthSamples / sixteenths;
    float sum[16] {}, cnt[16] {};
    for (int t : onsets)
    {
        const double pos = t / s16;
        const double k = std::floor (pos + 0.5);
        const float off = (float) (pos - k);
        if (std::fabs (off) > 0.45f) continue;
        const int idx = ((int) k % 16 + 16) % 16;
        sum[idx] += off; cnt[idx] += 1.0f;
    }
    int filled = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (cnt[i] > 0) { out.off[i] = std::clamp (sum[i] / cnt[i], -0.15f, 0.4f); ++filled; }
        else out.off[i] = 0.0f;
    }
    return filled >= 4;
}

// ---------------------------------------------------------------- ljuden
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

struct Event { int offset; int lane; float vel; bool dub; int note; };

class Engine
{
public:
    static constexpr int kMaxEvents = 128;

    // ---- inställningar (sätts varje block) ----
    struct Controls
    {
        float minus = -1, friction = 1, parasite = 0.6f, swing = -1, dub = -1, rot = 0.5f, ceiling = 1.0f, pump = -1;
        float level[kLanes] { 1, 1, 1, 1, 1, 1, 1, 1 };
        bool mute[kLanes] {};
        float variation = 0.5f;
        int root = 9;         // A
        float out = 1.0f;
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        dlL.assign ((size_t) (sr * 3.0f) + 8, 0.0f); dlR.assign (dlL.size(), 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (dlL.begin(), dlL.end(), 0.0f); std::fill (dlR.begin(), dlR.end(), 0.0f);
        dw = 0;
        for (auto& v : voice) v = Voice {};
        for (auto& f : ceilL) f.reset();
        lastStep = -1000000; numPending = 0; rotStartPpq = -1.0; voidEnv = 0; outEnv = 0; pumpEnv = 0;
        dHpL.reset(); dHpR.reset(); dLpL.reset(); dLpR.reset(); bedLp.reset(); crushHold = 0; crushCnt = 0;
    }

    // Genomet byts från message-tråden via processorn (dubbelbuffrat där). Här kopieras det per block.
    void setGenome (const Genome& g) noexcept { gen = g; }
    void setParasite (const Parasite& p) noexcept { para = p; }
    void resetRot() noexcept { rotStartPpq = -1.0; }

    // Renderar n sampel. Skriver (inte adderar). Fyller events (för MIDI ut). Returnerar antal events.
    int process (float* L, float* R, int n, bool playing, double ppq, double bpm, const Controls& c, Event* out, int maxOut)
    {
        const Skola& s = skolor()[std::clamp (gen.skola, 0, kNumSkolor - 1)];
        const Sound& d = gen.snd;
        const float minus = c.minus >= 0 ? c.minus : d.minus;
        const float swing = c.swing >= 0 ? pick (s.swing, c.swing) : d.swing;
        const float dubP = c.dub >= 0 ? c.dub : d.dub;
        const float pump = c.pump >= 0 ? c.pump : d.pump;
        const double ppqPerSample = bpm / 60.0 / sr;
        const double sixteenth = 0.25;
        int nOut = 0;

        // ---- schemalägg slagen i [ppq, ppq+n) ----
        if (playing)
        {
            if (rotStartPpq < 0 || ppq < rotStartPpq) rotStartPpq = ppq;
            const double end = ppq + n * ppqPerSample;
            const long first = (long) std::floor (ppq / sixteenth) - 2;   // titta bakåt: slag kan ligga sent
            const long last = (long) std::floor (end / sixteenth);
            for (long k = first; k <= last; ++k)
            {
                bool anyHit = false;
                bool laneHit[kLanes] {};
                float laneVel[kLanes] {};
                for (int l = 0; l < kLanes; ++l)
                {
                    if (l == Spoke) continue;
                    if (hitAt (l, k, minus, c.variation, laneVel[l])) { laneHit[l] = true; if (l != Kick) anyHit = true; }
                }
                const bool kickHere = laneHit[Kick];
                for (int l = 0; l < kLanes; ++l)
                    if (laneHit[l] && s.lane[l].notOnKick && kickHere && l != Kick) laneHit[l] = false;
                // SPÖKE: bara i tystnaden
                if (! anyHit && ! kickHere && hitAt (Spoke, k, 0.0f, c.variation, laneVel[Spoke])) laneHit[Spoke] = true;

                for (int l = 0; l < kLanes; ++l)
                {
                    if (! laneHit[l] || c.mute[l]) continue;
                    // tid: rutnät + sväng + parasit + friktion (ms)
                    double t = (double) k * sixteenth;
                    if (l != Kick)
                    {
                        if ((k & 1) != 0) t += swing * sixteenth;
                        t += c.parasite * para.off[((k % 16) + 16) % 16] * sixteenth;
                        const auto& lr = s.lane[l];
                        const float fricMs = c.friction * lerp (lr.fricLo, lr.fricHi, h01 ((uint32_t) k, (uint32_t) l, 0xF71Cu));
                        t += fricMs * 0.001 * bpm / 60.0;
                    }
                    if (t < ppq || t >= end) continue;
                    const int off = std::clamp ((int) ((t - ppq) / ppqPerSample), 0, n - 1);
                    const bool dub = l != Kick && l != Bas && h01 ((uint32_t) k, (uint32_t) l, 0xD0Bu) < dubP * (l == Stab ? 2.0f : 1.0f);
                    if (numPending < kMaxEvents) pending[numPending++] = { off, l, laneVel[l], dub, noteFor (l, k, c.root) };
                }
            }
        }
        else rotStartPpq = -1.0;

        std::sort (pending, pending + numPending, [] (const Event& a, const Event& b) { return a.offset < b.offset; });

        // ---- RUTTNA ----
        float rotRatio = 0.0f;
        if (playing && rotStartPpq >= 0)
        {
            const double bars = (ppq - rotStartPpq) / 4.0;
            const double rotBars = 7.0 * bpm / 4.0;     // ungefär sju minuter
            rotRatio = (float) std::min (1.0, bars / rotBars) * c.rot;
        }
        const float ceilingHz = std::clamp (d.ceiling * c.ceiling, 300.0f, 20000.0f) * std::pow (2.0f, -3.2f * rotRatio);
        const float width = 1.0f - 0.7f * rotRatio;
        for (auto& f : ceilL) f.set (ceilingHz, 0.75f, sr);
        const float beatSec = (float) (60.0 / std::clamp (bpm, 40.0, 300.0));
        const float dTime = std::min ((float) dlL.size() - 4, s.delayBeats * beatSec * sr);
        dHpL.set (300, 0.7f, sr); dHpR.set (300, 0.7f, sr); dLpL.set (2200, 0.7f, sr); dLpR.set (2200, 0.7f, sr);
        bedLp.set (2500, 0.6f, sr);
        const float pumpRel = std::exp (-1.0f / (beatSec * 0.45f * sr));

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
            float kick = 0, bus = 0, send = 0, sL = 0, sR = 0;
            for (int l = 0; l < kLanes; ++l)
            {
                float v = renderVoice (l, d) * c.level[l];
                if (l == Kick) { kick += v; continue; }
                // FIENTLIG: ringmod och vikning/aliasing på perc
                if ((l >= Klick && l <= Skrap) || l == Spoke)
                {
                    if (d.ringmod > 0.001f) { rmPh += 310.0f / sr; if (rmPh > 1) rmPh -= 1; v = lerp (v, v * std::sin (kTwoPi * rmPh) * 1.6f, d.ringmod); }
                }
                const float pan = panOf (l) * width;
                sL += v * (1.0f - pan); sR += v * (1.0f + pan);
                send += v * voice[l].throwEnv;
                voice[l].throwEnv *= 0.9996f;
                bus += 0.0f;
            }
            // KASSETT: knaster som bädd
            if (d.crackle > 0.001f && rng.uni() < 0.0006f * d.crackle * 4) crackleImp = (rng.uni() * 2 - 1) * d.crackle;
            const float cr = crackleImp; crackleImp *= 0.6f;
            sL += cr * 0.5f; sR += cr * 0.4f;
            // KYLTORN: brusbädd som andas med kicken
            if (d.noiseBed > 0.001f) { const float nb = bedLp.lp (rng.uni() * 2 - 1) * d.noiseBed * 0.25f; sL += nb; sR += nb * 0.9f; }
            // KROSS
            if (d.crush > 0.001f)
            {
                const float hold = 1.0f + d.crush * 18.0f;
                if ((crushCnt += 1.0f) >= hold) { crushCnt = 0; crushHoldL = sL; crushHoldR = sR; }
                const float q = std::pow (2.0f, 12.0f - 8.0f * d.crush);
                sL = lerp (sL, std::round (crushHoldL * q) / q, d.crush); sR = lerp (sR, std::round (crushHoldR * q) / q, d.crush);
            }
            // pump
            pumpEnv *= pumpRel;
            const float duck = 1.0f - pump * 0.8f * pumpEnv;
            sL *= duck; sR *= duck;

            // DUB: bara de kastade slagen går in i källaren
            int r = dw - (int) dTime; while (r < 0) r += (int) dlL.size();
            const float dl = dlL[(size_t) r], dr = dlR[(size_t) r];
            dlL[(size_t) dw] = std::tanh (send * 1.2f + dLpL.lp (dHpL.hp (dr)) * d.delayFb);
            dlR[(size_t) dw] = std::tanh (dLpR.lp (dHpR.hp (dl)) * d.delayFb);
            if (++dw >= (int) dlL.size()) dw = 0;
            sL += dl * d.delayMix * 1.6f * duck; sR += dr * d.delayMix * 1.6f * duck;

            float Lx = kick + sL, Rx = kick + sR;
            Lx = ceilL[0].lp (Lx); Rx = ceilL[1].lp (Rx);
            Lx = std::tanh (Lx * c.out * 0.95f) * 0.95f; Rx = std::tanh (Rx * c.out * 0.95f) * 0.95f;
            if (! std::isfinite (Lx) || ! std::isfinite (Rx)) { reset(); Lx = Rx = 0; }
            L[i] = Lx; R[i] = Rx;
            const float a = std::max (std::fabs (Lx), std::fabs (Rx));
            outEnv = a > outEnv ? a : outEnv * 0.9993f;
        }
        // kvarvarande (sena) slag behålls inte: de räknas fram igen nästa block ur positionen
        numPending = 0;
        voidEnv = 1.0f - clamp01 (outEnv * 3.0f);
        rotShown = rotRatio;
        return nOut;
    }

    float voidEnv = 0.0f, rotShown = 0.0f;   // för panelen: tomrummet (1 = tyst) och hur ruttet det är
    const Genome& genome() const noexcept { return gen; }

private:
    struct Voice
    {
        float amp = 0, env = 0, coef = 0.99f, ph = 0, ph2 = 0, freq = 100, pitchEnv = 0, pitchCoef = 0.99f, click = 0;
        float throwEnv = 0, vel = 1;
        int age = 0;
        Svf f1, f2;
        float chordPh[4] {}, chordF[4] {};
    };

    float sr = 48000;
    Genome gen;
    Parasite para;
    Voice voice[kLanes];
    Event pending[kMaxEvents];
    int numPending = 0;
    long lastStep = 0;
    double rotStartPpq = -1.0;
    std::vector<float> dlL, dlR;
    int dw = 0;
    Svf ceilL[2], dHpL, dHpR, dLpL, dLpR, bedLp;
    float outEnv = 0, pumpEnv = 0, rmPh = 0, crackleImp = 0, crushHoldL = 0, crushHoldR = 0, crushCnt = 0, crushHold = 0;
    Rng rng { 7 };

    static float panOf (int l) noexcept
    {
        static const float p[kLanes] = { 0, 0, -0.25f, 0.3f, -0.35f, 0.4f, 0.0f, -0.45f };
        return p[l];
    }

    // Slår spåret på 16-del k? MINUS och VARIATION räknas här, deterministiskt.
    bool hitAt (int l, long k, float minus, float variation, float& vel) const noexcept
    {
        const auto& g = gen.lane[l];
        if (g.len <= 0) return false;
        const int idx = (int) (((k % g.len) + g.len) % g.len);
        const float w = g.weight[idx];
        bool hit = g.on (idx);
        // VARIATION: var fjärde takt vrids svaga steg (bara där regelboken tillåter slag)
        if (l != Kick && variation > 0.001f && w > 0.0f && w < 1.0f)
        {
            const uint32_t block = (uint32_t) ((k < 0 ? k - 63 : k) / 64);
            if (h01 (block, (uint32_t) l * 131u + (uint32_t) idx, gen.seed) < variation * 0.18f) hit = ! hit;
        }
        if (! hit) return false;
        // MINUS: de svaga stegen tas bort först
        if (l != Kick && minus > 0.001f && w < 1.0f)
            if (h01 ((uint32_t) k, (uint32_t) l, gen.seed ^ 0x5A5Au) < minus * (1.25f - w)) return false;
        vel = w >= 1.0f ? 1.0f : 0.55f + 0.45f * w;
        return true;
    }

    int noteFor (int l, long k, int root) const noexcept
    {
        const int base = 36 + ((root % 12) + 12) % 12; // C2..B2
        switch (l)
        {
            case Kick:  return 36;
            case Bas:
            {
                const int every = skolor()[gen.skola].basDropEvery;
                return base - 12 + ((every > 0 && (k % every) == every - 1) ? -5 : 0) + 12;
            }
            case Klick: return 37;
            case Hat:   return 42;
            case Tra:   return 76;
            case Skrap: return 70;
            case Stab:  return base + 12;
            default:    return 75;
        }
    }

    void trigger (const Event& ev, const Skola& s, const Sound& d) noexcept
    {
        auto& v = voice[ev.lane];
        v.vel = ev.vel; v.age = 0;
        if (ev.dub) v.throwEnv = 1.0f;
        switch (ev.lane)
        {
            case Kick:
                v.amp = 0.9f * ev.vel; v.coef = std::exp (-6.9f / (d.kickDecay * sr));
                v.pitchEnv = 1; v.pitchCoef = std::exp (-1.0f / (0.035f * sr)); v.freq = d.kickPitch; v.ph = 0; v.click = d.kickClick;
                break;
            case Bas:
                v.amp = 0.55f * ev.vel; v.coef = std::exp (-6.9f / (d.basDecay * sr)); v.freq = mtof ((float) ev.note); v.ph = 0;
                v.pitchEnv = 0.08f; v.pitchCoef = std::exp (-1.0f / (0.012f * sr));
                break;
            case Klick:
                v.amp = 0.5f * ev.vel; v.coef = std::exp (-6.9f / (0.004f * sr)); v.f1.set (lerp (2500, 6000, d.hatTone), 1.5f, sr);
                break;
            case Hat:
                v.amp = 0.32f * ev.vel; v.coef = std::exp (-6.9f / (std::max (0.001f, d.hatLen) * sr));
                v.f1.set (lerp (5000, 10000, d.hatTone), 0.7f, sr); v.freq = lerp (7000, 11000, d.hatTone); v.ph = 0;
                break;
            case Tra:
                v.amp = 0.5f * ev.vel; v.coef = std::exp (-6.9f / (d.traDecay * sr)); v.freq = d.traHz; v.ph = v.ph2 = 0;
                break;
            case Skrap:
                v.amp = 0.35f * ev.vel; v.coef = std::exp (-6.9f / (0.06f * sr)); v.f1.set (lerp (1500, 4000, d.hatTone), 2.5f, sr);
                break;
            case Stab:
            {
                v.amp = 0.28f * ev.vel; v.coef = std::exp (-6.9f / (0.5f * sr)); v.env = 1;
                static const int ch[4][4] = { { 0, 3, 7, 14 }, { 0, 3, 7, 10 }, { 0, 4, 7, 11 }, { 0, 7, 12, 19 } };
                const int c = std::clamp (s.chord, 0, 3);
                for (int k = 0; k < 4; ++k) { v.chordF[k] = mtof ((float) (ev.note + ch[c][k])); v.chordPh[k] = 0; }
                v.f1.set (d.stabCut, 1.2f, sr);
                break;
            }
            default: // Spöke: kontaktmik-skrap, tyst och kort
                v.amp = 0.2f * ev.vel; v.coef = std::exp (-6.9f / (0.03f * sr)); v.f1.set (1800, 3.0f, sr);
                break;
        }
    }

    float renderVoice (int l, const Sound& d) noexcept
    {
        (void) d;
        auto& v = voice[l];
        if (v.amp < 1.0e-5f) return 0.0f;
        float y = 0.0f;
        const Skola& s = skolor()[gen.skola];
        switch (l)
        {
            case Kick:
            {
                const float f = v.freq * (1.0f + 3.0f * v.pitchEnv * v.pitchEnv);
                v.ph += f / sr; if (v.ph >= 1) v.ph -= 1;
                y = std::sin (kTwoPi * v.ph) * v.amp + (rng.uni() * 2 - 1) * v.click * v.amp * 0.4f * v.pitchEnv * v.pitchEnv;
                y = std::tanh (y * 1.6f);
                v.pitchEnv *= v.pitchCoef;
                break;
            }
            case Bas:
            {
                v.ph += v.freq * (1.0f + v.pitchEnv) / sr; if (v.ph >= 1) v.ph -= 1;
                y = std::sin (kTwoPi * v.ph) * v.amp; v.pitchEnv *= v.pitchCoef;
                break;
            }
            case Klick: y = v.f1.bp (rng.uni() * 2 - 1) * v.amp * 2.0f; break;
            case Hat:
                if (s.hatSine) { v.ph += v.freq / sr; if (v.ph >= 1) v.ph -= 1; y = std::sin (kTwoPi * v.ph) * v.amp; }
                else y = v.f1.hp (rng.uni() * 2 - 1) * v.amp;
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
                const float crackle = rng.uni() < 0.08f ? (rng.uni() * 2 - 1) : 0.0f;
                y = v.f1.bp (crackle * 3.0f + (rng.uni() * 2 - 1) * 0.15f) * v.amp * 2.5f;
                break;
            }
            case Stab:
            {
                float x = 0;
                for (int k = 0; k < 4; ++k) { v.chordPh[k] += v.chordF[k] / sr; if (v.chordPh[k] >= 1) v.chordPh[k] -= 1; x += 2 * v.chordPh[k] - 1; }
                y = v.f1.lp (x * 0.25f) * v.amp;
                break;
            }
            default: y = v.f1.bp ((rng.uni() < 0.12f ? (rng.uni() * 2 - 1) : 0.0f) * 3.0f) * v.amp * 2.0f; break;
        }
        v.amp *= v.coef;
        return y;
    }
};

} // namespace tomrum
