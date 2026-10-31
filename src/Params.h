#pragma once
// TOMRUM — parametrarna. Allt som går att automatisera i DAW:en.

#include <juce_audio_processors/juce_audio_processors.h>
#include "Tomrum.h"

namespace tomrum
{
inline juce::String lid (int l, const char* key) { return "l" + juce::String (l) + "_" + key; }

inline juce::StringArray skolNamn()
{
    juce::StringArray a;
    for (int i = 0; i < kNumSkolor; ++i) a.add (juce::String::fromUTF8 (skolor()[i].name));
    return a;
}

inline juce::StringArray tonNamn() { return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;
    const auto U = [] (const char* s) { return String::fromUTF8 (s); };
    auto pct = [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; };
    auto f01 = [&] (const char* id, const char* name, float def)
    {
        L.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, String::fromUTF8 (name), NormalisableRange<float> (0.0f, 1.0f, 0.001f), def,
                                                      AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    };

    L.add (std::make_unique<AudioParameterChoice> (ParameterID { "skola", 1 }, "SKOLA", skolNamn(), 0));

    f01 ("minus", "MINUS", 0.3f);
    f01 ("svang", "SVÄNG", 0.5f);
    L.add (std::make_unique<AudioParameterFloat> (ParameterID { "friktion", 1 }, "FRIKTION", NormalisableRange<float> (0.0f, 2.0f, 0.001f), 1.0f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    f01 ("parasit", "PARASIT", 0.6f);
    f01 ("dub", "DUB", 0.3f);
    f01 ("pump", "PUMP", 0.4f);
    L.add (std::make_unique<AudioParameterFloat> (ParameterID { "tak", 1 }, "TAK", NormalisableRange<float> (0.25f, 2.0f, 0.001f, 0.6f), 1.0f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    f01 ("ruttna", "RUTTNA", 0.5f);
    f01 ("variation", "VARIATION", 0.5f);
    L.add (std::make_unique<AudioParameterChoice> (ParameterID { "grundton", 1 }, "GRUNDTON", tonNamn(), 9));
    L.add (std::make_unique<AudioParameterFloat> (ParameterID { "volym", 1 }, "VOLYM", NormalisableRange<float> (0.0f, 2.0f, 0.001f), 1.0f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
                                                      return v < 0.001f ? String ("-inf dB") : String (Decibels::gainToDecibels (v), 1) + " dB"; })));

    for (int l = 0; l < kLanes; ++l)
    {
        const String pre = U (laneName (l)) + " ";
        L.add (std::make_unique<AudioParameterFloat> (ParameterID { lid (l, "niva"), 1 }, pre + U ("NIVÅ"), NormalisableRange<float> (0.0f, 1.5f, 0.001f), 1.0f,
                                                      AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
        L.add (std::make_unique<AudioParameterBool> (ParameterID { lid (l, "tyst"), 1 }, pre + "TYST", false));
        L.add (std::make_unique<AudioParameterBool> (ParameterID { lid (l, "frys"), 1 }, pre + "FRYS", false));
    }

    L.add (std::make_unique<AudioParameterBool> (ParameterID { "spela", 1 }, "SPELA", false));
    L.add (std::make_unique<AudioParameterFloat> (ParameterID { "tempo", 1 }, "TEMPO", NormalisableRange<float> (60.0f, 180.0f, 0.1f), 124.0f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " BPM"; })));
    return L;
}
} // namespace tomrum
