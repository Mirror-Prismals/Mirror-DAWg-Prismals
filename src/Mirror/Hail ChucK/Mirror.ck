// Setup gain and reverb
Gain g => JCRev r => dac;
0.8 => r.mix;        // reverb wetness
0.5 => r.gain;
0.3 => g.gain;       // overall lower volume

// Chord frequencies — F#m7add11 style (Dire Dire Docks feeling)
[185.00, 220.00, 277.18, 329.63, 493.88] @=> float freqs[];

// Create oscillators for each note
SinOsc osc[freqs.size()];

for (0 => int i; i < freqs.size(); i++) {
    osc[i] => g;
    freqs[i] => osc[i].freq;
    0.12 => osc[i].gain;  // lower initial oscillator volume
}

// Play chord for 1.5 seconds before fade begins
1.5::second => now;

// Slowly fade tail out over 7 seconds exponentially
35 => int steps;             // more steps for smoother fade
7::second / steps => dur d;

for (0 => int j; j < steps; j++) {
    for (0 => int i; i < freqs.size(); i++) {
        osc[i].gain() * 0.85 => osc[i].gain;
    }
    d => now;
}

// Silence oscillators after fadeout
for (0 => int i; i < freqs.size(); i++) {
    0 => osc[i].gain;
}
