# TOMRUM · DAS LEERRAUM

Sväng byggd av det som inte spelas. TOMRUM är en trummaskin för tysk minimal: kick, sinusbas, klick, hatt, trä, skrap, stab och ett spöke. Allt slumpas, men aldrig hur som helst. Varje skola har en egen regelbok som slumpen inte kan bryta mot. VST3 och AU för macOS, VST3 för Windows. Ett plugin från **MASKINHALLEN** / Phunkshunz AUDIO.

## Ladda ner

- Mac (Apple Silicon, M1–M4): https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-macOS-AppleSilicon.zip
- Mac (Intel): https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-macOS-Intel.zip
- Mac (båda i en fil): https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-macOS.zip
- Windows: https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-Windows.zip

Ladda inte ner via den gröna knappen **Code**. Den ger bara källkoden.

**Mac:** dubbelklicka på zip-filen och kör sedan i Terminal: `bash ~/Downloads/TOMRUM/installera.command`
**Windows:** extrahera zip-filen och dubbelklicka på **installera.bat**.

TOMRUM ligger bland instrumenten.

## Kom igång

1. Lägg TOMRUM på ett instrumentspår och tryck **play** i DAW:en.
2. Välj en **SKOLA**.
3. Tryck **GENETIK** tills något fastnar. Tryck **MUTATION** för att låta det växa.

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

## Knapparna

| Knapp | Vad den gör |
|---|---|
| **GENETIK** | Nytt mönster och nytt ljud, inom skolans regler |
| **MUTATION** | Två eller tre spår föds om. Kicken står kvar. |
| **LJUD** | Nytt ljud, samma mönster |
| **Tärningen per spår** | Nytt mönster för bara det spåret |
| **NYTT LIV** | Nollställer RUTTNA |
| **ÅNGRA** | Tillbaka ett steg |

## Rattarna

| Ratt | Vad den gör |
|---|---|
| **MINUS** | Tar bort slag. De svaga först, kicken aldrig. |
| **FRIKTION** | Varje spår släpar sina millisekunder bakom rutnätet. Kicken ligger rakt. |
| **PARASIT** | Hur mycket otakt som smittar. |
| **SVÄNG** | Udda 16-delar släpar, bara inom skolans ram. |
| **DUB** | Chans att ett slag kastas ner i ekot. |
| **RUTTNA** | Inget låter likadant länge: ljudet mörknar och smalnar under ungefär sju minuter. |
| **TAK** | Lågpass över allt. |
| **PUMP** | Allt utom kicken trycks ner av kicken. |
| **VARIATION** | Var fjärde takt vrids svaga steg, inom regelboken. |
| **GRUNDTON** | Tonart för bas och stab. |

**SPÖKE** spelar bara i tystnaden, där inget annat spår slår.

## PARASIT: stjäl otakten ur en loop

Släpp en loop på rutan **PARASIT** (eller **LADDA LOOP**). TOMRUM letar upp slagen i loopen, mäter hur långt från rutnätet de ligger och låter alla spår släpa på samma sätt. **RENSA** går tillbaka till skolans egen otakt. Otakten sparas med låten.

## MIDI ut

Varje slag skickas också som MIDI på kanal 1 (kick 36, klick 37, hatt 42, bas och stab i grundtonen). Spela in MIDI:n på ett annat spår och kör den genom BETONG, ARMERING eller ett eget trumprogram.

## Presets

256 presets, 32 per skola, med egna namn. Välj i listan högst upp eller bläddra med ◀ ▶.

## Bygga själv

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE hämtas automatiskt. Motorn testas med `c++ -std=c++17 -Isrc tests/engine_test.cpp && ./a.out`.
