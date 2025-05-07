// Correct, minimal FM piano in ChucK — no bugs!  
SinOsc mod => blackhole;    // modulator connected only to blackhole  
SinOsc car => Envelope env => dac;   // carrier to output through envelope  
  
JCRev rev;  
env => rev => dac;  
  
// parameters  
2 => float modRatio;        // modulator ratio  
200 => float index;         // deviate by ±index Hz  
  
// tail reverb  
0.3 => rev.mix;  
  
// pitches  
[440., 523.25, 659.25, 880.] @=> float freqs[];  
  
while(true) {  
    for (0 => int i; i < freqs.cap(); i++) {  
        freqs[i] => float base;  
  
        base * modRatio => mod.freq;  
        base => car.freq; // start at neutral freq  
  
        // Envelope attack  
        env.value(0);  
        env.target(1.0);  
        env.duration(20::ms);  
        20::ms => now;  
  
        // sustain duration in samples, to do sample-accurate FM  
        250::ms => dur sustainDur;  
        (sustainDur / samp) $ int => int samps;  
  
        for (0 => int j; j < samps; j++) {  
            (base + mod.last() * index) => car.freq;  
            1::samp => now;  
        }  
  
        // Envelope release  
        env.target(0.0);  
        env.duration(40::ms);  
        40::ms => now;  
    }  
}
