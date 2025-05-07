// Mario 64-style bright organ (inside castle / slider feel)
// same as great_organ.ck but mono
// Global gain
Gain master => dac;
0.4 => master.gain;

// Envelope with punchy attack
ADSR env => master;
env.set(10::ms, 50::ms, 0.7, 200::ms);
env.keyOn();

// Vibrato/Leslie modulator
SinOsc vibrato => blackhole;
5.5 => vibrato.freq;   // mod speed (approximate Leslie)
// try 4~6 Hz

// Organ harmonic drawbars (sine waves + "click")
7 => int N;
[1, 2, 3, 4, 5, 6, 8] @=> int ratios[];
[1.0, 0.4, 0.2, 0.12, 0.1, 0.06, 0.05] @=> float amps[];

// Osc arrays
SinOsc oscs[N];

// Build the harmonics
for(0 => int i; i < N; i++)
{
    new SinOsc @=> oscs[i];
    oscs[i] => env;
    amps[i] => oscs[i].gain;
}

// Fundamental pitch (M64’s organ root = A4, but transpose up/down if desired)
220.0 => float baseFreq;

// Harmonic click attack (transient)
Impulse click => Gain clickEnv => env;
clickEnv.gain(0.15);
click => clickEnv;   // percussive click start
1 => click.next;

// Vibrato sweep + Leslie swirl
spork ~ vibratoSweep();

// Duration
4::second => now;
// fade out
env.keyOff();
2::second => now;

// --- Function for mod sweep ---
fun void vibratoSweep() {
    while(true)
    {
        for(0 => int i; i < N; i++)
        {
            (baseFreq * ratios[i]) + vibrato.last() * 6 => oscs[i].freq;
        }
        10::ms => now;
    }
}
