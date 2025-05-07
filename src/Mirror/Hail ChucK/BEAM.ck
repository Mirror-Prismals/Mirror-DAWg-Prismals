// Bubble pop: falling sine burst

SinOsc s => Gain g => dac;

// Set initial gain low
0 => g.gain;

// Duration of bubble (short!)
dur total;
50::ms => total;

// Frequency glide: high to low
float startFreq;
1200.0 => startFreq;

float endFreq;
400.0 => endFreq;

// Envelope
float startGain;
0.6 => startGain;

float endGain;
0.0 => endGain;

// Time resolution
dur step;
5::ms => step;

int steps;
(total / step) $ int => steps;

// Linear ramp down
int i;
for (0 => i; i < steps; i++) {
    float t;
    i $ float => t;
    t / steps => t;

    float freq;
    (1.0 - t) * startFreq + t * endFreq => freq;
    freq => s.freq;

    float gain;
    (1.0 - t) * startGain + t * endGain => gain;
    gain => g.gain;

    step => now;
}
