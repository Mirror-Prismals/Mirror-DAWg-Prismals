// ========== Vowel Class & Setup ==========
class Vowel {
    float f1;
    float f2;
    float f3;
    string name;
}

fun void setVowel(Vowel @v, float f1, float f2, float f3, string name) {
    f1 => v.f1;
    f2 => v.f2;
    f3 => v.f3;
    name => v.name;
}

// ========== Synthesis Function ==========
fun void sing(Vowel v, float pitchHz) {
    SinOsc base => dac;
    SinOsc f1 => dac;
    SinOsc f2 => dac;
    SinOsc f3 => dac;

    pitchHz => base.freq;
    v.f1 => f1.freq;
    v.f2 => f2.freq;
    v.f3 => f3.freq;

    0.3 => base.gain;
    0.15 => f1.gain;
    0.1 => f2.gain;
    0.1 => f3.gain;

    400::ms => now;

    base =< dac;
    f1 =< dac;
    f2 =< dac;
    f3 =< dac;
}

// ========== Main Control ==========
KBHit kb;

Vowel ah;  setVowel(ah, 800, 1200, 2500, "AH");
Vowel ee;  setVowel(ee, 300, 2200, 3000, "EE");
Vowel oh;  setVowel(oh, 400, 1000, 2400, "OH");
Vowel eh;  setVowel(eh, 500, 1700, 2500, "EH");
Vowel ih;  setVowel(ih, 400, 2000, 2600, "IH");
Vowel oo;  setVowel(oo, 350, 800, 2400, "OO");

<<< "Press keys: a e i o u h to trigger vowels!" >>>;

while (true) {
    kb => now;
    kb.getchar() => int c;

    if (c == 97) { // 'a'
        spork ~ sing(ah, 220.0);
    } else if (c == 101) { // 'e'
        spork ~ sing(ee, 330.0);
    } else if (c == 105) { // 'i'
        spork ~ sing(ih, 392.0);
    } else if (c == 111) { // 'o'
        spork ~ sing(oh, 294.0);
    } else if (c == 117) { // 'u'
        spork ~ sing(oo, 262.0);
    } else if (c == 104) { // 'h'
        spork ~ sing(eh, 349.0);
    } else {
        <<< "Unmapped key:", c >>>;
    }
}
