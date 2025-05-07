// Dire Dire / Jolly Roger Bay style pad, 2 octaves higher than before

4 => int numVoices;

// Root chord frequencies F major7 (F2, A2, C3, E3)
[ 87.31, 110.0, 130.81, 164.81 ] @=> float baseFreqs[];

// Arrays for oscillators and gains
SinOsc oscArray[numVoices];
Gain gainArray[numVoices];

// Detune amounts in Hz
[ 0.2, -0.3, 0.4, -0.4 ] @=> float detunes[];

// Master gain
Gain masterGain;
0.1 => masterGain.gain;
masterGain => dac;

// Calculate octave-up frequencies and initialize all UGens
int i;
for (0 => i; i < numVoices; i++)
{
    SinOsc s;     new SinOsc @=> s;
    Gain g;       new Gain @=> g;
    s => g => masterGain;
    s @=> oscArray[i];
    g @=> gainArray[i];

    0.3 => g.gain;
    (baseFreqs[i] * 4.0 + detunes[i]) => s.freq;
}

// LFO for vibrato/pulse
SinOsc lfo;  new SinOsc @=> lfo;
0.12 => lfo.freq;
lfo => blackhole;

// Main loop
while(true)
{
    for (0 => i; i < numVoices; i++)
    {
        float f;  (baseFreqs[i] * 4.0 + detunes[i]) => f;
        f + lfo.last() * 1.5 => oscArray[i].freq;
        (lfo.last() * 0.1) + 0.3 => gainArray[i].gain;
    }
    20::ms => now;
}
