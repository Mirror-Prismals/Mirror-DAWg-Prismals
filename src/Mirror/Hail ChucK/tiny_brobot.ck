// "Tiny Robot Wakes Up and Sings" - ChucK 
// Boot-up beeps: Stepped sine and glitch bursts
fun void bootBeep(float freq, dur t) {
    SinOsc s => Gain g => dac;
    0.09 => g.gain;
    freq => s.freq;
    t => now;
    0 => g.gain;
}

fun void glitchBurst(dur t) {
    Noise n => LPF f => Gain g => dac;
    1500 => f.freq;
    0.07 => g.gain;
    t => now;
    0 => g.gain;
}

// Boot sequence
[600, 800, 500, 1000, 750] @=> int freqs[];
for (0 => int i; i < freqs.cap(); i++) {
    bootBeep(freqs[i], 40::ms);
    if (i % 2 == 0) glitchBurst(15::ms);
}

// System online sweep
SinOsc sweep => Gain sg => dac;
0.08 => sg.gain;
400 => sweep.freq;
for (0 => int i; i < 45; i++) {
    (400 + i * 30) => sweep.freq;
    8::ms => now;
}
0 => sg.gain;

// Brief pause
120::ms => now;

// Celebration “singing” note with vibrato
SinOsc sing => Gain g => dac;
0.13 => g.gain;
1100 => sing.freq;

fun void vibrato() {
    while (true) {
        (Math.sin(now/second*25.0) * 18 + 1100) => sing.freq;
        4::ms => now;
    }
}
spork ~ vibrato();

400::ms => now;
0 => g.gain;

50::ms => now; // let the tail ring
