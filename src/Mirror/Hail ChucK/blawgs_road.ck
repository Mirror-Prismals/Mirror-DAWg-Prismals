// Tense, synthetic orchestral string stabs inspired by Bowser's Road in SM64

// Number of notes in the chord
int voices;
4 => voices;

// Create oscillator, envelope, and gain arrays
SawOsc s[voices];
ADSR env[voices];
Gain g[voices];

// Connect to output
Gain master;
master => dac;
0.7 => master.gain;

// Chord cluster frequencies (A3, C4, D4, F4)
float freqs[4];
220.0 => freqs[0];
261.63 => freqs[1];
293.66 => freqs[2];
349.23 => freqs[3];

// Vibrato settings
float vibFreq;
6.0 => vibFreq;
float vibDepth;
5.0 => vibDepth; // Hz deviation
float t;
0.0 => t;

int i;
for (0 => i; i < voices; i++)
{
    s[i] => env[i] => g[i] => master;
    0.3 => g[i].gain;
    freqs[i] => s[i].freq;
    10::ms => env[i].attackTime;
    50::ms => env[i].decayTime;
    0.2 => env[i].sustainLevel;
    100::ms => env[i].releaseTime;
}

// Vibrato function updates all osc freqs with LFO
fun void vibrato()
{
    int j;
    float lfo;
    float baseFreq;
    while (true)
    {
        // sine LFO at vibFreq hz
        vibFreq * 2 * 3.14159265359 * t => lfo;
        Math.sin(lfo) * vibDepth => lfo;
        for (0 => j; j < voices; j++)
        {
            freqs[j] + lfo => baseFreq;
            baseFreq => s[j].freq;
        }
        t + (5.0 / 1000.0) => t; // step time (5ms)
        5::ms => now;
    }
}

// Start vibrato updater
spork ~ vibrato();

// Chord stabs repeat
int count;
int stabs;
8 => stabs;

for (0 => count; count < stabs; count++)
{
    for (0 => i; i < voices; i++)
    {
        env[i].keyOn();
    }
    200::ms => now;

    for (0 => i; i < voices; i++)
    {
        env[i].keyOff();
    }
    150::ms => now;
}
