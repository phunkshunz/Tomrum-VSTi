#pragma once
// TOMRUM — PRESETS OCH SPARFORMAT. Ren C++ utan JUCE.
//
// 800 presets: 8 skolor x 100. Varje preset är ett fast frö till GENETIK, så det kommer
// alltid tillbaka exakt likadant. Genomet sparas som text i projektet, så ett tärnat
// groove överlever även om regelböckerna ändras i en senare version.

#include "Tomrum.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace tomrum
{
constexpr int kPresetsPerSkola = 100;
constexpr int kNumPresets = kNumSkolor * kPresetsPerSkola;

inline int presetSkola (int i) noexcept { return std::clamp (i / kPresetsPerSkola, 0, kNumSkolor - 1); }
inline uint32_t presetSeed (int i) noexcept { return 0x70A0u + (uint32_t) i * 7919u; }

inline std::string presetName (int i)
{
    static const char* w[kNumSkolor][25] = {
        { "DOMEN 4 PÅ NATTEN", "BETONGVÄGG", "SÄNKT", "BOTTENSLAM", "SCHAFFEL", "RHEN", "TRYCK", "KAJEN", "BRUNNEN", "DIMMAN",
          "SVALL", "SLAMMER", "KÖLVATTEN", "PELARE", "DÄMPAT", "UNDERJORD", "TUNG LUFT", "PUMPHUS", "MÖRKER", "SJUNKA",
          "NATTBUSS", "DYKARE", "GRUNDVATTEN", "SKUM", "TRYCKFALL" },
        { "PINCETT", "SPASTISK HATT", "SKALPELL", "1 MS", "NÅLSTICK", "STERIL", "VIT", "SINUSFALL", "LABB", "SUTUR",
          "KLORIN", "KAKEL", "MILLIMETER", "PUNKT", "NOLL", "SPRUTA", "LUPP", "SKÄRVA", "SKRAPA", "ISOLERAD",
          "STRÖMBRYTARE", "TOM", "RENT", "DIOD", "SNITT" },
        { "ASKKOPP", "TRÄ MARKOV", "KASTRULL", "15 AV 16", "KNARRSTOL", "UTBRÄND", "SEG", "MURKEN", "LÖVHÖG", "SLÄPIG",
          "BLÖT TVÅL", "TRÖTT", "GRUSVÄG", "SOLUPPGÅNG", "KORK", "LÅNG NATT", "GÅRDEN", "RUTTEN", "EFTERFEST", "VARV",
          "SOFFA", "SJUNKER", "RIS", "KÄLLARE", "TRÄGOLV" },
        { "TRASIG MPC", "INGEN FRAMTID", "7 AV 8", "RINGMOD", "VIKNING", "FIENTLIG", "SNUBBEL", "MASKINFEL", "FALSK FUNK", "KORTSLUTNING",
          "STÖRNING", "SKROT", "RELÄ", "SPÄRR", "KUGGE", "KASS", "GNISTA", "LÖSNING", "BRUS", "KOLLAPS",
          "FELKOD", "OSYNK", "MOTSTÅND", "KABEL", "BLY" },
        { "KYLTORN", "MOLL 9", "BANDEKO", "VIT ÅNGA", "HAMN", "DJUPET", "ÖVERGIVET", "KONDENS", "SKORSTEN", "ÅNGA",
          "VÄRMEVÄXLARE", "FÖRÅNGARE", "KONDENSOR", "DUBBEL", "TORN", "SKIKT", "TÅKA", "SÄNKA", "EKO", "KÄLLARE II",
          "FUKT", "SVALKA", "RÖKEN", "ISKALL", "LÅGTRYCK" },
        { "EUKLID", "SINUS", "PRIMTAL", "RUTNÄT", "PIXEL", "BINÄR", "FREKVENS", "IMPULS", "MATRIS", "MODUL",
          "KVADRAT", "VEKTOR", "PROTOKOLL", "SERIE", "INDEX", "NOD", "SKANNA", "FÄLT", "ÖVERTON", "SYNK",
          "KOD", "BIT", "PULS", "LINJE", "DATA" },
        { "REGN", "KVARGLÖMD", "BANDSALLAD", "KNASTER", "SPOLE", "TRÖTT MOTOR", "C60", "BAKSIDA", "NOSTALGI", "DAMMIG",
          "BALKONG", "ARKIV", "HÖSTEN", "GAMMAL KÄRLEK", "VINDEN", "DAGBOK", "BLEKT", "SOMMAR 97", "MINNE", "STOFT",
          "LÅDA", "TRASIG LINS", "VÄNTRUM", "TÅGFÖNSTER", "FILT" },
        { "TRÄKLOSS", "BONGO", "TRIOL", "ROLLANDE", "FASLÅST", "TORR", "METALL", "KNACK", "TAJT", "BAMBU",
          "PÄRLA", "SNÄCKA", "KVIST", "BARK", "STEN", "KOTTE", "GLÄNTA", "FRÖ", "SKAL", "STOCK",
          "LÅDA II", "PLANKA", "SPIK", "KLAPPER", "SNICKARE" } };
    const int s = presetSkola (i), k = i % kPresetsPerSkola;
    static const char* suffix[4] = { "", " II", " III", " IV" };
    char num[8];
    std::snprintf (num, sizeof num, "%03d", k + 1);
    return std::string (skolor()[s].name) + " " + num + " " + w[s][k % 25] + suffix[(k / 25) % 4];
}

// Bygger preset i. Frysta spår (bit per spår i keepMask) tas från 'current'.
inline Genome presetGenome (int i, const Genome& current, uint32_t keepMask)
{
    Genome g = mutate (Genome {}, presetSkola (i), presetSeed (i));
    for (int l = 0; l < kLanes; ++l)
        if ((keepMask >> l) & 1u) g.lane[l] = current.lane[l];
    return g;
}

// ---------------------------------------------------------------- sparformat
inline float* soundField (Sound& s, int i) noexcept
{
    float* f[] = { &s.kickPitch, &s.kickDecay, &s.kickClick, &s.basDecay, &s.hatLen, &s.hatTone, &s.traHz, &s.traDecay,
                   &s.stabCut, &s.ceiling, &s.delayFb, &s.delayMix, &s.pump, &s.ringmod, &s.crush, &s.crackle,
                   &s.noiseBed, &s.swing, &s.minus, &s.dub };
    return (i >= 0 && i < 20) ? f[i] : nullptr;
}
constexpr int kSoundFields = 20;

// "skola;frö;spår0;spår1;...;ljud"  spår = "längd:träffar(hex):vikt,vikt,..."  (vikt i tiondelar)
inline std::string genomeToString (const Genome& g)
{
    std::string s = std::to_string (g.skola) + ";" + std::to_string (g.seed);
    for (int l = 0; l < kLanes; ++l)
    {
        const auto& lg = g.lane[l];
        char head[48];
        std::snprintf (head, sizeof head, ";%d:%x:", lg.len, (unsigned) lg.hits);
        s += head;
        for (int i = 0; i < lg.len && i < kMaxLen; ++i)
        {
            if (i) s += ',';
            s += std::to_string ((int) std::lround (lg.weight[i] * 10.0f));
        }
    }
    s += ";";
    Sound snd = g.snd;
    for (int i = 0; i < kSoundFields; ++i)
    {
        char b[32];
        std::snprintf (b, sizeof b, "%s%.6g", i ? "," : "", (double) *soundField (snd, i));
        s += b;
    }
    return s;
}

inline std::vector<std::string> splitStr (const std::string& s, char sep)
{
    std::vector<std::string> out;
    size_t a = 0;
    for (;;)
    {
        const size_t b = s.find (sep, a);
        out.push_back (s.substr (a, b == std::string::npos ? std::string::npos : b - a));
        if (b == std::string::npos) break;
        a = b + 1;
    }
    return out;
}

inline bool genomeFromString (const std::string& text, Genome& out)
{
    const auto part = splitStr (text, ';');
    if ((int) part.size() != 2 + kLanes + 1) return false;
    Genome g;
    g.skola = std::clamp (std::atoi (part[0].c_str()), 0, kNumSkolor - 1);
    g.seed = (uint32_t) std::strtoul (part[1].c_str(), nullptr, 10);
    for (int l = 0; l < kLanes; ++l)
    {
        const auto f = splitStr (part[(size_t) (2 + l)], ':');
        if (f.size() != 3) return false;
        auto& lg = g.lane[l];
        lg.len = std::clamp (std::atoi (f[0].c_str()), 0, kMaxLen);
        lg.hits = (uint32_t) std::strtoul (f[1].c_str(), nullptr, 16);
        const auto w = splitStr (f[2], ',');
        for (int i = 0; i < kMaxLen; ++i)
            lg.weight[i] = (i < (int) w.size() && i < lg.len) ? std::clamp ((float) std::atoi (w[(size_t) i].c_str()) * 0.1f, 0.0f, 1.0f) : 0.0f;
    }
    const auto v = splitStr (part.back(), ',');
    if ((int) v.size() != kSoundFields) return false;
    for (int i = 0; i < kSoundFields; ++i)
    {
        const float x = (float) std::atof (v[(size_t) i].c_str());
        if (! std::isfinite (x)) return false;
        *soundField (g.snd, i) = x;
    }
    out = g;
    return true;
}

// Var i skolans spann ett absolut värde ligger (för rattarna), 0..1
inline float rangePos (const Range& r, float v) noexcept
{
    return r.hi > r.lo ? clamp01 ((v - r.lo) / (r.hi - r.lo)) : 0.5f;
}
} // namespace tomrum
