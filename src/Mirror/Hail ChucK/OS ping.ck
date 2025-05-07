// CHUCK-FRIENDLY iOS PING  
// === SETTINGS ===  
784 => float freq;      // G5 (balanced tone)  
0.5 => float volume;    // Max volume  
20::ms => dur attack;   // Quick but smooth onset  
400::ms => dur decay;   // Full control over fade  

// === SIGNAL CHAIN ===  
SinOsc osc => LPF filter => Gain vol => dac;  
4000 => filter.freq;    // Bright but not harsh  
0.1 => filter.Q;  

// === MANUAL ENVELOPE ===  
freq => osc.freq;  
0 => vol.gain;  

// Attack phase (fade in)  
for (0 => int i; i <= 100; i++) {  
    (i / 100.0) * volume => vol.gain;  
    (attack / 100.0) => now;  
}  

// Decay phase (exponential fade)  
now + decay => time end;  
while (now < end) {  
    (volume * Math.pow((end - now) / decay, 2)) => vol.gain;  
    5::ms => now;  // Update rate (smoother than shred cutoff)  
}  

// Let last sample play  
1::samp => now;  
