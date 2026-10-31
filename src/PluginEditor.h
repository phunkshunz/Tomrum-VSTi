#pragma once
#include "PluginProcessor.h"

// Tillfällig panel (steg 2). Bunkerpanelen kommer i steg 3.
class TomrumEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit TomrumEditor (TomrumProcessor& p) : juce::GenericAudioProcessorEditor (p) { setSize (520, 700); }
};
