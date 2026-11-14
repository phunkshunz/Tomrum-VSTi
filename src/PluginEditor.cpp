#include "PluginEditor.h"

using namespace juce;
using namespace Bunker;

namespace
{
String U (const char* s) { return sprak::trU (s); }
void label (Graphics& g, Rectangle<int> r, const String& t, Colour c, float size, Justification j = Justification::centredLeft, bool bold = true)
{
    g.setColour (c); g.setFont (FontOptions (size, bold ? Font::bold : Font::plain)); g.drawText (t, r, j, true);
}
void card (Graphics& g, Rectangle<int> r, const String& title, const String& sub = {})
{
    g.setColour (plate2); g.fillRoundedRectangle (r.toFloat(), 5.0f);
    g.setColour (edge); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 5.0f, 1.0f);
    if (title.isNotEmpty())
    {
        auto t = r.reduced (10, 6).removeFromTop (16);
        label (g, t, title, bone, 12.0f);
        if (sub.isNotEmpty()) label (g, t, sub, ash, 9.5f, Justification::centredRight, false);
    }
}
float getParam (TomrumProcessor& p, const char* id) { return p.apvts.getRawParameterValue (id)->load(); }
} // namespace

// ---------------------------------------------------------------- tärningen
void Dice::paintButton (Graphics& g, bool over, bool down)
{
    const float s = (float) jmin (getWidth(), getHeight()) - 2.0f;
    auto r = Rectangle<float> (s, s).withCentre (getLocalBounds().toFloat().getCentre());
    if (down) r = r.reduced (1.0f);
    g.setColour (amber.withAlpha (over ? 0.3f : 0.12f)); g.fillRoundedRectangle (r, s * 0.22f);
    g.setColour (amber); g.drawRoundedRectangle (r.reduced (0.6f), s * 0.22f, 1.1f);
    const float d = jmax (2.0f, s * 0.17f);
    for (auto p : { Point<float> (0.28f, 0.28f), { 0.72f, 0.28f }, { 0.5f, 0.5f }, { 0.28f, 0.72f }, { 0.72f, 0.72f } })
        g.fillEllipse (r.getX() + r.getWidth() * p.x - d * 0.5f, r.getY() + r.getHeight() * p.y - d * 0.5f, d, d);
}

// ---------------------------------------------------------------- KARTAN
void MapView::paint (Graphics& g)
{
    const auto p = pad();
    g.setColour (black); g.fillRoundedRectangle (p, 4.0f);
    // skolans tre städer och tre rörelser: rutnät och platser
    for (int i = 0; i < 3; ++i)
    {
        const float x = p.getX() + p.getWidth() * (float) i / 2.0f, y = p.getBottom() - p.getHeight() * (float) i / 2.0f;
        g.setColour (edge); g.drawLine (x, p.getY(), x, p.getBottom(), 1.0f); g.drawLine (p.getX(), y, p.getRight(), y, 1.0f);
    }
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j)
    {
        const float x = p.getX() + p.getWidth() * (float) i / 2.0f, y = p.getBottom() - p.getHeight() * (float) j / 2.0f;
        g.setColour (amberDk.withAlpha (0.8f)); g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }
    static const char* xs[3] = { u8"KÖLN", "BERLIN", "BUKAREST" };
    static const char* ys[3] = { "RAK", "RULL", "BRUTEN" };
    for (int i = 0; i < 3; ++i)
    {
        const float x = p.getX() + p.getWidth() * (float) i / 2.0f;
        const int lx = i == 0 ? (int) p.getX() : (i == 2 ? (int) p.getRight() - 80 : (int) x - 40);
        label (g, Rectangle<int> (lx, (int) p.getBottom() + 3, 80, 14), U (xs[i]), ash, 9.5f,
               i == 0 ? Justification::centredLeft : (i == 2 ? Justification::centredRight : Justification::centred));
        const float y = i == 0 ? p.getBottom() - 40.0f : (i == 2 ? p.getY() + 40.0f : p.getCentreY());
        g.saveState();
        g.addTransform (AffineTransform::rotation (-MathConstants<float>::halfPi, 12.0f, y));
        label (g, Rectangle<int> (12 - 40, (int) y - 7, 80, 14), U (ys[i]), ash, 9.5f, Justification::centred);
        g.restoreState();
    }
    // spåret av rörelsen och punkten
    for (int i = 0; i < trailN; ++i)
    {
        const auto q = trail[(size_t) i];
        const float a = (float) (i + 1) / (float) (trailN + 1);
        g.setColour (amber.withAlpha (0.25f * a));
        g.fillEllipse (p.getX() + q.x * p.getWidth() - 3.0f, p.getBottom() - q.y * p.getHeight() - 3.0f, 6.0f, 6.0f);
    }
    const float mx = getParam (proc, "mapx"), my = getParam (proc, "mapy");
    const float px = p.getX() + mx * p.getWidth(), py = p.getBottom() - my * p.getHeight();
    const float tom = getParam (proc, "tomrum");
    const float rad = 9.0f + 22.0f * (1.0f - tom);   // ju tätare desto större ring
    g.setColour (amber.withAlpha (0.12f)); g.fillEllipse (px - rad, py - rad, rad * 2, rad * 2);
    g.setColour (amber.withAlpha (0.6f)); g.drawEllipse (px - rad, py - rad, rad * 2, rad * 2, 1.0f);
    g.setColour (amber); g.fillEllipse (px - 6.0f, py - 6.0f, 12.0f, 12.0f);
    g.setColour (black); g.fillEllipse (px - 2.0f, py - 2.0f, 4.0f, 4.0f);
}

void MapView::tick()
{
    const Point<float> q (getParam (proc, "mapx"), getParam (proc, "mapy"));
    if (trailN == 0 || trail[(size_t) (trailN - 1)].getDistanceFrom (q) > 0.01f)
    {
        if (trailN == (int) trail.size()) { std::rotate (trail.begin(), trail.begin() + 1, trail.end()); --trailN; }
        trail[(size_t) trailN++] = q;
        repaint();
    }
}

void MapView::setFrom (Point<float> pos)
{
    const auto p = pad();
    const float x = jlimit (0.0f, 1.0f, (pos.x - p.getX()) / p.getWidth());
    const float y = jlimit (0.0f, 1.0f, (p.getBottom() - pos.y) / p.getHeight());
    proc.apvts.getParameter ("mapx")->setValueNotifyingHost (x);
    proc.apvts.getParameter ("mapy")->setValueNotifyingHost (y);
    repaint();
}
void MapView::mouseDown (const MouseEvent& e)
{
    proc.apvts.getParameter ("mapx")->beginChangeGesture(); proc.apvts.getParameter ("mapy")->beginChangeGesture();
    setFrom (e.position);
}
void MapView::mouseDrag (const MouseEvent& e) { setFrom (e.position); }
void MapView::mouseUp (const MouseEvent&)
{
    proc.apvts.getParameter ("mapx")->endChangeGesture(); proc.apvts.getParameter ("mapy")->endChangeGesture();
}

// ---------------------------------------------------------------- spåren
LaneRow::LaneRow (TomrumProcessor& p, int l) : proc (p), lane (l), dice (u8"Nytt mönster i det här spåret")
{
    las.setButtonText (U (u8"LÅS"));
    dens.naken = true;
    dens.attach (p.apvts, "dens" + String (l), u8"TÄTHET");
    dens.setAccent (l == tr2::Kick ? ash : amber);
    dens.slider.setTooltip (U ("Täthet: hur mycket spåret spelar jämfört med kartan."));
    addAndMakeVisible (dens);
    for (auto* b : { &las, &tyst })
    {
        b->setClickingTogglesState (true);
        b->setColour (TextButton::buttonOnColourId, b == &las ? amber : Colour (0xffc0392b));
        addAndMakeVisible (*b);
    }
    las.setTooltip (U ("LÅS: spåret behåller sitt mönster när du slumpar."));
    tyst.setTooltip (U ("Tysta spåret."));
    lasAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "las" + String (l), las);
    tystAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "mute" + String (l), tyst);
    dice.onClick = [this] { proc.slumpaSpar (lane); };
    addAndMakeVisible (dice);
    if (l == tr2::Kick) { dice.setEnabled (false); las.setEnabled (false); }
}

void LaneRow::resized()
{
    auto r = getLocalBounds().reduced (0, 3);
    r.removeFromLeft (54);
    cells = r.removeFromLeft (16 * 6 + 2);
    r.removeFromLeft (6);
    dens.setBounds (r.removeFromLeft (32).withSizeKeepingCentre (32, 32));
    r.removeFromLeft (4);
    las.setBounds (r.removeFromLeft (34).withSizeKeepingCentre (34, 20));
    r.removeFromLeft (3);
    tyst.setBounds (r.removeFromLeft (22).withSizeKeepingCentre (22, 20));
    r.removeFromLeft (3);
    dice.setBounds (r.removeFromLeft (22).withSizeKeepingCentre (20, 20));
}

void LaneRow::setPattern (const std::array<float, 16>& w, const std::array<bool, 16>& h, int len, int playStep, float fl)
{
    if (w != weight || h != hit || len != length || playStep != step || std::abs (fl - flash) > 0.02f)
    { weight = w; hit = h; length = len; step = playStep; flash = fl; repaint(); }
}

void LaneRow::paint (Graphics& g)
{
    auto r = getLocalBounds();
    const bool muted = getParam (proc, ("mute" + String (lane)).toRawUTF8()) > 0.5f;
    // blink + namn
    const float led = flash;
    g.setColour (amber.withAlpha (0.15f + 0.85f * led)); g.fillEllipse (2.0f, (float) r.getCentreY() - 3.0f, 6.0f, 6.0f);
    label (g, Rectangle<int> (12, 0, 44, getHeight()), U (tomrum::laneName (lane)), muted ? ash.withAlpha (0.5f) : bone, 10.5f);
    if (length != 16) label (g, Rectangle<int> (12, getHeight() / 2 + 7, 44, 11), String (length) + "/16", ash, 8.5f, Justification::centredLeft, false);
    // rutorna: skugga = kartans vikt, fylld = slår, ram = här spelas nu
    const float cw = (float) (cells.getWidth() - 2) / 16.0f, ch = (float) jmin (16, cells.getHeight() - 6);
    const float y0 = (float) cells.getCentreY() - ch * 0.5f;
    for (int i = 0; i < 16; ++i)
    {
        auto c = Rectangle<float> ((float) cells.getX() + 1.0f + cw * (float) i, y0, cw - 2.0f, ch);
        const bool beyond = i >= length;
        g.setColour (edge.withAlpha (beyond ? 0.4f : 1.0f)); g.fillRoundedRectangle (c, 1.5f);
        if (! beyond && weight[(size_t) i] > 0.02f) { g.setColour (amberDk.withAlpha (0.25f + 0.5f * jlimit (0.0f, 1.0f, weight[(size_t) i]))); g.fillRoundedRectangle (c.reduced (0.0f, c.getHeight() * (1.0f - jlimit (0.0f, 1.0f, weight[(size_t) i])) * 0.5f), 1.5f); }
        if (! beyond && hit[(size_t) i]) { g.setColour (muted ? ash : amber); g.fillRoundedRectangle (c.reduced (1.5f), 1.0f); }
        if (i == step) { g.setColour (bone); g.drawRoundedRectangle (c, 1.5f, 1.2f); }
    }
}

// ---------------------------------------------------------------- DRA UT MIDI
void MidiDrag::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (dragging ? amber.withAlpha (0.35f) : plate); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (amber); g.drawRoundedRectangle (r, 4.0f, 1.2f);
    label (g, getLocalBounds(), U ("DRA UT MIDI"), amber, 11.0f, Justification::centred);
}
void MidiDrag::mouseDrag (const MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true; repaint();
    const bool lat = proc.apvts.getRawParameterValue ("lat")->load() > 0.5f;
    const int bars = lat ? (roundToInt (proc.apvts.getRawParameterValue ("latlen")->load()) == 1 ? 128 : 64) : 8;
    const auto f = proc.exportMidi (bars);
    DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; repaint(); });
}

// ---------------------------------------------------------------- panelen
void TomrumEditor::knob (Knob& k, const char* id, const char* name, Colour c, const char* tip, bool sida)
{
    k.sida = sida;
    k.attach (proc.apvts, id, String::fromUTF8 (name));
    k.setAccent (c);
    k.slider.setTooltip (U (tip));
    addAndMakeVisible (k);
}
void TomrumEditor::combo (ComboBox& b, const char* id, const char* tip)
{
    if (auto* p = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id)))
        for (int i = 0; i < p->choices.size(); ++i) b.addItem (sprak::tr (p->choices[i]), i + 1);
    b.setTooltip (U (tip));
    comboAtt.push_back (std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, b));
    addAndMakeVisible (b);
}
void TomrumEditor::toggle (TextButton& b, const char* id, const char* tip)
{
    b.setClickingTogglesState (true);
    b.setColour (TextButton::buttonOnColourId, amber);
    b.setTooltip (U (tip));
    btnAtt.push_back (std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, b));
    addAndMakeVisible (b);
}

TomrumEditor::TomrumEditor (TomrumProcessor& p) : AudioProcessorEditor (p), proc (p), map (p), midiDrag (p)
{
    setLookAndFeel (&lnf);
    lnf.setColour (Slider::rotarySliderFillColourId, amber);
    lnf.setColour (ComboBox::arrowColourId, amber);
    lnf.setColour (PopupMenu::highlightedBackgroundColourId, amber);
    lnf.setColour (TextButton::buttonOnColourId, amber);

    undoBtn.setButtonText (U (u8"ÅNGRA")); latBtn.setButtonText (U (u8"LÅTEN")); monsterBtn.setButtonText (U (u8"NYTT MÖNSTER"));
    // huvudet
    addAndMakeVisible (presetBox);
    presetBox.setTooltip (U ("Lägen: varje skola på sex platser på kartan."));
    presetBox.onChange = [this] { const int id = presetBox.getSelectedId(); if (id > 0) proc.setCurrentProgram (id - 1); };
    for (auto* b : { &prevBtn, &nextBtn, &undoBtn, &allBtn, &monsterBtn }) addAndMakeVisible (*b);
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n); };
    nextBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    undoBtn.onClick = [this] { proc.undo(); };
    undoBtn.setTooltip (U ("Tillbaka till hur det var före senaste slumpen eller läget."));
    allBtn.onClick = [this] { proc.slumpaAllt(); };
    allBtn.setColour (TextButton::buttonColourId, amber.withAlpha (0.25f));
    allBtn.setTooltip (U ("Slumpa allt: mönster, ljud, plats på kartan och gung. Låsta spår behålls."));
    monsterBtn.onClick = [this] { proc.slumpaMonster(); };
    monsterBtn.setTooltip (U ("Nya mönster i alla spår som inte är låsta. Kicken står kvar."));
    spraket.accent = amber;
    spraket.onClick = [this] { sprak::sattEngelska (! sprak::engelska()); sprak::oversattAllt (*this); refreshPresetBox(); repaint(); };
    addAndMakeVisible (spraket);
    knob (ut, "ut", "UT", bone, "Utnivå.");

    // vänster
    combo (skolaBox, "skola", "Skolan: regelboken som färgar kartan och bestämmer ljudet.");
    combo (gungBox, "gungtyp", "Typ av sväng: RAK, MPC, SLÄP (allt lutar bakåt), BUKAREST (rullande), OTAKT (skolans egen).");
    knob (gung, "gung", "GUNG", amber, "Hur mycket sväng.");
    knob (friktion, "friktion", "FRIKTION", amber, "Varje spår släpar sina egna millisekunder bakom rutnätet. Kicken ligger alltid rakt.");
    combo (rootBox, "root", "Grundton för bas och stab.");
    combo (skalaBox, "skala", "Skalan som riffen håller sig i.");
    knob (riff, "riff", "RIFF", teal, "0 = bara grundtonen. Upp = basen och staben spelar små figurer i skalan.");
    knob (djup, "djup", "DJUP", teal, "DUB: hur många slag som kastas ner i ekot.");
    knob (kallare, "kallare", u8"KÄLLARE", teal, "Betongrummet. Kick och bas hålls torra.");
    knob (kassett, "kassett", "KASSETT", teal, "Band: tonen svajar lite, som en gammal kassett.");
    knob (ruttna, "ruttna", "RUTTNA", teal, "Taket sjunker och bredden smalnar ju längre låten går.");
    knob (pump, "pump", "PUMP", teal, "Allt utom kicken trycks ner av kicken.");
    knob (skolfarg, "skolfarg", u8"SKOLFÄRG", teal, "Hur mycket skolans egna mönster färgar kartan.");
    diceLjud.onClick = [this] { proc.slumpaLjud(); };
    addAndMakeVisible (diceLjud);

    // mitten
    addAndMakeVisible (map);
    map.setTooltip (U ("KARTAN: dra punkten. Åt sidan KÖLN, BERLIN, BUKAREST. Uppåt RAK, RULL, BRUTEN. Mönstren glider över i varandra."));
    knob (tomrumK, "tomrum", "TOMRUM", amber, "Hur tomt. De svaga slagen försvinner först. Kicken står kvar.");
    knob (variation, "variation", "VARIATION", amber, "Var fjärde takt flyttas vikterna lite, alltid inom kartan.");
    toggle (polyBtn, "poly", "POLY: spåren får skolans egna längder (15, 14, 7 ...) och glider mot varandra.");
    diceKarta.onClick = [this] { proc.slumpaKarta(); };
    addAndMakeVisible (diceKarta);
    toggle (latBtn, "lat", "LÅTEN: arrangerar själv över 64 eller 128 takter. Intro, break utan kick, topp och fill. Samma låt varje gång från början.");
    combo (latLenBox, "latlen", "Hur lång låten är innan den börjar om.");

    // höger: spåren
    for (int l = 0; l < tr2::kLanes; ++l) { rows[(size_t) l] = std::make_unique<LaneRow> (p, l); addAndMakeVisible (*rows[(size_t) l]); }

    // nederst: varianter
    for (int i = 0; i < TomrumProcessor::kVarianter; ++i)
    {
        auto& b = varBtn[(size_t) i];
        b.setButtonText (String (i + 1));
        b.setTooltip (U ("Variant: klicka för att byta (vid nästa takt). Med SPARA på: spara läget här. MIDI-tangent C3 till G3 byter också."));
        b.onClick = [this, i]
        {
            if (sparaLage) { proc.sparaVariant (i); sparaLage = false; sparaBtn.setToggleState (false, dontSendNotification); }
            else proc.valjVariant (i);
        };
        addAndMakeVisible (b);
    }
    sparaBtn.setClickingTogglesState (true);
    sparaBtn.setColour (TextButton::buttonOnColourId, Colour (0xffc0392b));
    sparaBtn.setTooltip (U ("Tryck SPARA och sedan en siffra: läget sparas där."));
    sparaBtn.onClick = [this] { sparaLage = sparaBtn.getToggleState(); };
    addAndMakeVisible (sparaBtn);
    toggle (midiUtBtn, "midiut", "MIDI UT: trummorna på kanal 1, basen på kanal 2, staben på kanal 3.");
    addAndMakeVisible (midiDrag);

    refreshPresetBox();
    sprak::oversattAllt (*this);
    setSize (kW, kH);
    startTimerHz (30);
}

TomrumEditor::~TomrumEditor() { setLookAndFeel (nullptr); }

void TomrumEditor::refreshPresetBox()
{
    presetBox.clear (dontSendNotification);
    String cat; int id = 1;
    PopupMenu* sub = nullptr; ignoreUnused (sub);
    for (const auto& pr : TomrumProcessor::presets())
    {
        if (pr.category != cat) { cat = pr.category; presetBox.addSectionHeading (sprak::tr (cat)); }
        presetBox.addItem (sprak::tr (pr.name), id++);
    }
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
}

void TomrumEditor::resized()
{
    // huvudet
    auto top = Rectangle<int> (0, 0, kW, 50).reduced (12, 10);
    top.removeFromLeft (220);
    prevBtn.setBounds (top.removeFromLeft (26)); top.removeFromLeft (4);
    presetBox.setBounds (top.removeFromLeft (200)); top.removeFromLeft (4);
    nextBtn.setBounds (top.removeFromLeft (26)); top.removeFromLeft (12);
    undoBtn.setBounds (top.removeFromLeft (70)); top.removeFromLeft (6);
    allBtn.setBounds (top.removeFromLeft (110)); top.removeFromLeft (6);
    monsterBtn.setBounds (top.removeFromLeft (110));
    ut.setBounds (top.removeFromRight (96).withSizeKeepingCentre (96, 34));
    top.removeFromRight (8);
    spraket.setBounds (top.removeFromRight (52).withSizeKeepingCentre (52, 20));

    const int y0 = 56, bodyH = 504;
    // vänster
    {
        const int x = 12, w = 270;
        skolaArea = { x, y0, w, 58 };
        skolaBox.setBounds (skolaArea.reduced (10, 6).withTrimmedTop (22));
        gungArea = { x, skolaArea.getBottom() + 8, w, 108 };
        { auto a = gungArea.reduced (10, 6).withTrimmedTop (22); gungBox.setBounds (a.removeFromTop (24)); a.removeFromTop (6);
          gung.setBounds (a.removeFromLeft (a.getWidth() / 2).withHeight (44)); friktion.setBounds (a.withHeight (44)); }
        tonArea = { x, gungArea.getBottom() + 8, w, 108 };
        { auto a = tonArea.reduced (10, 6).withTrimmedTop (22); auto row = a.removeFromTop (24);
          rootBox.setBounds (row.removeFromLeft (70)); row.removeFromLeft (6); skalaBox.setBounds (row); a.removeFromTop (6);
          riff.setBounds (a.removeFromLeft (a.getWidth() / 2).withHeight (44)); }
        ljudArea = { x, tonArea.getBottom() + 8, w, y0 + bodyH - (tonArea.getBottom() + 8) };
        { auto a = ljudArea.reduced (10, 6).withTrimmedTop (24);
          diceLjud.setBounds (ljudArea.getRight() - 32, ljudArea.getY() + 5, 22, 22);
          Knob* ks[6] = { &djup, &kallare, &kassett, &ruttna, &pump, &skolfarg };
          const int rh = jmin (52, a.getHeight() / 3);
          for (int i = 0; i < 6; ++i) ks[i]->setBounds (a.getX() + (i % 2) * (a.getWidth() / 2), a.getY() + (i / 2) * rh, a.getWidth() / 2, jmin (44, rh)); }
    }
    // mitten
    {
        const int x = 292, w = 392;
        mapArea = { x, y0, w, 410 };
        diceKarta.setBounds (mapArea.getRight() - 32, mapArea.getY() + 5, 22, 22);
        auto a = mapArea.reduced (8, 6).withTrimmedTop (22);
        auto bottom = a.removeFromBottom (46);
        map.setBounds (a);
        tomrumK.setBounds (bottom.removeFromLeft (150).withHeight (44)); bottom.removeFromLeft (4);
        variation.setBounds (bottom.removeFromLeft (150).withHeight (44));
        polyBtn.setBounds (bottom.removeFromRight (64).withSizeKeepingCentre (60, 24));
        latArea = { x, mapArea.getBottom() + 8, w, y0 + bodyH - (mapArea.getBottom() + 8) };
        auto la = latArea.reduced (10, 6).withTrimmedTop (22);
        auto row = la.removeFromTop (26);
        latBtn.setBounds (row.removeFromLeft (80)); row.removeFromLeft (8);
        latLenBox.setBounds (row.removeFromLeft (120)); row.removeFromLeft (10);
        latInfo = row;
    }
    // höger
    {
        const int x = 696, w = kW - 12 - 696;
        sparArea = { x, y0, w, bodyH };
        auto a = sparArea.reduced (8, 6).withTrimmedTop (24);
        const int rh = a.getHeight() / tr2::kLanes;
        for (int l = 0; l < tr2::kLanes; ++l) rows[(size_t) l]->setBounds (a.removeFromTop (rh));
    }
    // nederst
    {
        varArea = { 12, y0 + bodyH + 8, kW - 24, kH - (y0 + bodyH + 8) - 10 };
        auto a = varArea.reduced (10, 8);
        a.removeFromLeft (96);
        for (auto& b : varBtn) { b.setBounds (a.removeFromLeft (40)); a.removeFromLeft (5); }
        a.removeFromLeft (4);
        sparaBtn.setBounds (a.removeFromLeft (70)); a.removeFromLeft (20);
        midiDrag.setBounds (a.removeFromRight (150)); a.removeFromRight (8);
        midiUtBtn.setBounds (a.removeFromRight (80));
    }
}

void TomrumEditor::paint (Graphics& g)
{
    g.fillAll (black);
    g.setColour (plate); g.fillRect (0, 0, kW, 50);
    g.setColour (edge); g.drawHorizontalLine (50, 0.0f, (float) kW);
    label (g, { 14, 6, 140, 26 }, "TOMRUM", amber, 24.0f);
    label (g, { 15, 30, 210, 14 }, String::fromUTF8 (u8"DAS LEERRAUM  ·  MASKINHALLEN"), ash, 9.0f, Justification::centredLeft, false);

    card (g, skolaArea, U ("SKOLA"));
    card (g, gungArea, U ("GUNG"));
    card (g, tonArea, U ("TONART"));
    card (g, ljudArea, U ("LJUD"));
    card (g, mapArea, U ("KARTAN"));
    card (g, latArea, U ("LÅTEN"));
    card (g, sparArea, U ("SPÅREN"), U ("täthet · lås · tyst · tärning"));
    card (g, varArea, {});
    label (g, varArea.reduced (12, 0).withWidth (96), U ("VARIANTER"), bone, 12.0f);
    label (g, latInfo, latText, amber, 11.0f);

    // LÅTEN som tidslinje: delarna, och var vi är
    {
        auto tl = latArea.reduced (10, 6).withTrimmedTop (52).toFloat();
        const bool on = getParam (proc, "lat") > 0.5f;
        int nd = 0; const auto* dl = tr2::lat (nd);
        for (int i = 0; i < nd; ++i)
        {
            const float a = (float) dl[i].fromBar / 64.0f, b = (i + 1 < nd ? (float) dl[i + 1].fromBar : 64.0f) / 64.0f;
            auto seg = Rectangle<float> (tl.getX() + tl.getWidth() * a, tl.getY() + 12.0f, tl.getWidth() * (b - a) - 2.0f, tl.getHeight() - 12.0f);
            const float hgt = 0.35f + 0.65f * jlimit (0.0f, 1.0f, (float) __builtin_popcount (dl[i].lanes) / 8.0f + dl[i].dens);
            g.setColour ((dl[i].kick ? amberDk : edge).withAlpha (on ? 1.0f : 0.4f));
            g.fillRoundedRectangle (seg.withTrimmedTop (seg.getHeight() * (1.0f - hgt)), 2.0f);
            label (g, Rectangle<int> ((int) seg.getX(), (int) tl.getY(), (int) seg.getWidth(), 12), U (dl[i].name), on ? ash : ash.withAlpha (0.4f), 8.0f, Justification::centredLeft, false);
        }
        if (on && proc.running.load())
        {
            const int len = roundToInt (getParam (proc, "latlen")) == 1 ? 128 : 64;
            const double pos = std::fmod (proc.playPpq.load() / 4.0, (double) len) / (double) len;
            const float x = tl.getX() + tl.getWidth() * (float) pos;
            g.setColour (amber); g.fillRect (x - 1.0f, tl.getY() + 12.0f, 2.0f, tl.getHeight() - 12.0f);
        }
    }
}

void TomrumEditor::timerCallback()
{
    // spårens rutor: räknas med en egen motor (bara mönstret, inget ljud)
    const auto c = proc.currentControls();
    const double ppq = proc.playPpq.load();
    const bool run = proc.running.load();
    const long bar = run ? (long) std::floor (ppq / 4.0) : 0;
    for (int l = 0; l < tr2::kLanes; ++l)
    {
        std::array<float, 16> w {}; std::array<bool, 16> h {};
        const int len = preview.laneLength (c, l);
        for (int i = 0; i < 16; ++i) { float wi = 0; h[(size_t) i] = preview.previewHit (c, l, bar, i, wi); w[(size_t) i] = wi; }
        const int step = run ? (int) (((long) std::floor (ppq * 4.0)) % 16) : -1;
        float fl = proc.laneFlash[(size_t) l].load();
        proc.laneFlash[(size_t) l].store (fl * 0.82f);
        rows[(size_t) l]->setPattern (w, h, len, step, fl);
    }
    map.tick();

    // LÅTEN: var vi är
    String t;
    if (getParam (proc, "lat") > 0.5f)
    {
        const int d = proc.delNu.load();
        int nd = 0; const auto* dl = tr2::lat (nd);
        const int len = roundToInt (getParam (proc, "latlen")) == 1 ? 128 : 64;
        if (run && d >= 0 && d < nd) t = U (dl[d].name) + "  ·  " + U ("takt") + " " + String ((int) (((long) std::floor (ppq / 4.0)) % len) + 1) + " / " + String (len);
        else t = U ("tryck play");
    }
    if (t != latText) { latText = t; repaint (latArea); }

    // varianter: full = ljus, aktiv = bärnsten, väntande = blinkar
    const int act = proc.aktivVariant(), pend = proc.vantandeVariant();
    const bool blink = (Time::getMillisecondCounter() / 250) % 2 == 0;
    for (int i = 0; i < TomrumProcessor::kVarianter; ++i)
    {
        auto& b = varBtn[(size_t) i];
        Colour col = proc.harVariant (i) ? edge.brighter (0.4f) : plate;
        if (i == act) col = amber.withAlpha (0.8f);
        if (i == pend && blink) col = amber.withAlpha (0.4f);
        if (sparaLage) col = col.interpolatedWith (Colour (0xffc0392b), 0.35f);
        b.setColour (TextButton::buttonColourId, col);
    }
    if (proc.getCurrentProgram() != seenProgram) { seenProgram = proc.getCurrentProgram(); presetBox.setSelectedId (seenProgram + 1, dontSendNotification); }
    undoBtn.setEnabled (proc.canUndo());
    if (proc.stateVersion.load() != seenVersion) { seenVersion = proc.stateVersion.load(); repaint(); }
    map.repaint();
}
