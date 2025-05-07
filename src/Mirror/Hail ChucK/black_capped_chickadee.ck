// Black-capped Chickadee - "chick-a-dee-dee-dee"
// Using layered oscillators for complex timbre
SawOsc s1 => ADSR env1 => LPF filt => dac;
TriOsc s2 => env1;
Noise n => env1;

// Set up filter and envelope
(5::ms, 100::ms, 0.0, 50::ms) => env1.set;
2000 => filt.freq;
2 => filt.Q;
0.2 => s1.gain => s2.gain;
0.1 => n.gain;

// Chickadee call pattern
fun void chickadeeCall(float pitch, dur length) {
    pitch => s1.freq => s2.freq;
    1 => env1.keyOn;
    length => now;
    1 => env1.keyOff;
    50::ms => now;
}

while (true) {
    // The "chick-a" (higher pitched)
    chickadeeCall(4000, 50::ms);
    50::ms => now;
    chickadeeCall(3500, 50::ms);
    50::ms => now;
    
    // The "dee-dee-dee" (descending)
    chickadeeCall(3000, 30::ms);
    30::ms => now;
    chickadeeCall(2800, 30::ms);
    30::ms => now;
    chickadeeCall(2600, 30::ms);
    
    // Random pause between calls (1-3 seconds)
    Math.random2f(1.0, 3.0)::second => now;
}
