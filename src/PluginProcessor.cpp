#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Lagen.h"

using namespace juce;
using namespace tr2;

namespace
{
const StringArray kRoots { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
String u8 (const char* s) { return String::fromUTF8 (s); }
String lvlId (int l)  { return "lvl" + String (l); }
String muteId (int l) { return "mute" + String (l); }
String densId (int l) { return "dens" + String (l); }
String lasId (int l)  { return "las" + String (l); }
const char* laneNameOf (int l) { return tomrum::laneName (l); }
// MIDI ut: trummor på kanal 1 (GM-noter), basen på kanal 2, staben på kanal 3
int midiChannelOf (int lane) { return lane == Bas ? 2 : (lane == Stab ? 3 : 1); }
} // namespace

AudioProcessorValueTreeState::ParameterLayout TomrumProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + " %"; });
    auto f = [&] (const String& id, const String& name, float def) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.f, 1.f), def, pct)); };
    auto choice = [&] (const String& id, const String& name, const StringArray& items, int def) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def)); };

    { StringArray n; for (int i = 0; i < tomrum::kNumSkolor; ++i) n.add (u8 (tomrum::skolor()[i].name)); choice ("skola", "Skola", n, 0); }
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mapx", 1 }, "Karta X", NormalisableRange<float> (0.f, 1.f), 0.5f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v < 0.33f ? u8 ("KÖLN") : (v < 0.67f ? String ("BERLIN") : String ("BUKAREST")); })));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mapy", 1 }, "Karta Y", NormalisableRange<float> (0.f, 1.f), 0.3f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v < 0.33f ? String ("RAK") : (v < 0.67f ? String ("RULL") : String ("BRUTEN")); })));
    f ("tomrum", "Tomrum", 0.35f);
    f ("skolfarg", u8 ("Skolfärg"), 0.4f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "poly", 1 }, "Polyrytm", true));
    { StringArray n; for (int i = 0; i < kNumGung; ++i) n.add (u8 (gungName (i))); choice ("gungtyp", "Gung-typ", n, GMpc); }
    f ("gung", "Gung", 0.5f);
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "friktion", 1 }, "Friktion", NormalisableRange<float> (0.f, 2.f), 1.0f, pct));
    f ("variation", "Variation", 0.4f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lat", 1 }, u8 ("Låten"), false));
    choice ("latlen", u8 ("Låtens längd"), StringArray { "64 TAKTER", "128 TAKTER" }, 0);
    choice ("root", "Grundton", kRoots, 9);
    { StringArray n; for (int i = 0; i < kNumSkalor; ++i) n.add (u8 (skalaName (i))); choice ("skala", "Skala", n, SMoll); }
    f ("riff", "Riff", 0.3f);
    f ("djup", "Djup", 0.4f);
    f ("kallare", u8 ("Källare"), 0.3f);
    f ("kassett", "Kassett", 0.0f);
    f ("ruttna", "Ruttna", 0.0f);
    f ("pump", "Pump", 0.3f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "midiut", 1 }, "MIDI ut", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ut", 1 }, "Ut", NormalisableRange<float> (-24.f, 6.f, 0.1f), 0.f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));
    for (int k = 0; k < kLanes; ++k)
    {
        const String nm = u8 (laneNameOf (k));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { lvlId (k), 1 }, nm, NormalisableRange<float> (0.f, 1.f), 0.8f, pct));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { densId (k), 1 }, nm + u8 (" täthet"), NormalisableRange<float> (0.f, 1.f), 0.5f, pct));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { muteId (k), 1 }, nm + " tyst", false));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { lasId (k), 1 }, nm + u8 (" låst"), false));
    }
    return l;
}

TomrumProcessor::TomrumProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TOMRUM", createLayout())
{
    for (auto& s : laneSeed) s.store (1);
    for (auto& v : variantFull) v.store (false);
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
    numOffs = 0; lastBar = -1;
}

Tr2Controls TomrumProcessor::currentControls() const
{
    Tr2Controls c;
    c.skola = jlimit (0, tomrum::kNumSkolor - 1, roundToInt (param ("skola")));
    c.seed = soundSeed.load();
    for (int l = 0; l < kLanes; ++l) c.laneSeed[l] = laneSeed[(size_t) l].load();
    c.mapX = param ("mapx"); c.mapY = param ("mapy"); c.tomrum = param ("tomrum"); c.skolfarg = param ("skolfarg");
    c.poly = param ("poly") > 0.5f ? 1.0f : 0.0f;
    c.gungTyp = roundToInt (param ("gungtyp")); c.gung = param ("gung"); c.friktion = param ("friktion");
    c.variation = param ("variation");
    c.latOn = param ("lat") > 0.5f; c.latBars = roundToInt (param ("latlen")) == 1 ? 128 : 64;
    c.root = roundToInt (param ("root")); c.skala = roundToInt (param ("skala")); c.riff = param ("riff");
    c.djup = param ("djup"); c.kallare = param ("kallare"); c.kassett = param ("kassett"); c.rota = param ("ruttna");
    c.pumpAmt = param ("pump");
    c.out = Decibels::decibelsToGain (param ("ut"));
    for (int l = 0; l < kLanes; ++l)
    {
        c.level[l] = param (lvlId (l)) * 1.25f;
        c.laneDens[l] = param (densId (l));
        c.mute[l] = param (muteId (l)) > 0.5f;
    }
    return c;
}

void TomrumProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) { midi.clear(); return; }

    bool playing = false; double ppq = 0.0, bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q; else playing = false;
        }
    bpm = jlimit (40.0, 300.0, bpm);

    // MIDI in: C3..G3 väljer variant (byts vid nästa takt). Allt annat ignoreras.
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn())
        {
            const int slot = msg.getNoteNumber() - kVariantNote;
            if (slot >= 0 && slot < kVarianter && variantFull[(size_t) slot].load()) requestedVariant.store (slot);
        }
    }
    midi.clear();

    const long bar = (long) std::floor (ppq / 4.0);
    const int req = requestedVariant.load();
    if (req >= 0 && (! playing || bar != lastBar))
    {
        requestedVariant.store (-1);
        activeVariant.store (req);
        variantOverride.store (true);
        toParams.store (req);
        triggerAsyncUpdate();
    }
    lastBar = bar;

    if (rotReset.exchange (false)) engine.reset();

    Tr2Controls c = currentControls();
    if (variantOverride.load())
    {
        const int a = activeVariant.load();
        if (a >= 0)
        {
            const auto& v = variants[(size_t) a];
            c.mapX = v.mapX; c.mapY = v.mapY; c.tomrum = v.tomrum; c.gung = v.gung; c.gungTyp = v.gungTyp;
            c.variation = v.variation; c.riff = v.riff; c.djup = v.djup;
            for (int l = 0; l < kLanes; ++l) { c.laneDens[l] = v.dens[l]; c.laneSeed[l] = v.laneSeed[l]; }
        }
    }

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    HeapBlock<float> tmp; float* Rw = R;
    if (! Rw) { tmp.malloc ((size_t) n); Rw = tmp.get(); }
    Event ev[Engine::kMaxEvents];
    const int nev = engine.process (L, Rw, n, playing, ppq, bpm, c, ev, Engine::kMaxEvents);
    if (! R) for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + Rw[i]);

    // MIDI ut: släpp gamla noter, skicka nya
    const bool sendMidi = param ("midiut") > 0.5f;
    for (int i = 0; i < numOffs;)
    {
        auto& o = offs[(size_t) i];
        if (! playing || o.samplesLeft < n)
        {
            midi.addEvent (MidiMessage::noteOff (o.channel, o.note), playing ? jlimit (0, n - 1, o.samplesLeft) : 0);
            o = offs[(size_t) --numOffs];
        }
        else { o.samplesLeft -= n; ++i; }
    }
    const int gate = (int) (sr * 60.0 / bpm / 4.0 * 0.9);   // en 16-del
    for (int i = 0; i < nev; ++i)
    {
        const auto& e = ev[i];
        laneFlash[(size_t) e.lane].store (e.vel);
        if (! sendMidi) continue;
        const int ch = midiChannelOf (e.lane);
        midi.addEvent (MidiMessage::noteOn (ch, e.note, jlimit (0.05f, 1.0f, e.vel)), e.offset);
        if (numOffs < (int) offs.size()) offs[(size_t) numOffs++] = { e.note, ch, e.offset + gate };
        else midi.addEvent (MidiMessage::noteOff (ch, e.note), jmin (n - 1, e.offset + 1));
    }

    running.store (playing);
    playPpq.store (ppq);
    rotLevel.store (engine.rotShown);
    delNu.store (engine.lastDel);
}

// ---------------------------------------------------------------- VARIANTER
TomrumProcessor::Variant TomrumProcessor::captureVariant() const
{
    Variant v;
    v.mapX = param ("mapx"); v.mapY = param ("mapy"); v.tomrum = param ("tomrum"); v.gung = param ("gung");
    v.gungTyp = roundToInt (param ("gungtyp")); v.variation = param ("variation"); v.riff = param ("riff"); v.djup = param ("djup");
    for (int l = 0; l < kLanes; ++l) { v.dens[l] = param (densId (l)); v.laneSeed[l] = laneSeed[(size_t) l].load(); }
    return v;
}

void TomrumProcessor::writeVariantToParams (const Variant& v)
{
    setParam ("mapx", v.mapX); setParam ("mapy", v.mapY); setParam ("tomrum", v.tomrum); setParam ("gung", v.gung);
    setParam ("gungtyp", (float) v.gungTyp); setParam ("variation", v.variation); setParam ("riff", v.riff); setParam ("djup", v.djup);
    for (int l = 0; l < kLanes; ++l) { setParam (densId (l), v.dens[l]); laneSeed[(size_t) l].store (v.laneSeed[l]); }
    bump();
}

void TomrumProcessor::sparaVariant (int slot)
{
    slot = jlimit (0, kVarianter - 1, slot);
    variantFull[(size_t) slot].store (false);
    variants[(size_t) slot] = captureVariant();
    variantFull[(size_t) slot].store (true);
    activeVariant.store (slot);
    bump();
}

void TomrumProcessor::valjVariant (int slot)
{
    slot = jlimit (0, kVarianter - 1, slot);
    if (! variantFull[(size_t) slot].load()) return;
    if (running.load()) requestedVariant.store (slot);   // byts vid nästa takt
    else { activeVariant.store (slot); writeVariantToParams (variants[(size_t) slot]); }
}

void TomrumProcessor::handleAsyncUpdate()
{
    const int slot = toParams.exchange (-1);
    if (slot >= 0 && slot < kVarianter) writeVariantToParams (variants[(size_t) slot]);
    variantOverride.store (false);
}

void TomrumProcessor::variantToTree (const Variant& v, ValueTree& t)
{
    t.setProperty ("mx", v.mapX, nullptr); t.setProperty ("my", v.mapY, nullptr); t.setProperty ("tr", v.tomrum, nullptr);
    t.setProperty ("g", v.gung, nullptr); t.setProperty ("gt", v.gungTyp, nullptr); t.setProperty ("va", v.variation, nullptr);
    t.setProperty ("ri", v.riff, nullptr); t.setProperty ("dj", v.djup, nullptr);
    for (int l = 0; l < kLanes; ++l) { t.setProperty ("d" + String (l), v.dens[l], nullptr); t.setProperty ("s" + String (l), (int64) v.laneSeed[l], nullptr); }
}

TomrumProcessor::Variant TomrumProcessor::variantFromTree (const ValueTree& t)
{
    Variant v;
    v.mapX = t.getProperty ("mx", 0.5f); v.mapY = t.getProperty ("my", 0.3f); v.tomrum = t.getProperty ("tr", 0.35f);
    v.gung = t.getProperty ("g", 0.5f); v.gungTyp = t.getProperty ("gt", 1); v.variation = t.getProperty ("va", 0.4f);
    v.riff = t.getProperty ("ri", 0.3f); v.djup = t.getProperty ("dj", 0.4f);
    for (int l = 0; l < kLanes; ++l) { v.dens[l] = t.getProperty ("d" + String (l), 0.5f); v.laneSeed[l] = (uint32_t) (int64) t.getProperty ("s" + String (l), 1); }
    return v;
}

// ---------------------------------------------------------------- slump
void TomrumProcessor::setParam (const String& id, float plain)
{
    if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (plain));
}

void TomrumProcessor::pushUndo()
{
    ValueTree t = apvts.copyState().createCopy();
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
    t.removeProperty ("sseed", nullptr);
    for (int l = 0; l < kLanes; ++l) t.removeProperty ("seed" + String (l), nullptr);
    apvts.replaceState (t);
    bump();
}

void TomrumProcessor::slumpaMonster()
{
    pushUndo();
    for (int l = 0; l < kLanes; ++l)
        if (param (lasId (l)) < 0.5f && l != Kick) laneSeed[(size_t) l].store (freshSeed());
    bump();
}

void TomrumProcessor::slumpaSpar (int lane)
{
    if (lane < 0 || lane >= kLanes) return;
    pushUndo();
    laneSeed[(size_t) lane].store (freshSeed());
    bump();
}

void TomrumProcessor::slumpaLjud()
{
    pushUndo();
    soundSeed.store (freshSeed());
    bump();
}

void TomrumProcessor::slumpaKarta()
{
    pushUndo();
    setParam ("mapx", rng.nextFloat()); setParam ("mapy", rng.nextFloat() * 0.85f);
    setParam ("tomrum", 0.2f + 0.4f * rng.nextFloat());
    bump();
}

void TomrumProcessor::slumpaAllt()
{
    pushUndo();
    for (int l = 0; l < kLanes; ++l) if (param (lasId (l)) < 0.5f && l != Kick) laneSeed[(size_t) l].store (freshSeed());
    soundSeed.store (freshSeed());
    setParam ("mapx", rng.nextFloat()); setParam ("mapy", rng.nextFloat() * 0.85f);
    setParam ("tomrum", 0.2f + 0.4f * rng.nextFloat());
    setParam ("gungtyp", (float) rng.nextInt (kNumGung)); setParam ("gung", 0.3f + 0.5f * rng.nextFloat());
    setParam ("riff", rng.nextFloat() * 0.6f);
    bump();
}

// ---------------------------------------------------------------- DRA UT MIDI
File TomrumProcessor::exportMidi (int bars)
{
    bars = jlimit (1, 256, bars);
    auto c = currentControls();
    Engine e; const double rate = 48000.0, bpm = 124.0;
    e.prepare (rate);
    MidiMessageSequence seq[3];
    const int ppqRes = 960;
    std::vector<float> L (512), R (512);
    Event ev[Engine::kMaxEvents];
    const int total = (int) (bars * 4.0 * 60.0 / bpm * rate);
    for (int pos = 0; pos < total; pos += 512)
    {
        const int n = jmin (512, total - pos);
        const double ppq = pos * bpm / 60.0 / rate;
        const int ne = e.process (L.data(), R.data(), n, true, ppq, bpm, c, ev, Engine::kMaxEvents);
        for (int i = 0; i < ne; ++i)
        {
            const double t = (ppq + ev[i].offset * bpm / 60.0 / rate) * ppqRes;
            const int ch = midiChannelOf (ev[i].lane);
            auto& s = seq[ch - 1];
            s.addEvent (MidiMessage::noteOn (ch, ev[i].note, jlimit (0.05f, 1.0f, ev[i].vel)), t);
            s.addEvent (MidiMessage::noteOff (ch, ev[i].note), t + ppqRes / 4 * 0.9);
        }
    }
    MidiFile mf; mf.setTicksPerQuarterNote (ppqRes);
    for (auto& s : seq) { s.updateMatchedPairs(); mf.addTrack (s); }
    auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("TOMRUM");
    dir.createDirectory();
    const auto& sk = tomrum::skolor()[c.skola];
    auto file = dir.getChildFile (String ("TOMRUM ") + u8 (sk.name) + " " + String (bars) + " takter.mid");
    file.deleteFile();
    if (auto out = file.createOutputStream()) mf.writeTo (*out);
    return file;
}

// ---------------------------------------------------------------- lägen och tillstånd
const std::vector<TomrumPreset>& TomrumProcessor::presets() { return tomrumLagen(); }

void TomrumProcessor::applyPreset (const TomrumPreset& p)
{
    // allt tillbaka till grund, sedan lägets värden
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (prm))
        {
            const auto id = rp->getParameterID();
            if (id == "ut" || id == "midiut" || id.startsWith ("las")) continue;
            rp->setValueNotifyingHost (rp->getDefaultValue());
        }
    setParam ("skola", (float) p.skola);
    for (const auto& kv : p.values) setParam (kv.first, kv.second);
    soundSeed.store (p.seed);
    for (int l = 0; l < kLanes; ++l) laneSeed[(size_t) l].store (p.seed * 31u + (uint32_t) l * 977u + 1u);
    bump();
}

void TomrumProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= (int) presets().size()) return;
    pushUndo();
    currentProgram = index;
    applyPreset (presets()[(size_t) index]);
}

const String TomrumProcessor::getProgramName (int index)
{
    if (index < 0 || index >= (int) presets().size()) return {};
    const auto& p = presets()[(size_t) index];
    return p.category + " / " + p.name;
}

void TomrumProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState().createCopy();
    for (int l = 0; l < kLanes; ++l) state.setProperty ("seed" + String (l), (int64) laneSeed[(size_t) l].load(), nullptr);
    state.setProperty ("sseed", (int64) soundSeed.load(), nullptr);
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("aktiv", activeVariant.load(), nullptr);
    ValueTree vs ("VARIANTER");
    for (int i = 0; i < kVarianter; ++i)
        if (variantFull[(size_t) i].load()) { ValueTree t ("V"); t.setProperty ("slot", i, nullptr); variantToTree (variants[(size_t) i], t); vs.addChild (t, -1, nullptr); }
    state.removeChild (state.getChildWithName ("VARIANTER"), nullptr);
    state.addChild (vs, -1, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void TomrumProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto t = ValueTree::fromXml (*xml);
    for (int l = 0; l < kLanes; ++l) laneSeed[(size_t) l].store ((uint32_t) (int64) t.getProperty ("seed" + String (l), 1));
    soundSeed.store ((uint32_t) (int64) t.getProperty ("sseed", 1));
    currentProgram = t.getProperty ("program", 0);
    activeVariant.store (t.getProperty ("aktiv", -1));
    for (auto& v : variantFull) v.store (false);
    auto vs = t.getChildWithName ("VARIANTER");
    for (int i = 0; i < vs.getNumChildren(); ++i)
    {
        auto c = vs.getChild (i);
        const int slot = c.getProperty ("slot", -1);
        if (slot < 0 || slot >= kVarianter) continue;
        variants[(size_t) slot] = variantFromTree (c);
        variantFull[(size_t) slot].store (true);
    }
    t.removeChild (vs, nullptr);
    apvts.replaceState (t);
    bump();
}

AudioProcessorEditor* TomrumProcessor::createEditor() { return new TomrumEditor (*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TomrumProcessor(); }
