// --- Toto "Africa"-inspired FM + pad chord synth ---
// Balanced loudness, lush & long sustain, better detuning & vibrato

// Final output chain
Gain main => JCRev reverb => dac;
0.03 => main.gain;          // master loudness cap, safe headphone volume
0.2 => reverb.mix;          // gentle reverb

// Chord notes (F major9)
[349.23, 440, 523.25, 659.25, 392] @=> float freqs[];

// Slight detunes per note (chorus)
[-0.5, 0.3, -0.8, 0.5, -0.2] @=> float dt[];

5 => int n;

// FM and pad arrays
SinOsc carrier[n];
SinOsc mod[n];
Gain carGain[n];
Gain modGain[n];

SawOsc pad[n];
Gain padGain[n];

// Voice setup
for (0 => int i; i < n; i++)
{
    // FM voice
    SinOsc c => carGain[i] => main;
    SinOsc m => modGain[i] => blackhole;
    c @=> carrier[i];
    m @=> mod[i];

    (freqs[i] + dt[i]) => carrier[i].freq;
    (carrier[i].freq()*2.005) => mod[i].freq;

    1.0 => carGain[i].gain;
    0.5 => modGain[i].gain;         // tamed modulator loudness

    // Pad voice
    SawOsc p => LPF f => padGain[i] => main;
    freqs[i] => p.freq;
    2000 => f.freq;                 // mellow filter cutoff (~2kHz)
    1 => f.Q;
    0.1 => padGain[i].gain;        // pad layer low
    p @=> pad[i];
}

// Slow vibrato oscillator
SinOsc vibrato => blackhole;
5 => vibrato.freq;           // Hz
0.3 => float modDepth;       // vibrato depth, semitone swing

// Envelope parameters
300::ms => dur attack;
5::second => dur sustain;
8::second => dur decay;

// Compute sample counts
(second / samp) => float SRATE;

(attack / second * SRATE) $ int => int attackSamples;
(sustain / second * SRATE) $ int => int sustainSamples;
(decay / second * SRATE) $ int => int decaySamples;

attackSamples + sustainSamples + decaySamples => int totalSamples;

// Envelope + mod loop
0 => int t;
while (t < totalSamples)
{
    0.0 => float env;
    if(t < attackSamples)
    {
        (t $ float) / (attackSamples $ float) => env;
    }
    else if(t < attackSamples + sustainSamples)
    {
        1.0 => env;
    }
    else
    {
        1.0 - (((t - attackSamples - sustainSamples) $ float) / (decaySamples $ float)) * 0.95 => env;
    }

    // vibrato Hz offset (centered around 0)
    vibrato.last() * modDepth * 5.0 => float vibHz; 

    for(0 => int i; i < n; i++)
    {
        // Vibrato on carriers
        (freqs[i] + dt[i] + vibHz) => carrier[i].freq;
        (carrier[i].freq() * 2.005) => mod[i].freq;
        
        env => carGain[i].gain;
        0.5 * env => modGain[i].gain;     // consistent with above

        env * 0.1 => padGain[i].gain;     // pad fades out similarly
    }

    1::samp => now;
    t++;
}

// Fade out tail
1000::ms => now;
