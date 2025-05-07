// Set up a reverb effect for added space
JCRev reverb => dac;
reverb.mix(0.3);

// Helper function: returns a random duration between min and max
fun dur rrandd( dur min, dur max ) {
    return min + (max - min) * Std.rand2f(0, 1);
}

// Helper function: returns a random float between min and max
fun float rrandf( float min, float max ) {
    return Std.rand2f(min, max);
}

// Define a function that creates one splash event
fun void splash() {
    // Create a noise burst with its own envelope and filter
    Noise n => ADSR env => LPF filter => reverb;
    
    // Randomize filter cutoff (in Hz) for the splash
    rrandf(800, 3000) => filter.freq;
    
    // Randomize envelope parameters (attack, decay, sustain, release)
    rrandd(0.001::second, 0.005::second) => dur attack;
    rrandd(0.05::second, 0.3::second)  => dur decay;
    // A percussive splash effect with no sustain
    0 => float sustain;
    rrandd(0.1::second, 0.5::second)  => dur release;
    env.set(attack, decay, sustain, release);
    
    // Trigger the envelope
    env.keyOn();
    (attack + decay) => now;
    env.keyOff();
    release => now;
    
    // No explicit cleanup needed; unreferenced UGens are garbage collected.
}

// Infinite loop: spork a splash and wait a random duration before the next one
while( true ) {
    spork ~ splash();
    rrandd(50::ms, 500::ms) => dur waitTime;
    waitTime => now;
}
