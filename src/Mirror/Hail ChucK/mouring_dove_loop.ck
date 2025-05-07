// Mourning dove "coo" with richer overtones

for (0 => int n; n < 2; n++) {
    // Main tone
    SinOsc s => BPF f => Gain g => dac;
    // overtone an octave above
    SinOsc h => Gain hg => g;
    0.15 => hg.gain;
    // Noise breath layer
    Noise noise => LPF nf => g;
    1000 => nf.freq;
    0.05 => noise.gain;

    // vibrato oscillator
    SinOsc vibr => blackhole;
    5.0 => vibr.freq;
    1.0 => vibr.gain;
    10 => float vibrDepth;

    7 => f.Q;
    .6 => g.gain;

    // pitch sweep parameters
    580 => float startFreq;    // a bit smoother
    420 => float endFreq;
    2 * startFreq => h.freq;

    // time params
    80::ms => dur atk;
    200::ms => dur sus;
    100::ms => dur rel;

    0.4 => float maxAmp;
    0.0 => float amp;

    // attack: fade in
    for (0 => int i; i < atk/1::ms; i++) {
        amp + (maxAmp - amp)*0.4 => amp;
        amp => g.gain;
        1::ms => now;
    }

    // sustain + pitch down with vibrato
    (sus/1::ms) $ int => int steps;
    for (0 => int i; i < steps; i++) {
        startFreq + (endFreq - startFreq)*(i / (steps * 1.0)) => float cfreq;
        cfreq + vibrDepth * vibr.last() => float fcur;
        fcur => s.freq;
        2 * fcur => h.freq;
        fcur => f.freq;
        1::ms => now;
    }

    // release
    (rel/1::ms) $ int => int steps2;
    for (0 => int i; i < steps2; i++) {
        amp * 0.8 => amp;
        amp => g.gain;
        1::ms => now;
    }

    100::ms => now; // pause between coos
}
