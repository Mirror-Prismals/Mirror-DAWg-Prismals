// Crow "caw" sound in ChucK

// Function to make a single "caw"
fun void crowCaw(float pitch, float duration) {
    // Noise for raspiness
    Noise n => LPF f => Gain g => ADSR env => dac;
    // Add a formant for throatiness
    SinOsc formant => g;
    // Set filter and gain
    2000 => f.freq;
    0.6 => g.gain;
    // Envelope settings
    0.01::second => env.attackTime;
    0.12::second => env.decayTime;
    0.7 => env.sustainLevel;
    0.2::second => env.releaseTime;
    // Formant frequency for "caw"
    600 => formant.freq;
    0.3 => formant.gain;
    // Pitch modulation for crow-like inflection
    for (0 => int i; i < (duration * 1000 / 15); i++) {
        // Glide up then down
        pitch + Math.sin(i * 0.07) * 60 => f.freq;
        600 + Math.sin(i * 0.03) * 60 => formant.freq;
        10::ms => now;
    }
    // Trigger envelope
    env.keyOn();
    duration::second => now;
    env.keyOff();
    0.2::second => now;
    // Disconnect
    g =< dac;
}

// Sequence of caws for a "crow" call
for (0 => int i; i < 3; i++) {
    // Vary pitch and duration for realism
    crowCaw(1100 + Std.rand2f(-100, 100), 0.25 + Std.rand2f(-0.05, 0.05));
    0.12::second => now;
}
