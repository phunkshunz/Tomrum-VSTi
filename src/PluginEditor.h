#pragma once
// TILLFÄLLIG panel (steg 2): alla rattar som lista. Den riktiga panelen med KARTAN kommer i steg 3.
#include "PluginProcessor.h"

class TomrumEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit TomrumEditor (TomrumProcessor& p) : juce::GenericAudioProcessorEditor (p) { setSize (520, 640); }
};
