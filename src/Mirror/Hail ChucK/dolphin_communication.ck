// Dolphin Communication, 100% ChucK idiomatic

fun void whistle(float freqStart, float freqEnd, dur t) {
    SinOsc s => Gain g => dac;
    0.11 => g.gain;
    (t / 10::ms) $ int => int steps;
    0 => int idx;
    while (idx < steps) {
        freqStart + (freqEnd - freqStart) * (idx $ float / steps) => s.freq;
        10::ms => now;
        idx++;
    }
    0 => g.gain;
}

fun void chirp(float base, int count) {
    SinOsc s => Gain g => dac;
    0.09 => g.gain;
    0 => int i;
    while (i < count) {
        (base + Math.random2f(-200, 200)) => s.freq;
        7::ms => now;
        i++;
    }
    0 => g.gain;
}

fun void clickBurst(int count) {
    0 => int i;
    while (i < count) {
        Noise n => LPF f => Gain g => dac;
        (4000 + Math.random2(0, 8000)) => f.freq;
        0.13 => g.gain;
        4::ms => now;
        0 => g.gain;
        10::ms => now;
        i++;
    }
}

fun void dolphinPhrase() {
    spork ~ whistle(11000, 18000, 160::ms);
    60::ms => now;
    spork ~ chirp(13500, 18);
    55::ms => now;
    spork ~ clickBurst(Math.random2(3,8));
    35::ms => now;
    spork ~ whistle(18000, 9500, 90::ms);
    50::ms => now;
    spork ~ chirp(12000, 15);
}

dolphinPhrase();
0.7::second => now;
