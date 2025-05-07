// Mourning Dove 'coo' one-shot

SinOsc s => BPF f => Gain g => dac;
// a faint octave overtone
SinOsc h => Gain hg => g;
0.15 => hg.gain;
// soft breath layer
Noise n => LPF nf => g;
1000 => nf.freq;
0.03 => n.gain;

// vibrato for wobble
SinOsc vibr => blackhole;
5.0 => vibr.freq;
1.0 => vibr.gain;
10 => float vibrDepth;    // Hz vibrato depth

7 => f.Q;
0.5 => g.gain;

580 => float startFreq;
420 => float endFreq;
2 * startFreq => h.freq;

100::ms => dur atk;
250::ms => dur sus;
120::ms => dur rel;

// fade in
0.0 => float amp;
0.4 => float maxAmp;
for(0 => int i; i < atk/1::ms; i++) {
    amp + (maxAmp - amp)*0.5 => amp;
    amp => g.gain;
    1::ms => now;
}

// sweep down with vibrato
(sus/1::ms) $ int => int steps;
for(int i; i < steps; i++) {
    startFreq + (endFreq - startFreq)*(i/(steps*1.0)) => float cfreq;
    cfreq + vibrDepth * vibr.last() => float fcur;
    fcur => s.freq;
    2 * fcur => h.freq;
    fcur => f.freq;
    1::ms => now;
}

// fade out
(rel/1::ms) $ int => int rsteps;
for(int i; i < rsteps; i++) {
    amp * 0.8 => amp;
    amp => g.gain;
    1::ms => now;
}

0 => g.gain;
