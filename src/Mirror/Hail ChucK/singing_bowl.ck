// Fluid Simulation: Hand moving across water surface

// Set up reverb for ambient space
JCRev reverb => dac;
reverb.mix(0.3);

// --- Fluid Sine Layer ---
// This continuous sine oscillator provides a soft tonal base that gently modulates in pitch and amplitude.
SinOsc fluidSine => Gain fluidGain => reverb;
350 => fluidSine.freq;
0.15 => fluidGain.gain;

// LFO to modulate the sine oscillator parameters slowly
SinOsc sineLFO => blackhole;
0.1 => sineLFO.freq; // very slow modulation

// --- Texture Noise Layer ---
// A low-level noise source is filtered and barely audible—its subtle modulations add fluid texture.
Noise fluidNoise => LPF noiseFilter => Gain noiseGain => reverb;
0.07 => fluidNoise.gain;
400 => noiseFilter.freq;
4 => noiseFilter.Q;
0.05 => noiseGain.gain;

// LFO to modulate the noise filter cutoff slowly
SinOsc noiseLFO => blackhole;
0.07 => noiseLFO.freq; // slow modulation

// --- Continuous Modulation Loop ---
// Adjust parameters every 50ms to create a continuously evolving, fluid sound.
while (true) {
    // Normalize the sine LFO output (from -1..1 to 0..1)
    (sineLFO.last() + 1) / 2 => float normSine;
    // Modulate fluidSine frequency gently between 340 and 360 Hz
    340 + normSine * 20 => fluidSine.freq;
    // Modulate fluidSine amplitude slightly between 0.14 and 0.16
    0.14 + normSine * 0.02 => fluidGain.gain;
    
    // Normalize the noise LFO output (from -1..1 to 0..1)
    (noiseLFO.last() + 1) / 2 => float normNoise;
    // Modulate noise filter cutoff between 350 and 450 Hz for a subtle shifting texture
    350 + normNoise * 100 => noiseFilter.freq;
    
    50::ms => now;
}
