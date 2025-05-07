// A lush, soft Mario 64 inspired sine pad with vibrato

// parameters
4 => int voices;
Gain master => dac;
0.1 => master.gain;

SinOsc lfo => blackhole; 
0.2 => lfo.freq; // slow vibrato

SinOsc osc[voices];
Gain gain[voices];
[87.31*4, 110*4, 130.81*4, 164.81*4] @=> float freqs[];

// slight detune offsets
[0.2, -0.3, 0.1, -0.2] @=> float detunes[];

// attach & set
for(int i; i < voices; i++)
{
    new SinOsc @=> osc[i];
    new Gain @=> gain[i];
    osc[i] => gain[i] => master;
    0.4 => gain[i].gain;

    freqs[i] + detunes[i] => osc[i].freq;
}

// fade in
0 => float vol;
while(vol < 1.0)
{
    vol + 0.01 => vol;
    for(int i; i < voices; i++) (vol * 0.4) => gain[i].gain;
    50::ms => now;
}

// vibrato loop, forever
while(true){
    for(int i; i < voices; i++)
    {
        (freqs[i] + detunes[i]) + lfo.last()*2 => osc[i].freq;
    }
    10::ms => now;
}
