// Body tone
SinOsc s => ADSR envT => dac;

// Clicky attack
Noise n => BPF nFilt => ADSR envN => dac;

// Sharp noise click bandpass
2500 => nFilt.freq;
2 => nFilt.Q;

// Envelope: punchy energy
envT.set(1::ms, 5::ms, 0, 100::ms);
envN.set(0.5::ms, 1::ms, 0, 20::ms);

// Corps frequency ~80Hz, tight & punchy
80 => s.freq;

// Gain settings
1 => s.gain;
0.7 => envT.gain;

1 => n.gain;
0.4 => envN.gain;

// Optionally add subtle saturation after env — skipped here for simplicity

// Trigger kick
envT.keyOn();
envN.keyOn();

125::ms => now;
envT.keyOff();
envN.keyOff();

100::ms => now;
