// Inspired by Lethal Lava Land / Hazy Maze Cave ambience
// Evolving drone with noise wash and bass wobble

// --- bass drone ---
SinOsc bassOsc => Gain bassGain => dac;
0.3 => bassGain.gain;
float bassFreq;
50.0 => bassFreq;
bassFreq => bassOsc.freq;

// --- mid drone, two close detuned sines ---
SinOsc mid1 => Gain midGain => dac;
SinOsc mid2 => midGain;
0.15 => midGain.gain;
220.0 => mid1.freq;
223.0 => mid2.freq;

// --- noise wash ---
Noise n => LPF nfilt => Gain ngain => dac;
2000.0 => nfilt.freq;
0.07 => ngain.gain;

// sweep variables
float sweepFreq;
1000.0 => sweepFreq;

// bass LFO variables
float lfoPhase;
0.0 => lfoPhase;
float lfoFreq;
0.05 => lfoFreq; // very slow
float lfoDepth;
5.0 => lfoDepth;

// time step
dur step;
20::ms => step;

// main loop
while(true)
{
    // bass slow sine pitch wobble
    lfoPhase + (lfoFreq * 2.0 * pi * step / second) => lfoPhase;
    if(lfoPhase > 2.0 * pi)
    {
        lfoPhase - 2.0 * pi => lfoPhase;
    }
    float mod;
    Math.sin(lfoPhase) => mod;
    bassFreq + (lfoDepth * mod) => bassOsc.freq;

    // sweep noise filter gently upwards, reset at high point
    sweepFreq + 0.5 => sweepFreq;
    if(sweepFreq > 4000.0)
    {
        1000.0 => sweepFreq;
    }
    sweepFreq => nfilt.freq;

    step => now;
}
