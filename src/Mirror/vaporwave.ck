// FX chain
JCRev reverb => Gain master => dac;
0.9 => reverb.mix;
0.08 => master.gain;

// 5 lush chord oscillators
SqrOsc s[5];
for(0 => int i; i < 5; i++) {
    s[i] => reverb;  0.03 => s[i].gain;
}

// 2 bass octaves extra under the root
SqrOsc bass1 => reverb;
SqrOsc bass2 => reverb;
0.02 => bass1.gain;
0.02 => bass2.gain;

// helper
fun void maj9plusbass(float root) {
    root            => s[0].freq;
    root * 1.25     => s[1].freq;
    root * 1.5      => s[2].freq;
    root * 1.875    => s[3].freq;
    root * 2.25     => s[4].freq;

    root / 2        => bass1.freq;    // one octave beneath
    root / 4        => bass2.freq;    // two octaves beneath
}

// Chord roots: starting from A3 (220Hz!!)
220 => float A;                                      // A3
A * Math.pow(2, 7/12.0) => float E;                  // E4
A * Math.pow(2,-4/12.0) => float Fsharp;             // F#3
A * Math.pow(2,-7/12.0)*2 => float D;                // D4

// leaning slow vaporwave tempo
1.5::second => dur beat;
4*beat => dur bar;

// Loop progression
while(true) {
    maj9plusbass(A);      bar => now;
    maj9plusbass(E);      bar => now;
    maj9plusbass(Fsharp); bar => now;
    maj9plusbass(D);      bar => now;
}
