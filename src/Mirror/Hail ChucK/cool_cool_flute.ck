// Cheerful breathy flute playing a simple melody

SinOsc osc;
Noise breath;
Gain mix;
LPF filt;
Gain master;
SinOsc vibratoOsc;

// connect signal flow
osc => mix => filt => master => dac;
breath => mix;
vibratoOsc => blackhole;

// vibrato settings
float vibratoRate;
5.5 => vibratoRate;
vibratoRate => vibratoOsc.freq;

float vibratoDepth;
15.0 => vibratoDepth; // vibrato depth in Hz

// breath noise level
0.15 => breath.gain;

// mix level
0.8 => mix.gain;

// filter for tone smoothing
2500 => filt.freq;
1.0 => filt.Q;

// master volume
0.5 => master.gain;

// envelope times
float attackTime;
0.03 => attackTime;

float sustainTime;
0.35 => sustainTime; // duration per note
float releaseTime;
0.05 => releaseTime;

dur dt;
10::ms => dt;

// melody pitches (roughly the opening phrase notes in Hz)
int n;
8 => n;
float notes[8];

523.25 => notes[0];   // C5
659.25 => notes[1];   // E5
783.99 => notes[2];   // G5
698.46 => notes[3];   // F5
587.33 => notes[4];   // D5
659.25 => notes[5];   // E5
523.25 => notes[6];   // C5
392.00 => notes[7];   // G4

int i;
for (0 => i; i < n; i++)
{
    notes[i] => osc.freq;
    
    // envelope attack
    float t;
    0 => t;
    float amp;
    0 => amp;
    amp => osc.gain;
    
    while (t < attackTime)
    {
        t + (dt / second) => t;
        t / attackTime => amp;
        amp => osc.gain;
        
        vibratoOsc.last() * vibratoDepth + notes[i] => osc.freq;
        dt => now;
    }
    
    // sustain with vibrato
    float elapsed;
    0 => elapsed;
    while (elapsed < sustainTime)
    {
        vibratoOsc.last() * vibratoDepth + notes[i] => osc.freq;
        elapsed + (dt / second) => elapsed;
        dt => now;
    }
    
    // release
    amp => t;
    while (t > 0.0)
    {
        t - (dt / second / releaseTime) => t;
        if( t < 0.0 ) 0.0 => t;
        t => osc.gain;
        
        vibratoOsc.last() * vibratoDepth + notes[i] => osc.freq;
        dt => now;
    }
}

// allow tail to ring out
0.2::second => now;
