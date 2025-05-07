// House Sparrow - Rapid, chaotic chirps
Noise n => BPF filt => ADSR env => Gain g => dac;
TriOsc s => env;

// Filter settings (bright chirps)
2000 => filt.freq;
500 => filt.Q;
0.3 => g.gain;

// Envelope - sharp attacks
(5::ms, 100::ms, 0.0, 50::ms) => env.set;

// Main chirp function
fun void chirp(float baseFreq, float freqVar, dur length) {
    // Randomize frequency slightly
    baseFreq + Math.random2f(-freqVar, freqVar) => s.freq;
    // Mix noise and oscillator for texture
    Math.random2f(0.1, 0.3) => n.gain;
    Math.random2f(0.2, 0.5) => s.gain;
    
    1 => env.keyOn;
    length => now;
    1 => env.keyOff;
    50::ms => now;
}

while (true) {
    // Rapid burst of 3-6 chirps
    Math.random2(3, 6) => int numChirps;
    for (0 => int i; i < numChirps; i++) {
        chirp(2000, 500, 50::ms); // High, sharp chirps
        Math.random2f(0.05, 0.2)::second => now;
    }
    
    // Occasionally add a trill
    if (Math.random2(0, 3) == 0) {
        3 => int trillNotes;
        for (0 => int i; i < trillNotes; i++) {
            chirp(1500 + i*200, 300, 80::ms);
            80::ms => now;
        }
    }
    
    // Pause between call sequences
    Math.random2f(0.5, 2.0)::second => now;
}
