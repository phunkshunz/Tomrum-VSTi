// Ritar panelen till en PNG: TomrumSnapshot ut.png [läge] [en]
#include "../src/PluginEditor.h"
using namespace juce;
struct PH : AudioPlayHead { double ppq = 21.3; Optional<PositionInfo> getPosition() const override
    { PositionInfo p; p.setIsPlaying (true); p.setBpm (124); p.setPpqPosition (ppq); return p; } };
int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    TomrumProcessor p; PH ph; p.setPlayHead (&ph); p.setRateAndBufferSizeDetails (48000, 512); p.prepareToPlay (48000, 512);
    if (argc > 2) p.setCurrentProgram (atoi (argv[2]));
    if (argc > 3) sprak::sattEngelska (true);
    p.apvts.getParameter ("lat")->setValueNotifyingHost (1.0f);
    p.sparaVariant (0); p.sparaVariant (2);
    AudioBuffer<float> b (2, 512); MidiBuffer m;
    for (int i = 0; i < 40; ++i) { p.processBlock (b, m); ph.ppq += 512 / 48000.0 * 124 / 60.0; }
    std::unique_ptr<AudioProcessorEditor> ed (p.createEditor());
    if (auto* te = dynamic_cast<TomrumEditor*> (ed.get())) for (int i = 0; i < 6; ++i) te->tickForSnapshot();
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    File f (File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "tomrum.png"));
    f.deleteFile(); FileOutputStream out (f); PNGImageFormat().writeImageToStream (img, out);
    std::printf ("skrev %s (%dx%d)\n", f.getFullPathName().toRawUTF8(), ed->getWidth(), ed->getHeight());
}
