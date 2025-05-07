// Water Simulation Using Stacked Comb Filters and a Square Wave

// Global reverb for ambience
JCRev reverb => dac;
reverb.mix(0.3);

// Processing chain:
// SqrOsc -> Gain -> Comb Filter 1 -> Feedback Gain -> Comb Filter 2 -> Feedback Gain -> Low-pass filter -> Reverb
SqrOsc sqr => Gain g0 => DelayL dly1 => Gain fb1 => DelayL dly2 => Gain fb2 => LPF lpf => reverb;

// Base square wave parameters
200 => sqr.freq;        // Lower frequency for a smoother tone
0.25 => sqr.gain;
1.0 => g0.gain;

// Comb Filter 1 setup
50::ms => dly1.max;
20::ms => dly1.delay;
0.6 => fb1.gain;

// Comb Filter 2 setup
50::ms => dly2.max;
20::ms => dly2.delay;
0.6 => fb2.gain;

// Final low-pass filter to soften harshness
800 => lpf.freq;       // Low-pass cutoff frequency
2 => lpf.Q;            // Resonance factor

// LFO for modulating comb filter 1 delay time
SinOsc lfo1 => blackhole;
0.15 => lfo1.freq;

// LFO for modulating comb filter 2 delay time
SinOsc lfo2 => blackhole;
0.12 => lfo2.freq;

// Main modulation loop: update delay times every 20 ms
while (true) {
    // Map lfo1 output (-1 to 1) to delay range [15ms, 25ms] for comb filter 1
    ( (lfo1.last() + 1) / 2 * 10::ms + 15::ms ) => dly1.delay;
    
    // Map lfo2 output (-1 to 1) similarly for comb filter 2
    ( (lfo2.last() + 1) / 2 * 10::ms + 15::ms ) => dly2.delay;
    
    20::ms => now;
}
