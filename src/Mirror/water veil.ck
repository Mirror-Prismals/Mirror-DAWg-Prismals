// Underwater 'Water Veil' lush synth pad
4 => int N;
[87.31*4, 110*4, 130.81*4, 164.81*4] @=> float freqs[];
[0.7, -0.5, 0.9, -0.8] @=> float detunes[];

// Main gain
Gain master => dac;
0.2 => master.gain;

// Arrays of oscillators & filters
SqrOsc sqr[N];
Gain g[N];
LPF lpf[N];

// Build chord voices
for (0 => int i; i < N; i++)
{
    new SqrOsc @=> sqr[i];
    new Gain @=> g[i];
    new LPF @=> lpf[i];

    sqr[i] => g[i] => lpf[i] => master;

    (freqs[i] + detunes[i]) => sqr[i].freq;
    0.0 => g[i].gain; // initial fade-in = 0 gain
    (freqs[i] + detunes[i]) * 2 => lpf[i].freq; // cutoff above fundamental
}

// Vibrato
SinOsc vibrato => blackhole;
0.15 => vibrato.freq;

// Filter sweep
SinOsc sweep => blackhole;
0.02 => sweep.freq;

// Noise for water texture
Noise n => BPF bpf => Gain ngain => master;
800 => bpf.freq;
40 => bpf.Q;
0.0 => ngain.gain; // start at 0 for fade-in

// Fade in over ~5 seconds
0.0 => float vol;
while (vol <= 1.0)
{
    for (0 => int i; i < N; i++)
    {
        vol * 0.2 => g[i].gain; // up to 0.2
    }
    vol * 0.03 => ngain.gain;  // subtle texture

    vol + 0.01 => vol;
    50::ms => now;
}

// Evolving sound loop
while(true)
{
    for (0 => int i; i < N; i++)
    {
        (freqs[i] + detunes[i] + vibrato.last()*2) => sqr[i].freq;
        (freqs[i]*2) + sweep.last() * freqs[i] => lpf[i].freq;
    }

    800 + sweep.last() * 400 => bpf.freq; // noise filter moves
    10::ms => now;
}
