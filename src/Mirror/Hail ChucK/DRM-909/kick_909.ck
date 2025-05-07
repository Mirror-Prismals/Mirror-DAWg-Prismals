// 909 Kick Drum: sine w. mild pitch sweep + attack click

// Sine body
SinOsc s => ADSR envTone => dac;

// Attack click: noise burst, filtered bright
Noise n => BPF clickFilt => ADSR envClick => dac;

// Filter settings for attack click
3000 => clickFilt.freq;
2 => clickFilt.Q;

// Envelope for sine body: attack, decay ~400ms, no sustain
envTone.set(2::ms, 15::ms, 0, 400::ms);

// Envelope for click: very short noisy pop
envClick.set(1::ms, 2::ms, 0, 20::ms);

// Sine gain/body
1 => s.gain;
0.9 => envTone.gain;

// Noise gain/click
1 => n.gain;
0.4 => envClick.gain;    // balance click level

// Pitch sweep params: mild, fast sweep down
60 => float baseFreq;       // base final pitch (Hz)
80 => float sweepAmt;       // how much higher at start (~quick bang)
60::ms => dur sweepTime;    // fast sweep down (shorter than 808)

// --- Fire envelopes ---
envTone.keyOn();
envClick.keyOn();

// Pitch sweep: quick exponential drop
now => time t0;
while (now - t0 < sweepTime) {
    (baseFreq + sweepAmt * Math.exp(-5 * (now - t0)/sweepTime)) => s.freq;
    1::ms => now;
}
// lock to base
baseFreq => s.freq;

// Let body ring naturally per decay
400::ms => now;

envTone.keyOff();
envClick.keyOff();

200::ms => now;
