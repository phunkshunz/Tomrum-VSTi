#pragma once
#include "PluginProcessor.h"
#include "Sprak.h"
#include "Gui.h"

// TOMRUM 2: bunkerpanelen. Mattsvart och bärnsten, KARTAN i mitten.
namespace Bunker
{
const juce::Colour black   { 0xff0a0a0a };
const juce::Colour plate   { 0xff121212 };
const juce::Colour plate2  { 0xff191919 };
const juce::Colour edge    { 0xff262626 };
const juce::Colour amber   { 0xffffb000 };
const juce::Colour amberDk { 0xff8a5f00 };
const juce::Colour ash     { 0xff7d7a72 };
const juce::Colour bone    { 0xffd8d2c4 };
const juce::Colour teal    { 0xff4fd1e0 };
}

class Dice : public juce::Button
{
public:
    explicit Dice (const char* tip) : juce::Button ("dice") { setTooltip (sprak::trU (tip)); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics& g, bool over, bool down) override;
};

// KARTAN: dra punkten. Åt sidan KÖLN - BERLIN - BUKAREST, uppåt RAK - RULL - BRUTEN.
class MapView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MapView (TomrumProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    juce::Rectangle<float> pad() const { return getLocalBounds().toFloat().reduced (26.0f, 4.0f).withTrimmedBottom (18.0f); }
private:
    void setFrom (juce::Point<float>);
    TomrumProcessor& proc;
    std::array<juce::Point<float>, 24> trail {};
    int trailN = 0;
public:
    void tick();
};

// Ett spår: namn, blink, takten som rutor, täthet, lås, tärning, tyst
class LaneRow : public juce::Component
{
public:
    LaneRow (TomrumProcessor& p, int lane);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setPattern (const std::array<float, 16>& w, const std::array<bool, 16>& h, int len, int playStep, float flash);
private:
    TomrumProcessor& proc;
    int lane;
    Knob dens;
    juce::TextButton las, tyst { "M" };
    Dice dice;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lasAtt, tystAtt;
    std::array<float, 16> weight {};
    std::array<bool, 16> hit {};
    int length = 16, step = -1;
    float flash = 0;
    juce::Rectangle<int> cells;
};

// DRA UT MIDI: dra knappen till ett spår i DAW:en
class MidiDrag : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MidiDrag (TomrumProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTooltip (sprak::trU (u8"Dra till ett spår i DAW:en: 8 takter, eller hela låten om LÅTEN är på.")); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    TomrumProcessor& proc;
    bool dragging = false;
};

class TomrumEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit TomrumEditor (TomrumProcessor&);
    ~TomrumEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int kW = 1000, kH = 640;
    void tickForSnapshot() { timerCallback(); }

private:
    void timerCallback() override;
    void refreshPresetBox();
    void knob (Knob& k, const char* id, const char* name, juce::Colour c, const char* tip, bool sida = true);
    void combo (juce::ComboBox& b, const char* id, const char* tip);
    void toggle (juce::TextButton& b, const char* id, const char* tip);

    TomrumProcessor& proc;
    ArmLnF lnf;
    juce::TooltipWindow tips { this, 600 };
    sprak::Knapp spraket;

    juce::ComboBox presetBox, skolaBox, gungBox, rootBox, skalaBox, latLenBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, undoBtn, allBtn { "SLUMPA ALLT" };
    juce::TextButton latBtn, polyBtn { "POLY" }, midiUtBtn { "MIDI UT" }, sparaBtn { "SPARA" };
    juce::TextButton monsterBtn;
    std::array<juce::TextButton, TomrumProcessor::kVarianter> varBtn;
    Knob ut, gung, friktion, riff, djup, kallare, kassett, ruttna, pump, skolfarg, tomrumK, variation;
    Dice diceLjud { "Nytt ljud inom skolan" }, diceKarta { u8"Hoppa till en ny plats på kartan" };   // u8: trU läser UTF-8
    MapView map;
    MidiDrag midiDrag;
    std::array<std::unique_ptr<LaneRow>, tr2::kLanes> rows;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAtt;

    juce::Rectangle<int> skolaArea, gungArea, tonArea, ljudArea, mapArea, latArea, sparArea, varArea, latInfo;
    tr2::Engine preview;
    int seenVersion = -1, seenProgram = -1;
    bool sparaLage = false;
    juce::String latText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TomrumEditor)
};
