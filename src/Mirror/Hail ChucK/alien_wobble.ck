4 => int N;
[87.31*4, 110*4, 130.81*4, 164.81*4] @=> float freqs[];
[0.2, -0.3, 0.1, -0.2] @=> float detunes[];

SinOsc mod[N];
SinOsc car[N];
Gain g[N];

ADSR env => Gain master => dac;
env.set(3::second, 1::second, 0.5, 3::second);
0.15 => master.gain;
env.keyOn();

SinOsc vibrato => blackhole;
0.15 => vibrato.freq;

// Create voices
for (0=>int i; i<N; i++){
    new SinOsc @=> mod[i];
    new SinOsc @=> car[i];
    new Gain @=> g[i];
    
    mod[i] => blackhole;
    car[i] => g[i] => env;
    
    freqs[i] + detunes[i] => car[i].freq;
    1.5 * (freqs[i] + detunes[i]) => mod[i].freq;
    0.4 => g[i].gain;
}

// FM modulating thread
spork ~ modulate();

// Play for 20 seconds
20::second => now;

// Fade out
env.keyOff();
3::second => now;

// Done
me.exit();

fun void modulate() {
    while(true) {
        for (0=>int i; i<N; i++){
            freqs[i] + detunes[i]
            + vibrato.last()*2
            + mod[i].last()*25 => car[i].freq;
        }
        10::ms => now;
    }
}
