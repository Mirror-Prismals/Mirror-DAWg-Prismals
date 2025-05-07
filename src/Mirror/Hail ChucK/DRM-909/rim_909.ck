// 909 snare: bright noise + subtle tone + crisp transient

// Noise part
Noise n => BPF noiseFilt => ADSR envNoise => dac;

// Tone part ("body")
SinOsc tone => ADSR envTone => dac;

// Noise filter: bright
2000 => noiseFilt.freq;
2.5 => noiseFilt.Q;

// Envelope: fast attack, short-medium decay
envNoise.set(1::ms, 8::ms, 0, 180::ms);
envTone.set(1::ms, 20::ms, 0, 150::ms);

// Tuning the body tone
200 => tone.freq;

// Gains
1.0 => n.gain;           // noise source amplitude
0.5 => envNoise.gain;    // mix level
0.7 => envTone.gain;

// Optional: very subtle drive / bit crush would go here for more authentic crunch

// Fire both voices
envNoise.keyOn();
envTone.keyOn();

250::ms => now;

envNoise.keyOff();
envTone.keyOff();

200::ms => now;
