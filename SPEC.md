# TOMRUM – spec

Trumsynt med inbyggd generativ sekvens. Groovet byggs av tomrummet: regelböcker i stället för fri slump, otakt i millisekunder, subtraktion i stället för fler ljud.

## Filer

- `src/Skolor.h` – de 8 regelböckerna (mönster som sannolikhet per steg, Markov, friktion, min/max, förbud mot kicken, ljudets ramar, parasitmall)
- `src/Tomrum.h` – motorn: GENETIK (`mutate`), MINUS, SPÖKE, FRIKTION, PARASIT, RUTTNA, DUB, VARIATION och de 8 rösterna. Ren C++.
- `src/Presets.h` – 800 presets (8 × 100, fast frö per preset) och sparformatet för genomet (text)
- `src/Onset.h` – loop-örat: hittar slag (log-energi-flux, adaptiv tröskel, 45 ms spärr) och loopens längd i takter
- `src/Params.h` – parametrarna
- `src/PluginProcessor.*` – DAW-koppling, egen klocka, MIDI ut, genom via versionsräknare till ljudtråden, ångra, sparat läge
- `src/PluginEditor.*` – panelen
- `tests/engine_test.cpp` – motortest (ren C++, körs i varje bygge)

## Motorn

- **Tillståndslös i tid.** Ett slag bestäms av 16-delens nummer från låtens början, genomet och rattarna. Alla slumpbeslut i spelet är hashar.
- **Spår** har egen cykellängd (1–32), så 15/16, 7/8 och 5/7 mot kicken kommer ur regelboken.
- **MINUS:** sannolikhet = MINUS × (1,25 − vikt). Steg med vikt 1 (X) och kicken rörs aldrig.
- **SPÖKE:** spelar bara på 16-delar där inget annat spår och ingen kick slår.
- **Tid:** rutnät + SVÄNG (udda 16-delar) + PARASIT × mall + FRIKTION (ms ur spårets spann, fast per slag). Kicken bara rutnät.
- **RUTTNA:** tak × 2^(−3,2 × r), stereobredd × (1 − 0,7 r), r = min(1, takter sedan play / (7 × BPM / 4)) × RUTTNA.
- **DUB:** chans per slag (STAB dubbelt) att skickas till ekot (300 Hz–2,2 kHz i återkopplingen, tid = skolans taktslag).

## Panelen

Vantablack, mörkgrå linjer, blek bärnsten. Vänster: spåren med cykel, FRYS, TYST, nivå. Mitten: tomrummet, en ring per spår, skenet i mitten växer med tystnaden, röd båge = RUTTNA. Höger: skolan och parasitlådan. Nederst: rattarna.

## Val som gjordes utan dig (natten 31 okt)

- Åtta spår i stället för sex: SPÖKE och STAB behövdes för skolorna.
- Byta SKOLA på panelen kör GENETIK direkt. Automation av SKOLA väljer bara skolan.
- 800 presets, 100 per skola, namn på svenska utan artist- eller bolagsnamn.
- Loopar läses som 1, 2, 4 eller 8 takter vid aktuellt tempo.
- Inga skämtfunktioner (krasch, BPM-spärr). Pluggen ska vara stabil att sälja.
