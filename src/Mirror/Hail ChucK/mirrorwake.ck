// Mira’s expressive bird call — clean and working version

// --- Signal chain ---
// Sine oscillator → ADSR envelope → Reverb → dac
SinOsc s => ADSR env => NRev rev => dac;

// Vibrato parameters
0.1 => float vibratoDepth;
6.0 => float vibratoRate;

// Envelope settings
0.01::second => env.attackTime;
0.2::second => env.decayTime;
0.5 => env.sustainLevel;
0.3::second => env.releaseTime;

// Reverb mix
0.3 => rev.mix;

// Starting frequency (will be modulated)
800.0 => float baseFreq;
baseFreq => s.freq;

// Vibrato loop (modifies s.freq slightly over time)
fun void vibratoLoop() {
    while (true) {
        (now / second) $ float => float t;
        vibratoDepth * Math.sin(2 * Math.PI * vibratoRate * t) + 1.0 => float mod;
        baseFreq * mod => s.freq;
        1::ms => now;
    }
}
spork ~ vibratoLoop();

// Frequency pattern (a little tune)
[ 800.0, 950.0, 740.0, 1200.0 ] @=> float freqs[];

// Play one phrase of bird calls
fun void callPhrase() {
    for (0 => int i; i < freqs.cap(); i++) {
        freqs[i] => baseFreq; // update baseFreq for vibrato
        env.keyOn();
        250::ms => now;
        env.keyOff();
        150::ms => now;
    }
}

// Loop forever
while (true) {
    callPhrase();
    Std.rand2f(1.5, 4.0) * second => now;
}
