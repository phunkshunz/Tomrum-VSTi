#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Onset.h"

using namespace tomrum;

TomrumProcessor::TomrumProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TOMRUM", createLayout())
{
    auto P = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    pMinus = P ("minus"); pSvang = P ("svang"); pFriktion = P ("friktion"); pParasit = P ("parasit");
    pDub = P ("dub"); pPump = P ("pump"); pTak = P ("tak"); pRuttna = P ("ruttna"); pVariation = P ("variation");
    pGrundton = P ("grundton"); pVolym = P ("volym"); pSpela = P ("spela"); pTempo = P ("tempo");
    for (int l = 0; l < kLanes; ++l) { pNiva[(size_t) l] = P (lid (l, "niva")); pTyst[(size_t) l] = P (lid (l, "tyst")); }

    formats.registerBasicFormats();
    pending.reserve (256);

    // Startläge: det låter direkt.
    loadPreset (0);
    undoStack.clear();

    // Som fristående app finns ingen DAW: då startar TOMRUM med egen klocka.
    if (wrapperType == wrapperType_Standalone) setParam ("spela", 1.0f);
}

bool TomrumProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void TomrumProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    engine.prepare (sr);
    genomeSeen = 0; paraSeen = 0;   // läs in genom och parasit igen
    scratch.setSize (2, juce::jmax (512, samplesPerBlock));
    pending.clear();
    wasRunning = false;
    expectedPpq = -1.0e9;
}

// ---------------------------------------------------------------- ljudtråden
void TomrumProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    midi.clear();   // TOMRUM spelar själv; inkommande noter används inte
    if (n <= 0) return;

    {
        Genome g;
        if (genomeBox.read (g, genomeSeen)) engine.setGenome (g);
        Parasite p;
        if (paraBox.read (p, paraSeen)) engine.setParasite (p);
    }

    // Tid: följ DAW:en när den spelar, annars den egna klockan (SPELA).
    double bpm = (double) pTempo->load();
    double ppq = 0.0;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying())
                if (auto p = pos->getPpqPosition()) { hostPlaying = true; ppq = *p; }
        }
    bpm = juce::jlimit (40.0, 300.0, bpm);

    const bool spela = pSpela->load() > 0.5f;
    if (! hostPlaying)
    {
        if (spela && ! internalWasOn) { internalPpq = 0.0; engine.resetRot(); }
        ppq = internalPpq;
    }
    internalWasOn = spela;
    const bool running = hostPlaying || spela;
    const double end = ppq + n * bpm / 60.0 / sr;

    if (! running || ! wasRunning || std::abs (ppq - expectedPpq) > 1.0e-3)
    {
        for (auto& p : pending) midi.addEvent (juce::MidiMessage::noteOff (1, p.note), 0);
        pending.clear();
    }

    Engine::Controls c;
    c.minus = pMinus->load();
    c.swing = pSvang->load();
    c.friction = pFriktion->load();
    c.parasite = pParasit->load();
    c.dub = pDub->load();
    c.pump = pPump->load();
    c.ceiling = pTak->load();
    c.rot = pRuttna->load();
    c.variation = pVariation->load();
    c.root = juce::roundToInt (pGrundton->load());
    c.out = pVolym->load();
    for (int l = 0; l < kLanes; ++l)
    {
        c.level[l] = pNiva[(size_t) l]->load();
        c.mute[l] = pTyst[(size_t) l]->load() > 0.5f;
    }

    if (scratch.getNumSamples() < n) scratch.setSize (2, n, false, false, true);
    float* L = buffer.getNumChannels() > 0 ? buffer.getWritePointer (0) : scratch.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : scratch.getWritePointer (1);
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);

    const int ne = engine.process (L, R, n, running, ppq, bpm, c, events.data(), (int) events.size());

    // MIDI ut: varje slag som en not på kanal 1, så att TOMRUM kan styra andra ljud också.
    const int64_t gate = (int64_t) (0.06 * sr);
    for (int i = 0; i < ne; ++i)
    {
        const auto& e = events[(size_t) i];
        const int s = juce::jlimit (0, n - 1, e.offset);
        const int note = juce::jlimit (0, 127, e.note);
        for (size_t k = 0; k < pending.size(); ++k)
            if (pending[k].note == note)
            {
                midi.addEvent (juce::MidiMessage::noteOff (1, note), s);
                pending.erase (pending.begin() + (long) k);
                break;
            }
        midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (e.vel * 127.0f))), s);
        if (pending.size() < 200) pending.push_back ({ note, sampleClock + s + gate });
        if (e.lane >= 0 && e.lane < kLanes) uiHits[(size_t) e.lane].fetch_add (1, std::memory_order_relaxed);
    }
    for (size_t k = 0; k < pending.size();)
    {
        if (pending[k].at < sampleClock + n)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, pending[k].note), (int) juce::jlimit<int64_t> (0, n - 1, pending[k].at - sampleClock));
            pending.erase (pending.begin() + (long) k);
        }
        else ++k;
    }

    if (buffer.getNumChannels() == 1)
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + R[i]);

    sampleClock += n;
    expectedPpq = end;
    wasRunning = running;
    if (! hostPlaying && spela) internalPpq = end;

    uiPpq.store (ppq);
    uiBpm.store (bpm);
    uiPlaying.store (running);
    uiVoid.store (engine.voidEnv);
    uiRot.store (engine.rotShown);
}

// ---------------------------------------------------------------- genetik och presets
void TomrumProcessor::setParam (const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

int TomrumProcessor::skolaParam() const
{
    return juce::jlimit (0, kNumSkolor - 1, juce::roundToInt (apvts.getRawParameterValue ("skola")->load()));
}

uint32_t TomrumProcessor::frozenMask() const
{
    uint32_t m = 0;
    for (int l = 0; l < kLanes; ++l)
        if (apvts.getRawParameterValue (lid (l, "frys"))->load() > 0.5f) m |= 1u << l;
    return m;
}

Parasite TomrumProcessor::currentParasite() const
{
    if (customParasite) return customPara;
    Parasite p;
    p.fromSchool (skolor()[juce::jlimit (0, kNumSkolor - 1, current.skola)]);
    return p;
}

// Lägger ut ett nytt genom till ljudtråden. setKnobs: rattarna hoppar till genomets läge.
void TomrumProcessor::publish (const Genome& g, bool setKnobs)
{
    current = g;
    genomeBox.write (g);
    paraBox.write (currentParasite());
    if (setKnobs)
    {
        const Skola& s = skolor()[juce::jlimit (0, kNumSkolor - 1, g.skola)];
        setParam ("skola", (float) g.skola);
        setParam ("minus", g.snd.minus);
        setParam ("svang", rangePos (s.swing, g.snd.swing));
        setParam ("dub", g.snd.dub);
        setParam ("pump", g.snd.pump);
        setParam ("tak", 1.0f);
    }
    genomeVersion.fetch_add (1);
}

static uint32_t freshSeed() { return (uint32_t) juce::Random::getSystemRandom().nextInt() | 1u; }

void TomrumProcessor::chooseSkola (int skola)
{
    setParam ("skola", (float) juce::jlimit (0, kNumSkolor - 1, skola));
    genetik();
}

void TomrumProcessor::genetik()
{
    pushUndo();
    const int skola = skolaParam();
    const uint32_t frozen = frozenMask();
    Genome g = mutate (current, skola, freshSeed(), ~frozen & 0xFFu, true);
    for (int l = 0; l < kLanes; ++l)
        if ((frozen >> l) & 1u) g.lane[l] = current.lane[l];
    publish (g, true);
    currentPreset.store (-1);
}

void TomrumProcessor::genetikLane (int lane)
{
    if (lane < 0 || lane >= kLanes) return;
    pushUndo();
    Genome g = mutate (current, current.skola, freshSeed(), 1u << lane, false);
    g.seed = current.seed;   // resten av groovet rör sig exakt som förut
    publish (g, false);
    currentPreset.store (-1);
}

void TomrumProcessor::genetikLjud()
{
    pushUndo();
    Genome g = mutate (current, current.skola, freshSeed(), 0u, true);
    g.seed = current.seed;
    publish (g, true);
    currentPreset.store (-1);
}

void TomrumProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, kNumPresets - 1, index);
    pushUndo();
    publish (presetGenome (index, current, frozenMask()), true);
    currentPreset.store (index);
}

juce::String TomrumProcessor::presetLabel() const
{
    const int p = currentPreset.load();
    return p >= 0 ? juce::String::fromUTF8 (presetName (p).c_str()) : juce::String::fromUTF8 ("EGET GENOM");
}

// ---------------------------------------------------------------- PARASIT
juce::String TomrumProcessor::loadParasiteLoop (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr) return juce::String::fromUTF8 ("Kan inte läsa filen. Prova WAV eller AIFF.");
    const double fs = reader->sampleRate > 0 ? reader->sampleRate : 44100.0;
    const int len = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (fs * 32.0));
    if (len < (int) (fs * 0.5)) return juce::String::fromUTF8 ("Loopen är för kort.");

    juce::AudioBuffer<float> buf ((int) juce::jmax (1u, reader->numChannels), len);
    reader->read (&buf, 0, len, 0, true, true);
    std::vector<float> mono ((size_t) len, 0.0f);
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* d = buf.getReadPointer (ch);
        for (int i = 0; i < len; ++i) mono[(size_t) i] += d[i] / (float) buf.getNumChannels();
    }

    const auto onsets = detectOnsets (mono.data(), len, fs);
    const int bars = loopBars (len, fs, uiBpm.load());
    Parasite p;
    if (! extractParasite (onsets, len, bars * 16, p))
        return juce::String::fromUTF8 ("Hittade för få slag i loopen (") + juce::String ((int) onsets.size()) + ").";

    pushUndo();
    customPara = p;
    customParasite = true;
    parasiteFrom = file.getFileNameWithoutExtension();
    paraBox.write (customPara);
    genomeVersion.fetch_add (1);
    return juce::String::fromUTF8 ("Otakten sögs ur ") + parasiteFrom + " (" + juce::String ((int) onsets.size()) + " slag, "
           + juce::String (bars) + (bars == 1 ? " takt)" : " takter)");
}

void TomrumProcessor::forgetParasiteLoop()
{
    pushUndo();
    customParasite = false;
    parasiteFrom = {};
    paraBox.write (currentParasite());
    genomeVersion.fetch_add (1);
}

// ---------------------------------------------------------------- ångra och sparat läge
juce::ValueTree TomrumProcessor::snapshot()
{
    auto tree = apvts.copyState();
    juce::ValueTree g ("GENOM");
    g.setProperty ("data", juce::String (genomeToString (current)), nullptr);
    g.setProperty ("preset", currentPreset.load(), nullptr);
    if (customParasite)
    {
        juce::StringArray v;
        for (float x : customPara.off) v.add (juce::String (x, 4));
        g.setProperty ("parasit", v.joinIntoString (","), nullptr);
        g.setProperty ("parasitNamn", parasiteFrom, nullptr);
    }
    tree.removeChild (tree.getChildWithName ("GENOM"), nullptr);
    tree.appendChild (g, nullptr);
    return tree;
}

void TomrumProcessor::restore (const juce::ValueTree& tree)
{
    if (! tree.isValid()) return;
    auto g = tree.getChildWithName ("GENOM");
    auto copy = tree.createCopy();
    copy.removeChild (copy.getChildWithName ("GENOM"), nullptr);
    apvts.replaceState (copy);
    if (! g.isValid()) return;

    customParasite = false;
    parasiteFrom = {};
    const auto ps = g.getProperty ("parasit").toString();
    if (ps.isNotEmpty())
    {
        juce::StringArray v;
        v.addTokens (ps, ",", {});
        if (v.size() == 16)
        {
            for (int i = 0; i < 16; ++i) customPara.off[i] = juce::jlimit (-0.15f, 0.4f, v[i].getFloatValue());
            customParasite = true;
            parasiteFrom = g.getProperty ("parasitNamn").toString();
        }
    }
    Genome gen;
    if (genomeFromString (g.getProperty ("data").toString().toStdString(), gen)) publish (gen, false);
    else publish (presetGenome (0, current, 0), false);
    currentPreset.store ((int) g.getProperty ("preset", -1));
}

void TomrumProcessor::pushUndo()
{
    undoStack.push_back (snapshot());
    if (undoStack.size() > 40) undoStack.erase (undoStack.begin());
}

bool TomrumProcessor::undo()
{
    if (undoStack.empty()) return false;
    auto t = undoStack.back();
    undoStack.pop_back();
    restore (t);
    return true;
}

void TomrumProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = snapshot().createXml()) copyXmlToBinary (*xml, destData);
}

void TomrumProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            restore (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* TomrumProcessor::createEditor() { return new TomrumEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TomrumProcessor(); }
