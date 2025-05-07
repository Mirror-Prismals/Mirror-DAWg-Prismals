// CHUCK'S mobile OS PING DESIGNER 🎛️
// === CUSTOMIZE THESE ===
1660    => float pingFreq;   // Try 440 (A4), 784 (G5), or 1046 (C6)
1150    => float filterCutoff; // Lower = darker (try 500 to 2000)
0.8    => float pingVolume;  // 0.0 to 1.0
50::ms => dur pingDecay;     // Short = click, long = bloopy

// === ENGINE ===
SinOsc s => LPF filter => ADSR env => dac;
env.set(1::ms, pingDecay, 0.0, 1::ms);
filterCutoff => filter.freq;
0.1 => filter.Q;
pingVolume => s.gain;

// === PLAY PING ===
pingFreq => s.freq;
1 => env.keyOn;
1::ms => now;
1 => env.keyOff;
pingDecay => now;
