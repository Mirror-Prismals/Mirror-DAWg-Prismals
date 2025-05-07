//  EDM House Kick — one-shot

SinOsc kickOsc => Gain kickGain => dac;
kickGain.gain(0);

Noise clickNoise => BPF clickFilt => Gain clickGain => dac;
3000 => clickFilt.freq;
5 => clickFilt.Q;
0 => clickGain.gain;

// Parameters
80.0 => float startFreq;
45.0 => float endFreq;
0.8 => float maxGain;
50::ms => dur sweepTime;
300::ms => dur decayTime;

0.2 => float clickAmp;
10::ms => dur clickDuration;

fun void playKick() {
    now => time t0;

    // Loud click at start
    clickAmp => clickGain.gain;
    clickDuration => now;
    0 => clickGain.gain;
    
    // Initial freq and gain
    startFreq => kickOsc.freq;
    maxGain => kickGain.gain;

    // Sweep frequency down quickly
    while (now - t0 < sweepTime) {
        (now - t0) / sweepTime => float p;
        (startFreq * (1 - p)) + (endFreq * p) => kickOsc.freq;
        5::ms => now;
    }

    endFreq => kickOsc.freq;

    // Decay amplitude
    now => time t1;
    while (now - t1 < decayTime) {
        (now - t1) / decayTime => float p;
        (1.0 - p) * maxGain => kickGain.gain;
        5::ms => now;
    }

    0 => kickGain.gain;
}

// Fire the one-shot kick and wait for it to finish
spork ~ playKick();
(sweepTime + decayTime + 100::ms) => now;
