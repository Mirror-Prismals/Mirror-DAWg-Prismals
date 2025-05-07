// iOS BELL NOTIFICATION - CRYSTAL CLEAN
// === BELL SETTINGS ===
1660 => float baseFreq;  // Your sweet spot
0.8   => float volume;   // 0.0 - 1.0
80::ms => dur decayTime; // Bell fade-out

// === BELL ENGINE ===
SinOsc s1 => Gain bellMix => LPF filter => NRev rev => dac;
SinOsc s2 => bellMix; SinOsc s3 => bellMix;

// Harmonic series (bell-like)
baseFreq => s1.freq;
baseFreq * 1.5 => s2.freq; // Perfect fifth
baseFreq * 2.0 => s3.freq; // Octave

// Soften
1150 => filter.freq;
0.05 => filter.Q;
0.15 => rev.mix;
volume => bellMix.gain;

// === PLAY BELL ===
1 => s1.gain => s2.gain => s3.gain;
now + decayTime => time end;

while(now < end) {
    // Exponential decay
    (end - now) / decayTime => float decay;
    decay * volume => bellMix.gain;
    5::ms => now;
}
