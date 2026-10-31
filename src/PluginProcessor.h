#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Tomrum.h"
#include <deque>

struct TomrumPreset
{
    juce::String name;
    int skola;
    uint32_t seed;
    std::vector<std::pair<const char*, float>> values;   // parameter-id -> verkligt värde
};

class TomrumProcessor : public juce::AudioProcessor
{
public:
    TomrumProcessor();
    ~TomrumProcessor() override = default;

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
    double getTailLengthSeconds() const override { return std::numeric_limits<double>::infinity(); } // aldrig sövas av DAW:en

    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- message-tråden ----
    void genetik();                  // GENETIK: nytt genom inom skolans regler (mönster + ljud)
    void mutation();                 // MUTATION: två eller tre spår byts, kicken står kvar
    void mutateLane (int lane);      // tärning per spår
    void mutateSound();              // bara ljudet
    void undo();
    bool canUndo() const { return ! undoStack.empty(); }
    void resetRot() { rotReset.store (true); }

    juce::String loadParasite (const juce::File& f);   // tom = OK
    void clearParasite();
    bool hasOwnParasite() const { return ownParasite; }
    juce::String parasiteName() const { return parasiteFile; }
    tomrum::Parasite currentParasite() const;

    tomrum::Genome currentGenome() const;              // för panelen
    int skolaIndex() const;

    static const std::vector<TomrumPreset>& presets();

    juce::AudioProcessorValueTreeState apvts;

    // till panelen
    std::atomic<float> voidLevel { 1.0f }, rotLevel { 0.0f };
    std::atomic<double> playPpq { 0.0 };
    std::atomic<bool> running { false };
    std::array<std::atomic<float>, tomrum::kLanes> laneFlash {};
    std::array<float, 512> scope {};          // senaste blockets vågform (bara för visning)
    std::atomic<int> scopeWrite { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const char* id, float plain);
    void pushUndo();
    void bump() { genVersion.fetch_add (1); }
    void knobsFromGenome (const tomrum::Genome& g);
    void applyPreset (const TomrumPreset& p);
    uint32_t freshSeed() { return (uint32_t) rng.nextInt() ^ (uint32_t) juce::Time::getHighResolutionTicks(); }

    tomrum::Engine engine;
    std::array<std::atomic<uint32_t>, tomrum::kLanes> laneSeed {};
    std::atomic<uint32_t> soundSeed { 1 };
    std::atomic<int> genVersion { 1 };
    int seenVersion = 0, seenSkola = -1;

    // PARASIT ur loop
    std::array<std::atomic<float>, 16> paraOff {};
    std::atomic<bool> paraOwnFlag { false };
    std::atomic<int> paraVersion { 1 };
    int seenPara = 0;
    bool ownParasite = false;
    juce::String parasiteFile;

    std::atomic<bool> rotReset { false };
    std::atomic<double> hostBpm { 124.0 };
    std::deque<juce::ValueTree> undoStack;
    int currentProgram = 0;
    double sr = 48000.0;
    juce::Random rng;

    // MIDI ut: noter som ska släppas
    struct Off { int note; int samplesLeft; };
    std::array<Off, 64> offs {};
    int numOffs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TomrumProcessor)
};
