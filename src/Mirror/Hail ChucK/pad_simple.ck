// Simple pad sound similar to "docks" patch from Super Mario 64

// Create multiple detuned saw oscillators for a lush pad
SawOsc osc1 => Gain mix => LPF filt => Gain master => dac;
SawOsc osc2 => mix;
SawOsc osc3 => mix;
SawOsc osc4 => mix;

// Detune amounts
float detune1;
float detune2;
float detune3;
float detune4;

// Base frequency (MIDI note ~48, C3 low pad note)
float baseFreq;
130.81 => baseFreq;

// Set detuning offsets
0.99 => detune1;
1.01 => detune2;
0.97 => detune3;
1.03 => detune4;

// Assign detuned frequencies
baseFreq * detune1 => osc1.freq;
baseFreq * detune2 => osc2.freq;
baseFreq * detune3 => osc3.freq;
baseFreq * detune4 => osc4.freq;

// Set oscillator amplitudes
0.25 => osc1.gain;
0.25 => osc2.gain;
0.25 => osc3.gain;
0.25 => osc4.gain;

// Mix gain
0.7 => mix.gain;

// Filter settings for a soft pad character
800 => filt.freq;
1.5 => filt.Q;

// Master volume
0.5 => master.gain;

// Envelope
Envelope env => master;
0.0 => env.value;

// Attack and release times
0.5::second => dur attackTime;
2::second => dur releaseTime;

// Fade in (attack)
int numSteps;
100 => numSteps;
int i;
float envVal;
for (0 => i; i < numSteps; i++) {
    (i $ float) / (numSteps $ float) => envVal;
    envVal => env.value;
    attackTime / numSteps => now;
}

// Sustain
5::second => now;

// Fade out (release)
for (0 => i; i < numSteps; i++) {
    (numSteps - i) $ float / (numSteps $ float) => envVal;
    envVal => env.value;
    releaseTime / numSteps => now;
}

// Done
1::second => now;
