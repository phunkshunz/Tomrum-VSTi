#include "../src/PluginProcessor.h"
#include <cstdio>
using namespace juce;
static int fails = 0;
#define CHECK(c, ...) do { if (! (c)) { ++fails; std::printf ("FEL: " __VA_ARGS__); std::printf ("\n"); } } while (0)
struct PH : AudioPlayHead { int64 s = 0; double bpm = 124;
  Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setIsPlaying (true); p.setBpm (bpm); p.setPpqPosition (s / 48000.0 * bpm / 60.0); return p; } };

static void render (TomrumProcessor& p, PH& ph, int blocks, float& peak, double& rms, int& notesOn, int& notesOff, bool& nan)
{
    AudioBuffer<float> b (2, 512); MidiBuffer m; double ss = 0; long cnt = 0;
    for (int k = 0; k < blocks; ++k)
    {
        p.processBlock (b, m);
        for (const auto meta : m) { const auto msg = meta.getMessage(); if (msg.isNoteOn()) ++notesOn; if (msg.isNoteOff()) ++notesOff; }
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = b.getSample (c, i); if (! std::isfinite (x)) nan = true; peak = std::max (peak, std::abs (x)); ss += x * x; ++cnt; }
        ph.s += 512;
    }
    rms = std::sqrt (ss / cnt);
}

int main()
{
    ScopedJuceInitialiser_GUI init;
    TomrumProcessor p; PH ph; p.setPlayHead (&ph); p.setRateAndBufferSizeDetails (48000, 512); p.prepareToPlay (48000, 512);

    // 1) alla presets
    CHECK (p.getNumPrograms() == 256, "antal presets %d", p.getNumPrograms());
    StringArray names; float worstPeak = 0, quietest = 1;
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        names.addIfNotAlreadyThere (p.getProgramName (i));
        p.setCurrentProgram (i);
        float peak = 0; double rms = 0; int on = 0, off = 0; bool nan = false;
        ph.s = 0; render (p, ph, 470, peak, rms, on, off, nan);   // ~5 s
        CHECK (! nan, "preset %d NaN", i);
        CHECK (peak < 0.99f, "preset %d (%s) klipper %.2f", i, p.getProgramName (i).toRawUTF8(), peak);
        CHECK (rms > 0.01, "preset %d (%s) tyst %.4f", i, p.getProgramName (i).toRawUTF8(), rms);
        CHECK (on > 10, "preset %d få noter %d", i, on);
        worstPeak = std::max (worstPeak, peak); quietest = std::min (quietest, (float) rms);
    }
    CHECK (names.size() == 256, "namn inte unika: %d", names.size());
    std::printf ("256 presets: högsta topp %.2f, tystaste rms %.3f, unika namn %d\n", worstPeak, quietest, names.size());
    std::printf ("exempel: %s, %s, %s, %s\n", p.getProgramName (0).toRawUTF8(), p.getProgramName (37).toRawUTF8(), p.getProgramName (140).toRawUTF8(), p.getProgramName (255).toRawUTF8());

    // 2) MIDI: varje not-på släpps
    { p.setCurrentProgram (3); float pk = 0; double r = 0; int on = 0, off = 0; bool nan = false; ph.s = 0; render (p, ph, 2000, pk, r, on, off, nan);
      CHECK (std::abs (on - off) <= 8, "MIDI hänger: %d på, %d av", on, off); std::printf ("MIDI: %d på, %d av\n", on, off); }

    // 3) GENETIK/MUTATION rör aldrig kicken i MUTATION, ångra fungerar
    {
        p.setCurrentProgram (10);
        const auto g0 = p.currentGenome();
        for (int k = 0; k < 30; ++k) { p.mutation(); CHECK (p.currentGenome().lane[tomrum::Kick].hits == g0.lane[tomrum::Kick].hits, "MUTATION ändrade kicken"); }
        for (int k = 0; k < 30; ++k) p.undo();
        CHECK (p.currentGenome().lane[tomrum::Hat].hits == g0.lane[tomrum::Hat].hits, "ångra fungerar inte");
        p.genetik(); p.mutateLane (3); p.mutateSound();
        float pk = 0; double r = 0; int on = 0, off = 0; bool nan = false; render (p, ph, 300, pk, r, on, off, nan);
        CHECK (! nan && pk < 0.99f && r > 0.005, "efter GENETIK: topp %.2f rms %.4f", pk, r);
    }

    // 4) spara och läsa: samma genom, samma knappar
    {
        p.genetik(); p.mutateLane (5);
        const auto g = p.currentGenome();
        MemoryBlock mb; p.getStateInformation (mb);
        TomrumProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        const auto h = q.currentGenome();
        bool same = g.skola == h.skola; for (int l = 0; l < tomrum::kLanes; ++l) same &= g.lane[l].hits == h.lane[l].hits;
        CHECK (same, "genomet sparades inte");
        CHECK (std::abs (p.apvts.getRawParameterValue ("minus")->load() - q.apvts.getRawParameterValue ("minus")->load()) < 1e-4, "knappar sparades inte");
    }

    // 5) PARASIT ur en loop: 2 takter vid 124 bpm, udda 16-delar 20 % sena
    {
        const double sr = 44100; const int len = (int) (sr * 8 * 60.0 / 124.0); const double s16 = len / 32.0;
        AudioBuffer<float> b (1, len); b.clear();
        for (int k = 0; k < 32; ++k)
        {
            const int at = (int) (k * s16 + ((k & 1) ? s16 * 0.2 : 0));
            for (int i = 0; i < 2000 && at + i < len; ++i) b.setSample (0, at + i, 0.8f * std::exp (-i / 300.0f) * std::sin (i * 0.3f));
        }
        File f = File::getSpecialLocation (File::tempDirectory).getChildFile ("tomrum_loop.wav"); f.deleteFile();
        { WavAudioFormat w; std::unique_ptr<AudioFormatWriter> wr (w.createWriterFor (new FileOutputStream (f), sr, 1, 16, {}, 0)); wr->writeFromAudioSampleBuffer (b, 0, len); }
        const String err = p.loadParasite (f);
        CHECK (err.isEmpty(), "parasit: %s", err.toRawUTF8());
        const auto pa = p.currentParasite();
        std::printf ("parasit ur loop: steg0 %.3f steg1 %.3f steg2 %.3f steg3 %.3f\n", pa.off[0], pa.off[1], pa.off[2], pa.off[3]);
        CHECK (pa.off[1] > 0.12f && pa.off[3] > 0.12f && std::abs (pa.off[0]) < 0.05f, "parasit fel");
        f.deleteFile();
    }

    std::printf (fails ? "\n%d FEL\n" : "\nALLT GRÖNT\n", fails);
    return fails ? 1 : 0;
}
