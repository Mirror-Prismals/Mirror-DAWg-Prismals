// Ground Right-Click (Blue) Attack Sound Effect

// --- PARAMETERS ---
0.8 => float amp;                 // Overall amplitude
400::ms => dur attack;            // Attack duration
500::ms => dur sustain;           // Sustain duration
0.2 => float decayLevel;          // Decay level (0-1)
100::ms => dur release;           // Release time

// --- FREQUENCIES (Blue theme) ---
50 => float baseFreq;             // Bass undertone
350 => float mainFreq;            // Main blue carrier
550 => float highFreq;            // Brightness

// --- OSCILLATORS ---
SawOsc saw => Gain sawGain => blackhole;
SqrOsc sq => Gain sqGain => blackhole;
SinOsc sin => Gain sinGain => blackhole;
TriOsc tri => Gain triGain => blackhole;

// Set oscillator levels for blending
0.5 => sawGain.gain;
0.3 => sqGain.gain;
0.2 => sinGain.gain;
0.15 => triGain.gain;

// --- FM SYNTH ---
SinOsc mod => Gain modGain => blackhole;
SinOsc car => Gain carGain => blackhole;
0.25 => modGain.gain;
0.2 => carGain.gain;

// --- MIXER ---
Gain mix => LPF filter => JCRev reverb => DelayA delay => Gain limiter => dac;
1.0 => limiter.gain; // Set to 1.0, adjust if you hear clipping

// --- CONNECT OSCILLATORS TO MIXER ---
sawGain => mix;
sqGain => mix;
sinGain => mix;
triGain => mix;
carGain => mix;

// --- EFFECTS SETTINGS ---
2000 => filter.freq;
2.0 => filter.Q;
0.15 => reverb.mix;
25::ms => delay.delay;
0.15 => delay.gain;

// --- SOUND FUNCTION ---
fun void playGroundRightClick() {
    // Set oscillator frequencies
    mainFreq => saw.freq;
    mainFreq * 0.97 => sq.freq;
    baseFreq => sin.freq;
    highFreq * 1.1 => tri.freq;

    // FM setup: modulate car's frequency
    120 => car.freq;
    highFreq * 1.8 => mod.freq;
    spork ~ fmModulate();

    // ADSR envelope (manual, since ChucK ADSR can't control multiple sources easily)
    amp => mix.gain;
    0 => mix.gain;
    // Attack
    for (0 => int i; i < attack/10::ms; i++) {
        (amp * (i/(attack/10::ms))) => mix.gain;
        10::ms => now;
    }
    // Sustain
    amp => mix.gain;
    sustain => now;
    // Decay
    for (0 => int i; i < release/10::ms; i++) {
        (amp * (1.0 - (i/(release/10::ms))) * decayLevel) => mix.gain;
        10::ms => now;
    }
    0 => mix.gain;
}

// --- FM MODULATION COROUTINE ---
fun void fmModulate() {
    while (true) {
        // Simple FM: modulate car frequency with modulator
        120 + (mod.last() * 40) => car.freq;
        1::samp => now;
    }
}

// --- TRIGGER SOUND ---
playGroundRightClick();

// Wait a bit after sound to keep example tidy
1::second => now;
