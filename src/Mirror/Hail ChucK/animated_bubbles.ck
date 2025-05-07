// Animated Bubble Stream – fixed syntax version

// Setup a reverb for natural watery space
JCRev rev => dac;
0.3 => rev.mix;

// Helper: random dur between min and max
fun dur rrandd(dur min, dur max) {
    return min + (max - min) * Std.rand2f(0,1);
}

// Helper: random float
fun float rrandf(float min, float max) {
    return Std.rand2f(min,max);
}

// Spawn a single bubble event
fun void bubble() {
    SinOsc s => ADSR env => Pan2 p => rev;
    
    // Optional pop noise
    Noise n => BPF f => ADSR nEnv => p;
    0.1 => n.gain;
    2000 => f.freq;
    4 => f.Q;
    0.2 => nEnv.gain;
    2::ms => dur nAtk;
    30::ms => dur nDec;
    0 => float nSus;
    40::ms => dur nRel;
    nEnv.set(nAtk, nDec, nSus, nRel);
    
    // Envelope times for sine
    rrandd(3::ms, 10::ms) => dur attack;
    rrandd(50::ms, 150::ms) => dur decay;
    0 => float sustain;
    rrandd(30::ms, 100::ms) => dur release;
    env.set(attack, decay, sustain, release);
    
    // base freq & glide
    rrandf(250, 800) => float baseFreq;
    rrandf(100, 400) => float glideHz;
    0.1 => s.gain;
    
    // Random pan
    rrandf(-1,1) => p.pan;
    
    // Start envelopes
    nEnv.keyOn();
    env.keyOn();
    
    // Pitch glide over attack+decay
    attack + decay => dur totalTime;
    (totalTime / 5::ms) $ int => int steps;
    for(0 => int i; i < steps; i++) {
        baseFreq + ((glideHz * (i $ float)/steps)) => s.freq;
        5::ms => now;
    }
    
    env.keyOff();
    nEnv.keyOff();
    release > nRel ? release : nRel => now;
}

// Infinite loop: spawn bubble clusters
while(true) {
    2 + Std.rand2(0,2) => int bubbleCount;
    for(0 => int i; i < bubbleCount; i++) {
        spork ~ bubble();
        rrandd(10::ms, 30::ms) => now;
    }
    // pause between clusters
    rrandd(100::ms, 400::ms) => now;
}
