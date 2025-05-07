// Fluid Friction Simulation: Hand moving on water

// Global reverb for ambient space
JCRev reverb => dac;
reverb.mix(0.3);

// Continuous noise source
Noise n => LPF lp => DelayL dly => Gain g => reverb;
0.2 => n.gain;       // Subtle noise level
400 => lp.freq;      // Starting cutoff frequency for the low-pass filter
2 => lp.Q;           // Moderate resonance

// Setup delay: 
// We'll use the delay to add a subtle echo that is modulated to simulate the fluid, shifting character.
50::ms => dly.max;   // Maximum allowed delay time
40::ms => dly.delay; // Initial delay time
0.3 => dly.gain;     // Feedback amount (kept low for a gentle effect)

// Create a Gain unit to control overall amplitude before reverb
0.2 => g.gain;

// LFO for modulating the LPF cutoff frequency (simulating dynamic timbre changes)
SinOsc filterLFO => blackhole;
0.05 => filterLFO.freq; // Very slow modulation

// LFO for modulating the delay time (adding subtle movement)
SinOsc delayLFO => blackhole;
0.07 => delayLFO.freq; // Slow modulation

// LFO for modulating overall amplitude (adds dynamic texture)
SinOsc ampLFO => blackhole;
0.1 => ampLFO.freq; // Very slow amplitude modulation

// Continuous modulation loop
while (true) {
    // Modulate LPF cutoff between about 300 and 600 Hz
    300 + ((filterLFO.last() + 1) / 2) * 300 => lp.freq;
    
    // Modulate delay time between 30 and 70 milliseconds
    30::ms + (((delayLFO.last() + 1) / 2) * 40::ms) => dly.delay;
    
    // Modulate overall amplitude between 0.15 and 0.25
    0.15 + ((ampLFO.last() + 1) / 2) * 0.1 => g.gain;
    
    20::ms => now;
}
