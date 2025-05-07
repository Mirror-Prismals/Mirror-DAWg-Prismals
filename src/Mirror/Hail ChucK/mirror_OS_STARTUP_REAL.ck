// Windows 95 Startup Sound Impression in ChucK

// Chord: F#, A#, D#, G# (Windows 95 original voicing)
[369.99, 466.16, 622.25, 830.61] @=> float chord[];

// Envelope parameters
0.18::second => dur attack;
1.2::second => dur hold;
1.6::second => dur release;

// Reverb
NRev reverb;
0.28 => reverb.mix;
reverb => dac;

// Arrays for oscillators and gains
SinOsc s[4];
TriOsc t[4];
Gain g[4];

for (0 => int i; i < chord.size(); i++) {
    s[i] => g[i] => reverb;
    t[i] => g[i];
    chord[i] => s[i].freq;
    chord[i] => t[i].freq;
    0.01 => s[i].gain;
    0.01 => t[i].gain;
    0 => g[i].gain;
}

// Envelope: fade in
0 => float env;
while (env <= 1.0) {
    for (0 => int i; i < g.size(); i++) {
        env => g[i].gain;
    }
    env + 0.01 => env;
    (attack / 100) => now;
}

// Hold
for (0 => int i; i < g.size(); i++) {
    1.0 => g[i].gain;
}
hold => now;

// Fade out
1.0 => env;
while (env >= 0.0) {
    for (0 => int i; i < g.size(); i++) {
        env => g[i].gain;
    }
    env - 0.01 => env;
    (release / 100) => now;
}

// Silence
for (0 => int i; i < g.size(); i++) {
    0 => g[i].gain;
}

// Bell sparkle melody: la fa re te di do ra!
[1244.51, 987.77, 830.61, 659.25, 698.46, 739.99, 783.9] @=> float bells[];
0.1 => float bellLevel;

// Durations
0.18::second => dur normalDur;   // original note length
0.06::second => dur tripletDur;  // fast triplet feel (adjust as needed)
0.18::second => dur restDur;     // rest after first three notes

// Play first three notes (normal pace)
for (0 => int i; i < 3; i++) {
    SinOsc b => Gain bellGain => reverb;
    bells[i] => b.freq;
    1.0 => b.gain;
    0 => bellGain.gain;

    // Envelope for bell (fade in)
    0 => float bellEnv;
    while (bellEnv <= 1.0) {
        (bellEnv * bellLevel) => bellGain.gain;
        bellEnv + 0.05 => bellEnv;
        0.005::second => now;
    }
    // Envelope for bell (fade out)
    1.0 => bellEnv;
    while (bellEnv >= 0.0) {
        (bellEnv * bellLevel) => bellGain.gain;
        bellEnv - 0.05 => bellEnv;
        0.01::second => now;
    }
    0 => bellGain.gain;
    normalDur => now;
}

// Rest
restDur => now;

// Play last four notes as quick triplets
for (3 => int i; i < bells.size(); i++) {
    SinOsc b => Gain bellGain => reverb;
    bells[i] => b.freq;
    1.0 => b.gain;
    0 => bellGain.gain;

    // Envelope for bell (fade in)
    0 => float bellEnv;
    while (bellEnv <= 1.0) {
        (bellEnv * bellLevel) => bellGain.gain;
        bellEnv + 0.1 => bellEnv; // faster envelope for short notes
        0.002::second => now;
    }
    // Envelope for bell (fade out)
    1.0 => bellEnv;
    while (bellEnv >= 0.0) {
        (bellEnv * bellLevel) => bellGain.gain;
        bellEnv - 0.1 => bellEnv;
        0.003::second => now;
    }
    0 => bellGain.gain;
    tripletDur => now;
}

// Let reverb tail ring out
6.5::second => now;
