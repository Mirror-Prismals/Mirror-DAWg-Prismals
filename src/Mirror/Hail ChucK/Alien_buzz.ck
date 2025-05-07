

// Noise for rasp
Noise n => BPF raspy => Gain g1 => dac;
900 => raspy.freq;  // bandpass center for crow rasp
2 => raspy.Q;
0.2 => g1.gain;

// Distorted square for "bark"
SqrOsc sq => Gain g2 => dac;
200 => sq.freq;   // low, throaty
0.4 => g2.gain;
sq => Gain dist => dac;
2.0 => dist.gain; // overdrive
SinOsc lfo => blackhole; // for vibrato
7 => lfo.freq;

// Envelope
Envelope env => g1;
Envelope env2 => g2;
0.0 => env.value;
0.0 => env2.value;

// Envelope shape
fun void crow_env() {
    1.0 => env.target;
    1.0 => env2.target;
    0.03::second => env.duration;
    0.03::second => env2.duration;
    env.keyOn(); env2.keyOn();
    0.09::second => now;
    0.5 => env.target;
    0.5 => env2.target;
    0.07::second => env.duration;
    0.07::second => env2.duration;
    env.keyOn(); env2.keyOn();
    0.08::second => now;
    0.0 => env.target;
    0.0 => env2.target;
    0.08::second => env.duration;
    0.08::second => env2.duration;
    env.keyOff(); env2.keyOff();
    0.1::second => now;
}

// Pitch inflection and vibrato
fun void crow_pitch() {
    for (0 => int i; i < 180; i++) {
        200 - i => sq.freq;
        900 - i*2 => raspy.freq;
        200 + lfo.last() * 10 => sq.freq;
        900 + lfo.last() * 30 => raspy.freq;
        1::ms => now;
    }
}

// Run both in parallel
spork ~ crow_env();
crow_pitch();
