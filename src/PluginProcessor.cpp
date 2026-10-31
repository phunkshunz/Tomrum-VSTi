#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace tomrum;

namespace
{
const StringArray kRoots { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
String u8 (const char* s) { return String::fromUTF8 (s); }

String lvlId (int l) { return "lvl" + String (l); }
String muteId (int l) { return "mute" + String (l); }

// 0..1 i skolans ram -> knappens läge
float inRange (const tomrum::Range& r, float v) { return r.hi > r.lo ? jlimit (0.0f, 1.0f, (v - r.lo) / (r.hi - r.lo)) : 0.5f; }
} // namespace

AudioProcessorValueTreeState::ParameterLayout TomrumProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + " %"; });
    auto f = [&] (const String& id, const String& name, float def) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.f, 1.f), def, pct)); };

    {
        StringArray names; for (int i = 0; i < kNumSkolor; ++i) names.add (u8 (skolor()[i].name));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { "skola", 1 }, "Skola", names, 0));
    }
    f ("minus", "Minus", 0.2f);
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "friktion", 1 }, "Friktion", NormalisableRange<float> (0.f, 2.f), 1.0f, pct));
    f ("parasit", "Parasit", 0.6f);
    f ("svang", u8 ("Sväng"), 0.5f);
    f ("dub", "Dub", 0.3f);
    f ("ruttna", "Ruttna", 0.5f);
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tak", 1 }, "Tak", NormalisableRange<float> (0.f, 1.f), 0.7f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { const float m = std::pow (2.0f, (v - 0.7f) * 3.0f); return "x" + String (m, 2); })));
    f ("pump", "Pump", 0.3f);
    f ("variation", "Variation", 0.4f);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "root", 1 }, "Grundton", kRoots, 9));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ut", 1 }, "Ut", NormalisableRange<float> (-24.f, 6.f, 0.1f), 0.f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));
    for (int k = 0; k < kLanes; ++k)
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { lvlId (k), 1 }, u8 (laneName (k)), NormalisableRange<float> (0.f, 1.f), 0.8f, pct));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { muteId (k), 1 }, u8 (laneName (k)) + " tyst", false));
    }
    return l;
}

TomrumProcessor::TomrumProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TOMRUM", createLayout())
{
    for (auto& s : laneSeed) s.store (1);
    applyPreset (presets()[0]);
    undoStack.clear();
}

bool TomrumProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto o = layouts.getMainOutputChannelSet();
    return o == AudioChannelSet::stereo() || o == AudioChannelSet::mono();
}

void TomrumProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    engine.prepare (sr);
    seenVersion = 0; seenSkola = -1; seenPara = 0; numOffs = 0;
}

int TomrumProcessor::skolaIndex() const
{
    return jlimit (0, kNumSkolor - 1, roundToInt (apvts.getRawParameterValue ("skola")->load()));
}

Genome TomrumProcessor::currentGenome() const
{
    uint32_t ls[kLanes];
    for (int l = 0; l < kLanes; ++l) ls[l] = laneSeed[(size_t) l].load();
    return buildGenome (skolaIndex(), ls, soundSeed.load());
}

Parasite TomrumProcessor::currentParasite() const
{
    Parasite p;
    if (paraOwnFlag.load()) for (int i = 0; i < 16; ++i) p.off[i] = paraOff[(size_t) i].load();
    else p.fromSchool (skolor()[skolaIndex()]);
    return p;
}

void TomrumProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    midi.clear();
    if (n <= 0) return;

    bool playing = false; double ppq = 0.0, bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q; else playing = false;
        }
    bpm = jlimit (40.0, 300.0, bpm);
    hostBpm.store (bpm);

    // genomet räknas fram här ur fröna: ingen låsning, ingen allokering
    const int sk = skolaIndex();
    const int ver = genVersion.load();
    if (ver != seenVersion || sk != seenSkola)
    {
        uint32_t ls[kLanes];
        for (int l = 0; l < kLanes; ++l) ls[l] = laneSeed[(size_t) l].load();
        engine.setGenome (buildGenome (sk, ls, soundSeed.load()));
        seenVersion = ver;
        if (sk != seenSkola) seenPara = 0;
        seenSkola = sk;
    }
    if (paraVersion.load() != seenPara)
    {
        seenPara = paraVersion.load();
        engine.setParasite (currentParasite());
    }
    if (rotReset.exchange (false)) engine.resetRot();

    auto v = [this] (const String& id) { return apvts.getRawParameterValue (id)->load(); };
    Engine::Controls c;
    c.minus = v ("minus"); c.friction = v ("friktion"); c.parasite = v ("parasit"); c.swing = v ("svang");
    c.dub = v ("dub"); c.rot = v ("ruttna"); c.pump = v ("pump"); c.variation = v ("variation");
    c.ceiling = std::pow (2.0f, (v ("tak") - 0.7f) * 3.0f);
    c.root = roundToInt (v ("root"));
    c.out = Decibels::decibelsToGain (v ("ut"));
    for (int l = 0; l < kLanes; ++l) { c.level[l] = v (lvlId (l)) * 1.25f; c.mute[l] = v (muteId (l)) > 0.5f; }

    // MIDI ut: släpp gamla noter
    for (int i = 0; i < numOffs;)
    {
        auto& o = offs[(size_t) i];
        if (o.samplesLeft < n || ! playing)
        {
            midi.addEvent (MidiMessage::noteOff (1, o.note), playing ? jlimit (0, n - 1, o.samplesLeft) : 0);
            offs[(size_t) i] = offs[(size_t) --numOffs];
        }
        else { o.samplesLeft -= n; ++i; }
    }

    Event ev[Engine::kMaxEvents];
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    int nev = 0;
    if (R != nullptr) nev = engine.process (L, R, n, playing, ppq, bpm, c, ev, Engine::kMaxEvents);
    else
    {
        HeapBlock<float> tmp ((size_t) n);
        nev = engine.process (L, tmp.get(), n, playing, ppq, bpm, c, ev, Engine::kMaxEvents);
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + tmp[i]);
    }

    const int gate = (int) (sr * 0.04);
    for (int i = 0; i < nev; ++i)
    {
        const auto& e = ev[i];
        midi.addEvent (MidiMessage::noteOn (1, e.note, jlimit (0.05f, 1.0f, e.vel)), e.offset);
        laneFlash[(size_t) e.lane].store (e.vel);
        if (numOffs < (int) offs.size()) offs[(size_t) numOffs++] = { e.note, e.offset + gate };
        else midi.addEvent (MidiMessage::noteOff (1, e.note), jmin (n - 1, e.offset + 1));
    }

    running.store (playing);
    playPpq.store (ppq);
    voidLevel.store (engine.voidEnv);
    rotLevel.store (engine.rotShown);
    int w = scopeWrite.load();
    for (int i = 0; i < n; i += 2) { scope[(size_t) w] = L[i]; w = (w + 1) % (int) scope.size(); }
    scopeWrite.store (w);
}

// ---------------------------------------------------------------- GENETIK och MUTATION
void TomrumProcessor::setParam (const char* id, float plain)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
}

void TomrumProcessor::pushUndo()
{
    ValueTree t = apvts.copyState();
    for (int l = 0; l < kLanes; ++l) t.setProperty ("seed" + String (l), (int64) laneSeed[(size_t) l].load(), nullptr);
    t.setProperty ("sseed", (int64) soundSeed.load(), nullptr);
    undoStack.push_back (t);
    while (undoStack.size() > 40) undoStack.pop_front();
}

void TomrumProcessor::undo()
{
    if (undoStack.empty()) return;
    auto t = undoStack.back(); undoStack.pop_back();
    for (int l = 0; l < kLanes; ++l) laneSeed[(size_t) l].store ((uint32_t) (int64) t.getProperty ("seed" + String (l), 1));
    soundSeed.store ((uint32_t) (int64) t.getProperty ("sseed", 1));
    apvts.replaceState (t.createCopy());
    bump();
}

// Ljudets ramar följer med till knapparna, så att man ser vad GENETIK valde
void TomrumProcessor::knobsFromGenome (const Genome& g)
{
    const Skola& s = skolor()[g.skola];
    setParam ("minus", g.snd.minus);
    setParam ("svang", inRange (s.swing, g.snd.swing));
    setParam ("dub", jlimit (0.0f, 1.0f, g.snd.dub));
    setParam ("pump", jlimit (0.0f, 1.0f, g.snd.pump));
}

void TomrumProcessor::genetik()
{
    pushUndo();
    for (auto& s : laneSeed) s.store (freshSeed());
    soundSeed.store (freshSeed());
    knobsFromGenome (currentGenome());
    engine.resetRot(); rotReset.store (true);
    bump();
}

void TomrumProcessor::mutation()
{
    pushUndo();
    // två eller tre spår, aldrig kicken
    const int count = 2 + rng.nextInt (2);
    for (int i = 0; i < count; ++i)
        laneSeed[(size_t) (1 + rng.nextInt (kLanes - 1))].store (freshSeed());
    bump();
}

void TomrumProcessor::mutateLane (int lane)
{
    if (lane < 0 || lane >= kLanes) return;
    pushUndo();
    laneSeed[(size_t) lane].store (freshSeed());
    bump();
}

void TomrumProcessor::mutateSound()
{
    pushUndo();
    soundSeed.store (freshSeed());
    knobsFromGenome (currentGenome());
    bump();
}

// ---------------------------------------------------------------- PARASIT ur loop
String TomrumProcessor::loadParasite (const File& f)
{
    AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr) return u8 ("Kunde inte läsa filen");
    const double fsr = r->sampleRate > 0 ? r->sampleRate : 44100.0;
    const int len = (int) jmin<int64> (r->lengthInSamples, (int64) (fsr * 60));
    if (len < fsr * 0.5) return u8 ("Loopen är för kort");
    AudioBuffer<float> b ((int) jmax (1u, r->numChannels), len);
    r->read (&b, 0, len, 0, true, true);

    // energi per hopp, positiv förändring = anslag
    const int hop = 256;
    std::vector<float> env;
    for (int i = 0; i + hop <= len; i += hop)
    {
        float e = 0;
        for (int ch = 0; ch < b.getNumChannels(); ++ch) { auto* d = b.getReadPointer (ch, i); for (int k = 0; k < hop; ++k) e += d[k] * d[k]; }
        env.push_back (std::sqrt (e / hop));
    }
    std::vector<float> flux (env.size(), 0.0f);
    for (size_t i = 1; i < env.size(); ++i) flux[i] = jmax (0.0f, env[i] - env[i - 1]);
    std::vector<int> onsets;
    const int win = 16, minGap = (int) (0.06 * fsr / hop);
    int last = -1000;
    for (size_t i = 1; i + 1 < flux.size(); ++i)
    {
        float mean = 0; int cnt = 0;
        for (int k = (int) i - win; k <= (int) i + win; ++k) if (k >= 0 && k < (int) flux.size()) { mean += flux[(size_t) k]; ++cnt; }
        mean /= (float) jmax (1, cnt);
        if (flux[i] > mean * 1.8f + 1.0e-4f && flux[i] >= flux[i - 1] && flux[i] >= flux[i + 1] && (int) i - last >= minGap)
        {
            onsets.push_back ((int) i * hop); last = (int) i;
        }
    }

    // loopens längd i taktslag: närmaste av 4, 8, 16, 32, 64 vid värdens tempo
    const double beatsRaw = len / fsr * hostBpm.load() / 60.0;
    int beats = 4; double bestErr = 1e9;
    for (int bb : { 4, 8, 16, 32, 64 }) { const double e = std::abs (std::log (beatsRaw / bb)); if (e < bestErr) { bestErr = e; beats = bb; } }

    Parasite p;
    if (! extractParasite (onsets, len, beats * 4, p)) return u8 ("Hittade för få slag i loopen");
    for (int i = 0; i < 16; ++i) paraOff[(size_t) i].store (p.off[i]);
    paraOwnFlag.store (true);
    ownParasite = true;
    parasiteFile = f.getFileNameWithoutExtension();
    paraVersion.fetch_add (1);
    return {};
}

void TomrumProcessor::clearParasite()
{
    paraOwnFlag.store (false);
    ownParasite = false;
    parasiteFile.clear();
    paraVersion.fetch_add (1);
}

// ---------------------------------------------------------------- PRESETS
// 32 per skola: egna namn, eget frö, egna knappar. Alla inom skolans regler.
const std::vector<TomrumPreset>& TomrumProcessor::presets()
{
    static const std::vector<TomrumPreset> all = []
    {
        static const char* adj[16] = { "TYSTA", "KALLA", "BLÖTA", "SÄNKTA", "TOMMA", "DOVA", "SENA", "SKEVA",
                                       "NAKNA", "GLÖMDA", "RUTTNA", "DOLDA", "MÖRKA", "SNEA", "LÅNGSAMMA", "TRÖTTA" };
        static const char* noun[kNumSkolor][8] = {
            { "DJUPET", "SLUSSEN", "KAJEN", "MUDDRET", "VRAKET", "STRÖMMEN", "DIMMAN", "HAMNEN" },
            { "NÅLEN", "SKRUVEN", "URVERKET", "FJÄDERN", "SPRINTEN", "GRUSET", "SPÅNET", "LUPPEN" },
            { "GOLVET", "LÖVEN", "NATTEN", "FUKTEN", "SVETTEN", "KÄLLAREN", "TRAPPAN", "MORGONEN" },
            { "TRÅDEN", "SPOLEN", "GNISTAN", "KRETSEN", "RELÄET", "LÖDET", "STÖRNINGEN", "KORTSLUTNINGEN" },
            { "TORNET", "ÅNGAN", "KONDENSEN", "FLÄKTEN", "ISEN", "RÖRET", "TRYCKET", "FROSTEN" },
            { "KORRIDOREN", "LAMPAN", "SKALPELLEN", "SALEN", "KURVAN", "PULSEN", "GLASET", "RONDEN" },
            { "BANDET", "HUVUDET", "SUSET", "KNASTRET", "SPOLNINGEN", "ETIKETTEN", "MINNET", "KOPIAN" },
            { "KLOSSEN", "BARKEN", "STUBBEN", "VEDEN", "KVISTEN", "SÅGEN", "LADAN", "STOCKEN" } };
        static const int roots[6] = { 9, 2, 7, 0, 5, 4 };

        std::vector<TomrumPreset> v;
        for (int s = 0; s < kNumSkolor; ++s)
            for (int i = 0; i < 32; ++i)
            {
                TomrumPreset p;
                const int nn = i % 8, aa = (i / 8) * 4 + (nn + s) % 4;
                p.name = u8 (adj[aa]) + " " + u8 (noun[s][nn]);
                p.skola = s;
                p.seed = hash3 ((uint32_t) s, (uint32_t) i, 0x70u) | 1u;
                auto h = [&] (uint32_t k) { return h01 ((uint32_t) s, (uint32_t) i, k); };
                const bool calm = i == 0;
                p.values = {
                    { "friktion", calm ? 1.0f : 0.6f + 0.8f * h (1) },
                    { "parasit", calm ? 0.6f : 0.3f + 0.7f * h (2) },
                    { "ruttna", calm ? 0.4f : 0.15f + 0.75f * h (3) },
                    { "tak", calm ? 0.7f : 0.58f + 0.2f * h (4) },
                    { "variation", calm ? 0.35f : 0.15f + 0.6f * h (5) },
                    { "root", (float) roots[(int) (h (6) * 6) % 6] } };
                v.push_back (std::move (p));
            }
        return v;
    }();
    return all;
}

void TomrumProcessor::applyPreset (const TomrumPreset& p)
{
    setParam ("skola", (float) p.skola);
    for (int l = 0; l < kLanes; ++l) laneSeed[(size_t) l].store (p.seed);
    soundSeed.store (p.seed);
    const Genome g = buildGenome (p.skola, std::array<uint32_t, kLanes> { p.seed, p.seed, p.seed, p.seed, p.seed, p.seed, p.seed, p.seed }.data(), p.seed);
    knobsFromGenome (g);
    for (auto& kv : p.values) setParam (kv.first, kv.second);
    for (int l = 0; l < kLanes; ++l) { setParam (lvlId (l).toRawUTF8(), 0.8f); setParam (muteId (l).toRawUTF8(), 0.0f); }
    rotReset.store (true);
    bump();
}

void TomrumProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= (int) presets().size()) return;
    if (! undoStack.empty() || index != currentProgram) pushUndo();
    currentProgram = index;
    applyPreset (presets()[(size_t) index]);
}

const String TomrumProcessor::getProgramName (int index)
{
    if (index < 0 || index >= (int) presets().size()) return {};
    return presets()[(size_t) index].name;
}

// ---------------------------------------------------------------- spara / läsa
void TomrumProcessor::getStateInformation (MemoryBlock& dest)
{
    ValueTree t = apvts.copyState();
    for (int l = 0; l < kLanes; ++l) t.setProperty ("seed" + String (l), (int64) laneSeed[(size_t) l].load(), nullptr);
    t.setProperty ("sseed", (int64) soundSeed.load(), nullptr);
    t.setProperty ("program", currentProgram, nullptr);
    t.setProperty ("paraOwn", paraOwnFlag.load(), nullptr);
    t.setProperty ("paraName", parasiteFile, nullptr);
    String ps; for (int i = 0; i < 16; ++i) ps << String (paraOff[(size_t) i].load(), 4) << (i < 15 ? "," : "");
    t.setProperty ("para", ps, nullptr);
    if (auto xml = t.createXml()) copyXmlToBinary (*xml, dest);
}

void TomrumProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto t = ValueTree::fromXml (*xml);
    for (int l = 0; l < kLanes; ++l) laneSeed[(size_t) l].store ((uint32_t) (int64) t.getProperty ("seed" + String (l), 1));
    soundSeed.store ((uint32_t) (int64) t.getProperty ("sseed", 1));
    currentProgram = t.getProperty ("program", 0);
    auto parts = StringArray::fromTokens (t.getProperty ("para").toString(), ",", "");
    for (int i = 0; i < 16; ++i) paraOff[(size_t) i].store (i < parts.size() ? parts[i].getFloatValue() : 0.0f);
    ownParasite = (bool) t.getProperty ("paraOwn", false);
    paraOwnFlag.store (ownParasite);
    parasiteFile = t.getProperty ("paraName").toString();
    apvts.replaceState (t);
    paraVersion.fetch_add (1);
    bump();
}

AudioProcessorEditor* TomrumProcessor::createEditor() { return new TomrumEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TomrumProcessor(); }
