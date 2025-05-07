// Common Grackle - Raspy calls (no distortion)
SawOsc voice => ADSR env => LPF filt => NRev rev => dac;

// Set up sound
(5::ms, 200::ms, 0.0, 100::ms) => env.set;
2000 => filt.freq;
0.1 => rev.mix;
0.5 => voice.gain;

// Grackle call patterns
fun void highScreech() {
    Math.random2f(1500, 3000) => voice.freq;
    1 => env.keyOn;
    Math.random2f(0.1, 0.3)::second => now;
    1 => env.keyOff;
}

fun void lowCreak() {
    Math.random2f(400, 800) => voice.freq;
    1 => env.keyOn;
    Math.random2f(0.2, 0.5)::second => now;
    1 => env.keyOff;
}

// Main loop
while (true) {
    // Alternate between screeches and creaks
    if (Math.random2(0, 1)) {
        highScreech();
    } else {
        lowCreak();
    }
    // Natural pauses between calls
    Math.random2f(0.5, 2.0)::second => now;
}
