// Set up a reverb unit for natural ambience and connect it to the DAC
JCRev reverb => dac;
reverb.mix(0.3);

// Simple splash using a noise burst
fun void splashNoise() {
    // Create a noise generator, envelope, and send directly to the reverb chain
    Noise n => ADSR env => reverb;
    
    // Set gain so the noise is audible
    0.5 => n.gain;
    
    // Use a fast envelope: quick attack, short decay, no sustain, short release
    5::ms => dur attack;
    20::ms => dur decay;
    0 => float sustain;
    50::ms => dur release;
    env.set(attack, decay, sustain, release);
    
    // Trigger the envelope
    env.keyOn();
    (attack + decay) => now;
    env.keyOff();
    release => now;
}

// Simple splash using a sine tone
fun void splashTone() {
    // Create a sine oscillator, envelope, and send directly to the reverb chain
    SinOsc tone => ADSR env => reverb;
    
    // Set gain so the tone is audible
    0.5 => tone.gain;
    
    // Set a fixed frequency for now (adjust if desired)
    800 => tone.freq;
    
    // Use an envelope that holds the tone a little longer
    10::ms => dur attack;
    40::ms => dur decay;
    1 => float sustain;
    100::ms => dur release;
    env.set(attack, decay, sustain, release);
    
    // Trigger the envelope
    env.keyOn();
    (attack + decay) => now;
    env.keyOff();
    release => now;
}

// Main loop: trigger a noise splash and a tone splash one after the other
while( true ) {
    splashNoise();
    splashTone();
    500::ms => now;
}
