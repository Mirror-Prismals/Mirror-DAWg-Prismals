// LinnDrum-style digital snare: tight filtered noise burst

Noise n => BPF bandF => LPF hiCut => ADSR env => dac;

// bright snap bandpass
2000 => bandF.freq;
2.5  => bandF.Q;

// soften ultra fizz
5000 => hiCut.freq;

// Snappy envelope settings
env.set(1::ms, 10::ms, 0, 120::ms);

// Noise amplitude
1 => n.gain;
// Envelope amplitude
0.9 => env.gain;

// trigger snare
env.keyOn();
150::ms => now;
env.keyOff();
100::ms => now;
