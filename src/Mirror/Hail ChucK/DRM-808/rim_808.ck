// One-shot 808 snare drum
SinOsc tone1 => ADSR env1 => Gain g1 => dac;
SinOsc tone2 => ADSR env2 => Gain g2 => dac;
Noise n => BPF noiseFilt => ADSR envN => Gain g3 => dac;

// Noise bandpass filter
1800 => noiseFilt.freq;
2 => noiseFilt.Q;

// Envelope attack/decay/sustain/release
env1.set(5::ms, 20::ms, 0, 200::ms);
env2.set(5::ms, 20::ms, 0, 200::ms);
envN.set(2::ms, 10::ms, 0, 180::ms);

// Tuned tone frequencies (body)
365 => tone1.freq;
545 => tone2.freq;

// Mix levels
0.2 => g1.gain;
0.1 => g2.gain;
0.5 => g3.gain;

// Noise amplitude
1 => n.gain;

// Trigger one-shot snare
env1.keyOn();
env2.keyOn();
envN.keyOn();

300::ms => now;  // hold time while ringing

// Release envelopes
env1.keyOff();
env2.keyOff();
envN.keyOff();

300::ms => now;  // fade tail for full decay before ChucK exits
