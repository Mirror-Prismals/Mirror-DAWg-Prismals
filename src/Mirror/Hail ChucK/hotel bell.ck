// CHUCK'S HOTEL BELL (Clean & Polite)
// === MAIN SETTINGS ===
323.25 => float freq;  // C5 (pleasant mid-range)
0.5 => float volume;  // Quiet elegance
100::ms => dur decay; // Short but smooth

// === SIGNAL CHAIN ===
SinOsc osc => LPF filter => ADSR env => dac;
env.set(1::ms, decay, 0.0, 1::ms); // No attack, natural decay

// Mallet-like warmth
freq => osc.freq;
freq * 4 => filter.freq; // Bright but not piercing
0.7 => filter.Q;        // Gentle resonance bump
volume => osc.gain;

// === PLAY ===
1 => env.keyOn;
decay => now;
