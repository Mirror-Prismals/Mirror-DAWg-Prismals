// Swoosh (body flying through air)
Noise whoosh => LPF whooshFilt => Gain whooshVol => dac;
2000 => whooshFilt.freq;
0.02 => whooshVol.gain;

0.1::second => now; // brief whoosh

0 => whooshVol.gain;

// SPLASH (impact)
Noise splash => LPF splashFilt => Gain splashVol => JCRev verb => dac;
10000 => splashFilt.freq;
0.5 => splashVol.gain;
0.7 => verb.mix;

// Use a function for splash burst
fun void doSplash() {
    0.7 => splashVol.gain;
    80::ms => now;
    0.4 => splashVol.gain;
    100::ms => now;
    0.15 => splashVol.gain;
    100::ms => now;
    0 => splashVol.gain;
}
spork ~ doSplash();

// Bubbles/Ripples
fun void bubbles() {
    for (0 => int i; i < 7; i++) {
        Noise n => LPF f => Gain g => dac;
        Math.random2(1000, 2500) => f.freq;
        0.02 => g.gain;
        40::ms => now;
        0 => g.gain;
        80::ms => now;
    }
}
spork ~ bubbles();

1.5::second => now;
