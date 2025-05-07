// Northern Cardinal - Pure whistled tones
SinOsc s => ADSR env => NRev rev => dac;
(10::ms, 800::ms, 0.0, 100::ms) => env.set;
0.05 => rev.mix;
0.4 => s.gain;

// Cardinal's classic 3-note phrase
fun void cheerCall() {
    [78, 76, 74] @=> int notes[]; // MIDI notes (E5, D#5, D5)
    for (0 => int i; i < notes.size(); i++) {
        // Slide between notes for naturalism
        SlideTo(Std.mtof(notes[i]), 50::ms);
        1 => env.keyOn;
        300::ms => now;
        1 => env.keyOff;
        100::ms => now;
    }
}

// Alternative "purdy" phrase
fun void purdyCall() {
    [74, 77, 76] @=> int notes[]; // D5, F5, E5
    for (0 => int i; i < notes.size(); i++) {
        SlideTo(Std.mtof(notes[i]), 50::ms);
        1 => env.keyOn;
        400::ms => now;
        1 => env.keyOff;
        100::ms => now;
    }
}

// Smooth pitch slides
fun void SlideTo(float targetFreq, dur slideTime) {
    s.freq() => float currentFreq;
    (targetFreq - currentFreq) / (slideTime / 10::ms) => float step;
    
    for (0::ms => dur t; t < slideTime; 10::ms +=> t) {
        currentFreq + step => s.freq;
        10::ms => now;
    }
    targetFreq => s.freq;
}

// Main loop
while (true) {
    // Alternate between call types
    if (Math.random2(0, 1)) {
        cheerCall(); // "cheer-cheer-cheer"
    } else {
        purdyCall(); // "purdy-purdy-purdy"
    }
    // Natural pause between phrases
    2.0::second => now;
}
