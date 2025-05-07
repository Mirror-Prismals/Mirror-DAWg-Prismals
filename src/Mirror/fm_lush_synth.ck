// Better Dire Dire Docks pad inspired FM lush synth — working ChucK version!

// ------------ Setup --------------

// Number of chord notes (Fmaj7)
4 => int voices;

// Frequencies, two octaves higher
[87.31*4, 110*4, 130.81*4, 164.81*4] @=> float freqs[];

// Detune offsets to thicken
[0.2, -0.3, 0.3, -0.2] @=> float detunes[];

// Master gain
Gain master => dac;
0.1 => master.gain;

// LFO vibrato
SinOsc lfo => blackhole;
0.2 => lfo.freq;

// Envelope (slow fade attack, long release)
ADSR env => master;
env.set(3::second, 1::second, 0.5, 3::second);
env.keyOn();

// Arrays for modulation & carrier
SinOsc modArray[voices];
SinOsc carArray[voices];
Gain carGain[voices];

for(0 => int i; i < voices; i++) {
    new SinOsc @=> modArray[i];
    new SinOsc @=> carArray[i];
    new Gain @=> carGain[i];
    
    modArray[i] => blackhole;
    carArray[i] => carGain[i] => env;
    
    // Base freqs + detune
    freqs[i] + detunes[i] => float f;
    f => carArray[i].freq;
    2.0 * f => modArray[i].freq; // ratio 2:1 typical FM

    0.2 => carGain[i].gain; // modest voice gain
}

// -------------- Loop --------------
while(true)
{
    for(0 => int i; i < voices; i++) {
        // Carrier frequency + vibrato + FM modulation
        (freqs[i] + detunes[i]) 
        + lfo.last() * 2
        + modArray[i].last() * 80 // FM amount
        => carArray[i].freq;
    }
    10::ms => now;
}
