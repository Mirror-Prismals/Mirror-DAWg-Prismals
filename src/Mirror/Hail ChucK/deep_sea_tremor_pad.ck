// DEEP SEA TREMOR PAD — ULTRA LOUD VERSION 

// Two strong detuned sines + louder noise
SinOsc bass1 => Gain bassGain => JCRev rev => dac;
SinOsc bass2 => bassGain;
Noise n => BPF filt => Gain noiseGain => rev;

// Reverb
0.6 => rev.mix;

// Frequencies
30 => float baseFreq;
baseFreq => bass1.freq;
baseFreq * 1.01 => bass2.freq; // slight detune

// LOUDER — double each component
1.0 => bass1.gain;    // max safe gain per osc
1.0 => bass2.gain;    
1.0 => bassGain.gain; // summed bass total (~2x single sine)

0.4 => noiseGain.gain; // more noise for thick body

// Filter params
80 => filt.freq;
5 => filt.Q;

// LFOs for movement
SinOsc lfoBass => blackhole;
SinOsc lfoFilt => blackhole;
SinOsc lfoNoiseGain => blackhole;
SinOsc chorusLFO => blackhole;

0.02 => lfoBass.freq;
0.01 => lfoFilt.freq;
0.015 => lfoNoiseGain.freq;
0.2 => chorusLFO.freq;

// Depths
12 => float bassDepth;            
50 => float filtDepth;
0.2 => float noiseGainDepth;
1.5 => float chorusDepth;

// Base values
baseFreq => float bassBaseFreq;
filt.freq() => float baseFiltFreq;

// *** NO extra master gain to limit loudness ***

// Loop
while (true)
{
    bassBaseFreq + bassDepth * lfoBass.last() + chorusDepth * chorusLFO.last() => bass1.freq;
    bass1.freq() * 1.01 => bass2.freq;
    
    baseFiltFreq + filtDepth * lfoFilt.last() => filt.freq;
    0.15 + noiseGainDepth * lfoNoiseGain.last() => noiseGain.gain;

    20::ms => now;
}
