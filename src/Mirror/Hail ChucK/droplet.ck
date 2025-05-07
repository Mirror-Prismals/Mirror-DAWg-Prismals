// Water Splash Simulation
// Combines filtered noise and a damped sine tone for a natural water sound.

JCRev reverb => dac;
reverb.mix(0.35);

// Helper: random duration between two values
fun dur rrandd(dur min, dur max) {
    return min + (max - min) * Std.rand2f(0, 1);
}

// Helper: random float between two values
fun float rrandf(float min, float max) {
    return Std.rand2f(min, max);
}

// Splash noise: simulates the chaotic breakup of a water droplet
fun void splashNoise() {
    Noise n => LPF filter => ADSR env => reverb;
    
    // Set gain so the noise is audible but not overwhelming
    0.35 => n.gain;
    // Use a low-pass filter to smooth out the noise:
    rrandf(600, 1200) => filter.freq;
    1.5 => filter.Q;
    
    // Use a slightly slower envelope for a natural impact:
    15::ms => dur attack;
    80::ms => dur decay;
    0 => float sustain;
    150::ms => dur release;
    env.set(attack, decay, sustain, release);
    
    env.keyOn();
    (attack + decay) => now;
    env.keyOff();
    release => now;
}

// Splash tone: a damped sine tone to mimic the water drop's resonance
fun void splashTone() {
    SinOsc tone => ADSR env => reverb;
    
    0.3 => tone.gain;
    // Choose a low-mid frequency for a soft "ring"
    rrandf(300, 500) => tone.freq;
    
    // A very quick attack and decay with a short release for a damped effect:
    10::ms => dur attack;
    40::ms => dur decay;
    0 => float sustain;
    120::ms => dur release;
    env.set(attack, decay, sustain, release);
    
    env.keyOn();
    (attack + decay) => now;
    env.keyOff();
    release => now;
}

// Combine both layers into one splash event.
fun void splash() {
    spork ~ splashNoise();
    spork ~ splashTone();
}

// Infinite loop: trigger splash events at randomized intervals.
while(true) {
    splash();
    rrandd(800::ms, 1800::ms) => dur pause;
    pause => now;
}
