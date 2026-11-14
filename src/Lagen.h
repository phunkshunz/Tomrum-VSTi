#pragma once
// TOMRUM — LÄGEN. Varje skola får platser på kartan med eget gung, riff och djup.
#include "PluginProcessor.h"

inline const std::vector<TomrumPreset>& tomrumLagen()
{
    static const std::vector<TomrumPreset> all = []
    {
        std::vector<TomrumPreset> v;
        struct Plats { const char* name; float x, y, tomrum; int gung; float gungAmt, riff, djup, kallare; };
        static const Plats platser[] = {
            { u8"Första Timmen",   0.15f, 0.10f, 0.40f, 1, 0.45f, 0.15f, 0.30f, 0.25f },
            { u8"Mellan Slagen",   0.30f, 0.45f, 0.30f, 4, 0.60f, 0.30f, 0.35f, 0.30f },
            { u8"Källarkvart",     0.50f, 0.20f, 0.45f, 2, 0.50f, 0.20f, 0.70f, 0.45f },
            { u8"Rullande Golv",   0.80f, 0.50f, 0.25f, 3, 0.65f, 0.45f, 0.30f, 0.30f },
            { u8"Bruten Morgon",   0.65f, 0.85f, 0.30f, 3, 0.55f, 0.55f, 0.40f, 0.35f },
            { u8"Tomt Rum",        0.40f, 0.30f, 0.65f, 1, 0.35f, 0.10f, 0.55f, 0.50f },
        };
        for (int s = 0; s < tomrum::kNumSkolor; ++s)
        {
            int k = 0;
            for (const auto& p : platser)
            {
                TomrumPreset t;
                t.category = juce::String::fromUTF8 (tomrum::skolor()[s].name);
                t.name = juce::String::fromUTF8 (p.name);
                t.skola = s;
                t.seed = (uint32_t) (1000 + s * 97 + k * 13);
                t.values = { { "mapx", p.x }, { "mapy", p.y }, { "tomrum", p.tomrum }, { "gungtyp", (float) p.gung }, { "gung", p.gungAmt },
                             { "riff", p.riff }, { "djup", p.djup }, { "kallare", p.kallare }, { "root", (float) ((9 + s * 5) % 12) } };
                v.push_back (std::move (t));
                ++k;
            }
        }
        return v;
    }();
    return all;
}
