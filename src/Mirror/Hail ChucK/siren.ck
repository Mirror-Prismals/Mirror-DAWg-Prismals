// Continuous Fluid Friction Simulation

// Global reverb for ambience
JCRev reverb => dac;
reverb.mix(0.2);

// --- Noise Layer ---
// A continuous noise source that is low-pass filtered to create a smooth, evolving texture.
Noise n => LPF filter => Gain noiseGain => reverb;
0.12 => n.gain;         // low-level noise
1.0 => noiseGain.gain;
400 => filter.freq;      // starting cutoff frequency
2 => filter.Q;          // moderate resonance

// --- Friction Tone Layer ---
// A continuous sine oscillator that provides a subtle scraping or friction tone.
SinOsc friction => Pan2 frictionPan => reverb;
0.08 => friction.gain;   // low-level tone
400 => friction.freq;    // base frequency
0 => frictionPan.pan;    // centered

// --- Modulation Sources ---
// LFO for noise layer modulation (affects filter cutoff and noise gain)
SinOsc noiseLFO => blackhole;
0.05 => noiseLFO.freq;   // very slow modulation

// LFO for friction tone modulation (affects frequency and panning)
SinOsc frictionLFO => blackhole;
0.1 => frictionLFO.freq; // very slow modulation

// --- Main Continuous Modulation Loop ---
// Adjust parameters every 20 ms to create slowly evolving textures.
while (true) {
    // Normalize noiseLFO output (-1 to 1 becomes 0 to 1)
    (noiseLFO.last() + 1) / 2 => float modNorm;
    
    // Map normalized value to a cutoff frequency between 300 Hz and 600 Hz
    300 + modNorm * 300 => filter.freq;
    
    // Slightly modulate noise amplitude (range 0.1 to 0.15)
    0.1 + modNorm * 0.05 => n.gain;
    
    // Modulate friction tone frequency: vary between 350 Hz and 450 Hz
    350 + ((frictionLFO.last() + 1) / 2) * 100 => friction.freq;
    
    // Modulate friction panning (left to right)
    frictionLFO.last() => frictionPan.pan;
    
    20::ms => now;
}
