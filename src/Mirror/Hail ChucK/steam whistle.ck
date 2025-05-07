//--------------------------------------------------------
// Evolving steam / vapor synth in ChucK
//--------------------------------------------------------

Noise hiss => BPF f1 => Gain g => JCRev r => dac;
SinOsc whistle => Gain wg => r;
SinOsc whistle2 => Gain wg2 => r;

0.2 => wg.gain;
0.2 => wg2.gain;
0.5 => g.gain;
0.4 => r.mix;

// Filter bandpass: mid-high airy range
3000 => f1.freq;
6 => f1.Q;

// Initialize whistle tones
2000 => whistle.freq;
2200 => whistle2.freq;

// Envelope function for hiss level
fun void envelope()
{
    g.gain(0);
    0.1 :: second => dur fade;
    g.gain() => float target;

    // fade-in
    for (0 => int i; i < 10; i++)
    {
        i / 10.0 * 0.5 => g.gain;
        fade/10 => now;
    }

    // stay on
    2::second => now;

    // fade-out
    for (9 => int i; i >=0; i--)
    {
        i / 10.0 * 0.5 => g.gain;
        fade/10 => now;
    }
}

// Automation loop
spork ~ envelope();

// Animate whistle sweeps and filters
spork ~ modulate();

// splash background noise optional from before
// spork ~ splashLoop();

//------------------------------------------
// Modulator function
//------------------------------------------
fun void modulate()
{
    while (true)
    {
        // Slowly glide whistle frequencies for 'valve screech' feel
        Std.rand2f(1300,2600) => whistle.freq;
        Std.rand2f(1500,2800) => whistle2.freq;

        // Sweep BPF center for airy jets
        Std.rand2f(2000,5000) => f1.freq;

        // change over random period
        Std.rand2(2,5)::second => now;
    }
}

while(true) 100::ms => now;
