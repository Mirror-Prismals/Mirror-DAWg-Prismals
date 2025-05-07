// Crow Synth 1.0 - Auditory Bird Taxidermy

0.3::second => dur cawTime;
1.0 => float ampGain;

Noise n => LPF filter => Pan2 pan => Gain master => dac;

Std.rand2f(400, 1000) => float rootFreq;
rootFreq => float startFreq;
rootFreq + Std.rand2f(300, 800) => float endFreq;

0.001 => filter.freq;
1.0 => filter.Q;

ampGain => master.gain;

Envelope env => master;
0.0 => env.value;

spork ~ crow_caw();

fun void crow_caw() {
    // Attack
    1.0 => env.target;
    0.05::second => env.duration;
    env.keyOn();

    // Frequency sweep
    startFreq => float f;
    (endFreq - startFreq)/(cawTime/ms) => float fstep;
    for (0 => int i; i < (cawTime/ms); i++) {
        f + fstep*i => filter.freq;
        1::ms => now;
    }

    // Release
    0.0 => env.target;
    0.1::second => env.duration;
    env.keyOff();
    0.1::second => now;
}

cawTime + 0.2::second => now;
