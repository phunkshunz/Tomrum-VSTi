# TOMRUM

Trumsynt med självgående sekvens, som VST3 och AU för macOS (VST3 för Windows). Phunkshunz AUDIO, serien **MASKINHALLEN**. Syskon till BETONG, GLAPP, FASFEL, GRUND, KULVERT och de andra.

TOMRUM bygger groovet av det som inte spelas. Åtta egna ljud, en sekvens som skrivs efter en skolas regelbok, en otakt som ligger exakt fel och ett ljud som sakta ruttnar under natten. Tryck play så låter den direkt.

Hela specen finns i [SPEC.md](SPEC.md). Manualen (engelska, PDF) ligger i zip-filen.

## Ladda ner

Mac: https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-macOS.zip

Windows (VST3): https://github.com/phunkshunz/Tomrum-VSTi/releases/latest/download/TOMRUM-Windows.zip

Ladda inte ner via den gröna knappen **Code**. Den ger bara källkoden, inte pluginet.

## Installera

**Mac:** dubbelklicka på zip-filen i Hämtade filer. Öppna sedan Terminal och kör:

```
bash ~/Downloads/TOMRUM/installera.command
```

**Stäng Bitwig helt och starta igen.** TOMRUM finns bland instrumenten. Kräver macOS 11 eller nyare, Intel eller Apple Silicon.

**Windows:** packa upp och lägg `TOMRUM.vst3` i `C:\Program Files\Common Files\VST3`.

## Kom igång på en minut

1. Lägg TOMRUM på ett instrumentspår och tryck play. Ingen DAW? Den fristående appen startar själv, annars **SPELA**.
2. Välj **SKOLA**. Byte kör GENETIK direkt.
3. Tryck **GENETIK** tills något fastnar, eller bläddra bland presets med pilarna.
4. **F** (frys) spåren du gillar. Tryck GENETIK igen: bara de andra ändras.
5. Klicka på ett spårs **namn** för att bara slå om det spåret. **LJUD** behåller groovet och byter ljud.
6. Vrid upp **MINUS** och hör groovet bli bättre ju tommare det blir.
7. Gick det för långt? **ÅNGRA** (upp till 40 steg).

## Vad finns i maskinen

| Del | Vad den gör |
|---|---|
| **8 ljud** | KICK, BAS, KLICK, HATT, TRÄ, SKRAP, STAB (ackord) och SPÖKE, som bara spelar där inget annat slår. Allt syntas inne i pluggen. |
| **8 skolor** | UNDER VATTEN, PINCETT, RUTTEN SVÄNG, FIENTLIG FUNK, KYLTORN, KLINIK, KASSETT och TRÄKLOSS. Varje skola är en regelbok för rytmik, otakt och ljudbild. |
| **GENETIK** | Nytt groove och nytt ljud ur regelboken. Aldrig fri slump: Markov-regler, min/max slag per cykel, förbud mot slag på kicken. |
| **MINUS** | Stryker slag, de svaga först, kicken aldrig. Samma slag försvinner alltid på samma ställe. |
| **SVÄNG / FRIKTION** | Udda 16-delar släpar, och varje spår ligger sina egna millisekunder bakom rutnätet. Kicken ligger alltid rakt. |
| **PARASIT** | Skolans otakt, eller släpp en trumloop på panelen så suger TOMRUM ut hur skevt den ligger och lägger det på sina egna slag. |
| **DUB** | Enstaka slag kastas ner i källaren (ekot). Resten står torrt. |
| **PUMP / TAK** | Kicken trycker ner resten. Lågpasset ovanför allt. |
| **RUTTNA** | Från play sjunker taket och stereon krymper sakta under ungefär sju minuter. |
| **VARIATION** | Var fjärde takt vrids svaga steg, alltid inom regelboken. |
| **800 presets** | 100 per skola. Frysta spår lämnas i fred vid preset-byte. Eget groove sparas i projektet. |
| **MIDI ut** | Varje slag går också ut som en not på kanal 1. |

Allt räknas ur låtpositionen: spolar man tillbaka låter samma takt exakt likadant, så loopar och export blir stabila.

## Bygga själv

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build --config Release
```

JUCE 8 hämtas automatiskt. GitHub Actions bygger varje push till `main` och lägger den som senaste release.

Tester: `g++ -std=c++17 -Isrc tests/engine_test.cpp -o engine_test && ./engine_test` testar regelböckerna, motorn, alla 800 presets, sparformatet och loop-örat.
