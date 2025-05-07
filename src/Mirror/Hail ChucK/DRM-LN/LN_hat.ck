Noise n => BPF bp => LPF lpf => Gain sat => ADSR env => dac;

// Bright noisy snap
8000 => bp.freq;   
3 => bp.Q;

// Gentle LP cutoff
7000 => lpf.freq;

// Subtle overdrive for grit
1.2 => sat.gain; 

// Envelope: crisp & dry decay
env.set(0.5::ms, 2::ms, 0, 80::ms);
0.7 => env.gain;

// Noise amplitude
1 => n.gain;

// Fire!
env.keyOn();
90::ms => now;
env.keyOff();
50::ms => now;
