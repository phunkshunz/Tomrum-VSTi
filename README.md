# TOMRUM 2 · DAS LEERRAUM

Din groovepartner för tysk minimal. TOMRUM spelar direkt när du trycker play: åtta egna ljud (kick, bas, klick, hatt, trä, skrap, stab, spöke), och sväng byggd av det som inte spelas. VST3 och AU för macOS, VST3 för Windows. Ett plugin från **MASKINHALLEN** / Phunkshunz AUDIO.

## Ladda ner

- Mac (Apple Silicon, M1–M4): https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-macOS-AppleSilicon.zip
- Mac (Intel): https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-macOS-Intel.zip
- Mac (båda i en fil): https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-macOS.zip
- Windows: https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-Windows.zip

Ladda inte ner via den gröna knappen **Code**. Den ger bara källkoden.

**Mac:** dubbelklicka på zip-filen och kör sedan i Terminal: `bash ~/Downloads/TOMRUM/installera.command`
**Windows:** extrahera zip-filen och dubbelklicka på **installera.bat**.

TOMRUM ligger bland instrumenten.

## Nytt i TOMRUM 2

- **KARTAN**: dra punkten. Åt sidan KÖLN (rakt, rent), BERLIN (dub), BUKAREST (rullande). Uppåt RAK, RULL, BRUTEN. Mönstren glider över i varandra.
- **TOMRUM**: hur tomt. De svaga slagen försvinner först, kicken står kvar. SPÖKE spelar bara i tystnaden.
- **GUNG**: RAK, MPC, SLÄP, BUKAREST eller OTAKT, plus FRIKTION: varje spår släpar sina egna millisekunder.
- **LÅTEN**: arrangerar själv över 64 eller 128 takter, med intro, break utan kick, topp och fill. Samma låt varje gång från början.
- **TONART och RIFF**: basen och staben spelar små figurer i skalan (moll, dorisk, frygisk, penta).
- **VARIANTER**: 8 platser. Tryck SPARA och en siffra. Byt med siffran eller MIDI-tangent C3–G3, alltid vid nästa takt.
- **LÅS** per spår när du slumpar, **ÅNGRA**, **MIDI UT** (trummor kanal 1, bas 2, stab 3) och **DRA UT MIDI**.
- **LJUD**: DJUP (dub-kast), KÄLLARE (betongrum), KASSETT, RUTTNA, PUMP. 64 lägen.

## Kom igång

1. Lägg TOMRUM på ett instrumentspår och tryck **play** i DAW:en.
2. Välj ett läge, eller en **SKOLA**.
3. Dra punkten på **KARTAN** och vrid **TOMRUM**. Gillar du något: **SPARA** och en siffra.
4. **DRA UT MIDI** till ett spår om du vill bygga vidare i DAW:en.

## Skolorna

| Skola | Ljud |
|---|---|
| **UNDER VATTEN** | dub i djupet: ackord som kastas i ekot, allt annat torrt |
| **PINCETT** | subtraktion: millimeterklick, sinusbas på 17 steg |
| **RUTTEN SVÄNG** | allt släpar, trä och skrap i lös sväng |
| **FIENTLIG FUNK** | ringmod och kross, funk som biter |
| **KYLTORN** | dub i ett kyltorn: brusbädd som andas med kicken |
| **KLINIK** | sinusskurar, rent och kliniskt |
| **KASSETT** | knaster, band, varmt och slitet |
| **TRÄKLOSS** | trä, kvistar och stubbar |

Varje skola bestämmer var slagen får ligga, hur tätt de får komma, vad som är förbjudet (till exempel slag på kicken), svängen och ljudets ramar. Rutorna i **MÖNSTER** visar regeln: skuggad ruta = slag tillåtet, tjock kant = alltid slag.

## Bygga själv

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE hämtas automatiskt. Motorn testas med `c++ -std=c++17 -Isrc tests/engine_test.cpp && ./a.out`.
