// Underwater FM bell E-piano inspired by Dire Dire, idiomatic ChucK syntax

4 => int N;
[87.31*4, 110*4, 130.81*4, 164.81*4] @=> float freqs[];
[0.2, -0.3, 0.1, -0.2] @=> float detunes[];

// Envelope
ADSR env => Gain master => dac;
env.set(500::ms, 1::second, 0.4, 2::second);
0.2 => master.gain;
env.keyOn();

// Vibrato oscillator
SinOsc vibrato => blackhole;
0.3 => vibrato.freq;

// Arrays
SinOsc mod[N];
SinOsc car[N];
Gain g[N];

// Proper ChucK loop style: 
for (0 => int i; i < N; i++)
{
    new SinOsc @=> mod[i]; 
    new SinOsc @=> car[i]; 
    new Gain @=> g[i];

    mod[i] => blackhole;
    car[i] => g[i] => env;
    0.4 => g[i].gain;
    
    (freqs[i] + detunes[i]) => float base;
    base => car[i].freq;
    2.0 * base => mod[i].freq;  // bell overtone ratio
}

// FM + vibrato modulation
spork ~ modLoop();

// Play chord for some seconds
20::second => now;
env.keyOff();
3::second => now;
me.exit();

fun void modLoop()
{
    float amt;
    while(true)
    {
        // Animate bell brightness/depth slowly (0..70Hz)
        Math.fabs(Math.sin(now / second * 0.1 * 2 * pi)) * 70 => amt;

        for (0 => int i; i < N; i++)
        {
            (freqs[i] + detunes[i]) 
            + vibrato.last() * 2 
            + mod[i].last() * amt 
            => car[i].freq;
        }
        10::ms => now;
    }
}
