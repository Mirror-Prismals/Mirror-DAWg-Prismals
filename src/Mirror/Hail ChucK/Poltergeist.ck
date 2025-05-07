// Resonator Bank Fluid Simulation

// Global reverb to add depth and space
JCRev reverb => dac;
reverb.mix(0.3);

// Helper function: returns a random duration between two values
fun dur rrandd(dur min, dur max) {
    return min + (max - min) * Std.rand2f(0, 1);
}

// Helper function: returns a random float between two values
fun float rrandf(float min, float max) {
    return Std.rand2f(min, max);
}

// A resonator: a sine oscillator with an envelope, representing one "mode" of the water surface.
fun void resonator() {
    // Create a sine oscillator, envelope, and a gain unit
    SinOsc osc => ADSR env => Gain g => reverb;
    
    // Choose a random frequency in a range that might feel "water-like"
    rrandf(300, 800) => osc.freq;
    // Set oscillator gain low for subtlety
    0.15 => osc.gain;
    
    // Envelope settings:
    10::ms => dur attack;                        // very fast attack
    rrandd(300::ms, 800::ms) => dur decay;         // moderate decay time
    0.6 => float sustain;                        // sustain level
    100::ms => dur release;                        // short release
    
    env.set(attack, decay, sustain, release);
    
    // Trigger the envelope
    env.keyOn();
    (attack + decay) => now;
    env.keyOff();
    release => now;
}

// Main loop: continuously trigger resonators at random intervals,
// so that overlapping events create a fluid, evolving sound texture.
while (true) {
    spork ~ resonator();
    // New resonator every 50-150 milliseconds
    rrandd(50::ms, 150::ms) => now;
}
