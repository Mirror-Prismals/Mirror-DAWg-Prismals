// Fundamental frequency
1900.0 => float fundamentalFreq;

// Number of harmonics (must not exceed 10)
10 => int numHarmonics;

// Harmonic gains (initialized)
[0.6, 0.5, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2] @=> float harmonicGain[];

// Noise gain
0.5 => float noiseGain;

// Envelope parameters
5::ms    => dur attackTime;
120::ms  => dur decayTime;
0.0      => float sustainLevel; // You may want to use a nonzero value here
80::ms   => dur releaseTime;

// UGens
SinOsc s[10];
Noise n;
Gain noiseAmp => blackhole; // Prevents warnings if not connected
ADSR env => dac;

// Connect harmonics to envelope
for (0 => int i; i < numHarmonics; i++) {
    fundamentalFreq * (i + 1) => s[i].freq;
    harmonicGain[i] => s[i].gain;
    s[i] => env;
}

// Connect noise to envelope
noiseGain => noiseAmp.gain;
n => noiseAmp => env;

// Set envelope parameters
env.set(attackTime, decayTime, sustainLevel, releaseTime);

// Trigger envelope
env.keyOn();

// Hold note for attack + decay + sustain
(attackTime + decayTime + 200::ms) => now;

// Trigger release
env.keyOff();

// Wait for release to finish
releaseTime + 100::ms => now;
