// just a bunch of Mira noises
fun void makeSound(float freq, dur length) {
    SinOsc s => dac;
    freq => s.freq;
    0.3 => s.gain;
    length => now;
    s =< dac;
}

spork ~ makeSound(440, 200::ms); // beep
spork ~ makeSound(660, 150::ms); // boop
spork ~ makeSound(330, 300::ms); // blip
spork ~ makeSound(550, 100::ms); // bleep
spork ~ makeSound(250, 500::ms); // bloop

// add a little wobble
fun void wubble() {
    TriOsc wob => dac;
    80 => wob.freq;
    0.2 => wob.gain;
    1::second => now;
    wob =< dac;
}
spork ~ wubble();

// and a sparkly chime!
fun void chime() {
    SqrOsc c => dac;
    1200 => c.freq;
    0.1 => c.gain;
    50::ms => now;
    c =< dac;
}
for (0 => int i; i < 4; i++) {
    spork ~ chime();
    100::ms => now;
}
