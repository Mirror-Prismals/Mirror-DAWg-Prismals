// House Finch - Rising then falling chirps
SinOsc s => ADSR env => dac;
(5::ms, 200::ms, 0.0, 100::ms) => env.set;
0.3 => s.gain;

// Finch call pattern
[78, 80, 78, 76, 74] @=> int notes[];

while (true) {
    for (0 => int i; i < notes.size(); i++) {
        Std.mtof(notes[i]) => s.freq;
        1 => env.keyOn;
        200::ms => now;
        1 => env.keyOff;
        100::ms => now;
        
        // Add slight randomness between notes
        Math.random2f(0.8, 1.2)::second => now;
    }
    
    // Longer pause between call sequences
    3.0::second => now;
}
