#include "PluginEditor.h"

using namespace juce;
using namespace Bunker;
using namespace tomrum;

namespace
{
String u8 (const char* s) { return String::fromUTF8 (s); }
const char* kLaneTips[kLanes] = {
    "KICK: står alltid rakt på rutnätet. MINUS och MUTATION rör den aldrig.",
    "BAS: sinus som faller i tonhöjd på skolans egna steg.",
    "KLICK: kort brusklick.",
    "HATT: brus, eller sinusskurar i KLINIK.",
    "TRÄ: träkloss med två toner.",
    "SKRAP: knaster och grus.",
    "STAB: ackordet, kastas ofta ner i ekot.",
    "SPÖKE: spelar bara i tystnaden, där inget annat slår." };
}

// ---------------------------------------------------------------- skopet
void VoidScope::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float v = proc.voidLevel.load();
    const bool run = proc.running.load();
    g.setColour (black);
    g.fillRoundedRectangle (r, 6.0f);
    // skenet: ju tystare, desto mer lyser skärmen
    if (run && v > 0.05f)
    {
        ColourGradient glow (amber.withAlpha (0.28f * v), r.getCentreX(), r.getCentreY(), amber.withAlpha (0.0f), r.getX(), r.getY(), true);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r, 6.0f);
    }
    // rutnät
    g.setColour (edge);
    for (int i = 1; i < 8; ++i) g.drawVerticalLine ((int) (r.getX() + r.getWidth() * i / 8.0f), r.getY() + 4, r.getBottom() - 4);
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX() + 4, r.getRight() - 4);

    // vågen
    Path p;
    const int N = (int) proc.scope.size();
    const int w0 = proc.scopeWrite.load();
    for (int i = 0; i < N; ++i)
    {
        const float x = r.getX() + 6 + (r.getWidth() - 12) * i / (float) (N - 1);
        const float y = r.getCentreY() - jlimit (-1.0f, 1.0f, proc.scope[(size_t) ((w0 + i) % N)]) * r.getHeight() * 0.42f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (amber.withAlpha (0.25f));
    g.strokePath (p, PathStrokeType (4.0f));
    g.setColour (amber);
    g.strokePath (p, PathStrokeType (1.3f));

    // texten som kommer fram i tystnaden
    g.setFont (FontOptions (30.0f, Font::bold).withHorizontalScale (1.25f));
    g.setColour (amber.withAlpha (run ? 0.08f + 0.6f * v * v : 0.35f));
    g.drawText ("T O M R U M", r.reduced (10), Justification::centred);

    // RUTTNA-mätaren
    const float rot = proc.rotLevel.load();
    auto bar = r.removeFromBottom (14).reduced (10, 4);
    g.setColour (edge); g.fillRect (bar);
    g.setColour (amberDk); g.fillRect (bar.withWidth (bar.getWidth() * rot));
    g.setColour (ash); g.setFont (FontOptions (9.0f));
    g.drawText ("RUTTNA " + String (roundToInt (rot * 100)) + " %", r.removeFromBottom (14).reduced (10, 0), Justification::centredRight);
    g.setColour (edge);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
}

// ---------------------------------------------------------------- mönstret
void GridView::paint (Graphics& g)
{
    const Skola& s = skolor()[gen.skola];
    const double ppq = proc.playPpq.load();
    const bool run = proc.running.load();
    const long k = (long) std::floor (ppq * 4.0);
    const int cells = 32;
    const float x0 = (float) kLeft, cw = (getWidth() - kLeft - 8) / (float) cells;
    for (int l = 0; l < kLanes; ++l)
    {
        const auto& lg = gen.lane[l];
        const float y = (float) rowY (l) + 3, h = (float) rowH() - 6;
        const float flash = proc.laneFlash[(size_t) l].load();
        // namn
        g.setColour (flash > 0.01f ? amber.interpolatedWith (bone, 1.0f - flash) : bone);
        g.setFont (FontOptions (12.0f, Font::bold));
        g.drawText (u8 (laneName (l)), 8, (int) y - 6, 80, (int) h, Justification::centredLeft);
        g.setColour (ash); g.setFont (FontOptions (8.5f));
        String info = String (lg.len) + " steg";
        if (s.lane[l].notOnKick) info << u8 (" · ej på kick");
        if (l == Kick) info = u8 ("rak · rörs aldrig");
        if (l == Spoke) info = u8 ("bara i tystnaden");
        g.drawText (info, 8, (int) (y + h * 0.5f + 3), 90, 11, Justification::centredLeft);

        const int play = run && lg.len > 0 ? (int) (((k % lg.len) + lg.len) % lg.len) : -1;
        for (int c = 0; c < cells; ++c)
        {
            const int i = lg.len > 0 ? c % lg.len : 0;
            const bool repeat = c >= lg.len;
            auto cell = Rectangle<float> (x0 + c * cw + 1.5f, y, cw - 3.0f, h);
            const float w = lg.len > 0 ? lg.weight[i] : 0.0f;
            const float a = repeat ? 0.35f : 1.0f;
            g.setColour (((c / 4) % 2 == 0 ? Colour (0xff222222) : Colour (0xff1b1b1b)).withMultipliedAlpha (a));
            g.fillRoundedRectangle (cell, 2.5f);
            if (w > 0.0f)   // regelboken tillåter slag här
            {
                g.setColour (amberDk.withAlpha ((0.15f + 0.6f * w) * a));
                g.drawRoundedRectangle (cell.reduced (0.5f), 2.5f, w >= 1.0f ? 1.6f : 0.8f);
            }
            if (lg.len > 0 && lg.on (i))
            {
                const bool now = i == play && ! repeat;
                g.setColour ((now ? bone : amber).withAlpha (a * (0.45f + 0.55f * w)));
                g.fillRoundedRectangle (cell.reduced (cw * 0.16f, h * 0.18f), 2.0f);
            }
            if (i == play && ! repeat)
            {
                g.setColour (bone.withAlpha (0.5f));
                g.drawRoundedRectangle (cell, 2.5f, 1.0f);
            }
        }
    }
}

// ---------------------------------------------------------------- otakten
void ParasiteView::paint (Graphics& g)
{
    const auto p = proc.currentParasite();
    auto r = getLocalBounds().toFloat();
    g.setColour (black); g.fillRoundedRectangle (r, 4.0f);
    const float mid = r.getY() + r.getHeight() * 0.3f;
    g.setColour (edge); g.drawHorizontalLine ((int) mid, r.getX() + 4, r.getRight() - 4);
    const float bw = (r.getWidth() - 8) / 16.0f;
    for (int i = 0; i < 16; ++i)
    {
        const float o = p.off[i];
        const float hh = std::abs (o) / 0.4f * r.getHeight() * 0.62f;
        auto b = Rectangle<float> (r.getX() + 4 + i * bw + 1.5f, o >= 0 ? mid : mid - hh, bw - 3.0f, jmax (1.0f, hh));
        g.setColour (o >= 0 ? amber.withAlpha (0.85f) : bone.withAlpha (0.7f));
        g.fillRect (b);
    }
    g.setColour (ash); g.setFont (FontOptions (9.0f));
    g.drawText (proc.hasOwnParasite() ? u8 ("ur: ") + proc.parasiteName() : u8 ("skolans egen otakt"), r.reduced (6, 2), Justification::bottomLeft);
    g.drawText (u8 ("upp = tidigt · ner = sent"), r.reduced (6, 2), Justification::bottomRight);
}

// ---------------------------------------------------------------- panelen
TomrumEditor::TomrumEditor (TomrumProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    lnf.setColour (TextButton::buttonOnColourId, amber);
    lnf.setColour (Slider::rotarySliderFillColourId, amber);
    lnf.setColour (ComboBox::arrowColourId, amber);
    lnf.setColour (PopupMenu::highlightedBackgroundColourId, amber);
    lnf.setColour (ResizableWindow::backgroundColourId, black);

    for (auto* c : std::initializer_list<Component*> { &scope, &grid, &para }) addAndMakeVisible (c);
    para.setTooltip (u8 ("Släpp en loop här: TOMRUM suger ut otakten (var slagen ligger mot rutnätet) och låter alla spår släpa likadant."));

    for (int i = 0; i < kNumSkolor; ++i)
    {
        auto& b = skolaBtn[i];
        b.setButtonText (u8 (skolor()[i].name));
        b.setClickingTogglesState (false);
        b.setTooltip (u8 (skolor()[i].sub));
        b.onClick = [this, i] { setSkola (i); };
        addAndMakeVisible (b);
    }

    auto big = [this] (TextButton& b, const String& t, const String& tip, std::function<void()> fn)
    {
        b.setButtonText (t); b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b);
    };
    big (genBtn, "GENETIK", u8 ("Nytt genom: mönster och ljud. Aldrig fri slump: allt följer skolans regelbok."),
         [this] { proc.genetik(); setStatus (u8 ("GENETIK: nytt genom inom ") + u8 (skolor()[proc.skolaIndex()].name)); });
    big (mutBtn, "MUTATION", u8 ("Två eller tre spår föds om. Kicken står kvar."),
         [this] { proc.mutation(); setStatus (u8 ("MUTATION: några spår har muterat")); });
    big (soundBtn, "LJUD", u8 ("Nytt ljud inom skolans ramar, samma mönster."),
         [this] { proc.mutateSound(); setStatus (u8 ("Nytt ljud, samma mönster")); });
    big (undoBtn, u8 ("ÅNGRA"), u8 ("Tillbaka ett steg"), [this] { proc.undo(); setStatus (u8 ("Ångrat")); });
    big (rotBtn, u8 ("NYTT LIV"), u8 ("Nollställ RUTTNA: ljudet blir friskt igen och börjar ruttna om."),
         [this] { proc.resetRot(); setStatus (u8 ("RUTTNA nollställd")); });
    big (presetBtn, "", u8 ("256 presets, 32 per skola"), [this] { presetMenu(); });
    big (prevBtn, u8 ("◀"), u8 ("Förra preset"), [this] { stepPreset (-1); });
    big (nextBtn, u8 ("▶"), u8 ("Nästa preset"), [this] { stepPreset (1); });
    big (loopBtn, "LADDA LOOP", u8 ("Välj en loop att suga otakten ur"), [this] { chooseLoop(); });
    big (clearBtn, "RENSA", u8 ("Tillbaka till skolans egen otakt"), [this] { proc.clearParasite(); para.repaint(); setStatus (u8 ("Skolans egen otakt")); });
    genBtn.setColour (TextButton::buttonColourId, amberDk);
    genBtn.setColour (TextButton::textColourOffId, black);

    for (int l = 0; l < kLanes; ++l)
    {
        laneDice[l].onClick = [this, l] { proc.mutateLane (l); setStatus (u8 (laneName (l)) + u8 (": nytt mönster inom regeln")); };
        addAndMakeVisible (laneDice[l]);
        muteBtn[l].setButtonText ("M");
        muteBtn[l].setClickingTogglesState (true);
        muteBtn[l].setTooltip (u8 ("Tysta ") + u8 (laneName (l)));
        addAndMakeVisible (muteBtn[l]);
        muteAtt[l] = std::make_unique<BtnAtt> (proc.apvts, "mute" + String (l), muteBtn[l]);
        lvl[l].setSliderStyle (Slider::LinearBar);
        lvl[l].setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        lvl[l].setColour (Slider::trackColourId, amberDk);
        lvl[l].setColour (Slider::backgroundColourId, plate2);
        lvl[l].setTooltip (String::fromUTF8 (kLaneTips[l]));
        addAndMakeVisible (lvl[l]);
        lvlAtt[l] = std::make_unique<SldAtt> (proc.apvts, "lvl" + String (l), lvl[l]);
    }

    struct K { Knob* k; const char* id; const char* name; const char* tip; };
    for (auto& k : { K { &kMinus, "minus", "MINUS", "Tar bort slag. De svaga först, kicken aldrig." },
                     K { &kFrik, "friktion", "FRIKTION", "Varje spår släpar sina millisekunder bakom rutnätet." },
                     K { &kPara, "parasit", "PARASIT", "Hur mycket av otakten (skolans eller loopens) som smittar." },
                     K { &kSving, "svang", "SVÄNG", "Udda 16-delar släpar. Bara inom skolans ram." },
                     K { &kDub, "dub", "DUB", "Chans att ett slag kastas ner i ekot." },
                     K { &kRot, "ruttna", "RUTTNA", "Hur fort ljudet mörknar och smalnar (ungefär sju minuter till fullt)." },
                     K { &kTak, "tak", "TAK", "Lågpasset över allt." },
                     K { &kPump, "pump", "PUMP", "Allt utom kicken trycks ner av kicken." },
                     K { &kVar, "variation", "VARIATION", "Var fjärde takt vrids svaga steg, inom regelboken." },
                     K { &kUt, "ut", "UT", "Utnivå" } })
    {
        k.k->attach (proc.apvts, k.id, u8 (k.name));
        k.k->setAccent (amber);
        k.k->slider.setTooltip (u8 (k.tip));
        addAndMakeVisible (*k.k);
    }
    rootBox.addItemList (StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    rootBox.setTooltip ("Grundton för bas och stab");
    addAndMakeVisible (rootBox);
    rootAtt = std::make_unique<BoxAtt> (proc.apvts, "root", rootBox);

    addChildComponent (about);
    setSize (1200, 780);
    startTimerHz (30);
    setStatus (u8 ("Tryck play i DAW:en. GENETIK för nytt, MUTATION för att låta det växa. MIDI ut på kanal 1."));
}

TomrumEditor::~TomrumEditor() { setLookAndFeel (nullptr); }

void TomrumEditor::setSkola (int i)
{
    if (auto* p = proc.apvts.getParameter ("skola")) p->setValueNotifyingHost (p->convertTo0to1 ((float) i));
    setStatus (u8 (skolor()[i].name) + u8 (" · ") + u8 (skolor()[i].sub));
}

void TomrumEditor::timerCallback()
{
    const int v = proc.genomeVersion();
    if (v != lastVersionShown) { lastVersionShown = v; grid.setGenome (proc.currentGenome()); para.repaint(); }
    const int sk = proc.skolaIndex();
    for (int i = 0; i < kNumSkolor; ++i) skolaBtn[i].setToggleState (i == sk, dontSendNotification);
    const int prog = proc.getCurrentProgram();
    presetBtn.setButtonText (String (prog + 1) + "  " + proc.getProgramName (prog));
    undoBtn.setEnabled (proc.canUndo());
    for (auto& f : proc.laneFlash) f.store (f.load() * 0.82f);
    scope.repaint(); grid.repaint();
}

void TomrumEditor::presetMenu()
{
    PopupMenu m;
    const auto& ps = TomrumProcessor::presets();
    for (int s = 0; s < kNumSkolor; ++s)
    {
        PopupMenu sub;
        for (int i = 0; i < (int) ps.size(); ++i)
            if (ps[(size_t) i].skola == s) sub.addItem (i + 1, ps[(size_t) i].name, true, i == proc.getCurrentProgram());
        m.addSubMenu (u8 (skolor()[s].name), sub, true, nullptr, s == proc.skolaIndex());
    }
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (presetBtn), [this] (int r)
    {
        if (r > 0) { proc.setCurrentProgram (r - 1); setStatus (proc.getProgramName (r - 1)); }
    });
}

void TomrumEditor::stepPreset (int dir)
{
    const int n = proc.getNumPrograms();
    const int i = ((proc.getCurrentProgram() + dir) % n + n) % n;
    proc.setCurrentProgram (i);
    setStatus (proc.getProgramName (i));
}

void TomrumEditor::chooseLoop()
{
    chooser = std::make_unique<FileChooser> ("Välj en loop", File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
    {
        if (fc.getResult().existsAsFile()) filesDropped ({ fc.getResult().getFullPathName() }, 0, 0);
    });
}

void TomrumEditor::filesDropped (const StringArray& files, int, int)
{
    dragging = false;
    for (auto& f : files)
    {
        const File file (f);
        if (! file.existsAsFile()) continue;
        const String err = proc.loadParasite (file);
        setStatus (err.isEmpty() ? u8 ("PARASIT: otakten är uppsugen ur ") + file.getFileNameWithoutExtension() : err);
        break;
    }
    para.repaint(); repaint();
}

void TomrumEditor::mouseDown (const MouseEvent& e)
{
    if (logoArea.contains (e.getPosition())) { about.setBounds (getLocalBounds()); about.setVisible (true); about.toFront (false); }
}

void TomrumEditor::paint (Graphics& g)
{
    g.fillAll (black);
    // betongstruktur: svaga prickar
    Random r (4);
    g.setColour (Colours::white.withAlpha (0.018f));
    for (int i = 0; i < 2500; ++i) g.fillRect (r.nextInt (getWidth()), r.nextInt (getHeight()), 1, 1);

    // rubrik
    auto h = header;
    g.setColour (amber);
    g.setFont (FontOptions (34.0f, Font::bold).withHorizontalScale (1.1f));
    g.drawText ("TOMRUM", h.removeFromLeft (190), Justification::centredLeft);
    g.setColour (ash);
    g.setFont (FontOptions (11.0f));
    g.drawText (u8 ("DAS LEERRAUM  ·  sväng av det som inte spelas"), h.removeFromLeft (320), Justification::centredLeft);
    g.setColour (amber.withAlpha (0.8f));
    g.setFont (FontOptions (11.0f, Font::bold));
    g.drawText ("MASKINHALLEN", logoArea, Justification::centredRight);

    auto plateBox = [&] (Rectangle<int> a, const String& title, const String& sub = {})
    {
        g.setColour (plate); g.fillRoundedRectangle (a.toFloat(), 6.0f);
        g.setColour (edge); g.drawRoundedRectangle (a.toFloat().reduced (0.5f), 6.0f, 1.0f);
        // nitar i hörnen
        g.setColour (edge.brighter (0.3f));
        for (auto p : { a.getTopLeft(), a.getTopRight(), a.getBottomLeft(), a.getBottomRight() })
            g.fillEllipse ((float) p.x + (p.x == a.getX() ? 6 : -10), (float) p.y + (p.y == a.getY() ? 6 : -10), 4, 4);
        auto t = a.reduced (16, 8).removeFromTop (14);
        g.setColour (bone); g.setFont (FontOptions (11.5f, Font::bold));
        g.drawText (title, t, Justification::centredLeft);
        if (sub.isNotEmpty()) { g.setColour (ash); g.setFont (FontOptions (9.5f)); g.drawText (sub, t, Justification::centredRight); }
    };
    const Skola& s = skolor()[proc.skolaIndex()];
    plateBox (pSkola, "SKOLA", String (s.bpm) + " BPM");
    g.setColour (amber.withAlpha (0.9f)); g.setFont (FontOptions (11.0f));
    g.drawFittedText (u8 (s.sub), pSkola.reduced (16, 10).removeFromBottom (30), Justification::centredLeft, 2);
    plateBox (pMach, "GENETIK", u8 ("slumpen följer regelboken"));
    plateBox (pScope, "", {});
    plateBox (pGrid, u8 ("MÖNSTER"), u8 ("skuggad ruta = regeln tillåter slag · tjock kant = alltid"));
    plateBox (pKnobs, "KONTROLL", {});
    plateBox (pPara, "PARASIT", u8 ("släpp en loop"));
    if (dragging)
    {
        g.setColour (amber.withAlpha (0.15f)); g.fillRoundedRectangle (pPara.toFloat(), 6.0f);
        g.setColour (amber); g.drawRoundedRectangle (pPara.toFloat().reduced (1.0f), 6.0f, 2.0f);
    }

    g.setColour (ash); g.setFont (FontOptions (10.5f));
    g.drawText (status, statusArea, Justification::centredLeft);
}

void TomrumEditor::resized()
{
    auto r = getLocalBounds().reduced (14, 8);
    header = r.removeFromTop (48);
    {
        auto h = header;
        logoArea = h.removeFromRight (120);
        h.removeFromRight (10);
        nextBtn.setBounds (h.removeFromRight (34).reduced (0, 10));
        h.removeFromRight (4);
        presetBtn.setBounds (h.removeFromRight (240).reduced (0, 10));
        h.removeFromRight (4);
        prevBtn.setBounds (h.removeFromRight (34).reduced (0, 10));
        h.removeFromRight (10);
        undoBtn.setBounds (h.removeFromRight (80).reduced (0, 10));
    }
    statusArea = r.removeFromBottom (20);
    r.removeFromBottom (6);

    auto top = r.removeFromTop (190);
    pSkola = top.removeFromLeft (440);
    top.removeFromLeft (10);
    pScope = top.removeFromRight (380);
    top.removeFromRight (10);
    pMach = top;
    {
        auto a = pSkola.reduced (14, 10); a.removeFromTop (20); a.removeFromBottom (32);
        const int bw = a.getWidth() / 4, bh = a.getHeight() / 2;
        for (int i = 0; i < kNumSkolor; ++i)
            skolaBtn[i].setBounds (a.getX() + (i % 4) * bw + 2, a.getY() + (i / 4) * bh + 2, bw - 4, bh - 4);
        auto m = pMach.reduced (14, 10); m.removeFromTop (22);
        genBtn.setBounds (m.removeFromTop (58).reduced (2));
        auto row = m.removeFromTop (44);
        const int w3 = row.getWidth() / 3;
        mutBtn.setBounds (row.removeFromLeft (w3).reduced (2));
        soundBtn.setBounds (row.removeFromLeft (w3).reduced (2));
        rotBtn.setBounds (row.reduced (2));
        scope.setBounds (pScope.reduced (6));
    }
    r.removeFromTop (10);

    auto bottom = r.removeFromBottom (150);
    r.removeFromBottom (10);
    pGrid = r;
    {
        auto gArea = pGrid.reduced (8, 6); gArea.removeFromTop (22);
        grid.setBounds (gArea);
        for (int l = 0; l < kLanes; ++l)
        {
            const int y = gArea.getY() + grid.rowY (l), h = grid.rowH();
            const int x = gArea.getX() + 98;
            laneDice[l].setBounds (x, y + h / 2 - 11, 22, 22);
            muteBtn[l].setBounds (x + 26, y + h / 2 - 11, 24, 22);
            lvl[l].setBounds (x + 54, y + h / 2 - 5, 60, 10);
        }
    }
    pPara = bottom.removeFromRight (330);
    bottom.removeFromRight (10);
    pKnobs = bottom;
    {
        auto k = pKnobs.reduced (12, 8); k.removeFromTop (16);
        auto rootCol = k.removeFromRight (70);
        rootBox.setBounds (rootCol.withSizeKeepingCentre (64, 26));
        const int kw = k.getWidth() / 10;
        for (auto* kn : { &kMinus, &kFrik, &kPara, &kSving, &kDub, &kRot, &kTak, &kPump, &kVar, &kUt })
            kn->setBounds (k.removeFromLeft (kw).reduced (3, 0));
        auto p = pPara.reduced (12, 8); p.removeFromTop (20);
        auto btns = p.removeFromBottom (28);
        loopBtn.setBounds (btns.removeFromLeft (btns.getWidth() / 2).reduced (2));
        clearBtn.setBounds (btns.reduced (2));
        p.removeFromBottom (4);
        para.setBounds (p);
    }
    about.setBounds (getLocalBounds());
}
