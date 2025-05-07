// Damped Resonator Bank for Fluid Water Simulation

// Global reverb to add space and blend events
JCRev reverb => dac;
reverb.mix(0.35);

// Helper function: returns a random duration between two durations
fun dur rrandd(dur min, dur max) {
    return min + (max - min) * Std.rand2f(0, 1);
}

// Helper function: returns a random float between two values
fun float rrandf(float min, float max) {
    return Std.rand2f(min, max);
}

// Resonator: a damped sine burst representing a micro-plop on water
fun void resonator() {
    SinOsc osc => ADSR env => Gain g => reverb;
    
    // Set a lower frequency range for a warmer, less chime-like tone
    rrandf(250, 400) => osc.freq;
    0.1 => osc.gain; // Lower gain for subtle events
    
    // Envelope settings:
    5::ms => dur attack;            // Extremely quick attack
    rrandd(150::ms, 300::ms) => dur decay;   // Short decay for a quick drop-off
    0 => float sustain;             // No sustain for a percussive plop
    50::ms => dur release;          // Rapid release to end the sound
    
    env.set(attack, decay, sustain, release);
    
    env.keyOn();
    (attack + decay) => now;        // Let the initial burst play out
    env.keyOff();
    release => now;                 // Finish the envelope
}

// Main loop: continuously trigger resonators at short, random intervals.
// Overlapping events should blend into a fluid, shifting texture.
while (true) {
    spork ~ resonator();
    rrandd(80::ms, 200::ms) => now;
}
