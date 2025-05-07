// --- Liquid Collisions ---
// Random overlapping resonator bursts + splashes = underwater "tinks", hits, coins, bubbly clangs.

JCRev rev => dac;
rev.mix(0.35);

// Helpers
fun dur rrandd(dur min, dur max) { return min + (max - min)*Std.rand2f(0,1); }
fun float rrandf(float min, float max) { return Std.rand2f(min,max); }

// One collision event = ping + splash
fun void collision() {
    // Metallic ping 
    SinOsc ping => ADSR pingEnv => Gain g1 => rev;
    rrandf(350, 1000) => ping.freq;
    0.1 => ping.gain;
    5::ms => dur atk;
    rrandd(150::ms,400::ms) => dur dec;
    0 => float sus;
    100::ms => dur rel;
    pingEnv.set(atk, dec, sus, rel);

    // Noise splash transient
    Noise n => BPF filt => ADSR splashEnv => Gain g2 => rev;
    rrandf(1000, 2500) => filt.freq;
    4 => filt.Q;
    0.2 => n.gain;
    2::ms => dur natk;
    30::ms => dur ndec;
    0 => float nsus;
    50::ms => dur nrel;
    splashEnv.set(natk, ndec, nsus, nrel);

    0.12 => g1.gain; // metallic ping level
    0.08 => g2.gain; // splash level

    splashEnv.keyOn();
    pingEnv.keyOn();

    // Slight upward or downward pitch glide (tiny slide)
    (atk + dec) => dur glideDur;
    (glideDur / 5::ms) $ int => int steps;
    ping.freq() => float f0;
    f0 + rrandf(-20,20) => float f1; // slide range +/-20 Hz

    for(0=>int i; i<steps; i++){
        f0 + (f1-f0)*(i$float/steps) => ping.freq;
        5::ms => now;
    }

    pingEnv.keyOff();
    splashEnv.keyOff();
    rel > nrel ? rel : nrel => now;
}

// Loop: random collision clusters
while(true)
{
    1 + Std.rand2(0,2) => int hits;
    for(0=>int i; i < hits; i++){
        spork ~ collision();
        rrandd(15::ms, 50::ms) => now;
    }
    rrandd(120::ms, 300::ms) => now;
}
