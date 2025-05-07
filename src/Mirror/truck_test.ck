// truck_test.ck
// Simple script that synthesizes a truck engine-like rumble sound using filtered noise and low frequency oscillation

Noise noiseSource;
LPF lowpass;
Gain gain;
SinOsc lfo;

// connect the chain: noise -> lowpass -> gain -> dac
noiseSource => lowpass => gain => dac;

// set initial filter cutoff for engine rumble
200.0 => lowpass.freq;
1.0 => lowpass.Q;

// set gain level
0.3 => gain.gain;

// set LFO parameters for modulation (simulate engine variations)
6.0 => lfo.freq;
0.1 => lfo.gain;

// variable to hold modulated cutoff frequency
float baseFreq;
200.0 => baseFreq;

float lfoValue;
float modFreq;

while(true)
{
    // get current LFO value scaled
    lfo.last() => lfoValue;
    
    baseFreq + (lfoValue * 100.0) => modFreq;
    
    modFreq => lowpass.freq;
    
    // advance time slightly for audio processing
    10::ms => now;
}
