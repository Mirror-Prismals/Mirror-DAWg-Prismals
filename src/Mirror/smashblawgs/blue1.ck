// Ground Right Click (Blue) - Energy Attack Sound (FM Synthesis, Corrected)

SinOsc mod => blackhole;
SinOsc car => Gain g => dac;

440 => float baseFreq;
220 => mod.freq;
0.16 => g.gain;

// Envelope and shimmer not used in this version
0.01::second => dur step;

// Main burst with FM
for (0 => int i; i < 12; i++) {
    (220 + 60*Math.cos(i*0.8)) => mod.freq;
    0.16 * Math.pow(0.7, i) => g.gain;
    // FM: modulate carrier frequency in real time
    step / 100 => dur sampleStep;
    for (0 => int j; j < 100; j++) {
        (baseFreq + 60 * mod.last()) => car.freq;
        sampleStep => now;
    }
}

// Add a quick, bright "sparkle"
TriOsc sparkle => Gain sg => dac;
1800 => sparkle.freq;
0.1 => sg.gain;
0.03::second => now;
0 => sg.gain;

// Fade out
0 => g.gain;
