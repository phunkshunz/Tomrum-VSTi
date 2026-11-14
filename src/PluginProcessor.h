#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Motor.h"
#include <deque>

using Tr2Controls = tr2::Engine::Controls;

// Ett läge: skola, frön och rattvärden (parameter-id -> verkligt värde)
struct TomrumPreset
{
    juce::String category, name;
    int skola = 0;
    uint32_t seed = 1;
    std::vector<std::pair<juce::String, float>> values;
};

class TomrumProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    static constexpr int kVarianter = 8;
    static constexpr int kVariantNote = 48;   // C3..G3 byter variant

    TomrumProcessor();
    ~TomrumProcessor() override { cancelPendingUpdate(); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TOMRUM"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return std::numeric_limits<double>::infinity(); }

    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- message-tråden
    void slumpaMonster();             // nya mönster i alla spår som inte är LÅSTA
    void slumpaSpar (int lane);       // ett spår
    void slumpaLjud();                // nytt ljud inom skolan
    void slumpaKarta();               // hoppa till en ny plats på kartan
    void slumpaAllt();
    void undo();
    bool canUndo() const { return ! undoStack.empty(); }
    void resetRot() { rotReset.store (true); }

    // VARIANTER: 8 platser. Spara läget, byt med knapp eller MIDI-tangent (byter vid nästa takt)
    void sparaVariant (int slot);
    void valjVariant (int slot);      // från panelen
    bool harVariant (int slot) const { return variantFull[(size_t) juce::jlimit (0, kVarianter - 1, slot)].load(); }
    int aktivVariant() const { return activeVariant.load(); }
    int vantandeVariant() const { return requestedVariant.load(); }

    // DRA UT MIDI: skriver låten (eller 8 takter) som MIDI-fil och returnerar filen
    juce::File exportMidi (int bars);

    Tr2Controls currentControls() const;
    static const std::vector<TomrumPreset>& presets();
    void applyPreset (const TomrumPreset& p);

    juce::AudioProcessorValueTreeState apvts;

    // till panelen
    std::atomic<float> rotLevel { 0.0f };
    std::atomic<double> playPpq { 0.0 };
    std::atomic<bool> running { false };
    std::atomic<int> delNu { -1 };
    std::array<std::atomic<float>, tr2::kLanes> laneFlash {};
    std::atomic<int> stateVersion { 1 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const juce::String& id, float plain);
    float param (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void pushUndo();
    void bump() { stateVersion.fetch_add (1); }
    uint32_t freshSeed() { return ((uint32_t) rng.nextInt() ^ (uint32_t) juce::Time::getHighResolutionTicks()) | 1u; }
    void handleAsyncUpdate() override;

    // VARIANT: det som sparas på en plats
    struct Variant
    {
        float mapX = 0.5f, mapY = 0.3f, tomrum = 0.35f, gung = 0.5f, variation = 0.4f, riff = 0.3f, djup = 0.4f;
        int gungTyp = 1;
        float dens[tr2::kLanes] {};
        uint32_t laneSeed[tr2::kLanes] {};
    };
    Variant captureVariant() const;
    void writeVariantToParams (const Variant& v);
    static void variantToTree (const Variant& v, juce::ValueTree& t);
    static Variant variantFromTree (const juce::ValueTree& t);

    tr2::Engine engine;
    std::array<std::atomic<uint32_t>, tr2::kLanes> laneSeed {};
    std::atomic<uint32_t> soundSeed { 1 };

    // varianterna: skrivs på message-tråden, läses i ljudtråden (byts sällan, kopieras under en flagga)
    std::array<Variant, kVarianter> variants {};
    std::array<std::atomic<bool>, kVarianter> variantFull {};
    std::atomic<int> requestedVariant { -1 }, activeVariant { -1 };
    std::atomic<bool> variantOverride { false };
    Variant overrideV;               // bara ljudtråden skriver, message-tråden läser i handleAsyncUpdate
    std::atomic<int> toParams { -1 };
    long lastBar = -1;

    std::atomic<bool> rotReset { false };
    std::deque<juce::ValueTree> undoStack;
    int currentProgram = 0;
    double sr = 48000.0;
    juce::Random rng;

    struct Off { int note, channel, samplesLeft; };
    std::array<Off, 96> offs {};
    int numOffs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TomrumProcessor)
};
