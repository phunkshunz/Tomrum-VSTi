// TOMRUM 2-pluggen: TomrumTest (cmake -DTOMRUM_TESTS=ON)
//   LÄGEN      alla lägen låter, inga toppar, inga NaN
//   MIDI UT    lika många noter av som på
//   VARIANT    sparas, byts vid nästa takt via MIDI-tangent, följer med i projektet
//   LÅS        slump rör inte låsta spår
//   DRA UT     MIDI-filen har noter
#include "../src/PluginProcessor.h"
#include <cstdio>
using namespace juce;
static int fails = 0;
static void check (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what); if (! ok) ++fails; }

struct PH : AudioPlayHead
{
    int64 s = 0; double bpm = 124; bool playing = true;
    Optional<PositionInfo> getPosition() const override
    { PositionInfo p; p.setIsPlaying (playing); p.setBpm (bpm); p.setPpqPosition (s / 48000.0 * bpm / 60.0); return p; }
};

struct Stats { float peak = 0; double rms = 0; int on = 0, off = 0; bool nan = false; };
static Stats render (TomrumProcessor& p, PH& ph, int blocks, MidiBuffer* inject = nullptr)
{
    Stats st; AudioBuffer<float> b (2, 512); double ss = 0; long cnt = 0;
    for (int k = 0; k < blocks; ++k)
    {
        MidiBuffer m; if (inject && k == 0) m = *inject;
        p.processBlock (b, m);
        for (const auto meta : m) { const auto msg = meta.getMessage(); if (msg.isNoteOn()) ++st.on; if (msg.isNoteOff()) ++st.off; }
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = b.getSample (c, i); if (! std::isfinite (x)) st.nan = true; st.peak = std::max (st.peak, std::abs (x)); ss += x * x; ++cnt; }
        ph.s += 512;
    }
    st.rms = std::sqrt (ss / (double) cnt);
    return st;
}

int main()
{
    ScopedJuceInitialiser_GUI init;
    TomrumProcessor p; PH ph; p.setPlayHead (&ph); p.setRateAndBufferSizeDetails (48000, 512); p.prepareToPlay (48000, 512);

    // ---- LÄGEN
    {
        float worst = 0, quiet = 1; bool ok = true;
        for (int i = 0; i < p.getNumPrograms(); ++i)
        {
            p.setCurrentProgram (i); ph.s = 0; p.prepareToPlay (48000, 512);
            const auto st = render (p, ph, 400);
            worst = std::max (worst, st.peak); quiet = std::min (quiet, (float) st.rms);
            ok = ok && ! st.nan && st.peak <= 1.0f && st.rms > 0.005;
        }
        std::printf ("LÄGEN    %d lägen, högsta topp %.2f, tystaste rms %.3f\n", p.getNumPrograms(), worst, quiet);
        check (ok, "alla lägen låter, inga toppar, inga NaN");
    }
    // ---- MIDI UT
    {
        p.setCurrentProgram (3); ph.s = 0; p.prepareToPlay (48000, 512);
        auto st = render (p, ph, 400);
        ph.playing = false; const auto st2 = render (p, ph, 2); ph.playing = true;
        std::printf ("MIDI UT  %d på, %d av\n", st.on, st.off + st2.off);
        check (st.on > 20 && st.on == st.off + st2.off, "MIDI ut: varje not släpps");
    }
    // ---- VARIANT
    {
        p.setCurrentProgram (0); ph.s = 0; p.prepareToPlay (48000, 512);
        render (p, ph, 5);                             // transporten går, mitt i första takten
        p.sparaVariant (0);
        p.apvts.getParameter ("mapx")->setValueNotifyingHost (1.0f);
        p.apvts.getParameter ("tomrum")->setValueNotifyingHost (0.9f);
        p.sparaVariant (1);
        MidiBuffer in; in.addEvent (MidiMessage::noteOn (1, TomrumProcessor::kVariantNote + 0, 1.0f), 0);
        render (p, ph, 10, &in);                       // mitt i takten: inte bytt än
        const bool waited = p.aktivVariant() == 1;
        render (p, ph, 200);                           // nästa takt
        const bool switched = p.aktivVariant() == 0;
        MemoryBlock mb; p.getStateInformation (mb);
        TomrumProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        std::printf ("VARIANT  väntar %s, byter %s, i projektet %s\n", waited ? "ja" : "nej", switched ? "ja" : "nej", q.harVariant (1) ? "ja" : "nej");
        check (waited && switched && q.harVariant (0) && q.harVariant (1) && ! q.harVariant (2), "varianter byts vid nästa takt och sparas i projektet");
    }
    // ---- LÅS
    {
        auto before = p.currentControls();
        p.apvts.getParameter ("las3")->setValueNotifyingHost (1.0f);
        p.slumpaMonster();
        auto after = p.currentControls();
        check (before.laneSeed[3] == after.laneSeed[3] && before.laneSeed[2] != after.laneSeed[2], "slump rör inte låsta spår");
        p.undo();
        check (p.currentControls().laneSeed[2] == before.laneSeed[2], "ÅNGRA tar tillbaka");
    }
    // ---- DRA UT
    {
        const auto f = p.exportMidi (8);
        MidiFile mf; FileInputStream in (f); const bool read = in.openedOk() && mf.readFrom (in);
        int notes = 0; for (int t = 0; t < mf.getNumTracks(); ++t) for (auto* e : *mf.getTrack (t)) notes += e->message.isNoteOn();
        std::printf ("DRA UT   %s, %d noter\n", f.getFileName().toRawUTF8(), notes);
        check (read && notes > 30, "MIDI-filen har noter");
    }
    std::printf ("\n%s\n", fails ? "FEL" : "ALL OK");
    return fails ? 1 : 0;
}
