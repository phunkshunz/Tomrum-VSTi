#include "PluginEditor.h"

using namespace tomrum;
using namespace tomrum::ui;

static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

juce::Font tomrum::ui::mono (float h, bool bold)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, bold ? juce::Font::bold : juce::Font::plain));
}

// ================================================================ LookAndFeel
Laf::Laf()
{
    setColour (juce::ResizableWindow::backgroundColourId, C::bg);
    setColour (juce::PopupMenu::backgroundColourId, C::panel);
    setColour (juce::PopupMenu::textColourId, C::light);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, C::amberDim);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::ComboBox::textColourId, C::light);
    setColour (juce::TooltipWindow::backgroundColourId, C::panel);
    setColour (juce::TooltipWindow::textColourId, C::light);
    setColour (juce::TooltipWindow::outlineColourId, C::lineHi);
    setColour (juce::TextButton::textColourOffId, C::light);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::Label::textColourId, C::light);
}

void Laf::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const auto b = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float a = a0 + pos * (a1 - a0);

    juce::Path track;
    track.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
    g.setColour (C::line);
    g.strokePath (track, juce::PathStrokeType (2.0f));

    juce::Path val;
    val.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a, true);
    g.setColour (s.isMouseOverOrDragging() ? C::amber : C::amber.withAlpha (0.75f));
    g.strokePath (val, juce::PathStrokeType (2.0f));

    // en kort mätarlinje, inget rattlock
    const juce::Point<float> p1 (c.x + std::sin (a) * r * 0.35f, c.y - std::cos (a) * r * 0.35f);
    const juce::Point<float> p2 (c.x + std::sin (a) * r * 0.8f, c.y - std::cos (a) * r * 0.8f);
    g.setColour (C::light.withAlpha (0.8f));
    g.drawLine ({ p1, p2 }, 1.5f);
}

void Laf::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cy = (float) y + (float) h * 0.5f;
    g.setColour (C::line);
    g.fillRect ((float) x, cy - 1.0f, (float) w, 2.0f);
    g.setColour (s.isMouseOverOrDragging() ? C::amber : C::amber.withAlpha (0.6f));
    g.fillRect ((float) x, cy - 1.0f, pos - (float) x, 2.0f);
    g.fillRect (pos - 1.0f, cy - 4.0f, 2.0f, 8.0f);
}

void Laf::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const bool hero = b.getProperties().getWithDefault ("hero", false);
    if (on || (hero && down)) { g.setColour (hero ? C::amber : C::amberDim.brighter (0.2f)); g.fillRect (r); }
    else if (hero) { g.setColour (C::amber.withAlpha (over ? 0.22f : 0.12f)); g.fillRect (r); }
    else if (over) { g.setColour (C::lineHi.withAlpha (0.6f)); g.fillRect (r); }
    g.setColour (hero ? C::amber.withAlpha (0.8f) : (on ? C::amberDim : C::lineHi));
    g.drawRect (r, 1.0f);
}

void Laf::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    const bool hero = b.getProperties().getWithDefault ("hero", false);
    const bool on = b.getToggleState() || (hero && down);
    g.setFont (hero ? mono (15.0f, true) : mono (12.0f, true));
    g.setColour (on ? juce::Colours::black : (hero ? C::amber : (b.isEnabled() ? C::light : C::grey.withAlpha (0.5f))));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
}

void Laf::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    g.setColour (box.isMouseOver (true) ? C::lineHi.withAlpha (0.5f) : C::panel);
    g.fillRect (0, 0, w, h);
    g.setColour (C::lineHi);
    g.drawRect (0, 0, w, h, 1);
    juce::Path tri;
    const float cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    tri.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour (C::amber.withAlpha (0.8f));
    g.fillPath (tri);
}

void Laf::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 28, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void Laf::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (C::panel);
    g.setColour (C::lineHi);
    g.drawRect (0, 0, w, h, 1);
}

// ================================================================ Knob
Knob::Knob()
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.onDragStart = [this] { repaint(); };
    slider.onDragEnd = [this] { repaint(); };
    slider.onValueChange = [this] { if (slider.isMouseButtonDown()) repaint(); };
    addAndMakeVisible (slider);
}

void Knob::attach (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& n, const juce::String& tip)
{
    name = n;
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    slider.setTooltip (tip);
    if (auto* p = s.getParameter (id))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void Knob::resized() { slider.setBounds (getLocalBounds().withTrimmedBottom (18)); }

void Knob::paint (juce::Graphics& g)
{
    const bool drag = slider.isMouseButtonDown();
    g.setFont (mono (11.0f, true));
    g.setColour (drag ? C::amber : C::grey);
    g.drawText (drag ? slider.getTextFromValue (slider.getValue()) : name, getLocalBounds().removeFromBottom (16), juce::Justification::centred, false);
}

// ================================================================ LaneRow
LaneRow::LaneRow (TomrumProcessor& p, int l) : proc (p), lane (l)
{
    auto& s = proc.apvts;
    frys.setClickingTogglesState (true);
    tyst.setClickingTogglesState (true);
    frys.setTooltip (U ("FRYS: GENETIK och presets lämnar spåret i fred."));
    tyst.setTooltip (U ("TYST: stäng av spåret."));
    frysAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, lid (l, "frys"), frys);
    tystAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, lid (l, "tyst"), tyst);
    niva.setSliderStyle (juce::Slider::LinearHorizontal);
    niva.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    niva.setTooltip (U ("Spårets nivå. Dubbelklick = 100 %."));
    niva.setDoubleClickReturnValue (true, 1.0);
    nivaAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, lid (l, "niva"), niva);
    for (auto* c : std::initializer_list<juce::Component*> { &frys, &tyst, &niva }) addAndMakeVisible (*c);
    setTooltip (U ("Klicka på namnet: GENETIK bara på det här spåret."));
}

void LaneRow::resized()
{
    auto r = getLocalBounds().reduced (0, 4);
    nameArea = r.removeFromLeft (64);
    niva.setBounds (r.removeFromRight (58));
    r.removeFromRight (6);
    tyst.setBounds (r.removeFromRight (24).withSizeKeepingCentre (24, 22));
    r.removeFromRight (4);
    frys.setBounds (r.removeFromRight (24).withSizeKeepingCentre (24, 22));
    r.removeFromRight (10);
    stepArea = r;
}

void LaneRow::mouseUp (const juce::MouseEvent& e)
{
    if (nameArea.contains (e.getPosition())) proc.genetikLane (lane);
}

void LaneRow::tick (const Genome& g, double ppq, bool playing)
{
    lg = g.lane[lane];
    const int hits = proc.uiHits[(size_t) lane].load (std::memory_order_relaxed);
    if (hits != lastHits) { flash = 1.0f; lastHits = hits; }
    else flash *= 0.8f;
    head = (playing && lg.len > 0) ? (int) (((long) std::floor (ppq * 4.0) % lg.len + lg.len) % lg.len) : -1;
    repaint();
}

void LaneRow::paint (juce::Graphics& g)
{
    const bool muted = tyst.getToggleState();
    const bool frozen = frys.getToggleState();

    // namnet: lyser kort när spåret slår
    g.setFont (mono (12.0f, true));
    g.setColour (muted ? C::grey.withAlpha (0.4f) : C::light.interpolatedWith (C::amber, flash).withAlpha (hover ? 1.0f : 0.85f));
    g.drawText (U (laneName (lane)), nameArea.withTrimmedLeft (2), juce::Justification::centredLeft, false);
    if (frozen)
    {
        g.setColour (C::amberDim);
        g.fillRect (nameArea.getX(), nameArea.getCentreY() - 6, 2, 12);
    }

    // stegen: cykelns längd syns, så polyrytmen syns
    if (lg.len <= 0) return;
    const int maxCells = 32;
    const float cw = (float) stepArea.getWidth() / (float) maxCells;
    const float cy = (float) stepArea.getCentreY();
    for (int i = 0; i < lg.len; ++i)
    {
        const float x = (float) stepArea.getX() + (float) i * cw;
        const auto cell = juce::Rectangle<float> (x + 1.0f, cy - 6.0f, juce::jmax (1.0f, cw - 2.0f), 12.0f);
        if (lg.on (i))
        {
            const float w = lg.weight[i] >= 1.0f ? 1.0f : 0.45f + 0.5f * lg.weight[i];
            g.setColour ((muted ? C::grey : C::amber).withAlpha (w * (i == head ? 1.0f : 0.8f)));
            g.fillRect (cell);
        }
        else if (lg.weight[i] > 0.0f)
        {
            // regelboken tillåter slag här: en svag skugga
            g.setColour (C::lineHi.withAlpha (0.4f + 0.6f * lg.weight[i]));
            g.fillRect (cell.withSizeKeepingCentre (cell.getWidth(), 2.0f));
        }
        else
        {
            g.setColour (C::line);
            g.fillRect (cell.withSizeKeepingCentre (2.0f, 2.0f));
        }
        if (i == head)
        {
            g.setColour (C::light.withAlpha (0.7f));
            g.drawRect (cell.expanded (1.0f), 1.0f);
        }
    }
    // cykelns slut
    g.setColour (C::grey.withAlpha (0.5f));
    const float endX = (float) stepArea.getX() + (float) lg.len * cw;
    g.drawLine (endX, cy - 9.0f, endX, cy + 9.0f, 1.0f);
    g.setFont (mono (9.0f));
    if (lg.len != 16) g.drawText (juce::String (lg.len), juce::Rectangle<float> (endX + 3.0f, cy - 6.0f, 22.0f, 12.0f), juce::Justification::centredLeft, false);
}

// ================================================================ VoidScope
void VoidScope::tick (const Genome& g)
{
    gen = g;
    const float v = proc.uiPlaying.load() ? proc.uiVoid.load() : 0.15f;
    glow += (v - glow) * 0.25f;
    for (int l = 0; l < kLanes; ++l)
    {
        const int h = proc.uiHits[(size_t) l].load (std::memory_order_relaxed);
        if (h != lastHits[(size_t) l]) { flash[(size_t) l] = 1.0f; lastHits[(size_t) l] = h; }
        else flash[(size_t) l] *= 0.82f;
    }
    repaint();
}

void VoidScope::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.47f;

    // skenet i tomrummet: ju tystare, desto mer lyser det
    {
        juce::ColourGradient grad (C::amber.withAlpha (0.10f + 0.42f * glow), c.x, c.y, C::amber.withAlpha (0.0f), c.x + R * (0.45f + 0.5f * glow), c.y, true);
        g.setGradientFill (grad);
        g.fillEllipse (juce::Rectangle<float> (R * 2.0f, R * 2.0f).withCentre (c));
    }

    const double ppq = proc.uiPpq.load();
    const bool playing = proc.uiPlaying.load();
    const float inner = R * 0.28f;
    const float ringStep = (R - inner) / (float) kLanes;

    for (int l = 0; l < kLanes; ++l)
    {
        const auto& lg = gen.lane[l];
        const float r = inner + ringStep * ((float) l + 0.5f);
        g.setColour (C::line);
        g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), 1.0f);
        if (lg.len <= 0) continue;
        const int head = playing ? (int) (((long) std::floor (ppq * 4.0) % lg.len + lg.len) % lg.len) : -1;
        for (int i = 0; i < lg.len; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) lg.len;
            const juce::Point<float> p (c.x + std::sin (a) * r, c.y - std::cos (a) * r);
            if (lg.on (i))
            {
                const bool now = i == head;
                const float s = now ? 3.4f + 2.5f * flash[(size_t) l] : 2.4f;
                g.setColour (now ? C::amber : C::amber.withAlpha (0.35f + 0.4f * lg.weight[i]));
                g.fillEllipse (juce::Rectangle<float> (s * 2.0f, s * 2.0f).withCentre (p));
            }
            else if (lg.weight[i] > 0.0f)
            {
                g.setColour (C::lineHi);
                g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre (p));
            }
        }
        if (head >= 0)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) head / (float) lg.len;
            g.setColour (C::light.withAlpha (0.15f));
            g.drawLine (c.x + std::sin (a) * (r - ringStep * 0.4f), c.y - std::cos (a) * (r - ringStep * 0.4f),
                        c.x + std::sin (a) * (r + ringStep * 0.4f), c.y - std::cos (a) * (r + ringStep * 0.4f), 1.0f);
        }
    }

    // RUTTNA: en båge runt kanten som växer med tiden
    const float rot = proc.uiRot.load();
    if (rot > 0.001f)
    {
        juce::Path p;
        p.addCentredArc (c.x, c.y, R + 6.0f, R + 6.0f, 0.0f, 0.0f, juce::MathConstants<float>::twoPi * rot, true);
        g.setColour (C::red.withAlpha (0.7f));
        g.strokePath (p, juce::PathStrokeType (1.5f));
    }

    // mitten
    g.setFont (mono (11.0f, true));
    g.setColour (C::bg.withAlpha (0.85f));
    g.fillEllipse (juce::Rectangle<float> (inner * 1.6f, inner * 1.6f).withCentre (c));
    g.setColour (C::amber.withAlpha (0.4f + 0.6f * glow));
    g.drawText (U (skolor()[juce::jlimit (0, kNumSkolor - 1, gen.skola)].name), juce::Rectangle<float> (inner * 2.4f, 16.0f).withCentre (c.translated (0.0f, -6.0f)), juce::Justification::centred, false);
    g.setFont (mono (10.0f));
    g.setColour (C::grey);
    g.drawText (juce::String (proc.uiBpm.load(), 1) + " BPM", juce::Rectangle<float> (inner * 2.0f, 14.0f).withCentre (c.translated (0.0f, 10.0f)), juce::Justification::centred, false);
}

// ================================================================ ParasiteView
bool ParasiteView::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (f.endsWithIgnoreCase (".wav") || f.endsWithIgnoreCase (".aif") || f.endsWithIgnoreCase (".aiff") || f.endsWithIgnoreCase (".flac"))
            return true;
    return false;
}

void ParasiteView::filesDropped (const juce::StringArray& files, int, int)
{
    dragging = false;
    for (auto& f : files)
        if (isInterestedInFileDrag (juce::StringArray (f)))
        {
            status = proc.loadParasiteLoop (juce::File (f));
            break;
        }
    repaint();
}

void ParasiteView::mouseUp (const juce::MouseEvent&)
{
    chooser = std::make_unique<juce::FileChooser> (U ("Välj en loop att suga otakten ur"), juce::File(), "*.wav;*.aif;*.aiff;*.flac");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile()) { status = proc.loadParasiteLoop (f); repaint(); }
                          });
}

void ParasiteView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (dragging ? C::amber.withAlpha (0.12f) : C::panel);
    g.fillRect (r);
    juce::Path border;
    border.addRectangle (r.reduced (0.5f));
    juce::Path dashed;
    const float dash[] = { 4.0f, 3.0f };
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, border, dash, 2);
    g.setColour (dragging ? C::amber : C::lineHi);
    g.fillPath (dashed);

    auto inner = r.reduced (10.0f);
    g.setFont (mono (11.0f, true));
    g.setColour (C::grey);
    g.drawText (proc.hasCustomParasite() ? U ("PARASIT: ") + proc.parasiteName().toUpperCase() : U ("PARASIT: SKOLANS OTAKT"),
                inner.removeFromTop (14.0f), juce::Justification::centredLeft, true);

    // otakten, 16 staplar
    auto bars = inner.removeFromTop (inner.getHeight() - 30.0f).reduced (0.0f, 6.0f);
    const auto para = proc.currentParasite();
    const float zero = bars.getY() + bars.getHeight() * 0.3f;   // tidigt ryms ovanför, sent nedåt
    g.setColour (C::line);
    g.drawLine (bars.getX(), zero, bars.getRight(), zero, 1.0f);
    const float bw = bars.getWidth() / 16.0f;
    for (int i = 0; i < 16; ++i)
    {
        const float v = para.off[i];
        const float h = v / 0.4f * (bars.getBottom() - zero);
        const float x = bars.getX() + (float) i * bw + 2.0f;
        g.setColour ((i % 4 == 0 ? C::light : C::amber).withAlpha (0.75f));
        if (h >= 0) g.fillRect (x, zero, bw - 4.0f, juce::jmax (1.0f, h));
        else g.fillRect (x, zero + h, bw - 4.0f, -h);
    }

    g.setFont (mono (10.0f));
    g.setColour (C::grey);
    g.drawFittedText (status.isNotEmpty() ? status : U ("Släpp en trumloop här (eller klicka). TOMRUM suger ut hur skevt den ligger och lägger det på sina egna slag."),
                      inner.toNearestInt(), juce::Justification::centredLeft, 3);
}

// ================================================================ Editor
TomrumEditor::TomrumEditor (TomrumProcessor& p)
    : AudioProcessorEditor (&p), proc (p), scope (p), parasite (p)
{
    setLookAndFeel (&laf);
    auto& s = proc.apvts;

    skola.addItemList (skolNamn(), 1);
    skola.setTooltip (U ("Skolan är regelboken: rytmik, otakt och ljudbild. Byte = GENETIK direkt."));
    skola.onChange = [this]
    {
        const int i = skola.getSelectedItemIndex();
        if (i >= 0 && i != proc.genome().skola) proc.chooseSkola (i);
    };
    addAndMakeVisible (skola);

    prevBtn.onClick = [this] { const int c = proc.currentPreset.load(); proc.loadPreset (c <= 0 ? kNumPresets - 1 : c - 1); };
    nextBtn.onClick = [this] { const int c = proc.currentPreset.load(); proc.loadPreset (c < 0 || c >= kNumPresets - 1 ? 0 : c + 1); };
    presetBtn.onClick = [this] { showPresetMenu(); };
    presetBtn.setTooltip (U ("800 presets, 100 per skola. Frysta spår lämnas i fred."));
    genetikBtn.getProperties().set ("hero", true);
    genetikBtn.setTooltip (U ("GENETIK: nytt groove och nytt ljud ur skolans regelbok. Aldrig fri slump. Frysta spår lämnas i fred."));
    genetikBtn.onClick = [this] { proc.genetik(); };
    ljudBtn.setTooltip (U ("Nytt ljud ur skolan, samma groove."));
    ljudBtn.onClick = [this] { proc.genetikLjud(); };
    angraBtn.setTooltip (U ("Ångra (upp till 40 steg)."));
    angraBtn.onClick = [this] { proc.undo(); };
    spela.setClickingTogglesState (true);
    spela.setTooltip (U ("Egen klocka när DAW:en står still. Spelar DAW:en följer TOMRUM den."));
    spelaAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, "spela", spela);
    tempo.attach (s, "tempo", "TEMPO", U ("Tempo för den egna klockan."));
    glomBtn.setTooltip (U ("Släpp loopen och gå tillbaka till skolans egen otakt."));
    glomBtn.onClick = [this] { proc.forgetParasiteLoop(); parasite.status = {}; };
    for (auto* c : std::initializer_list<juce::Component*> { &prevBtn, &nextBtn, &presetBtn, &genetikBtn, &ljudBtn, &angraBtn, &spela, &tempo, &glomBtn, &scope, &parasite })
        addAndMakeVisible (*c);

    for (int l = 0; l < kLanes; ++l)
    {
        lanes.push_back (std::make_unique<LaneRow> (proc, l));
        addAndMakeVisible (*lanes.back());
    }

    minus.attach (s, "minus", "MINUS", U ("Tar bort slag. De svaga stegen går först, kicken aldrig. Groovet byggs av det som inte spelas."));
    svang.attach (s, "svang", U ("SVÄNG"), U ("Udda 16-delar släpar, inom skolans spann."));
    friktion.attach (s, "friktion", "FRIKTION", U ("Varje spår släpar sina egna millisekunder bakom rutnätet. Kicken ligger alltid rakt."));
    parasit.attach (s, "parasit", "PARASIT", U ("Hur mycket av otakten (skolans eller loopens) som läggs på slagen."));
    dub.attach (s, "dub", "DUB", U ("Chansen att ett enskilt slag kastas ner i källaren. Resten står torrt."));
    pump.attach (s, "pump", "PUMP", U ("Allt utom kicken trycks ner av kicken."));
    tak.attach (s, "tak", "TAK", U ("Lågpasset ovanför allt. 100 % = skolans tak."));
    ruttna.attach (s, "ruttna", "RUTTNA", U ("Inget låter likadant länge: taket sjunker och stereon krymper sakta under ungefär sju minuter. Startar om när du trycker play."));
    variation.attach (s, "variation", "VARIATION", U ("Var fjärde takt vrids svaga steg, alltid inom regelboken."));
    volym.attach (s, "volym", "VOLYM", U ("Utnivå."));
    for (auto* k : { &minus, &svang, &friktion, &parasit, &dub, &pump, &tak, &ruttna, &variation, &volym }) addAndMakeVisible (*k);

    grundton.addItemList (tonNamn(), 1);
    grundton.setTooltip (U ("Grundton för bas och ackord."));
    grundtonAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "grundton", grundton);
    addAndMakeVisible (grundton);

    setSize (1180, 720);
    startTimerHz (30);
}

TomrumEditor::~TomrumEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TomrumEditor::showPresetMenu()
{
    juce::PopupMenu m;
    const int cur = proc.currentPreset.load();
    for (int s = 0; s < kNumSkolor; ++s)
    {
        juce::PopupMenu sub;
        const int a = s * kPresetsPerSkola;
        for (int block = 0; block < kPresetsPerSkola / 25; ++block)
        {
            juce::PopupMenu part;
            for (int i = a + block * 25; i < a + block * 25 + 25; ++i)
                part.addItem (i + 1, U (presetName (i).c_str()), true, i == cur);
            sub.addSubMenu (juce::String (block * 25 + 1) + "-" + juce::String (block * 25 + 25), part, true, nullptr,
                            cur >= a + block * 25 && cur < a + block * 25 + 25);
        }
        m.addSubMenu (U (skolor()[s].name), sub, true, nullptr, cur >= a && cur < a + kPresetsPerSkola);
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn),
                     [this] (int r) { if (r > 0) proc.loadPreset (r - 1); });
}

void TomrumEditor::timerCallback()
{
    const int v = proc.genomeVersion.load();
    if (v != seenVersion) { seenVersion = v; gen = proc.genome(); repaint (infoArea); parasite.repaint(); }

    if (skola.getSelectedItemIndex() != gen.skola) skola.setSelectedItemIndex (gen.skola, juce::dontSendNotification);
    const auto label = proc.presetLabel();
    if (presetBtn.getButtonText() != label) presetBtn.setButtonText (label);
    angraBtn.setEnabled (proc.canUndo());

    const double ppq = proc.uiPpq.load();
    const bool playing = proc.uiPlaying.load();
    for (auto& l : lanes) l->tick (gen, ppq, playing);
    scope.tick (gen);
}

void TomrumEditor::paint (juce::Graphics& g)
{
    g.fillAll (C::bg);

    // ordmärket
    auto t = topArea;
    g.setFont (mono (22.0f, true));
    g.setColour (C::light);
    g.drawText ("T O M R U M", t.removeFromLeft (190).withTrimmedLeft (20), juce::Justification::centredLeft, false);
    g.setFont (mono (9.0f));
    g.setColour (C::grey);
    g.drawText (U ("PHUNKSHUNZ AUDIO  /  MASKINHALLEN"), juce::Rectangle<int> (20, topArea.getBottom() - 16, 260, 12), juce::Justification::centredLeft, false);

    // linjer som delar panelen
    g.setColour (C::line);
    g.drawHorizontalLine (topArea.getBottom(), 0.0f, (float) getWidth());
    g.drawHorizontalLine (knobArea.getY(), 0.0f, (float) getWidth());
    g.drawVerticalLine (laneArea.getRight() + 10, (float) topArea.getBottom(), (float) knobArea.getY());
    g.drawVerticalLine (infoArea.getX() - 10, (float) topArea.getBottom(), (float) knobArea.getY());

    // rubriker
    g.setFont (mono (10.0f, true));
    g.setColour (C::grey);
    g.drawText (U ("SPÅR   (klicka namnet = GENETIK på spåret)"), laneArea.getX(), laneArea.getY() - 2, laneArea.getWidth(), 14, juce::Justification::centredLeft, false);
    g.drawText ("SKOLA", infoArea.getX(), infoArea.getY() - 2, 100, 14, juce::Justification::centredLeft, false);

    // skolans beskrivning
    const Skola& s = skolor()[juce::jlimit (0, kNumSkolor - 1, gen.skola)];
    g.setFont (mono (16.0f, true));
    g.setColour (C::amber);
    g.drawText (U (s.name), infoArea.getX(), infoArea.getY() + 16, infoArea.getWidth(), 22, juce::Justification::centredLeft, false);
    g.setFont (mono (11.0f));
    g.setColour (C::light.withAlpha (0.75f));
    g.drawFittedText (U (s.sub), infoArea.getX(), infoArea.getY() + 42, infoArea.getWidth(), 48, juce::Justification::topLeft, 4);
    g.setColour (C::grey);
    g.drawText (juce::String (s.bpm) + U (" BPM i skolan"), infoArea.getX(), infoArea.getY() + 94, infoArea.getWidth(), 14, juce::Justification::centredLeft, false);

    g.setFont (mono (10.0f));
    g.setColour (C::grey.withAlpha (0.8f));
    g.drawFittedText (U ("MIDI ut: varje slag blir en not på kanal 1. Lägg en trummaskin efter TOMRUM för att dubbla slagen."),
                      infoArea.getX(), knobArea.getY() - 44, infoArea.getWidth(), 36, juce::Justification::bottomLeft, 3);
}

void TomrumEditor::resized()
{
    auto r = getLocalBounds();
    topArea = r.removeFromTop (70);
    {
        auto t = topArea.reduced (16, 16);
        t.removeFromLeft (270);
        tempo.setBounds (t.removeFromRight (56).withSizeKeepingCentre (56, 60).translated (0, 4));
        t.removeFromRight (8);
        spela.setBounds (t.removeFromRight (70));
        t.removeFromRight (18);
        angraBtn.setBounds (t.removeFromRight (70));
        t.removeFromRight (6);
        ljudBtn.setBounds (t.removeFromRight (64));
        t.removeFromRight (6);
        genetikBtn.setBounds (t.removeFromRight (150));
        t.removeFromRight (18);
        skola.setBounds (t.removeFromLeft (190));
        t.removeFromLeft (12);
        prevBtn.setBounds (t.removeFromLeft (30));
        nextBtn.setBounds (t.removeFromRight (30));
        presetBtn.setBounds (t.reduced (4, 0));
    }

    knobArea = r.removeFromBottom (120);
    {
        auto k = knobArea.reduced (20, 14);
        grundton.setBounds (k.removeFromRight (80).withSizeKeepingCentre (80, 28).translated (0, -8));
        k.removeFromRight (12);
        const int w = k.getWidth() / 10;
        for (auto* kn : { &minus, &svang, &friktion, &parasit, &dub, &pump, &tak, &ruttna, &variation, &volym })
            kn->setBounds (k.removeFromLeft (w).withSizeKeepingCentre (juce::jmin (w, 84), 90));
    }

    auto mid = r.reduced (20, 16);
    laneArea = mid.removeFromLeft (440).withTrimmedTop (16);
    mid.removeFromLeft (20);
    infoArea = mid.removeFromRight (250).withTrimmedTop (16);
    mid.removeFromRight (20);
    scopeArea = mid;
    scope.setBounds (scopeArea);

    {
        auto in = infoArea;
        in.removeFromTop (120);
        parasite.setBounds (in.removeFromTop (180));
        in.removeFromTop (8);
        glomBtn.setBounds (in.removeFromTop (26).removeFromLeft (130));
    }

    auto la = laneArea.withTrimmedTop (6);
    const int h = la.getHeight() / kLanes;
    for (auto& l : lanes) l->setBounds (la.removeFromTop (h));
}
