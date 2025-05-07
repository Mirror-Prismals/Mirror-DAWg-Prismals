// ===============================================  
// CHUCK'S POLITE PING NOTIFICATION  
// - Warm fundamental  
// - No artificial cuts  
// - Smooth exponential decay  
// ===============================================  

// Settings (tweak these!)  
784 => float baseFreq;    // G5 (balanced)  
0.5 => float volume;     // Max volume  
20::ms => dur attack;    // Fade-in time  
300::ms => dur decay;    // Total decay  

// Signal chain  
TriOsc osc => LPF filter => Gain vol => NRev rev => dac;  

// Tone shaping  
baseFreq => osc.freq;  
baseFreq * 3 => filter.freq;  // Roll off highs  
0.1 => filter.Q;              // No resonance spikes  
0.15 => rev.mix;              // Hint of space  
volume => vol.gain;  

// Manual envelope for smooth fade  
fun void playPing() {  
    // Attack phase  
    for (0 => int i; i <= 100; i++) {  
        (i / 100.0) * volume => vol.gain;  
        (attack / 100.0) => now;  
    }  

    // Decay phase (exponential curve)  
    now + decay => time end;  
    while (now < end) {  
        (volume * Math.pow((end - now) / decay, 1.7)) => vol.gain;  
        5::ms => now;  
    }  
}  

// Play it  
playPing();  
// ===============================================  
