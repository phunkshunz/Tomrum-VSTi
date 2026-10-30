#pragma once
// TOMRUM — REGELBÖCKERNA
// Varje skola är en regelbok. MUTATION slumpar aldrig fritt: den väljer bara inom bokens gränser.
//
// Per spår:
//   mönster   upp till tre varianter. En siffra per steg = sannolikhet (0 = aldrig, 9 = 90 %, X = alltid).
//             Längden på strängen är spårets cykel: 16 = rak takt, 15 = 15/16, 14 = 7/8, 5/7 = polyrytm.
//   folj      Markov: hur sannolikheten ändras om förra steget slog (>1 = slag drar slag, <1 = glesa).
//   friktion  hur många ms spåret släpar bakom rutnätet (min, max). Kicken släpar aldrig.
//   minSlag/maxSlag  hur många slag per cykel som är tillåtet. Bryts det, slumpas om.
//   intePåKick  spåret får inte slå samtidigt som kicken (basen, öppna hattar ...).
// Plus ljudet: kick, bas, hattar, trä, skrap, stab, tak (lågpass), dub, pump, ringmod, kross, knaster.

namespace tomrum
{

enum Lane : int { Kick = 0, Bas, Klick, Hat, Tra, Skrap, Stab, Spoke, kLanes };

inline const char* laneName (int l) noexcept
{
    static const char* n[kLanes] = { "KICK", "BAS", "KLICK", "HATT", "TRÄ", "SKRAP", "STAB", "SPÖKE" };
    return (l >= 0 && l < kLanes) ? n[l] : "?";
}

struct LaneRule
{
    const char* pattern[3];   // nullptr = variant saknas. Första måste finnas (eller hela spåret tomt).
    float follow;             // Markov
    float fricLo, fricHi;     // ms
    int minHits, maxHits;
    bool notOnKick;
};

struct Range { float lo, hi; };

struct Skola
{
    const char* name;
    const char* sub;          // en rad om ljudet
    int bpm;
    Range swing;              // udda 16-delar släpar (andel av en 16-del)
    Range minus;              // MINUS: andel slag som tas bort (inte kicken)
    Range dub;                // DUB-SMET: chans att ett slag kastas i ekot
    LaneRule lane[kLanes];
    // ljudet
    Range kickPitch, kickDecay, kickClick;  // Hz, s, 0..1
    Range basDecay;                         // s
    int basDropEvery;                       // basen faller i tonhöjd var n:te steg (0 = aldrig)
    Range hatLen;                           // s (0.001 = 1 ms klick)
    Range hatTone;                          // 0 mörk .. 1 ljus
    bool hatSine;                           // KLINIK: sinusskurar i stället för brus
    Range traHz;                            // träklossens tonhöjd
    Range traDecay;                         // s
    int chord;                              // 0 moll9, 1 moll7, 2 maj7, 3 kvint, 4 inget
    Range stabCut;                          // Hz
    Range ceiling;                          // tak-lågpass Hz
    Range delayFb, delayMix;
    float delayBeats;                       // ekots tid i taktslag
    Range pump;                             // allt utom kicken trycks ner av kicken
    Range ringmod, crush;                   // FIENTLIG
    Range crackle;                          // knaster/vinyl
    Range noiseBed;                         // brusbädd som pumpar (KYLTORN)
    const char* parasite;                   // skolans egen otakt: 16 förskjutningar, siffra = hundradelar av en 16-del, '-' = före
};

// Mallar för otakten (PARASIT). Varje tecken = ett steg, 0–9 = 0–36 % av en 16-del sent, a–c = 3–9 % tidigt.
// Används tills man släpper en egen loop på panelen, då tas otakten ur den.

inline const Skola* skolor()
{
    static const Skola s[] = {
    // ---------------------------------------------------------------- UNDER VATTEN
    { "UNDER VATTEN", "allt dränkt: 2 kHz-tak, dub, pump mot en oändlig fyra", 124,
      { 0.22f, 0.33f }, { 0.25f, 0.45f }, { 0.25f, 0.45f },
      {
        { { "X000X000X000X000", nullptr, nullptr }, 1.0f, 0, 0, 4, 4, false },
        { { "0030003000300030", nullptr, nullptr }, 0.6f, 4, 9, 2, 6, true },
        { { "0000500000005000", "0000600000006006", nullptr }, 0.7f, 6, 10, 1, 4, false },
        { { "0080008000800080", "3080308030803080", nullptr }, 0.9f, 6, 10, 3, 8, false },
        { { "0000003000000030", "0003000000300000", nullptr }, 0.5f, 8, 14, 0, 3, false },
        { { "2000200020002000", nullptr, nullptr }, 0.8f, 8, 14, 0, 6, false },
        { { "0070000000700000", "0000007000000000", "0070000000000070" }, 0.4f, 6, 12, 1, 3, true },
        { { "00000", nullptr, nullptr }, 1.0f, 0, 0, 0, 0, false },
      },
      { 44, 52 }, { 0.45f, 0.75f }, { 0.1f, 0.3f }, { 0.25f, 0.5f }, 0,
      { 0.02f, 0.05f }, { 0.1f, 0.35f }, false, { 400, 700 }, { 0.04f, 0.08f },
      1, { 500, 1100 }, { 1500, 2200 }, { 0.65f, 0.82f }, { 0.55f, 0.85f }, 0.75f,
      { 0.6f, 0.85f }, { 0, 0 }, { 0, 0.1f }, { 0.05f, 0.15f }, { 0.05f, 0.15f },
      "0408040804080408" },
    // ---------------------------------------------------------------- PINCETT
    { "PINCETT", "subtraktion: 1 ms klick, sinusbas som faller var 17:e steg", 128,
      { 0.0f, 0.06f }, { 0.65f, 0.85f }, { 0.0f, 0.12f },
      {
        { { "X000X000X000X000", nullptr, nullptr }, 1.0f, 0, 0, 4, 4, false },
        { { "00700070007000700", "0070000700700007", nullptr }, 0.7f, 0, 3, 2, 6, true },
        { { "9999999999999999", "8989898989898989", nullptr }, 1.0f, 0, 2, 6, 16, false },
        { { "0090009000900090", "0090009000900099", nullptr }, 1.0f, 0, 2, 3, 6, false },
        { { "000600000006", "0006000000060", nullptr }, 0.5f, 1, 4, 1, 3, false },
        { { "00000", nullptr, nullptr }, 1.0f, 0, 0, 0, 0, false },
        { { "0000000000000000", "0000005000000000", nullptr }, 0.3f, 0, 2, 0, 1, true },
        { { "00700", "0070000", nullptr }, 1.0f, 0, 3, 1, 2, false },
      },
      { 48, 58 }, { 0.12f, 0.22f }, { 0.4f, 0.7f }, { 0.08f, 0.2f }, 17,
      { 0.001f, 0.002f }, { 0.6f, 1.0f }, false, { 900, 1600 }, { 0.01f, 0.03f },
      4, { 1500, 3000 }, { 12000, 18000 }, { 0.3f, 0.5f }, { 0.08f, 0.2f }, 0.75f,
      { 0.15f, 0.35f }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
      "0000000000000000" },
    // ---------------------------------------------------------------- RUTTEN SVÄNG
    { "RUTTEN SVÄNG", "otakten: trä och kastruller 14 ms bakom, 15/16 som glider", 124,
      { 0.12f, 0.24f }, { 0.25f, 0.45f }, { 0.08f, 0.2f },
      {
        { { "X000X000X000X000", nullptr, nullptr }, 1.0f, 0, 0, 4, 4, false },
        { { "0060006000600060", "0060060000600600", "0006006000060060" }, 0.8f, 6, 12, 2, 6, true },
        { { "0003000300030003", "000300030030003", nullptr }, 0.8f, 10, 18, 2, 6, false },
        { { "0060006000600060", "3060306030603060", "006000600060006" }, 0.9f, 8, 14, 3, 9, false },
        { { "507000507000507", "5070050700507", "500700500700507" }, 1.1f, 10, 18, 3, 8, false },
        { { "300300300300300", "30030030030003", nullptr }, 0.9f, 12, 18, 2, 6, false },
        { { "0000000000000000", "0000005000000000", nullptr }, 0.3f, 8, 14, 0, 1, true },
        { { "30300", "3003000", nullptr }, 1.0f, 12, 18, 1, 3, false },
      },
      { 46, 54 }, { 0.3f, 0.5f }, { 0.1f, 0.2f }, { 0.12f, 0.35f }, 0,
      { 0.015f, 0.04f }, { 0.3f, 0.6f }, false, { 650, 1400 }, { 0.03f, 0.07f },
      4, { 800, 1500 }, { 9000, 14000 }, { 0.35f, 0.55f }, { 0.12f, 0.25f }, 0.75f,
      { 0.2f, 0.4f }, { 0, 0 }, { 0, 0 }, { 0.05f, 0.18f }, { 0, 0 },
      "0326041503260415" },
    // ---------------------------------------------------------------- FIENTLIG FUNK
    { "FIENTLIG FUNK", "maskinell fientlighet: 7/8 över fyran, ringmod och vikning", 130,
      { 0.0f, 0.1f }, { 0.2f, 0.4f }, { 0.05f, 0.18f },
      {
        { { "X000X000X000X000", "X000X000X00X0000", nullptr }, 1.0f, 0, 0, 4, 5, false },
        { { "00700700007000", "07000700070070", nullptr }, 0.9f, 0, 6, 3, 6, true },
        { { "70070070070070", "7007007007", nullptr }, 1.2f, 0, 6, 3, 8, false },
        { { "90909090909090", "99099099099099", nullptr }, 1.0f, 0, 4, 5, 12, false },
        { { "5005005", "5000500", nullptr }, 1.0f, 2, 8, 2, 3, false },
        { { "3003003", "300030003", nullptr }, 1.0f, 2, 8, 1, 4, false },
        { { "0000700000000000", "0000000000700000", nullptr }, 0.5f, 0, 4, 0, 1, true },
        { { "7000700", "70007", nullptr }, 1.0f, 0, 6, 1, 2, false },
      },
      { 50, 58 }, { 0.4f, 0.7f }, { 0.2f, 0.4f }, { 0.1f, 0.3f }, 0,
      { 0.005f, 0.03f }, { 0.6f, 1.0f }, false, { 1200, 2400 }, { 0.02f, 0.05f },
      3, { 1200, 2500 }, { 10000, 16000 }, { 0.3f, 0.55f }, { 0.1f, 0.25f }, 0.5f,
      { 0.25f, 0.45f }, { 0.25f, 0.6f }, { 0.25f, 0.6f }, { 0, 0 }, { 0, 0 },
      "0a0705b1080a0306" },
    // ---------------------------------------------------------------- KYLTORN
    { "KYLTORN", "dub i ett kyltorn: moll 9 på takten efter, band som tappar diskant", 122,
      { 0.0f, 0.08f }, { 0.15f, 0.35f }, { 0.15f, 0.3f },
      {
        { { "X000X000X000X000", nullptr, nullptr }, 1.0f, 0, 0, 4, 4, false },
        { { "0000000000000000", "0000000800000000", nullptr }, 0.5f, 0, 4, 0, 2, true },
        { { "0000800000008000", nullptr, nullptr }, 1.0f, 2, 6, 2, 2, false },
        { { "0070007000700070", "3070307030703070", nullptr }, 1.0f, 2, 6, 4, 8, false },
        { { "00000", nullptr, nullptr }, 1.0f, 0, 0, 0, 0, false },
        { { "5555555555555555", nullptr, nullptr }, 1.0f, 4, 8, 4, 16, false },
        { { "0080000000800000", "0000008000000000", "0080008000000000" }, 0.6f, 2, 6, 1, 3, true },
        { { "00000", nullptr, nullptr }, 1.0f, 0, 0, 0, 0, false },
      },
      { 46, 52 }, { 0.45f, 0.75f }, { 0.15f, 0.3f }, { 0.3f, 0.6f }, 0,
      { 0.02f, 0.05f }, { 0.15f, 0.4f }, false, { 400, 600 }, { 0.04f, 0.06f },
      0, { 600, 1400 }, { 4500, 7000 }, { 0.7f, 0.85f }, { 0.55f, 0.9f }, 0.75f,
      { 0.45f, 0.7f }, { 0, 0 }, { 0, 0 }, { 0.02f, 0.08f }, { 0.15f, 0.35f },
      "0100010001000100" },
    // ---------------------------------------------------------------- KLINIK
    { "KLINIK", "iskall matematik: euklidiska klick, sinus och brusskurar", 120,
      { 0.0f, 0.0f }, { 0.1f, 0.3f }, { 0.0f, 0.06f },
      {
        { { "X000X000X000X000", "X0000000X0000000", nullptr }, 1.0f, 0, 0, 2, 4, false },
        { { "0000009000000090", "0090000000900000", nullptr }, 1.0f, 0, 0, 1, 3, true },
        { { "9009009009009009", "900900900", "90090090090" }, 1.0f, 0, 0, 3, 7, false },
        { { "9009009", "900909", "9000900" }, 1.0f, 0, 0, 2, 4, false },
        { { "90000", "9000000", nullptr }, 1.0f, 0, 0, 1, 2, false },
        { { "000090000000", "0009000000000", nullptr }, 1.0f, 0, 0, 1, 2, false },
        { { "0000000000000000", nullptr, nullptr }, 1.0f, 0, 0, 0, 0, true },
        { { "9000000", "900000000", nullptr }, 1.0f, 0, 0, 1, 1, false },
      },
      { 60, 90 }, { 0.6f, 1.0f }, { 0.0f, 0.05f }, { 0.05f, 0.25f }, 0,
      { 0.002f, 0.012f }, { 0.7f, 1.0f }, true, { 2000, 6000 }, { 0.005f, 0.02f },
      4, { 3000, 6000 }, { 18000, 20000 }, { 0.2f, 0.4f }, { 0.0f, 0.1f }, 0.5f,
      { 0.0f, 0.15f }, { 0, 0.1f }, { 0, 0.2f }, { 0, 0 }, { 0, 0 },
      "0000000000000000" },
    // ---------------------------------------------------------------- KASSETT
    { "KASSETT", "kvarglömt band i regnet: knastret blir hattarna, 10 kHz-tak", 118,
      { 0.08f, 0.18f }, { 0.2f, 0.4f }, { 0.05f, 0.15f },
      {
        { { "X000X000X000X000", nullptr, nullptr }, 1.0f, 0, 0, 4, 4, false },
        { { "0060006000600060", "0060006000600606", nullptr }, 0.8f, 2, 8, 2, 6, true },
        { { "0000700000007000", nullptr, nullptr }, 1.0f, 4, 10, 2, 2, false },
        { { "00000", nullptr, nullptr }, 1.0f, 0, 0, 0, 0, false },
        { { "0000300000000300", "0300000000300000", nullptr }, 0.6f, 4, 10, 0, 3, false },
        { { "6464646464646464", "6363636363636363", nullptr }, 1.0f, 4, 12, 6, 16, false },
        { { "0070000000000000", "0000000700000000", nullptr }, 0.4f, 4, 10, 1, 2, true },
        { { "00000", nullptr, nullptr }, 1.0f, 0, 0, 0, 0, false },
      },
      { 44, 50 }, { 0.5f, 0.75f }, { 0.05f, 0.15f }, { 0.15f, 0.35f }, 0,
      { 0.01f, 0.03f }, { 0.2f, 0.45f }, false, { 500, 900 }, { 0.03f, 0.06f },
      2, { 1200, 2200 }, { 8500, 10500 }, { 0.4f, 0.6f }, { 0.15f, 0.3f }, 0.75f,
      { 0.15f, 0.3f }, { 0, 0 }, { 0.05f, 0.2f }, { 0.35f, 0.6f }, { 0, 0 },
      "0205020502050205" },
    // ---------------------------------------------------------------- TRÄKLOSS
    { "TRÄKLOSS", "sjukt tajt botten, torra träklossar som dansar i trioler", 128,
      { 0.3f, 0.48f }, { 0.15f, 0.3f }, { 0.0f, 0.08f },
      {
        { { "X000X000X000X000", nullptr, nullptr }, 1.0f, 0, 0, 4, 4, false },
        { { "00X000X000X000X0", "0X0X0X0X0X0X0X0X", nullptr }, 1.0f, 0, 2, 4, 8, true },
        { { "0000000700000007", "0000070000000700", nullptr }, 0.7f, 2, 6, 1, 3, false },
        { { "0070007000700070", "0070007000700077", nullptr }, 1.0f, 2, 5, 4, 6, false },
        { { "706070607060706", "7060070607060706", "70607006070607" }, 1.2f, 3, 8, 4, 10, false },
        { { "0300030003000300", nullptr, nullptr }, 0.8f, 3, 8, 0, 4, false },
        { { "0000000000000000", nullptr, nullptr }, 1.0f, 0, 0, 0, 0, true },
        { { "50500", "5050500", nullptr }, 1.0f, 3, 8, 1, 3, false },
      },
      { 48, 54 }, { 0.12f, 0.2f }, { 0.04f, 0.1f }, { 0.06f, 0.14f }, 0,
      { 0.008f, 0.02f }, { 0.4f, 0.7f }, false, { 700, 1700 }, { 0.015f, 0.04f },
      4, { 1500, 2500 }, { 13000, 17000 }, { 0.3f, 0.45f }, { 0.05f, 0.12f }, 0.5f,
      { 0.35f, 0.55f }, { 0, 0 }, { 0, 0 }, { 0, 0.05f }, { 0, 0 },
      "0306030603060306" },
    };
    return s;
}
constexpr int kNumSkolor = 8;

} // namespace tomrum
