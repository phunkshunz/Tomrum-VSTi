#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace tomrum::ui
{
// Färgerna: vantablack, betonggrått och en sjukligt blek bärnsten.
namespace C
{
    const juce::Colour bg       { 0xff040404 };
    const juce::Colour panel    { 0xff090909 };
    const juce::Colour line     { 0xff1b1b1b };
    const juce::Colour lineHi   { 0xff2a2a2a };
    const juce::Colour grey     { 0xff5c5c5c };
    const juce::Colour light    { 0xffb4b1aa };
    const juce::Colour amber    { 0xffe3b264 };
    const juce::Colour amberDim { 0xff6b5430 };
    const juce::Colour red      { 0xffa3402e };
}

juce::Font mono (float h, bool bold = false);

class Laf : public juce::LookAndFeel_V4
{
public:
    Laf();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return mono (13.0f, true); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return mono (12.0f, true); }
    juce::Font getPopupMenuFont() override { return mono (13.0f); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
};

// En ratt med namn under. Värdet visas i stället för namnet medan man drar.
class Knob : public juce::Component
{
public:
    Knob();
    void attach (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name, const juce::String& tip);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
private:
    juce::String name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// Ett spår: namn (klick = GENETIK på spåret), stegen, FRYS, TYST och nivå.
class LaneRow : public juce::Component, public juce::SettableTooltipClient
{
public:
    LaneRow (TomrumProcessor&, int lane);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void tick (const Genome& g, double ppq, bool playing);
private:
    TomrumProcessor& proc;
    int lane;
    LaneGenome lg;
    int head = -1;
    float flash = 0.0f;
    int lastHits = 0;
    bool hover = false;
    juce::TextButton frys { "F" }, tyst { "T" };
    juce::Slider niva;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> frysAtt, tystAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> nivaAtt;
    juce::Rectangle<int> nameArea, stepArea;
};

// Mitten: tomrummet. Ringar per spår, och ett blekt sken som växer när ingenting spelar.
class VoidScope : public juce::Component
{
public:
    explicit VoidScope (TomrumProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void tick (const Genome& g);
private:
    TomrumProcessor& proc;
    Genome gen;
    float glow = 0.0f;
    std::array<float, kLanes> flash {};
    std::array<int, kLanes> lastHits {};
};

// Visar parasitens otakt: 16 staplar, sent uppåt och tidigt nedåt.
class ParasiteView : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit ParasiteView (TomrumProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragging = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragging = false; repaint(); }
    void mouseUp (const juce::MouseEvent&) override;
    juce::String status;
private:
    TomrumProcessor& proc;
    bool dragging = false;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace tomrum::ui

class TomrumEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TomrumEditor (TomrumProcessor&);
    ~TomrumEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void showPresetMenu();

    TomrumProcessor& proc;
    tomrum::ui::Laf laf;
    juce::TooltipWindow tips { this, 600 };

    juce::ComboBox skola;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, presetBtn, genetikBtn { "GENETIK" }, ljudBtn { "LJUD" }, angraBtn { juce::String::fromUTF8 ("ÅNGRA") };
    juce::TextButton spela { "SPELA" }, glomBtn { juce::String::fromUTF8 ("GLÖM LOOPEN") };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> spelaAtt;
    tomrum::ui::Knob tempo;

    tomrum::ui::VoidScope scope;
    tomrum::ui::ParasiteView parasite;
    std::vector<std::unique_ptr<tomrum::ui::LaneRow>> lanes;

    tomrum::ui::Knob minus, svang, friktion, parasit, dub, pump, tak, ruttna, variation, volym;
    juce::ComboBox grundton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> grundtonAtt;

    juce::Rectangle<int> topArea, laneArea, scopeArea, infoArea, knobArea;
    int seenVersion = -1;
    tomrum::Genome gen;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TomrumEditor)
};
