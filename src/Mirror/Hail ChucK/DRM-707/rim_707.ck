// One-shot 707-style snare drum: bright noise burst, quick decay

Noise noise => BPF filter => ADSR env => dac;

// params: bandpass center and Q control the brightness
2000 => filter.freq;
2 => filter.Q;

// envelope: fast attack, fast decay
env.set(2::ms, 10::ms, 0, 150::ms);

// noise gain
1 => noise.gain;

// trigger envelope
env.keyOn();

200::ms => now;
env.keyOff();
200::ms => now;
