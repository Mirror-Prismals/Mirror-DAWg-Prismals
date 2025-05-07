

//------------------------------------------------------
// Signal Chain Setup
//------------------------------------------------------

// Harmonic chain
SawOsc saw => LPF lpf => ADSR harmEnv => Gain harmGain => PRCRev rev;
// Noisy chain
Noise noise => BPF noiseBPF => ADSR noiseEnv => Gain noiseGain => rev;

// Stereo panning: send the reverb output to the pan, then to the dac
Pan2 pan => dac;
rev => pan;

//------------------------------------------------------
// Initial Parameter Setup
//------------------------------------------------------
23 => float baseFreq;
baseFreq => saw.freq;
0.3 => saw.gain;
0.4 => noise.gain;

// LPF settings for harmonic part
800 => lpf.freq;
2 => lpf.Q;

// BPF settings for noise part
2000 => noiseBPF.freq;
0.5 => noiseBPF.Q;

// Envelope settings for bursts
harmEnv.set(0.005::second, 0.15::second, 0.0, 0.05::second);
noiseEnv.set(0.005::second, 0.1::second, 0.0, 0.05::second);

// Reverb for wet, sloppy quality
0.15 => rev.mix;

// Global master amplitude envelope (via a Gain with an attached Envelope)
// You can later use this chain to shape the overall amplitude if needed.
Gain masterGain => dac;
0.8 => masterGain.gain;  // Set a base amplitude

// Optional master envelope attached to masterGain (for slow decays)
// Here we use a Step-to-Envelope chain as in your example:
Step masterStep => Envelope masterEnv => masterGain;
1 => masterEnv.value;
masterEnv.duration(10::second);
masterEnv.keyOn();

//------------------------------------------------------
// Additional Modulation: LFO for Vibrato Effect
//------------------------------------------------------
SinOsc lfo => blackhole; // Use blackhole as we only need its output
.9 => lfo.freq;        // Very slow LFO for subtle pitch modulation

//------------------------------------------------------
// Timing Variables
//------------------------------------------------------
0.1::second => dur baseInterval;
dur additionalDelay;

//------------------------------------------------------
// Main Infinite Loop: Evolving Slurping Sound
//------------------------------------------------------
while(true)
{
    // Randomize filter characteristics for organic variation
    Math.random2f(600, 1200) => lpf.freq;
    Math.random2f(1500, 3000) => noiseBPF.freq;
    Math.random2f(0.5, 2.5) => lpf.Q;
    
    // Use LFO to add a gentle vibrato to the harmonic pitch
    baseFreq + (lfo.last() * 5) => saw.freq;
    
    // Randomize gains to vary the mix of harmonic and noisy content per burst
    Math.random2f(0.2, 0.8) => saw.gain;
    Math.random2f(0.3, 0.7) => noise.gain;
    
    // Randomize stereo panning (-1 for left, 1 for right)
    Math.random2f(-1.0, 1.0) => pan.pan;
    
    // Occasionally trigger double-time pulses for extra rhythmic interest
    if ( Math.randomf() > 0.85 )
        0.1::second => additionalDelay;
    else
        0.0::second => additionalDelay;
    
    // Trigger both envelopes to create a burst of slurpy sound
    harmEnv.keyOn();
    noiseEnv.keyOn();
    
    // Hold the burst for a short moment
    0.1::second => now;
    
    // Release the envelopes
    harmEnv.keyOff();
    noiseEnv.keyOff();
    
    // Wait for the base interval plus any additional delay before next burst
    baseInterval + additionalDelay => now;
    
    // Gradually slow down the rhythm over time
    baseInterval * 1.01 => baseInterval;
    
    // Occasionally refresh the master envelope to inject dynamic variation
    if ( Math.randomf() > 0.95 )
        masterEnv.keyOn();
}
