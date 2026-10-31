#pragma once
#include "PluginProcessor.h"
#include "Gui.h"
#include "About.h"

// TOMRUM: bunkerpanelen. Mattsvart, bärnsten, ett oscilloskop som lyser upp i tystnaden.
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
}

class Dice : public juce::Button
{
public:
    explicit Dice (const juce::String& tip) : juce::Button ("dice") { setTooltip (tip); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        using namespace Bunker;
        const float s = (float) juce::jmin (getWidth(), getHeight()) - 2.0f;
        auto r = juce::Rectangle<float> (s, s).withCentre (getLocalBounds().toFloat().getCentre());
        if (down) r = r.reduced (1.0f);
        g.setColour (amber.withAlpha (over ? 0.3f : 0.12f));
        g.fillRoundedRectangle (r, s * 0.22f);
        g.setColour (amber);
        g.drawRoundedRectangle (r.reduced (0.6f), s * 0.22f, 1.1f);
        const float d = juce::jmax (2.0f, s * 0.17f);
        for (auto p : { juce::Point<float> (0.28f, 0.28f), { 0.72f, 0.28f }, { 0.5f, 0.5f }, { 0.28f, 0.72f }, { 0.72f, 0.72f } })
            g.fillEllipse (r.getX() + r.getWidth() * p.x - d * 0.5f, r.getY() + r.getHeight() * p.y - d * 0.5f, d, d);
    }
};

// Tomrumsskopet: vågformen, och ett sken som växer när det blir tyst.
class VoidScope : public juce::Component
{
public:
    explicit VoidScope (TomrumProcessor& p) : proc (p) {}
    void paint (juce::Graphics& g) override;
private:
    TomrumProcessor& proc;
};

// Mönstret: 8 spår, regelbokens tillåtna steg som skuggor, slagen i bärnsten.
class GridView : public juce::Component
{
public:
    explicit GridView (TomrumProcessor& p) : proc (p) {}
    void paint (juce::Graphics& g) override;
    void setGenome (const tomrum::Genome& g) { gen = g; repaint(); }
    static constexpr int kLeft = 222;
    int rowY (int l) const { return 6 + l * rowH(); }
    int rowH() const { return (getHeight() - 12) / tomrum::kLanes; }
private:
    TomrumProcessor& proc;
    tomrum::Genome gen;
};

class ParasiteView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ParasiteView (TomrumProcessor& p) : proc (p) {}
    void paint (juce::Graphics& g) override;
private:
    TomrumProcessor& proc;
};

class TomrumEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    explicit TomrumEditor (TomrumProcessor&);
    ~TomrumEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& files, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragging = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragging = false; repaint(); }

private:
    void timerCallback() override;
    void setStatus (const juce::String& s) { status = s; repaint (statusArea); }
    void presetMenu();
    void stepPreset (int dir);
    void chooseLoop();
    void setSkola (int i);

    TomrumProcessor& proc;
    ArmLnF lnf;
    juce::TooltipWindow tips { this, 600 };

    using BtnAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using BoxAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SldAtt = juce::AudioProcessorValueTreeState::SliderAttachment;

    VoidScope scope { proc };
    GridView grid { proc };
    ParasiteView para { proc };

    juce::TextButton skolaBtn[tomrum::kNumSkolor];
    juce::TextButton genBtn, mutBtn, soundBtn, undoBtn, rotBtn, presetBtn, prevBtn, nextBtn, loopBtn, clearBtn;
    Dice laneDice[tomrum::kLanes] {
        Dice ("Slumpa kicken (inom skolans regler)"), Dice ("Slumpa basen"), Dice ("Slumpa klicket"), Dice ("Slumpa hatten"),
        Dice ("Slumpa träet"), Dice ("Slumpa skrapet"), Dice ("Slumpa stabben"), Dice ("Slumpa spöket") };
    juce::TextButton muteBtn[tomrum::kLanes];
    juce::Slider lvl[tomrum::kLanes];
    std::unique_ptr<BtnAtt> muteAtt[tomrum::kLanes];
    std::unique_ptr<SldAtt> lvlAtt[tomrum::kLanes];

    Knob kMinus, kFrik, kPara, kSving, kDub, kRot, kTak, kPump, kVar, kUt;
    juce::ComboBox rootBox;
    std::unique_ptr<BoxAtt> rootAtt;
    std::unique_ptr<juce::FileChooser> chooser;
    AboutOverlay about { "TOMRUM", juce::String::fromUTF8 ("Das Leerraum · sväng av tomrum") };

    juce::String status;
    bool dragging = false;
    int lastVersionShown = -1;
    juce::Rectangle<int> header, pSkola, pMach, pScope, pGrid, pKnobs, pPara, statusArea, logoArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TomrumEditor)
};
