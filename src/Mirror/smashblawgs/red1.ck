// Ground Left Click (Red) - Smash Attack Sound

// Create a punchy percussive sound
SqrOsc osc => LPF filt => Gain g => dac;
0.18 => g.gain;

// Set frequency for a “smash” feel
180 => osc.freq;

// Add a quick pitch drop for impact
0.01::second => dur step;
for (0 => int i; i < 8; i++) {
    (180 - i*12) => osc.freq;
    (3500 - i*400) => filt.freq;
    0.04 * Math.pow(0.7, i) => g.gain;
    step => now;
}

// Add a little “snap” with noise
Noise n => Gain ng => dac;
0.09 => ng.gain;
0.02::second => now;
0 => ng.gain;

// Fade out
0 => g.gain;
