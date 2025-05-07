// Ground Left-Click Attack (Red, Punchy, Impactful)
SawOsc saw => ADSR adsr => dac;
saw.gain(0.5);
saw.freq(150.0);
adsr.set(20::ms, 20::ms, 0.8, 50::ms);
adsr.keyOn();

// Add a secondary noise hit for impact
Noise noise => ADSR adsr2 => dac;
noise.gain(0.8);
adsr2.set(1::ms, 50::ms, 0.7, 10::ms);
adsr2.keyOn();

// Add a bass transient for punch
TriOsc bass => ADSR adsr3 => dac;
bass.gain(0.3);
bass.freq(50.0);
adsr3.set(5::ms, 20::ms, 0.0, 10::ms);
adsr3.keyOn();

// Modulate the saw hit slightly for grit
SinOsc modulator => blackhole;
modulator.freq(120.0);
modulator.gain(0.05);
modulator => saw => blackhole;

// Set sample rate for stability
1::samp => now;

// Turn off all ADSRs after effect
adsr.keyOff();
adsr2.keyOff();
adsr3.keyOff();

// Wait for fade-out
100::ms => now;
