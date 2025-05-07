// EK Style EDM Snare - One shot

// Noise burst
Noise n => BPF lp => ADSR env1 => dac;

// Tuned transient (body punch)
SinOsc s => ADSR env2 => dac;

// Set noise filter cutoff (a bit nasal)
3000 => lp.freq;
2 => lp.Q;

// Set envelopes
0 => env1.keyOn;

// Quick attack, fast decay on noise
1::ms => env1.attackTime;
100::ms => env1.decayTime;
0.0 => env1.sustainLevel;
50::ms => env1.releaseTime;

// For body transient envelope
1::ms => env2.attackTime;
100::ms => env2.decayTime;
0.0 => env2.sustainLevel;
50::ms => env2.releaseTime;

// Set pitch thump
200 => s.freq; // Tune to taste
0.8 => s.gain; // transient loudness

// Noise loudness
0.5 => env1.gain;

// Fire envelopes
env1.keyOn();
env2.keyOn();

// Wait for envelope to decay
200::ms => now;

// Silence (optional, clean up)
env1.keyOff();
env2.keyOff();
