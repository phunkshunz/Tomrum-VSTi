#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Params.h"
#include "Presets.h"

// Genomet byts från meddelandetråden och läses av ljudtråden. En versionsräknare
// (udda = skrivs just nu) gör att ljudtråden aldrig får ett halvskrivet genom:
// då spelar den vidare på det förra ett block till.
template <typename T>
struct Mailbox
{
    void write (const T& v)
    {
        ver.fetch_add (1, std::memory_order_acq_rel);
        data = v;
        ver.fetch_add (1, std::memory_order_acq_rel);
    }
    bool read (T& out, uint32_t& lastSeen)
    {
        const uint32_t v1 = ver.load (std::memory_order_acquire);
        if ((v1 & 1u) != 0 || v1 == lastSeen) return false;
        T tmp = data;
        std::atomic_thread_fence (std::memory_order_acquire);
        if (ver.load (std::memory_order_relaxed) != v1) return false;
        out = tmp;
        lastSeen = v1;
        return true;
    }
    T data {};
    std::atomic<uint32_t> ver { 0 };
};

class TomrumProcessor : public juce::AudioProcessor
{
public:
    TomrumProcessor();
    ~TomrumProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TOMRUM"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return std::numeric_limits<double>::infinity(); } // sövs aldrig av Bitwig

    int getNumPrograms() override { return tomrum::kNumPresets; }
    int getCurrentProgram() override { return juce::jmax (0, currentPreset.load()); }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override { return juce::String::fromUTF8 (tomrum::presetName (index).c_str()); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---------------------------------------------------------------- för panelen (meddelandetråden)
    juce::AudioProcessorValueTreeState apvts;

    void chooseSkola (int skola);      // byt skola från panelen: sätter SKOLA och kör GENETIK
    void genetik();                    // nytt groove + nytt ljud, frysta spår lämnas i fred
    void genetikLane (int lane);       // bara ett spår
    void genetikLjud();                // bara ljudet, samma mönster
    void loadPreset (int index);
    bool undo();
    bool canUndo() const { return ! undoStack.empty(); }

    // PARASIT: sug otakten ur en loop. Returnerar en rad att visa på panelen.
    juce::String loadParasiteLoop (const juce::File& file);
    void forgetParasiteLoop();
    bool hasCustomParasite() const { return customParasite; }
    juce::String parasiteName() const { return parasiteFrom; }
    tomrum::Parasite currentParasite() const;

    tomrum::Genome genome() const { return current; }   // senaste genomet (meddelandetråden)
    juce::String presetLabel() const;

    // panelens ögon in i ljudtråden
    std::atomic<double> uiPpq { 0.0 }, uiBpm { 124.0 };
    std::atomic<bool> uiPlaying { false };
    std::atomic<float> uiVoid { 1.0f }, uiRot { 0.0f };
    std::array<std::atomic<int>, tomrum::kLanes> uiHits {};
    std::atomic<int> currentPreset { -1 };
    std::atomic<int> genomeVersion { 0 };

private:
    void setParam (const juce::String& id, float value);
    void publish (const tomrum::Genome& g, bool setKnobs);
    uint32_t frozenMask() const;
    int skolaParam() const;
    void pushUndo();
    juce::ValueTree snapshot();
    void restore (const juce::ValueTree& tree);

    tomrum::Genome current;
    tomrum::Parasite customPara;
    bool customParasite = false;
    juce::String parasiteFrom;

    Mailbox<tomrum::Genome> genomeBox;
    Mailbox<tomrum::Parasite> paraBox;
    uint32_t genomeSeen = 0, paraSeen = 0;

    tomrum::Engine engine;
    std::array<tomrum::Event, 256> events {};

    std::atomic<float> *pMinus, *pSvang, *pFriktion, *pParasit, *pDub, *pPump, *pTak, *pRuttna, *pVariation, *pGrundton, *pVolym, *pSpela, *pTempo;
    std::array<std::atomic<float>*, tomrum::kLanes> pNiva {}, pTyst {};

    struct PendingOff { int note; int64_t at; };
    std::vector<PendingOff> pending;
    int64_t sampleClock = 0;
    double expectedPpq = -1.0e9;
    bool wasRunning = false;
    double internalPpq = 0.0;
    bool internalWasOn = false;
    double sr = 48000.0;
    juce::AudioBuffer<float> scratch;

    std::vector<juce::ValueTree> undoStack;
    juce::AudioFormatManager formats;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TomrumProcessor)
};
