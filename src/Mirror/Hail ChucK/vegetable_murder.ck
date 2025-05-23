// "Vegetable Murder" Foley SFX (Gore Splat & Crunch)

// Crunch layer
fun void crunch() {
    Noise n => LPF f => Gain g => dac;
    1200 => f.freq;
    0.7 => g.gain;
    40::ms => now;
    0.25 => g.gain;
    25::ms => now;
    0 => g.gain;
}

// Wet splat layer
fun void splat() {
    Noise n => LPF f => Gain g => dac;
    600 => f.freq;
    0.8 => g.gain;
    30::ms => now;
    0.3 => g.gain;
    60::ms => now;
    0 => g.gain;
}

// Gory gush layer
fun void gush() {
    Noise n => LPF f => ADSR env => Gain g => dac;
    900 => f.freq;
    0.7 => g.gain;
    env.set(5::ms, 60::ms, 0.2, 70::ms);
    env.keyOn();
    110::ms => now;
    env.keyOff();
    80::ms => now;
    0 => g.gain;
}

// Bone snap (pitch-bending sine)
fun void snap() {
    SinOsc s => ADSR env => Gain g => dac;
    0.15 => g.gain;
    600 => s.freq;
    env.set(2::ms, 18::ms, 0.1, 15::ms);
    env.keyOn();
    for (0 => int i; i < 16; i++) {
        (600 + Math.random2f(0,70)) => s.freq;
        2::ms => now;
    }
    env.keyOff();
    25::ms => now;
    0 => g.gain;
}

// Thump layer
fun void thump() {
    Noise n => LPF f => ADSR env => Gain g => dac;
    110 => f.freq;
    0.22 => g.gain;
    env.set(3::ms, 35::ms, 0.0, 20::ms);
    env.keyOn();
    50::ms => now;
    env.keyOff();
    50::ms => now;
    0 => g.gain;
}

// Layer everything together (staggered for realism)
spork ~ thump();
0.01::second => now;
spork ~ crunch();
0.025::second => now;
spork ~ splat();
spork ~ gush();
0.04::second => now;
spork ~ snap();

0.3::second => now; // let it all play
